#pragma once

#include <stdint.h>

namespace kernel
{
    class logger;


    enum class key_state: uint8_t
    {
        not_pressed,
        pressed,
        released
    };

    enum class keyboard_key: uint16_t
    {
        unknown = 0x0000,
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

        backspace = 0x000E,
        tab = 0x000F,

        q = 0x0010,
        w = 0x0011,
        e = 0x0012,
        r = 0x0013,
        t = 0x0014,
        y = 0x0015,
        u = 0x0016,
        i = 0x0017,
        o = 0x0018,
        p = 0x0019,

        enter = 0x001C,
        left_ctrl = 0x001D,

        a = 0x001E,
        s = 0x001F,
        d = 0x0020,
        f = 0x0021,
        g = 0x0022,
        h = 0x0023,
        j = 0x0024,
        k = 0x0025,
        l = 0x0026,

        left_shift = 0x002A,

        z = 0x002C,
        x = 0x002D,
        c = 0x002E,
        v = 0x002F,
        b = 0x0030,
        n = 0x0031,
        m = 0x0032,

        right_shift = 0x0036,
        left_alt = 0x0038,
        space = 0x0039,
        caps_lock = 0x003A
    };

    struct keyboard_event
    {
        uint8_t raw_scancode;
        uint8_t key_code;
        keyboard_key key;
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