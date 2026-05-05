#pragma once

#include <stdint.h>

#define KEY_LIST    \
    X(escape , 0x0001)  \
    X(digit_1, 0x0002)  \
    X(digit_2, 0x0003)  \
    X(digit_3, 0x0004)  \
    X(digit_4, 0x0005)  \
    X(digit_5, 0x0006)  \
    X(digit_6, 0x0007)  \
    X(digit_7, 0x0008)  \
    X(digit_8,0x0009)   \
    X(digit_9, 0x000A)  \
    X(digit_0, 0x000B)  \
    X(backspace, 0x000E)    \
    X(tab, 0x000F)  \
    X(q, 0x0010)    \
    X(w, 0x0011)    \
    X(e, 0x0012)    \
    X(r, 0x0013)    \
    X(t, 0x0014)    \
    X(y, 0x0015)    \
    X(u, 0x0016)    \
    X(i, 0x0017)    \
    X(o, 0x0018)    \
    X(p, 0x0019)    \
    X(enter, 0x001C)    \
    X(left_ctrl, 0x001D)    \
    X(a, 0x001E)    \
    X(s, 0x001F)    \
    X(d, 0x0020)    \
    X(f, 0x0021)    \
    X(g, 0x0022)    \
    X(h, 0x0023)    \
    X(j, 0x0024)    \
    X(k, 0x0025)    \
    X(l, 0x0026)    \
    X(left_shift, 0x002A)   \
    X(z, 0x002C)    \
    X(x, 0x002D)    \
    X(c, 0x002E)    \
    X(v, 0x002F)    \
    X(b, 0x0030)    \
    X(n, 0x0031)    \
    X(m, 0x0032)    \
    X(right_shift, 0x0036)  \
    X(left_alt, 0x0038)    \
    X(space, 0x0039)    \
    X(caps_lock, 0x003A)    

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
        #define X(key, key_code)    \
            key = key_code,    \
        KEY_LIST
        #undef X
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