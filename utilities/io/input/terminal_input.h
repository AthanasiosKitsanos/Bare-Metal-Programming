#pragma once

#include <stdint.h>

namespace terminal
{
    class input
    {
        static constexpr uint8_t command_capacity{128};
        char command_buffer[command_capacity + 1];
        char* cursor;
        char* data_end;
        const char* buffer_end;

        [[gnu::always_inline]]
        inline bool buffer_full() const noexcept { return data_end == buffer_end - 1; }

        [[gnu::always_inline]]
        inline bool buffer_begin() const noexcept { return cursor == command_buffer; }

        public:
            explicit input() noexcept;

            bool add_character(const char c) noexcept;

            void delete_character() noexcept;

            [[gnu::always_inline]]
            inline const char* read_buffer() const noexcept { return command_buffer; }

            [[gnu::always_inline]]
            inline void move_cursor_left() noexcept
            { 
                if(cursor == command_buffer) return;
                --cursor;
            }

            [[gnu::always_inline]]
            inline void move_cursor_right() noexcept
            { 
                if(cursor == data_end) return;
                ++cursor;
            }

            void reset_buffer() noexcept;
    };
}