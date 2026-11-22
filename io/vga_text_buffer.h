#pragma once

#include "tools/types.h"
#include "tools/arch.h"
#include "io/internal/vga_types.h"
#include "tools/builtin.h"

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
        static constexpr uint16_t default_entry{static_cast<uint16_t>((default_color << 8) | ' ')};
        static constexpr uint32_t default_entry_32{static_cast<uint32_t>((default_entry << 16) | default_entry)};

        inline static constinit io::types_t::state_t state{.index{}, .column = 0, .row = 0, .base_row = 0, .active_color = default_color, .reserved{}};

        static void clear() noexcept;
        static void remove_last_char() noexcept;

        [[REGPARM(1)]]
        static vga_state_packed move_backwards(vga_state_packed snapshot) noexcept;

        [[REGPARM(1)]]
        static vga_state_packed next_line(vga_state_packed snapshot) noexcept;

        // No inline to avoid inlining inside a hotpath and taking more bytes in L1i  cache lines
        // Called about once per 14400 characters and more often when lines are cut short by '\n', hence cold.
        [[gnu::noinline]] [[REGPARM(1)]] [[gnu::cold]]
        static vga_state_packed reset(vga_state_packed) noexcept;

        [[REGPARM(3)]] [[gnu::hot]]
        static vga_state_packed write_on_screen(vga_state_packed state_snapshot, const char* ptr, size_t length) noexcept;

        [[REGPARM(2)]]
        static vga_state_packed put(vga_state_packed state_snapshot, const char c) noexcept;

        [[REGPARM(2)]]
        static vga_state_packed move_cursor_left_n(vga_state_packed state_snapshot, const uint8_t count) noexcept;

        [[REGPARM(1)]]
        static void move_cursor_right_n(const uint8_t) noexcept;

        [[gnu::always_inline]]
        static vga_state_packed next_row(vga_state_packed state_snapshot) noexcept
        {
            if constexpr(types_t::wide)
            {
                state_snapshot += static_cast<vga_state_packed>(vga_width - static_cast<uint8_t>(state_snapshot >> layout::isolate_column));
            }
            state_snapshot = next_line(state_snapshot);
            return state_snapshot;
        }

        [[gnu::always_inline]]
        inline static volatile uint16_t* begin() noexcept { return reinterpret_cast<volatile uint16_t*>(VGA_BASE); }

        [[gnu::always_inline]]
        inline static volatile uint32_t* begin_32() noexcept { return reinterpret_cast<volatile uint32_t*>(VGA_BASE); }

        [[gnu::always_inline]]
        constexpr static inline color_code make_color(vga_color foreground, vga_color background) noexcept
        {
            return static_cast<color_code>(foreground) | (static_cast<color_code>(background) << 4);
        }

        // In case of using a uint32_t , make sure that the color is at bits 8...15,
        // since the uint32_t path uses & 0xFF00u
        template<typename T>
        [[gnu::always_inline]]
        constexpr static inline uint16_t make_entry(T color, const unsigned char c) noexcept
        {
            constexpr bool is_8_bit{types::equal<T, color_code>::value};
            static_assert(types::equal<T, uint32_t>::value || is_8_bit, "The color must to be a 32-bit or 8-bit value");
            if constexpr(is_8_bit)
            {
                return static_cast<uint16_t>((color << 8) | c);
            }
            else
            {
                color &= 0xFF00u;
                return static_cast<uint16_t>(color | c);
            }
        }

        [[gnu::always_inline]]
        inline static uint16_t get_position(const vga_state_packed s) noexcept
        {
            if constexpr(!types_t::wide)
            {
                constexpr uint32_t mask{0xFFu};

                // (base_row + row) * vga_width + column
                uint32_t pos{(s >> layout::isolate_base_row) & mask};
                pos += (s >> layout::isolate_row) & mask;
                pos *= vga_width;
                pos += static_cast<uint8_t>(s);
                return static_cast<uint16_t>(pos);
            }
            else return static_cast<uint16_t>(s);
        }

        [[gnu::always_inline]]
        inline static volatile uint16_t* cell(const vga_state_packed s) noexcept { return reinterpret_cast<volatile uint16_t*>(VGA_BASE + (get_position(s) << 1)); }

        [[gnu::always_inline]]
        inline static volatile uint32_t* cell_32(const vga_state_packed s) noexcept { return reinterpret_cast<volatile uint32_t*>(VGA_BASE + (get_position(s) << 1)); }


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
