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
        for(; *text != '0'; ++text) ++length;
        return length;
    }

    public:
        // Constructor
        terminal() noexcept = default;

        // Public methods
        void initialize() noexcept;
        void put_char(char c) noexcept;
        void write(const char* data, size_t size) noexcept;
        void write_string(const char* text) noexcept;
};