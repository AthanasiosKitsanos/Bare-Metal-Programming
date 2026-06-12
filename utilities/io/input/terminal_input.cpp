#include "terminal_input.h"

namespace
{
    [[gnu::always_inline]]
    inline void move_data_right(char* end, const char* begin, const uint8_t step) noexcept
    {
        for(; end > begin; --end) *end = *(end - step);
    }

    [[gnu::always_inline]]
    inline void move_data_left(char* begin, const char* end, const uint8_t step) noexcept
    {
        for(; begin < end; ++begin) *begin = *(begin + step);
    }

    [[gnu::always_inline]]
    inline uint8_t string_length(const char* string) noexcept
    {
        uint8_t length{0};
        for(; *string != '\0'; ++string) ++length;
        return length;
    }
}

namespace terminal
{
    input::input(): command_buffer{}, cursor{command_buffer}, data_end{command_buffer}, buffer_end{command_buffer + command_capacity}
    {}

    bool input::add_character(const char c) noexcept
    {
        if(buffer_full()) return false;
        if(*cursor != '\0')
        {
            move_data_right(data_end, cursor, 1);
        }
        *cursor = c;
        ++cursor;
        *(++data_end) = '\0';
        return true;
    }

    bool input::add_string(const char* string) noexcept
    {
        const uint8_t length{string_length(string)};
        if(data_end + length >= buffer_end) return false;
        data_end += length;
        if(*cursor != '\0')
        {
            move_data_right(data_end, cursor + length - 1, length);   
        }
        for(; *string != '\0'; ++string)
        {
            *cursor = *string;
            ++cursor;
        }
        return true;
    }

    bool input::delete_character() noexcept
    {
        if(buffer_begin()) return false;
        --cursor;
        move_data_left(cursor, data_end, 1);
        *(--data_end) = '\0';
        return true;
    }

    void input::reset_buffer() noexcept
    {
        cursor = command_buffer;
        data_end = command_buffer;
        *cursor = '\0';
    }

    void input::go_to_last_printable_input() noexcept
    {
        --data_end;
        while(*data_end == ' ') --data_end;
        *(++data_end) = '\0';
    }
}