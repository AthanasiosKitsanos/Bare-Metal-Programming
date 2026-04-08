#pragma once

#include "vga_text_buffer.h"
#include "vga_hardware_cursor.h"
#include <stddef.h>

class terminal
{
    // Private Members
    vga_text_buffer buffer;
    vga_hardware_cursor cursor;

    // Private Methods
    void new_line() noexcept;
    size_t string_length(const char* text) const noexcept;

    void put_char_no_sync(char c) noexcept;
    void write_unsigned_no_sync(uint32_t value) noexcept;
    void write_signed_no_sync(int32_t value) noexcept;

    inline __attribute__((always_inline)) void line_start() noexcept { buffer.move_to_line_start(); }
    inline void __attribute__((always_inline)) sync_cursor() noexcept { cursor.set_position(buffer.cursor_position()); }

    public:
        // Constructor
        terminal() noexcept = default;

        // Public methods
        void initialize() noexcept;
        void write(const char* data, size_t size) noexcept;
        void write_string(const char* text) noexcept;

        terminal& operator<<(const char c) noexcept;
        terminal& operator<<(const char* text) noexcept;
        terminal& operator<<(const uint32_t value) noexcept;
        terminal& operator<<(const int32_t value) noexcept;
};