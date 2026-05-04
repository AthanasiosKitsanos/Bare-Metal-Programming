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

    kernel::logger* g_keyboard_logger{nullptr};

    volatile uint8_t g_last_scancode{0};
    volatile uint32_t g_keyboard_events{0};

    volatile bool g_extended_pending{false};

    kernel::keyboard_event g_last_event{};

    kernel::keyboard_key map_scancode_set_1_key(const uint8_t key_code, const bool extended) noexcept
    {
        if(extended) return kernel::keyboard_key::unknown;
        switch(key_code)
        {
            case 0x01: return kernel::keyboard_key::escape;
            case 0x02: return kernel::keyboard_key::digit_1;
            case 0x03: return kernel::keyboard_key::digit_2;
            case 0x04: return kernel::keyboard_key::digit_3;
            case 0x05: return kernel::keyboard_key::digit_4;
            case 0x06: return kernel::keyboard_key::digit_5;
            case 0x07: return kernel::keyboard_key::digit_6;
            case 0x08: return kernel::keyboard_key::digit_7;
            case 0x09: return kernel::keyboard_key::digit_8;
            case 0x0A: return kernel::keyboard_key::digit_9;
            case 0x0B: return kernel::keyboard_key::digit_0;
            case 0x1C: return kernel::keyboard_key::enter;
            case 0x1E: return kernel::keyboard_key::a;
            case 0x2E: return kernel::keyboard_key::c;
            case 0x30: return kernel::keyboard_key::b;
            case 0x39: return kernel::keyboard_key::space;
            default:
                return kernel::keyboard_key::unknown;
        }
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