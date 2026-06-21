#pragma once

#include <stdint.h>
#include <stddef.h>

#define _MM_MALLOC_H_INCLUDED

using color_code = uint8_t;

static_assert(sizeof(color_code) == 1, "color_code must be exactly 1 byte\n");
static_assert(sizeof(uint16_t) == 2, "VGA Text entry must be exactly 2 bytes\n");

enum class vga_color: color_code
{
    black = 0x0,
    blue = 0x1,
    green = 0x2,
    cyan = 0x3,
    red = 0x4,
    magenta = 0x5,
    brown = 0x6,
    light_gray = 0x7,
    dark_gray = 0x8,
    light_blue = 0x9,
    light_green = 0xA,
    light_cyan = 0xB,
    light_red = 0xC,
    light_magenta = 0xD,
    yellow = 0xE,
    white = 0xF
};

namespace terminal
{
    class vga_text_buffer
    {
        // Private Members
        static constexpr uint8_t vga_width{80};
        static constexpr uint8_t vga_height{25};
        static constexpr uint16_t length{(vga_width * vga_height) << 1};
        static constexpr uintptr_t VGA_BASE{0xB8000};
        static constexpr color_code default_color{static_cast<color_code>(vga_color::white) | (static_cast<color_code>(vga_color::black) << 4)};

        uint8_t base_row;
        uint8_t row;
        uint8_t column;
        color_code active_color;
        
        [[gnu::always_inline]]
        inline static volatile uint16_t* begin() noexcept { return reinterpret_cast<volatile uint16_t*>(VGA_BASE); }

        [[gnu::always_inline]]
        inline static volatile uint32_t* begin_32() noexcept { return reinterpret_cast<volatile uint32_t*>(VGA_BASE); }

        [[gnu::always_inline]]
        constexpr static inline color_code make_color(vga_color foreground, vga_color background) noexcept 
        {
            return static_cast<color_code>(foreground) | (static_cast<color_code>(background) << 4);
        }

        [[gnu::always_inline]]
        constexpr static inline uint16_t make_entry(unsigned char c, color_code color) noexcept
        {
            return static_cast<uint16_t>(c) | static_cast<uint16_t>(color << 8);
        }

        [[gnu::always_inline]]
        inline volatile uint16_t* cell() noexcept { return reinterpret_cast<volatile uint16_t*>(VGA_BASE + (get_position() << 1)); }

        [[gnu::always_inline]]
        inline volatile uint32_t* cell_32() noexcept { return reinterpret_cast<volatile uint32_t*>(VGA_BASE + (get_position() << 1)); }

        void clear_row() noexcept;
        void reset() noexcept;

        public:
            // Constructor
            vga_text_buffer() noexcept;

            // Public Methods
            void clear() noexcept;
            void put(char c) noexcept;
            void remove_last_char() noexcept;
            void move_forward() noexcept;
            void move_backwards() noexcept;

            // Inline Public Methods
            void move_to_next_line() noexcept;

            [[gnu::always_inline]]
            inline void line_start() noexcept { column = 0; }

            [[gnu::always_inline]]
            inline void set_color_code(color_code color) noexcept { active_color = color; }

            [[gnu::always_inline]]
            inline void set_color(vga_color foreground, vga_color background) noexcept { active_color = make_color(foreground, background); }

            [[gnu::always_inline]]
            inline color_code current_color_code() const noexcept { return active_color; }

            [[gnu::always_inline]]
            inline bool at_buffer_end() const noexcept { return row >= vga_height; }

            [[gnu::always_inline]]
            inline color_code get_default_color_code() const noexcept { return default_color; }

            [[gnu::always_inline]]
            inline uint16_t get_position() const noexcept { return static_cast<uint16_t>((base_row + row) * vga_width + column); }
    };
}