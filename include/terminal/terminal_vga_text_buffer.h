#pragma once

#include <stdint.h>
#include <stddef.h>

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

namespace kernel
{
    class vga_text_buffer
    {
        // Private Members
        static constexpr size_t vga_width{80};
        static constexpr size_t vga_height{25};
        static constexpr uintptr_t vga_address{0xB8000};
        static constexpr color_code default_color{static_cast<color_code>(vga_color::white) | (static_cast<color_code>(vga_color::black) << 4)};
        
        volatile uint16_t* const begin;
        volatile uint16_t* const end;
        volatile uint16_t* current;
        color_code active_color;

        // Private Methods
        // Inline Private Methods
        static inline color_code __attribute__((always_inline)) make_color(vga_color foreground, vga_color background) noexcept 
        {
            return static_cast<color_code>(foreground) | (static_cast<color_code>(background) << 4);
        }

        static inline uint16_t __attribute__((always_inline)) make_entry(unsigned char c, color_code color) noexcept
        {
            return static_cast<uint16_t>(c) | static_cast<uint16_t>(color << 8);
        }

        public:
            // Constructor
            vga_text_buffer() noexcept;

            // Public Methods
            void clear() noexcept;
            void put(char c) noexcept;
            void move_to_line_start() noexcept;
            void move_to_next_line() noexcept;
            void scroll() noexcept;

            // Inline Public Methods
            inline void __attribute__((always_inline)) set_color_code(color_code color) noexcept { active_color = color; }
            inline void __attribute__((always_inline)) set_color(vga_color foreground, vga_color background) noexcept { active_color = make_color(foreground, background); }
            inline color_code __attribute__((always_inline)) current_color_code() const noexcept { return active_color; }
            inline bool __attribute__((always_inline)) at_buffer_end() const noexcept { return current == end; }
            inline size_t __attribute__((always_inline)) cursor_position() const noexcept { return static_cast<size_t>(current - begin); }
            inline color_code __attribute__((always_inline)) get_default_color_code() const noexcept { return default_color; }
    };
}