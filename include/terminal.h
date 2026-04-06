#pragma once

#include "vga_text_buffer.h"
#include <stddef.h>

class terminal
{
    // Private Members
    vga_text_buffer buffer;

    // Private Methods
    void new_line() noexcept;
    size_t string_length(const char* text) const noexcept;

    public:
        // Constructor
        terminal() noexcept = default;

        // Public methods
        inline __attribute__((always_inline)) void initialize() noexcept { buffer.clear(); }
        void put_char(char c) noexcept;
        void write(const char* data, size_t size) noexcept;
        void write_string(const char* text) noexcept;

};