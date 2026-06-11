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

        [[gnu::always_inline]]
        inline bool buffer_empty() const noexcept { return (data_end - command_buffer) == 0; }

        [[gnu::always_inline]]
        inline bool is_cnde_synced() const noexcept { return cursor == data_end; }

        [[gnu::always_inline]]
        inline uint8_t get_distance() const noexcept { return static_cast<uint8_t>(data_end - cursor);}

        public:
            explicit input() noexcept;

            bool add_character(const char c) noexcept;

            bool add_string(const char* string) noexcept;

            bool delete_character() noexcept;

            [[gnu::always_inline]]
            const char* get_cursor() const noexcept { return cursor; }
            [[gnu::always_inline]]
            inline const char* read_buffer() const noexcept { return command_buffer; }

            [[gnu::always_inline]]
            inline uint8_t cursor_to_data_end() const noexcept {  return get_distance(); }
            
            [[gnu::always_inline]]
            inline uint8_t get_input_count() const noexcept
            {
                return static_cast<uint8_t>(data_end - command_buffer);
            }

            [[gnu::always_inline]]
            inline bool is_empty() const noexcept { return buffer_empty(); }

            [[gnu::always_inline]]
            inline bool is_buffer_synched() const noexcept { return is_cnde_synced(); }

            [[gnu::always_inline]]
            inline bool move_cursor_left() noexcept
            { 
                if(cursor == command_buffer) return false;
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

            void go_to_last_printable_input() noexcept;
    };
}