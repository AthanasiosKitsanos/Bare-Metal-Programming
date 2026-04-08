#include "terminal.h"

constexpr int32_t int_min{-2147483648};
constexpr int32_t int_max{2147483647};
constexpr uint32_t uint_max{4294967295};
constexpr const char* int_min_char{"-2147483648"};
constexpr const char* int_max_char{"2147483647"};
constexpr const char* uint_max_char{"4294967294"};

// Private Methods
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

void terminal::put_char_no_sync(char c) noexcept
{
    switch(c)
    {
        case '\r':
            line_start();
            break;
        case '\n':
            new_line();
            break;
        default:
            buffer.put(c);
            if(buffer.at_buffer_end()) buffer.scroll();
    }
}

// Public Methods
void terminal::initialize() noexcept
{
    buffer.clear();
    cursor.enable();
    sync_cursor();
}

void terminal::write(const char* data, size_t size) noexcept
{
    const char* const end_of_data{data + size};
    for(; data < end_of_data; ++data) put_char_no_sync(*data);
    sync_cursor();
}

void terminal::write_string(const char* text) noexcept
{
    write(text, string_length(text));
}

// Operators
terminal& terminal::operator<<(const char c) noexcept
{
    put_char_no_sync(c);
    sync_cursor();
    return *this;
}

terminal& terminal::operator<<(const char* text) noexcept
{
    for(char c{*text}; c != '\0'; c = *text)
    {
        put_char_no_sync(c);
        ++text;
    }
    sync_cursor();
    return *this;
}

terminal& terminal::operator<<(uint32_t value) noexcept
{
    write_unsigned_no_sync(value);
    sync_cursor();
    return *this;
}

terminal& terminal::operator<<(int32_t value) noexcept
{
    write_signed_no_sync(value);
    sync_cursor();
    return *this;
}