#pragma once

#include <stdint.h>
#include <stddef.h>

class vga_text_buffer
{
    // Private Members
    volatile uint16_t* const begin;
    volatile uint16_t* const end;
    volatile uint16_t* current;
    static constexpr uint8_t black{0x0};
    static constexpr uint8_t white{0xF};
    uint8_t color;

    // Private Methods
    inline static uint8_t __attribute__((always_inline)) make_color(uint8_t foreground, uint8_t background) noexcept
    {
        return static_cast<uint8_t>(foreground | background << 4);
    }

    inline static uint16_t __attribute__((always_inline)) make_entry(unsigned char c, uint8_t color) noexcept
    {
        return static_cast<uint16_t>(c) | static_cast<uint16_t>(color << 8);
    }

    public:
        // Public Members
        static constexpr size_t vga_width{80};
        static constexpr size_t vga_height{25};
        static constexpr uintptr_t vga_address{0xB8000};

        // Constructor
        vga_text_buffer() noexcept: begin(reinterpret_cast<volatile uint16_t*>(vga_address)), end(begin + vga_width * vga_height), current(begin), color(make_color(white, black)) {}

        // Public Methods
        void clear() noexcept
        {
            if(current < end)
            {
                current = begin;
                for(; current < end; ++current) *current = make_entry(' ', color);
                current = begin;
            }
        }

        void put(char c) noexcept
        {
            if(current == end) return;
            *current = make_entry(c, color);
            ++current;
        }

        void move_to_line_start() noexcept;
        void move_to_next_line() noexcept;

        bool at_line_end() const noexcept;
        bool at_buffer_end() const noexcept;
};