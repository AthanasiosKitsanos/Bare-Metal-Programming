#include "keyboard.h"
#include "kernel_logger.h"
#include "terminal_io_registers.h"

#define KERNEL_DEBUG

namespace
{
    constexpr uint16_t data_port{0x60};
    constexpr uint16_t status_port{0x64};

    constexpr uint8_t output_buffer_full{0x01};
    constexpr uint8_t release_mask{0x80};
    constexpr uint8_t key_code_mask{0x7F};
    constexpr uint8_t extended_prefix{0xE0};
    constexpr uint8_t normal_key_map_size{128};

    kernel::logger* g_keyboard_logger{nullptr};

    volatile uint8_t g_last_scancode{0};
    volatile uint32_t g_keyboard_events{0};

    volatile bool g_extended_pending{false};

    kernel::keyboard_event g_last_event{};

    static_assert(static_cast<uint16_t>(kernel::keyboard_key::unknown) == 0x0000);

    struct normal_key_map_table
    {
        kernel::keyboard_key entries[normal_key_map_size];

        constexpr normal_key_map_table(): entries{}
        {
            entries[0x01] = kernel::keyboard_key::escape;
            entries[0x02] = kernel::keyboard_key::digit_1;
            entries[0x03] = kernel::keyboard_key::digit_2;
            entries[0x04] = kernel::keyboard_key::digit_3;
            entries[0x05] = kernel::keyboard_key::digit_4;
            entries[0x06] = kernel::keyboard_key::digit_5;
            entries[0x07] = kernel::keyboard_key::digit_6;
            entries[0x08] = kernel::keyboard_key::digit_7;
            entries[0x09] = kernel::keyboard_key::digit_8;
            entries[0x0A] = kernel::keyboard_key::digit_9;
            entries[0x0B] = kernel::keyboard_key::digit_0;
            entries[0x0E] = kernel::keyboard_key::backspace;
            entries[0x0F] = kernel::keyboard_key::tab;
            entries[0x10] = kernel::keyboard_key::q;
            entries[0x11] = kernel::keyboard_key::w;
            entries[0x12] = kernel::keyboard_key::e;
            entries[0x13] = kernel::keyboard_key::r;
            entries[0x14] = kernel::keyboard_key::t;
            entries[0x15] = kernel::keyboard_key::y;
            entries[0x16] = kernel::keyboard_key::u;
            entries[0x17] = kernel::keyboard_key::i;
            entries[0x18] = kernel::keyboard_key::o;
            entries[0x19] = kernel::keyboard_key::p;
            entries[0x1C] = kernel::keyboard_key::enter;
            entries[0x1D] = kernel::keyboard_key::left_ctrl;
            entries[0x1E] = kernel::keyboard_key::a;
            entries[0x1F] = kernel::keyboard_key::s;
            entries[0x20] = kernel::keyboard_key::d;
            entries[0x21] = kernel::keyboard_key::f;
            entries[0x22] = kernel::keyboard_key::g;
            entries[0x23] = kernel::keyboard_key::h;
            entries[0x24] = kernel::keyboard_key::j;
            entries[0x25] = kernel::keyboard_key::k;
            entries[0x26] = kernel::keyboard_key::l;
            entries[0x2A] = kernel::keyboard_key::left_shift;
            entries[0x2C] = kernel::keyboard_key::z;
            entries[0x2D] = kernel::keyboard_key::x;
            entries[0x2E] = kernel::keyboard_key::c;
            entries[0x2F] = kernel::keyboard_key::v;
            entries[0x30] = kernel::keyboard_key::b;
            entries[0x31] = kernel::keyboard_key::n;
            entries[0x32] = kernel::keyboard_key::m;
            entries[0x36] = kernel::keyboard_key::right_shift;
            entries[0x38] = kernel::keyboard_key::left_alt;
            entries[0x39] = kernel::keyboard_key::space;
            entries[0x3A] = kernel::keyboard_key::caps_lock;
        }
    };

    constexpr normal_key_map_table normal_key_map{};

    kernel::keyboard_key map_scancode_set_1_key(const uint8_t key_code, const bool extended) noexcept
    {
        if(extended) return kernel::keyboard_key::unknown;
        return *(normal_key_map.entries + key_code);
    }
}

namespace kernel
{
    void set_keyboard_logger(logger* log) noexcept { g_keyboard_logger = log; }

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
        const keyboard_event event
        {
            scancode,
            key_code,
            key,
            ((scancode & release_mask) != 0 ? key_state::released : key_state::pressed),
            extended,
            true
        };

        g_extended_pending = false;

        g_last_event = event;
        ++g_keyboard_events;

        #ifdef KERNEL_DEBUG
            if(g_keyboard_logger)
            {
                g_keyboard_logger->info() << kernel::hex << "Keyboard event: raw=" << event.raw_scancode << " key=" << event.key_code << " extended=" << event.extended
                << " mapped=" << static_cast<uint32_t>(event.key) << (event.state == key_state::pressed ? " pressed\n" : " released\n") << kernel::dec;
            }
        #endif
    }

    uint8_t last_keyboard_scancode() noexcept { return g_last_scancode; }
    uint32_t keyboard_event_count() noexcept { return g_keyboard_events; }

    keyboard_event last_keyboard_event() noexcept { return g_last_event; }
    bool has_keyboard_event() noexcept { return g_last_event.valid; }
}