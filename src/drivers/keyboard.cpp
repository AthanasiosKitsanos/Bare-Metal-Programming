#include "kernel_pic.h"
#include "keyboard.h"
#include "terminal_io_registers.h"
#include "keyboard_key_list_n_map.h"
#include "kernel_interrupt_frame.h"
#include "kernel_interrupt_guard.h"

namespace
{
    constexpr uint16_t data_port{0x60};
    constexpr uint16_t status_port{0x64};

    constexpr uint8_t output_buffer_full{0x01};
    constexpr uint8_t input_buffer_full{0x02};

    constexpr uint32_t keyboard_timeout{100000};
    constexpr uint8_t keyboard_ack{0xFA};
    constexpr uint8_t set_leds_command{0xED};
    constexpr uint8_t all_leds_off{0x00};

    constexpr uint8_t release_mask{0x80};
    constexpr uint8_t key_code_mask{0x7F};
    constexpr uint8_t extended_prefix{0xE0};

    static_assert(static_cast<uint16_t>(driver::keyboard_key::unknown) == 0x0000);

    volatile bool g_extended_pending{false};

    driver::keyboard_modifier_state g_modifier_state{};

    constexpr uint8_t normal_key_map_size{128};
    static_assert(normal_key_map_size == static_cast<uint16_t>(key_code_mask) + 1);
    struct key_list
    {
        driver::keyboard_key entries[normal_key_map_size];

        constexpr key_list(): entries{}
        {
            #define X(key, key_code)    \
                entries[key_code] = driver::keyboard_key::key;
            DRIVER_KEYBOARD_KEY_LIST
            #undef X
        }
    };
    constexpr key_list normal_key_map{};

    struct normal_character_map_table
    {
        char entries[normal_key_map_size];

        constexpr normal_character_map_table(): entries{}
        {
            #define X(character, key_code)  \
                entries[key_code] = character;
            DRIVER_KEYBOARD_NORMAL_KEY_MAPPING
            #undef X
        }
    };
    constexpr normal_character_map_table normal_characters_table{};

    struct shifted_character_map_table
    {
        char entries[normal_key_map_size];

        constexpr shifted_character_map_table(): entries{}
        {
            #define X(character, key_code)  \
                entries[key_code] = character;
            DRIVER_KEYBOARD_SHIFTED_KEY_MAPPING
            #undef X
        }
    };
    constexpr shifted_character_map_table shifted_characters_table{};

    struct extended_key_map_table
    {
        driver::keyboard_key entries[normal_key_map_size];

        constexpr extended_key_map_table(): entries{}
        {
            #define X(key, key_code)    \
                entries[key_code] = driver::keyboard_key::key;
            DRIVER_KEYBOARD_EXTENDED_KEY_MAPPING
            #undef X
        }
    };
    constexpr extended_key_map_table extended_key_table{};

    driver::keyboard_key map_scancode_set_1_key(const uint8_t key_code, const bool extended) noexcept
    {
        if(extended) return *(extended_key_table.entries + key_code);
        return *(normal_key_map.entries + key_code);
    }

    char get_normal_character(const driver::keyboard_key key) noexcept { return *(normal_characters_table.entries + static_cast<uint16_t>(key)); }

    char get_shifted_character(const driver::keyboard_key key) noexcept { return *(shifted_characters_table.entries + static_cast<uint16_t>(key)); }

    void update_modifier_state(const driver::keyboard_key key, const driver::key_state state) noexcept
    {
        switch(key)
        {
            case driver::keyboard_key::left_shift:
                g_modifier_state.left_shift_down = (state == driver::key_state::pressed);
                break;
            case driver::keyboard_key::right_shift:
                g_modifier_state.right_shift_down = (state == driver::key_state::pressed);
                break;
            case driver::keyboard_key::left_ctrl:
                g_modifier_state.left_ctrl_down = (state == driver::key_state::pressed);
                break;
            case driver::keyboard_key::right_ctrl:
                g_modifier_state.right_ctrl_down = (state == driver::key_state::pressed);
                break;
            case driver::keyboard_key::left_alt:
                g_modifier_state.left_alt_down = (state == driver::key_state::pressed);
                break;
            case driver::keyboard_key::right_alt:
                g_modifier_state.right_alt_down = (state == driver::key_state::pressed);
                break;
            case driver::keyboard_key::caps_lock:
                if(state == driver::key_state::pressed)
                {
                    if(!g_modifier_state.caps_lock_down)
                    {
                        g_modifier_state.caps_lock_down = true;
                        g_modifier_state.caps_lock_on = !g_modifier_state.caps_lock_on;
                    }
                }
                else g_modifier_state.caps_lock_down = false;
                break;
            default:
                break;
        }
    }

    bool wait_input_buffer_clear() noexcept
    {
        for(uint32_t attempt{0}; attempt < keyboard_timeout; ++attempt)
        {
            if((kernel::inb(status_port) & input_buffer_full) == 0) return true;
            kernel::io_wait();
        }
        return false;
    }

    bool wait_output_buffer_full() noexcept
    {
        for(uint32_t attempt{0}; attempt < keyboard_timeout; ++attempt)
        {
            if((kernel::inb(status_port) & output_buffer_full) != 0) return true;
            kernel::io_wait();
        }
        return false;
    }

    bool read_keyboard_ack() noexcept
    {
        if(!wait_output_buffer_full()) return false;
        const uint8_t response{kernel::inb(data_port)};
        return response == keyboard_ack;
    }

    bool send_keyboard_byte_and_wait_ack(const uint8_t byte) noexcept
    {
        if(!wait_input_buffer_clear()) return false;
        kernel::outb(data_port, byte);
        return read_keyboard_ack();
    }

    void flush_keyboard_output_buffer() noexcept
    {
        for(uint32_t attempt{0}; attempt < keyboard_timeout; ++attempt)
        {
            if((kernel::inb(status_port) & output_buffer_full) == 0) return;
            static_cast<void>(kernel::inb(data_port));
            kernel::io_wait();
        }
    }

    constexpr uint8_t keyboard_event_queue_size{64};
    constexpr uint8_t keyboard_event_queue_mask{keyboard_event_queue_size - 1};
    static_assert((keyboard_event_queue_size & keyboard_event_queue_mask) == 0);
    struct keyboard_event_queue
    {
        driver::keyboard_event entries[keyboard_event_queue_size];
        uint32_t dropped; 
        driver::keyboard_event* volatile head;
        driver::keyboard_event* volatile tail;
        uint8_t count;

        constexpr keyboard_event_queue() noexcept: entries{}, dropped{}, head{entries}, tail{entries}, count{} {}
    };
    keyboard_event_queue g_keyboard_event_queue{};

    [[gnu::always_inline]]
    inline driver::keyboard_event* next_keyboard_event_queue_pointer(driver::keyboard_event* current) noexcept
    {
        return g_keyboard_event_queue.entries + static_cast<uint8_t>((current - g_keyboard_event_queue.entries + 1) & keyboard_event_queue_mask);
    }

    void commit_keyboard_event() noexcept
    {
        g_keyboard_event_queue.tail = next_keyboard_event_queue_pointer(g_keyboard_event_queue.tail);
        ++g_keyboard_event_queue.count;
    }
}

namespace driver
{
    bool initialize_keyboard() noexcept
    {
        g_modifier_state = driver::keyboard_modifier_state{};
        flush_keyboard_output_buffer();
        if(!send_keyboard_byte_and_wait_ack(set_leds_command)) return false;
        if(!send_keyboard_byte_and_wait_ack(all_leds_off)) return false;
        return true;
    }

    bool try_translate_text_event(const keyboard_event* event, char* out_character) noexcept
    {
        if(!is_text_input_candidate_event(event)) return false;
        const bool shift_pressed{is_shift_active(&event->modifiers)};
        const bool caps_on{is_caps_lock_active(&event->modifiers)};

        if(is_letter_key(event->key))
        {
            *out_character = shift_pressed != caps_on ? get_shifted_character(event->key) : get_normal_character(event->key);
        }
        else
        {
            *out_character = shift_pressed ? get_shifted_character(event->key) : get_normal_character(event->key);
        }
        return *out_character != '\0';
    }

    void handle_keyboard_interrupt(kernel::interrupt_frame* frame) noexcept
    {
        static_cast<void>(frame);
        const uint8_t status{kernel::inb(status_port)};
        if((status & output_buffer_full) == 0) return;

        const uint8_t scancode{kernel::inb(data_port)};
        if(scancode == extended_prefix)
        {
            g_extended_pending = true;
            return;
        }

        const bool extended{g_extended_pending};
        const uint8_t key_code{static_cast<uint8_t>(scancode & key_code_mask)};
        const keyboard_key key{map_scancode_set_1_key(key_code, extended)};
        const key_state state{(scancode & release_mask) != 0 ? key_state::released : key_state::pressed};
        g_extended_pending = false;
        update_modifier_state(key, state);

        if(g_keyboard_event_queue.count == keyboard_event_queue_size)
        {
            ++g_keyboard_event_queue.dropped;
            return;
        }

        g_keyboard_event_queue.tail->raw_scancode = scancode;
        g_keyboard_event_queue.tail->key_code = key_code;
        g_keyboard_event_queue.tail->key = key;
        g_keyboard_event_queue.tail->state = state;
        g_keyboard_event_queue.tail->extended = extended;

        g_keyboard_event_queue.tail->modifiers = g_modifier_state;
        commit_keyboard_event();
    }

    keyboard_modifier_state current_keyboard_modifier_state() noexcept
    {
        kernel::interrupt_guard guard{};
        return g_modifier_state;
    }

    bool poll_keyboard_event(keyboard_event* out_event) noexcept
    {
        kernel::interrupt_guard guard{};
        if(g_keyboard_event_queue.count == 0) return false;
        *out_event = *g_keyboard_event_queue.head;
        g_keyboard_event_queue.head = next_keyboard_event_queue_pointer(g_keyboard_event_queue.head);
        --g_keyboard_event_queue.count;
        return true;
    }

    bool has_pending_keyboard_event() noexcept
    {
        kernel::interrupt_guard guard{};
        return g_keyboard_event_queue.count != 0;
    }
    
    uint8_t pending_keyboard_event_count() noexcept
    {
        kernel::interrupt_guard guard{};
        return g_keyboard_event_queue.count;
    }

    uint32_t dropped_keyboard_event_count() noexcept
    {
        kernel::interrupt_guard guard{};
        return g_keyboard_event_queue.dropped;
    }
}