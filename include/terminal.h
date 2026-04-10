#pragma once

#include "vga_text_buffer.h"
#include "vga_hardware_cursor.h"
#include <stddef.h>
#include <stdint.h>
#include "hex32.h"

class terminal
{
    // Private Members
    vga_text_buffer buffer;
    vga_hardware_cursor cursor;

    // Private Methods
    void new_line() noexcept;
    size_t string_length(const char* text) const noexcept;
    void write_unsigned_no_sync(uint32_t value) noexcept;
    void write_signed_no_sync(int32_t value) noexcept;
    void write_pointer_no_sync(uintptr_t value) noexcept;
    void write_hex_no_sync(uint32_t value) noexcept;
    void put_char_no_sync(char c) noexcept;

    // Inline Methods
    inline char __attribute__((always_inline)) hex_digit(uint8_t nibble) const noexcept
    {
        if(nibble < 10) return static_cast<char>('0' + nibble);
        return static_cast<char>('A' + (nibble - 10));
    }

    inline void __attribute__((always_inline)) put_hex_prefix() noexcept
    {
        put_char_no_sync('0');
        put_char_no_sync('x');
    }

    inline void __attribute__((always_inline)) line_start() noexcept { buffer.move_to_line_start(); }
    inline void __attribute__((always_inline)) sync_cursor() noexcept { cursor.set_position(buffer.cursor_position()); }

    public:
        // Constructor
        terminal() noexcept = default;

        // Public methods
        void initialize() noexcept;
        void write(const char* data, size_t size) noexcept;
        void write_string(const char* text) noexcept;

        // Operators
        terminal& operator<<(const char) noexcept;
        terminal& operator<<(const char*) noexcept;
        terminal& operator<<(const uint32_t) noexcept;
        terminal& operator<<(const int32_t) noexcept;
        terminal& operator<<(const bool) noexcept;
        terminal& operator<<(hex32 value) noexcept;
        terminal& operator<<(const void* ptr) noexcept;

        template<size_t N>
        terminal& operator<<(const char (&text)[N]) noexcept
        {
            size_t length{0};
            for(const char* curr{text}; length < N && *curr != '\0'; ++length)
            {
                ++curr;
            }
            write(text, length);
            return *this;
        }
};