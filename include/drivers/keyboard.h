#pragma once

#include <stdint.h>

namespace kernel
{
    class interrupt_frame;
}

namespace driver
{
    enum class key_state: uint8_t
    {
        released,
        pressed,
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
        minus = 0x000C,
        equals = 0x000D,
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
        left_bracket = 0x001A,
        right_bracket = 0x001B,
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
        semicolon = 0x0027,
        apostrophe = 0x0028,
        backtick = 0x0029,
        left_shift = 0x002A,
        back_slash = 0x002B,
        z = 0x002C,
        x = 0x002D,
        c = 0x002E,
        v = 0x002F,
        b = 0x0030,
        n = 0x0031,
        m = 0x0032,
        comma = 0x0033,
        period = 0x0034,
        slash = 0x0035,
        right_shift = 0x0036,
        left_alt = 0x0038,
        space = 0x0039,
        caps_lock = 0x003A,
        f1 = 0x003B,
        f2 = 0x003C,
        f3 = 0x003D,
        f4 = 0x003E,
        f5 = 0x003F,
        f6 = 0x0040,
        f7 = 0x0041,
        f8 = 0x0042,
        f9 = 0x0043,
        f10 = 0x0044,
        f11 = 0x0057,
        f12 = 0x0058,
        right_ctrl = 0xE01D,
        right_alt = 0xE038, 
        home = 0xE047,
        arrow_up = 0xE048,
        page_up = 0xE049,
        arrow_left = 0xE04B,
        arrow_right = 0xE04D,
        end = 0xE04F,
        arrow_down = 0xE050,
        page_down = 0xE051,
        insert = 0xE052,
        delete_key = 0xE053,
    };

    struct keyboard_modifier_state
    {
        bool left_shift_down;
        bool right_shift_down;
        bool left_ctrl_down;
        bool right_ctrl_down;
        bool left_alt_down;
        bool left_alt_down;
        bool caps_lock_down;
        bool caps_lock_on;
    };

    struct keyboard_event
    {
        uint8_t raw_scancode;
        uint8_t key_code;
        keyboard_key key;
        key_state state;
        bool extended;
        keyboard_modifier_state modifiers;
    };

    bool initialize_keyboard() noexcept;
    void handle_keyboard_interrupt(kernel::interrupt_frame* frame) noexcept;

    bool poll_keyboard_event(keyboard_event* out_event) noexcept;
    bool has_pending_keyboard_event() noexcept;
    uint8_t pending_keyboard_event_count() noexcept;
    uint32_t dropped_keyboard_event_count() noexcept;

    bool try_translate_text_event(const keyboard_event* event, char* out_character) noexcept;

    keyboard_modifier_state current_keyboard_modifier_state() noexcept;

    // Modifier State Helpers
    [[gnu::always_inline]]
    inline bool is_shift_active(const keyboard_modifier_state* state) noexcept { return state->left_shift_down || state->right_shift_down; }

    [[gnu::always_inline]]
    inline bool is_ctrl_active(const keyboard_modifier_state* state) noexcept { return state->left_ctrl_down || state->right_ctrl_down; }

    [[gnu::always_inline]]
    inline bool is_alt_active(const keyboard_modifier_state* state) noexcept { return state->left_alt_down; }

    [[gnu::always_inline]]
    inline bool is_caps_lock_active(const keyboard_modifier_state* state) noexcept { return state->caps_lock_on; }

    // Key Classification Helper
    [[gnu::always_inline]]
    inline bool is_modifier_key(const keyboard_key key) noexcept
    {
        switch(key)
        {
            case keyboard_key::left_shift:
            case keyboard_key::right_shift:
            case keyboard_key::left_ctrl:
            case keyboard_key::left_alt:
            case keyboard_key::caps_lock:
                return true;
            default:
                return false;
        }
    }

    [[gnu::always_inline]]
    inline bool is_letter_key(const keyboard_key key) noexcept
    {
        return (key >= keyboard_key::q && key <= keyboard_key::p) || (key >= keyboard_key::a && key <= keyboard_key::l) || (key >= keyboard_key::z && key <= keyboard_key::m);
    }

    [[gnu::always_inline]]
    inline bool is_digit_key(const keyboard_key key) noexcept { return (key >= keyboard_key::digit_1 && key <= keyboard_key::digit_0); }

    [[gnu::always_inline]]
    inline bool is_space_key(const keyboard_key key) noexcept { return key == keyboard_key::space; }

    [[gnu::always_inline]]
    inline bool is_symbol_key(const keyboard_key key) noexcept
    {
        if(key >= keyboard_key::semicolon && key <= keyboard_key::backtick) return true;
        if(key >= keyboard_key::comma && key <= keyboard_key::slash) return true;
        switch(key)
        {
            case keyboard_key::minus:
            case keyboard_key::equals:
            case keyboard_key::left_bracket:
            case keyboard_key::right_bracket:
            case keyboard_key::back_slash:
                return true;
            default:
                return false;
        }
    }

    [[gnu::always_inline]]
    inline bool is_text_key(const keyboard_key key) noexcept { return is_letter_key(key) || is_digit_key(key) || is_space_key(key) || is_symbol_key(key); }

    [[gnu::always_inline]]
    inline bool is_control_key(const keyboard_key key) noexcept
    {
        switch(key)
        {
            case keyboard_key::escape:
            case keyboard_key::backspace:
            case keyboard_key::tab:
            case keyboard_key::enter:
                return true;
            default:
                return false;
        }
    }

    // Event Classification Helpers
    [[gnu::always_inline]]
    inline bool is_pressed_event(const keyboard_event* event) noexcept { return event->state == key_state::pressed; }

    [[gnu::always_inline]]
    inline bool is_released_event(const keyboard_event* event) noexcept { return event->state == key_state::released; }

    [[gnu::always_inline]]
    inline bool is_modifier_event(const keyboard_event* event) noexcept { return is_modifier_key(event->key); }

    [[gnu::always_inline]]
    inline bool is_non_modifier_event(const keyboard_event* event) noexcept { return !is_modifier_event(event); }

    [[gnu::always_inline]]
    inline bool is_non_modifier_press_event(const keyboard_event* event) noexcept { return is_pressed_event(event) && is_non_modifier_event(event); }

    [[gnu::always_inline]]
    inline bool is_known_key(const keyboard_key key) noexcept { return key != keyboard_key::unknown; }

    [[gnu::always_inline]]
    inline bool is_unknown_key(const keyboard_key key) noexcept { return !is_known_key(key); }

    [[gnu::always_inline]]
    inline bool is_known_key_event(const keyboard_event* event) noexcept { return is_known_key(event->key); }

    [[gnu::always_inline]]
    inline bool is_unknown_key_event(const keyboard_event* event) noexcept { return is_unknown_key(event->key); }

    [[gnu::always_inline]]
    inline bool is_input_candidate_event(const keyboard_event* event) noexcept
    {
        return is_pressed_event(event) && is_known_key_event(event) && is_non_modifier_event(event);
    }

    [[gnu::always_inline]]
    inline bool is_text_input_candidate_event(const keyboard_event* event) noexcept { return is_input_candidate_event(event) && is_text_key(event->key); }

    [[gnu::always_inline]]
    inline bool is_control_input_candidate_event(const keyboard_event* event) noexcept { return is_input_candidate_event(event) && is_control_key(event->key); }
}