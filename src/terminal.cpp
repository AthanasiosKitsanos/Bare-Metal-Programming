#include "terminal.h"

void terminal::new_line() noexcept
{
    buffer.move_to_next_line();
    if(buffer.at_buffer_end()) buffer.scroll();
}

size_t terminal::string_length(const char* text) const noexcept
{
    size_t length{0};
    for(; *text != '\0'; ++text) ++length;
    return length;
}

void terminal::put_char(char c) noexcept
{
    if(c == '\n') new_line();
    else
    {
        buffer.put(c);
        if(buffer.at_buffer_end()) buffer.scroll();
    }
}

void terminal::write(const char* data, size_t size) noexcept
{
    const char* const end_of_data{data + size};
    for(; data < end_of_data; ++data) put_char(*data);
}

void terminal::write_string(const char* text) noexcept
{
    write(text, string_length(text));
}