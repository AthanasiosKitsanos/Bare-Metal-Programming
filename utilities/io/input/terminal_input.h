#pragma once

#include <stdint.h>
#include <stddef.h>

namespace driver::keyboard
{
    enum class keyboard_key: uint16_t;
}

namespace terminal
{
    class output;

    class input
    {
        static constexpr uint8_t input_capacity{128};
        output* const m_output;
        char* cursor;
        char* data_end;
        char input_buffer[input_capacity + 1];
        bool input_ready;

        [[gnu::always_inline]]
        inline bool buffer_full() const noexcept { return data_end == (input_buffer + input_capacity); }

        [[gnu::always_inline]]
        inline bool buffer_begin() const noexcept { return cursor == input_buffer; }

        [[gnu::always_inline]]
        inline bool buffer_empty() const noexcept { return data_end == input_buffer; }

        bool add_character(const char c) noexcept;

        bool add_string(const char* string) noexcept;

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

        void reset_buffer() noexcept;

        void trim_end() noexcept;

        void start_data_receiving() noexcept;

        void handle_escape() noexcept;
        void handle_backspace() noexcept;
        void handle_tab() noexcept;
        void handle_enter() noexcept;
        void control_key_dispatch(const driver::keyboard::keyboard_key key) noexcept;

        void handle_home() noexcept;
        void handle_arrow_up() noexcept;
        void handle_page_up() noexcept;
        void handle_arrow_left() noexcept;
        void handle_arrow_right() noexcept;
        void handle_end() noexcept;
        void handle_arrow_down() noexcept;
        void handle_page_down() noexcept;
        void navigation_key_dispatch(const driver::keyboard::keyboard_key key) noexcept;

        public:
            explicit input(output* out) noexcept;

            [[gnu::always_inline]]
            inline void start() noexcept { start_data_receiving(); }

            [[gnu::always_inline]]
            inline void reset() noexcept { reset_buffer(); }

            [[gnu::always_inline]]
            uint8_t count() const noexcept { return data_end - input_buffer; }

            [[gnu::always_inline]]
            inline const char* get_cursor() const noexcept { return cursor; }

            [[gnu::always_inline]]
            inline const char* read_buffer() const noexcept{ return input_buffer; }
    };
}