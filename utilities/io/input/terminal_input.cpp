#include "terminal_input.h"

namespace
{
    [[gnu::always_inline]]
    inline void move_data_right(char* end, const char* begin) noexcept
    {
        for(; end > begin; --end) *end = *(end - 1);
    }

    [[gnu::always_inline]]
    inline void move_data_left(char* begin, const char* end) noexcept
    {
        for(; begin < end; ++begin) *begin = *(begin + 1);
    }

    [[gnu::always_inline]]
    inline uint8_t string_length(const char* string) noexcept
    {
        uint8_t length{0};
        for(; *string != '\0'; ++string) ++length;
        return length;
    }

    [[gnu::always_inline]]
    inline void copy_data(char* to_start, const char* const to_end, const char* from) noexcept
    {
        for(; to_start < to_end; ++to_start)
        {
            *to_start = *from;
            ++from;
        }
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
            move_data_right(data_end, cursor);
        }
        *cursor = c;
        ++cursor;
        ++data_end;
        return true;
    }

    bool input::add_string(const char* string) noexcept
    {
        uint8_t string_count{string_length(string)};
        if(data_end + string_count >= buffer_end) return false;
        if(*cursor != '\0')
        {
            uint8_t copy_array_size{static_cast<uint8_t>(data_end - cursor)};
            char copy_array[copy_array_size];
            copy_data(copy_array, copy_array + copy_array_size, cursor);
            copy_data(cursor + string_count, data_end + string_count, copy_array);
        }
        for(; *string != '\0'; ++string)
        {
            *cursor = *string;
            ++cursor;
        }
        data_end += string_count;
        return true;
    }

    bool input::delete_character() noexcept
    {
        if(buffer_begin()) return false;
        --cursor;
        move_data_left(cursor, data_end);
        --data_end;
        return true;
    }

    void input::reset_buffer() noexcept
    {
        cursor = command_buffer;
        data_end = command_buffer;
        for(; cursor < buffer_end; ++cursor) *cursor = '\0';
        cursor = command_buffer;
    }

    void input::go_to_last_printable_input() noexcept
    {
        --data_end;
        while(*data_end == ' ') --data_end;
        *(++data_end) = '\0';
    }
}