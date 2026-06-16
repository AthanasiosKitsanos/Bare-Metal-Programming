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

namespace terminal
{
    class vga_text_buffer
    {
        // Private Members
        static constexpr uint8_t vga_width{80};
        static constexpr uint8_t vga_height{25};
        static constexpr uint16_t length{vga_width * vga_height};
        static_assert(length % 2 == 0);
        static_assert(vga_width % 2 == 0);
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

        // Private Methods
        // Inline Private Methods
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
        inline volatile uint16_t* cell() noexcept
        {
            uint16_t pos{get_position() * 2};
            return reinterpret_cast<volatile uint16_t*>(VGA_BASE + pos);
        }

        [[gnu::always_inline]]
        inline void clear_row() noexcept
        {
            constexpr uint8_t t_width{vga_width >> 1};
            constexpr uint16_t entry{make_entry(' ', default_color)};
            constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};

            volatile uint32_t* ptr{reinterpret_cast<volatile uint32_t*>(cell())};
            const volatile uint32_t* const end{ptr + t_width};
            for(; ptr < end; ++ptr)
            {
                *ptr = entry_32;
            }
        }

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
            [[gnu::always_inline]]
            inline void go_to_line_start() noexcept { column = 0;}

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
            inline uint16_t get_position() const noexcept
            {
                const uint8_t total_rows{(base_row + row)};
                const uint8_t physical_row{total_rows - (total_rows >= vga_height) * vga_height};
                return static_cast<uint16_t>(physical_row * vga_width + column);
            }
    };
}