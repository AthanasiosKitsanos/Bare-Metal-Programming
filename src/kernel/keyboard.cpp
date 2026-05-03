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

        const keyboard_event event
        {
            scancode,
            static_cast<uint8_t>(scancode & key_code_mask),
            ((scancode & release_mask) != 0 ? key_state::released : key_state::pressed),
            g_extended_pending,
            true
        };

        g_extended_pending = false;

        g_last_event = event;
        ++g_keyboard_events;

        #ifdef KERNEL_DEBUG
            if(g_keyboard_logger)
            {
                g_keyboard_logger->info() << kernel::hex << "Keyboard event: raw=" << event.raw_scancode << " key=" << event.key_code << kernel::dec
                << " extended=" << event.extended << (event.state == key_state::pressed ? " pressed\n" : " released\n");
            }
        #endif
    }

    uint8_t last_keyboard_scancode() noexcept { return g_last_scancode; }
    uint32_t keyboard_event_count() noexcept { return g_keyboard_events; }

    keyboard_event last_keyboard_event() noexcept { return g_last_event; }
    bool has_keyboard_event() noexcept { return g_last_event.valid; }
}