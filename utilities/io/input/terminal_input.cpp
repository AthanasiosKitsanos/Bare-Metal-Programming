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

    void input::delete_character() noexcept
    {
        if(buffer_begin()) return;
        --cursor;
        move_data_left(cursor, data_end);
        --data_end;
        *data_end = '\0';
    }

    void input::reset_buffer() noexcept
    {
        cursor = command_buffer;
        data_end = command_buffer;
        for(; cursor < buffer_end; ++cursor) *cursor = '\0';
        cursor = command_buffer;
    }
}