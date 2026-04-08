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
    
    inline void __attribute__((always_inline)) write_unsigned_no_sync(uint32_t value) noexcept
    {
        constexpr uint16_t count{10};
        char digits[count];
        char* end{digits + count};
        char* current{end};
        do
            {
                --current;
                *current = static_cast<char>('0' + (value % 10));
                value /= 10;
            }while(value != 0);
        for(; current < end; ++current) put_char_no_sync(*current);
    }
    
    inline void __attribute__((always_inline)) write_signed_no_sync(int32_t value) noexcept
    {
        constexpr uint16_t count{10};
        char digits[count];
        char* end{digits + count};
        char* current{end};
        if(value < 0)
        {
            put_char_no_sync('-');
            uint32_t magnitude{static_cast<uint32_t>(0) - static_cast<uint32_t>(value)};
            do
            {
                --current;
                *current = static_cast<char>('0' + (magnitude % 10));
                magnitude /= 10;
            }while(magnitude != 0);
            for(; current < end; ++current) put_char_no_sync(*current);
        }
        else
        {
            do
            {
                --current;
                *current = static_cast<char>('0' + (value % 10));
                value /= 10;
            }while(value != 0);
            for(; current < end; ++current) put_char_no_sync(*current);
        }
    }

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