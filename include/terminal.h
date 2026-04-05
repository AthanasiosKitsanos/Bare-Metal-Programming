#pragma once

#include "vga_text_buffer.h"
#include <stddef.h>

class terminal
{
    // Private Members
    vga_text_buffer buffer;

    // Private Methods
    void new_line() noexcept
    {
        buffer.move_to_next_line();
        if(buffer.at_buffer_end()) buffer.scroll();
    }

    size_t string_length(const char* text) const noexcept
    {
        size_t length{0};
        for(; *text != '\0'; ++text) ++length;
        return length;
    }

    public:
        // Constructor
        terminal() noexcept = default;

        // Public methods
        inline __attribute__((always_inline)) void initialize() noexcept { buffer.clear(); }

        void put_char(char c) noexcept
        {
            if(c == '\n') new_line();
            else
            {
                buffer.put(c);
                if(buffer.at_buffer_end()) buffer.scroll();
            }
        }

        void write(const char* data, size_t size) noexcept
        {
            const char* const end_of_data{data + size};
            for(; data < end_of_data; ++data) put_char(*data);
        }

        void write_string(const char* text) noexcept
        {
            write(text, string_length(text));
        }
};