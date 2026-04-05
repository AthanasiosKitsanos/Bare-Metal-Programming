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

    }
    size_t strlen(const char* text) const noexcept;

    public:
        // Constructor
        terminal() noexcept;

        // Public methods
        void initialize() noexcept;
        void put_char(char c) noexcept;
        void write(const char* data, size_t size) noexcept;
        void write_string(const char* text) noexcept;
};