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

    enum class keyboard_key: uint16_t
    {
        unknown = 0,
        escape = 0x0001,
        digit_1 = 0x0002,
        digit_2 = 0x0003,
        digit_3 = 0x0004,
        digit_4 = 0x0005,
        digit_5 = 0x0006,
        digit_6 = 0x0007,
        digit_7 = 0x0008,
        digit_8 = 0x0009,
        digit_9 = 0x000A,
        digit_0 = 0x000B,
        enter = 0x001C,
        space = 0x0039,
        a = 0x001E,
        b = 0x0030,
        c = 0x002E

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