#pragma once

#include <stdint.h>
#include <stddef.h>

namespace terminal { class output; }

namespace io
{
    using color_code = uint8_t;

    static_assert(sizeof(color_code) == 1, "color_code must be exactly 1 byte");
    static_assert(sizeof(uint16_t) == 2, "VGA Text entry must be exactly 2 bytes");

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

    class vga_text_buffer
    {
        friend terminal::output;

        // Private Members
        static constexpr uint8_t vga_width{80};
        static constexpr uint8_t vga_height{25};
        static constexpr uint8_t base_row_max{179};
        static constexpr uint16_t length{(vga_width * vga_height) << 1};
        static constexpr uintptr_t VGA_BASE{0xB8000};
        static constexpr color_code default_color{static_cast<color_code>(vga_color::white) | (static_cast<color_code>(vga_color::black) << 4)};

        struct alignas(4) state_t
        {
            uint8_t column;
            uint8_t row;
            uint8_t base_row;
            color_code active_color;
        };
        static_assert(sizeof(state_t) == 4, "state_t struct: Size must stay at 4 bytes");
        static_assert(alignof(state_t) == 4, "state_t struct: Alignment must stay at 4, to make sure it always fits into a cache line.");

        inline static constinit state_t state{.column = 0, .row = 0, .base_row = 0, .active_color = default_color};

        static void clear_row() noexcept;
        static void clear() noexcept;
        static void move_forward() noexcept;
        static void remove_last_char() noexcept;
        static void move_backwards() noexcept;
        static state_t move_to_next_line() noexcept;

        [[gnu::regparm(1)]]
        static uint32_t reset(state_t state) noexcept;

        [[gnu::regparm(1)]]
        static void put(char c) noexcept;

        [[gnu::regparm(1)]]
        static void move_cursor_left_n(const uint8_t) noexcept;

        [[gnu::regparm(1)]]
        static void move_cursor_right_n(const uint8_t) noexcept;

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
        inline static uint16_t get_position() noexcept { return static_cast<uint16_t>((state.base_row + state.row) * vga_width + state.column); }

        [[gnu::always_inline]]
        inline static volatile uint16_t* cell() noexcept { return reinterpret_cast<volatile uint16_t*>(VGA_BASE + (get_position() << 1)); }

        [[gnu::always_inline]]
        inline static volatile uint32_t* cell_32() noexcept { return reinterpret_cast<volatile uint32_t*>(VGA_BASE + (get_position() << 1)); }


        [[gnu::always_inline]]
        inline static void line_start() noexcept { state.column = 0; }

        [[gnu::always_inline]]
        inline static void set_color_code(color_code color) noexcept { state.active_color = color; }

        [[gnu::always_inline]]
        inline static void set_color(vga_color foreground, vga_color background) noexcept { state.active_color = make_color(foreground, background); }

        [[gnu::always_inline]]
        inline static color_code current_color_code() noexcept { return state.active_color; }

        [[gnu::always_inline]]
        inline static bool at_buffer_end() noexcept { return state.row >= vga_height; }

        [[gnu::always_inline]]
        inline static color_code get_default_color_code() noexcept { return default_color; }

        public:
            vga_text_buffer() = delete;
    };
}
