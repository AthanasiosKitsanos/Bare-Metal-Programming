#pragma once

#include <stdint.h>
#include <stddef.h>
#include "io/output/terminal_output.h"

namespace driver::keyboard
{
    enum class keyboard_key: uint16_t;
}

namespace terminal
{
    class input
    {
        static constexpr uint8_t input_capacity{128};
        char* cursor;
        char* data_end;
        char input_buffer[input_capacity + 1];
        output m_output;
        bool input_ready;

        [[gnu::always_inline]]
        inline bool buffer_full() const noexcept { return data_end == (input_buffer + input_capacity); }

        [[gnu::always_inline]]
        inline bool buffer_begin() const noexcept { return cursor == input_buffer; }

        [[gnu::always_inline]]
        inline bool buffer_empty() const noexcept { return data_end == input_buffer; }

        [[gnu::regparm(2)]]
        bool add_character(const char c) noexcept;

        [[gnu::regparm(2)]]
        bool add_string(const char* string) noexcept;

        [[gnu::regparm(1)]]
        bool delete_character() noexcept;

        [[gnu::always_inline]]
        inline bool move_cursor_left() noexcept
        { 
            if(cursor == input_buffer) return false;
            --cursor;
            return true;
        }

        [[gnu::always_inline]]
        inline bool move_cursor_right() noexcept
        { 
            if(cursor == data_end) return false;
            ++cursor;
            return true;
        }

        [[gnu::regparm(1)]]
        void reset_buffer() noexcept;

        [[gnu::regparm(1)]]
        void trim_end() noexcept;

        [[gnu::regparm(1)]]
        void start_data_receiving() noexcept;

        [[gnu::regparm(1)]]
        void handle_escape() noexcept;

        [[gnu::regparm(1)]]
        void handle_backspace() noexcept;

        [[gnu::regparm(1)]]
        void handle_tab() noexcept;

        [[gnu::regparm(1)]]
        void handle_enter() noexcept;

        [[gnu::regparm(2)]]
        void control_key_dispatch(const driver::keyboard::keyboard_key key) noexcept;

        [[gnu::regparm(1)]]
        void handle_home() noexcept;

        [[gnu::regparm(1)]]
        void handle_arrow_up() noexcept;

        [[gnu::regparm(1)]]
        void handle_page_up() noexcept;

        [[gnu::regparm(1)]]
        void handle_arrow_left() noexcept;

        [[gnu::regparm(1)]]
        void handle_arrow_right() noexcept;

        [[gnu::regparm(1)]]
        void handle_end() noexcept;

        [[gnu::regparm(1)]]
        void handle_arrow_down() noexcept;

        [[gnu::regparm(1)]]
        void handle_page_down() noexcept;

        [[gnu::regparm(2)]]
        void navigation_key_dispatch(const driver::keyboard::keyboard_key key) noexcept;

        public:
            input() noexcept;
            input(const input&) = delete;
            input& operator=(const input&) = delete;

            [[gnu::always_inline]]
            inline void start() noexcept { start_data_receiving(); }

            [[gnu::always_inline]]
            inline void reset() noexcept { reset_buffer(); }

            [[gnu::always_inline]]
            inline uint8_t count() const noexcept { return data_end - input_buffer; }

            [[gnu::always_inline]]
            inline const char* get_cursor() const noexcept { return cursor; }

            [[gnu::always_inline]]
            inline const char* read_buffer() const noexcept{ return input_buffer; }
    };
}