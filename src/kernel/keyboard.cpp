#include "kernel_pic.h"
#include "keyboard.h"
#include "kernel_logger.h"
#include "terminal_io_registers.h"
#include "kernel_keyboard_key_list.h"

#define KERNEL_KEYBOARD_DEBUG

namespace
{
    constexpr uint16_t data_port{0x60};
    constexpr uint16_t status_port{0x64};

    constexpr uint8_t output_buffer_full{0x01};
    constexpr uint8_t input_buffer_full{0x02};

    constexpr uint32_t keyboard_timeout{100000};
    constexpr uint8_t keyboard_ack{0xFA};
    constexpr uint8_t keyboard_resend{0xFE};
    constexpr uint8_t set_leds_command{0xED};
    constexpr uint8_t led_caps_lock{0x04};
    constexpr uint8_t all_leds_off{0x00};

    constexpr uint8_t release_mask{0x80};
    constexpr uint8_t key_code_mask{0x7F};
    constexpr uint8_t extended_prefix{0xE0};
    constexpr uint8_t normal_key_map_size{128};

    static_assert(static_cast<uint16_t>(kernel::keyboard_key::unknown) == 0x0000);
    static_assert(normal_key_map_size == static_cast<uint16_t>(key_code_mask) + 1);

    volatile uint8_t g_last_scancode{0};
    volatile uint32_t g_keyboard_events{0};

    volatile bool g_extended_pending{false};

    kernel::logger* g_keyboard_logger{nullptr};
    kernel::keyboard_event g_last_event{};
    kernel::keyboard_modifier_state g_modifier_state{};

    struct normal_key_map_table
    {
        kernel::keyboard_key entries[normal_key_map_size];

        constexpr normal_key_map_table(): entries{}
        {
            #define X(key, key_code)    \
                entries[key_code] = kernel::keyboard_key::key;
            KERNEL_KEYBOARD_KEY_LIST
            #undef X
        }
    };

    constexpr normal_key_map_table normal_key_map{};

    #define X(key, key_code)    \
        static_assert(normal_key_map.entries[key_code] == kernel::keyboard_key::key);
    KERNEL_KEYBOARD_KEY_LIST
    #undef X
    static_assert(normal_key_map.entries[127] == kernel::keyboard_key::unknown);

    kernel::keyboard_key map_scancode_set_1_key(const uint8_t key_code, const bool extended) noexcept
    {
        if(extended) return kernel::keyboard_key::unknown;
        return *(normal_key_map.entries + key_code);
    }

    const char* keyboard_key_name(const kernel::keyboard_key key) noexcept
    {
        switch(key)
        {
            case kernel::keyboard_key::unknown: return "Unknown";
            #define X(key, key_code)    \
                case kernel::keyboard_key::key: return #key;
            KERNEL_KEYBOARD_KEY_LIST
            #undef X

            default:
                return "Unknown";
        }
    }

    void update_modifier_state(const kernel::keyboard_event* event) noexcept
    {
        switch(event->key)
        {
            case kernel::keyboard_key::left_shift:
                g_modifier_state.left_shift_down = (event->state == kernel::key_state::pressed);
                break;
            case kernel::keyboard_key::right_shift:
                g_modifier_state.right_shift_down = (event->state == kernel::key_state::pressed);
                break;
            case kernel::keyboard_key::left_ctrl:
                g_modifier_state.left_ctrl_down = (event->state == kernel::key_state::pressed);
                break;
            case kernel::keyboard_key::left_alt:
                g_modifier_state.left_alt_down = (event->state == kernel::key_state::pressed);
                break;
            case kernel::keyboard_key::caps_lock:
                if(event->state == kernel::key_state::pressed)
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
        g_keyboard_logger->debug() << "Response: " << response << " Keyboard Ack: " << keyboard_ack << '\n';
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
}

namespace kernel
{
    void set_keyboard_logger(logger* log) noexcept { g_keyboard_logger = log; }

    bool initialize_keyboard() noexcept
    {
        g_modifier_state = kernel::keyboard_modifier_state{};
        flush_keyboard_output_buffer();
        if(!send_keyboard_byte_and_wait_ack(set_leds_command)) return false;
        if(!send_keyboard_byte_and_wait_ack(all_leds_off)) return false;
        return true;
    }

    void handle_keyboard_interrupt() noexcept
    {
        const uint8_t status{inb(status_port)};
        if((status & output_buffer_full) == 0) return;

        const uint8_t scancode{inb(data_port)};
        g_last_scancode = scancode;

        if(scancode == extended_prefix)
        {
            g_extended_pending = true;
            return;
        }

        const bool extended{g_extended_pending};
        const uint8_t key_code{static_cast<uint8_t>(scancode & key_code_mask)};
        const keyboard_key key{map_scancode_set_1_key(key_code, extended)};
        keyboard_event event
        {
            scancode,
            key_code,
            key,
            ((scancode & release_mask) != 0 ? key_state::released : key_state::pressed),
            extended,
            true,
            keyboard_modifier_state{}
        };

        g_extended_pending = false;
        update_modifier_state(&event);
        event.modifiers = g_modifier_state;
        g_last_event = event;
        ++g_keyboard_events;

        #ifdef KERNEL_KEYBOARD_DEBUG
            if(g_keyboard_logger && event.state == kernel::key_state::pressed)
            {
                g_keyboard_logger->info() << kernel::hex << "Keyboard event: raw=" << event.raw_scancode << " key=" << event.key_code << " extended=" << event.extended
                << " mapped=" << static_cast<uint32_t>(event.key) << " key_name=" << keyboard_key_name(event.key)
                << kernel::dec << (event.state == key_state::pressed ? " pressed\n" : " released\n")
                << "mod= LSHIFT:" << event.modifiers.left_shift_down << " RSHIFT:" << event.modifiers.right_shift_down
                << " LCtrl:" << event.modifiers.left_ctrl_down << " LALT:" << event.modifiers.left_alt_down
                << " CAPS_DOWN:" << event.modifiers.caps_lock_down << " CAPS_ON:" << event.modifiers.caps_lock_on << '\n'; 
            }
        #endif
    }

    uint8_t last_keyboard_scancode() noexcept { return g_last_scancode; }
    uint32_t keyboard_event_count() noexcept { return g_keyboard_events; }

    keyboard_event last_keyboard_event() noexcept { return g_last_event; }
    bool has_keyboard_event() noexcept { return g_last_event.valid; }
    keyboard_modifier_state current_keyboard_modifier_state() noexcept { return g_modifier_state; }
}