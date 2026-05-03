#pragma once

#include <stdint.h>

namespace kernel
{
    class logger;


    enum class key_state: uint8_t
    {
        pressed,
        released
    };

    struct keyboard_event
    {
        uint8_t raw_scancode;
        uint8_t key_code;
        key_state state;
        bool extended;
        bool valid;
    };

    void set_keyboard_logger(logger* log) noexcept;
    void handle_keyboard_interrupt() noexcept;

    uint8_t last_keyboard_scancode() noexcept;
    uint32_t keyboard_event_count() noexcept;

    keyboard_event last_keyboard_event() noexcept;
    bool has_keyboard_event() noexcept;
}