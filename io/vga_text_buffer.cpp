#include "vga_text_buffer.h"
#include "hardware_cursor.h"
#include "cpu/features.h"
#include "tools/builtin.h"
#include "tools/cmp.h"
#include "io/internal/vga_templates.h"

namespace
{
    template<uint32_t entry>
    [[gnu::always_inline]]
    inline void clear_row(volatile uint32_t* const cell, const uint8_t bytes) noexcept
    {
        io::vga::g_fill_dispatch<entry>[cpu::features::get()](cell, bytes);
    }
}

namespace io
{
    [[gnu::regparm(3)]] [[gnu::hot]]
    vga_state_packed vga_text_buffer::write_on_screen(vga_state_packed state_snapshot, const char *ptr, size_t length) noexcept
    {
        const uint32_t color{static_cast<uint32_t>(state_snapshot >> 16)};
        volatile uint16_t* cell_ptr{cell(state_snapshot)};
        while(length > 0)
        {
            const size_t segment_length{cmp::min_cmov(length, static_cast<size_t>(vga_width - static_cast<uint8_t>(state_snapshot)))};
            const char* const end{ptr + segment_length};
            for(; ptr < end; ++ptr)
            {
                *cell_ptr = make_entry(color, *ptr);
                ++cell_ptr;
            }
            state_snapshot += segment_length;
            if(static_cast<uint8_t>(state_snapshot) >= vga_width)
            {
                state_snapshot = next_line(state_snapshot);
                cell_ptr = cell(state_snapshot);
            }
            length -= segment_length;
        }
        return state_snapshot;
    }

    [[gnu::noinline]] [[gnu::regparm(1)]] [[gnu::cold]]
    vga_state_packed vga_text_buffer::reset(vga_state_packed s) noexcept
    {
        static_assert(__has_unique_object_representations(state_t), "state_t reordered or changed, reset it to the previous member and order state");
        // uint32_t s{builtin::convert_type<uint32_t>(l_state)};

        const volatile uint32_t* source{begin_32() + ((static_cast<uint8_t>(s >> 16) * vga_width) >> 1)};
        constexpr uint16_t dst_width{vga_width >> 1};
        volatile uint32_t* destination{begin_32() + dst_width};

        const uint8_t idx{cpu::features::get()};
        constexpr uint8_t vga_height_m1{vga_height - 1};
        constexpr size_t copy_bytes{static_cast<size_t>(vga_height_m1 * vga_width << 1)};
        destination = io::vga::g_copy_dispatch<copy_bytes>[idx](source, destination);

        constexpr uint32_t values{static_cast<uint32_t>((1 << 16) | (vga_height_m1 << 8))};
        constexpr uint32_t mask{0xFF0000FF};
        s = static_cast<uint32_t>((s & mask) | values);

        constexpr uint8_t remaining_bytes{vga_width << 1};
        io::vga::g_fill_dispatch<default_entry_32>[idx](destination, remaining_bytes);
        return s;
    }

    void vga_text_buffer::clear() noexcept
    {
        io::vga::g_fill_dispatch<default_entry_32>[cpu::features::get()](begin_32(), length);

        vga_state_packed snapshot{builtin::convert_type<vga_state_packed>(state)};
        snapshot &= ~0xFFFFFFu;
        state = builtin::convert_type<state_t>(snapshot);

        hardware_cursor::set_display_start(0);
    }

    [[gnu::regparm(2)]]
    vga_state_packed vga_text_buffer::put(vga_state_packed state_snapshot, const char c) noexcept
    {
        const uint32_t color{state_snapshot >> 16};
        *cell(state_snapshot) = make_entry(color, c);
        ++state_snapshot;
        if(static_cast<uint8_t>(state_snapshot) >= vga_width)
        {
            state_snapshot = next_line(state_snapshot);
        }
        return state_snapshot;
    }

    void vga_text_buffer::remove_last_char() noexcept
    {
        move_backwards();
        const vga_state_packed p_state{builtin::convert_type<vga_state_packed>(state)};
        *cell(p_state) = make_entry(static_cast<uint32_t>(p_state >> 16), ' ');
    }

    [[gnu::regparm(1)]]
    vga_state_packed vga_text_buffer::next_line(vga_state_packed state_snapshot) noexcept
    {
            state_snapshot &= ~0xFFu;
            state_snapshot += 0x0100u;

            if(static_cast<uint8_t>(state_snapshot >> 8) == vga_height)
            {
                state_snapshot += (0x010000u - 0x100u);

                if(static_cast<uint8_t>(state_snapshot >> 16) > base_row_max)
                {
                    state_snapshot = reset(state_snapshot);
                }
                else
                {
                    constexpr uint8_t bytes{vga_width << 1};
                    clear_row<default_entry_32>(cell_32(state_snapshot), bytes);
                }
                hardware_cursor::set_display_start(static_cast<uint8_t>(state_snapshot >> 16) * vga_width);
            }
        return state_snapshot;
    }

    [[gnu::regparm(1)]]
    vga_state_packed vga_text_buffer::move_backwards(vga_state_packed state_snapshot) noexcept
    {
        if(static_cast<uint8_t>(state_snapshot) == 0x0u)
        {
            if((state_snapshot & 0xFF00u) == 0x0u)
            {
                if((state_snapshot & 0xFF0000u) == 0x0u) return state_snapshot;
                state_snapshot -= 0x10000u;
            }
            else state_snapshot -= 0x100u;
            state_snapshot |= static_cast<vga_state_packed>(vga_width - 1);
        }
        else state_snapshot -= 0x01u;
        io::hardware_cursor::set_display_start(static_cast<uint8_t>(state_snapshot >> 16) * vga_width);
        return state_snapshot;
    }

    [[gnu::regparm(1)]]
    void vga_text_buffer::move_cursor_left_n(const uint8_t count) noexcept
    {
        int16_t abs_pos{static_cast<int16_t>(state.row * vga_width + state.column - count)};
        const uint8_t new_row{static_cast<uint8_t>(abs_pos / vga_width)};
        const uint8_t new_column{static_cast<uint8_t>(abs_pos - new_row * vga_width)};

        state.base_row -= static_cast<uint8_t>(state.row - new_row);
        state.row = new_row;
        state.column = new_column;
    }

    [[gnu::regparm(1)]]
    void vga_text_buffer::move_cursor_right_n(const uint8_t count) noexcept
    {
        int16_t abs_pos{static_cast<int16_t>(state.row * vga_width + state.column + count)};
        const uint8_t new_row{static_cast<uint8_t>(abs_pos / vga_width)};
        const uint8_t new_column{static_cast<uint8_t>(abs_pos - new_row * vga_width)};

        state.base_row -= static_cast<uint8_t>(state.row - new_row);
        state.row = new_row;
        state.column = new_column;
    }
}
