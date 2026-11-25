#include "vga_text_buffer.h"
#include "hardware_cursor.h"
#include "cpu/features.h"
#include "tools/cmp.h"
#include "io/internal/vga_templates.h"
#include "tools/builtin.h"

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
    [[REGPARM(3)]] [[gnu::hot]] [[gnu::noinline]]
    vga_state_packed vga_text_buffer::write_on_screen(vga_state_packed state_snapshot, const char *ptr, size_t length) noexcept
    {
        const uint32_t color{static_cast<uint32_t>(state_snapshot >> layout::isolate_color)};
        volatile uint16_t* cell_ptr{cell(state_snapshot)};
        while(length > 0)
        {
            const size_t segment_length{cmp::min_cmov(length, static_cast<size_t>(vga_width - static_cast<uint8_t>(state_snapshot >> layout::isolate_column)))};
            const char* const end{ptr + segment_length};
            for(; ptr < end; ++ptr)
            {
                *cell_ptr = make_entry(color, *ptr);
                ++cell_ptr;
            }
            state_snapshot += (layout::step * segment_length);
            if(static_cast<uint8_t>(state_snapshot >> layout::isolate_column) >= vga_width)
            {
                state_snapshot = next_line(state_snapshot);
                cell_ptr = cell(state_snapshot);
            }
            length -= segment_length;
        }
        return state_snapshot;
    }

    [[gnu::noinline]] [[REGPARM(1)]] [[gnu::cold]]
    vga_state_packed vga_text_buffer::reset(vga_state_packed s) noexcept
    {
        const volatile uint32_t* source{begin_32() + ((static_cast<uint8_t>(s >> layout::isolate_base_row) * vga_width) >> 1)};
        constexpr uint16_t dst_width{vga_width >> 1};
        volatile uint32_t* destination{begin_32() + dst_width};

        const uint8_t idx{cpu::features::get()};
        constexpr uint8_t vga_height_m1{vga_height - 1};
        constexpr size_t copy_bytes{static_cast<size_t>(vga_height_m1 * vga_width << 1)};
        destination = vga::g_copy_dispatch<copy_bytes>[idx](source, destination);


        constexpr vga_state_packed values{layout::inc_base_row | (layout::inc_row * vga_height_m1) | (types_t::wide ? vga_height * vga_width : 0)};
        s = (s & layout::keep_color_n_reset) | values;

        constexpr uint8_t remaining_bytes{vga_width << 1};
        vga::g_fill_dispatch<default_entry_32>[idx](destination, remaining_bytes);
        return s;
    }

    [[gnu::noinline]]
    void vga_text_buffer::clear() noexcept
    {
        constexpr uint16_t length{static_cast<uint16_t>(vga_height * vga_width) << 1};
        vga::g_fill_dispatch<default_entry_32>[cpu::features::get()](begin_32(), length);

        vga_state_packed snapshot{builtin::convert_type<vga_state_packed>(state)};
        snapshot &= layout::keep_color_n_reset;
        state = builtin::convert_type<io::types_t::state_t>(snapshot);

        hardware_cursor::set_display_start(0);
    }

    [[REGPARM(2)]] [[gnu::noinline]]
    vga_state_packed vga_text_buffer::put(vga_state_packed state_snapshot, const char c) noexcept
    {
        const uint32_t color{static_cast<uint32_t>(state_snapshot >> layout::isolate_color)};
        *cell(state_snapshot) = make_entry(color, c);
        state_snapshot += layout::step;
        if(static_cast<uint8_t>(state_snapshot >> layout::isolate_column) >= vga_width)
        {
            state_snapshot = next_line(state_snapshot);
        }
        return state_snapshot;
    }

    [[gnu::noinline]] [[gnu::cold]] [[REGPARM(1)]]
    vga_state_packed vga_text_buffer::remove_last_char(vga_state_packed state_snapshot) noexcept
    {
        if((state_snapshot & ~layout::keep_color_n_reset) != 0)
        {
            state_snapshot = move_backwards(state_snapshot);
            *cell(state_snapshot) = make_entry(static_cast<uint32_t>(state_snapshot >> layout::isolate_color), ' ');
        }
        return state_snapshot;
    }

    [[REGPARM(1)]] [[gnu::noinline]]
    vga_state_packed vga_text_buffer::next_line(vga_state_packed state_snapshot) noexcept
    {
            state_snapshot &= layout::reset_column;
            state_snapshot += layout::inc_row;

            if(static_cast<uint8_t>(state_snapshot >> layout::isolate_row) == vga_height)
            {
                state_snapshot += layout::inc_base_row_dec_row;

                if(static_cast<uint8_t>(state_snapshot >> layout::isolate_base_row) > base_row_max)
                {
                    state_snapshot = reset(state_snapshot);
                }
                else
                {
                    constexpr uint8_t bytes{vga_width << 1};
                    clear_row<default_entry_32>(cell_32(state_snapshot), bytes);
                }
                hardware_cursor::set_display_start(static_cast<uint8_t>(state_snapshot >> layout::isolate_base_row) * vga_width);
            }
        return state_snapshot;
    }

    [[REGPARM(1)]] [[gnu::noinline]]
    vga_state_packed vga_text_buffer::move_backwards(vga_state_packed state_snapshot) noexcept
    {
        constexpr vga_state_packed e{static_cast<vga_state_packed>((vga_width << layout::isolate_column) - layout::step)};
        constexpr vga_state_packed base_row{layout::inc_base_row - e};
        constexpr vga_state_packed row{layout::inc_row - e};
        if(static_cast<uint8_t>(state_snapshot >> layout::isolate_column) == 0x0u)
        {
            if(static_cast<uint8_t>(state_snapshot >> layout::isolate_row) == 0x0u)
            {
                if(static_cast<uint8_t>(state_snapshot >> layout::isolate_base_row) == 0x0u)
                {
                    return state_snapshot;
                }
                state_snapshot -= base_row;
                io::hardware_cursor::set_display_start(static_cast<uint8_t>(state_snapshot >> layout::isolate_base_row) * vga_width);
            }
            else state_snapshot -= row;
        }
        else state_snapshot -= layout::step;
        return state_snapshot;
    }

    // The destination is one that is already written
    [[REGPARM(2)]] [[gnu::noinline]]
    vga_state_packed vga_text_buffer::move_cursor_left_n(vga_state_packed state_snapshot, const uint8_t count) noexcept
    {
        uint16_t pos{get_position(state_snapshot)};
        pos = pos - cmp::min_cmov(pos, static_cast<uint16_t>(count));
        const uint8_t abs_line{static_cast<uint8_t>(pos / vga_width)};
        const uint8_t old_base_row{static_cast<uint8_t>(state_snapshot >> layout::isolate_base_row)};
        const uint8_t new_base_row{cmp::min_cmov(old_base_row, abs_line)};

        if(old_base_row != new_base_row)
        {
            hardware_cursor::set_display_start(new_base_row * vga_width);
        }

        const uint8_t new_row{static_cast<uint8_t>(abs_line - new_base_row)};
        const uint8_t new_column{static_cast<uint8_t>(pos - (abs_line * vga_width))};
        state_snapshot &= layout::keep_color_n_reset;
        state_snapshot |= (static_cast<vga_state_packed>(new_base_row) << layout::isolate_base_row);
        state_snapshot |= (static_cast<vga_state_packed>(new_row) << layout::isolate_row);
        state_snapshot |= (static_cast<vga_state_packed>(new_column) << layout::isolate_column);
        if constexpr(types_t::wide) state_snapshot |= pos;
        return state_snapshot;
    }

    // The destination is one that is already written
    [[REGPARM(2)]] [[gnu::noinline]]
    vga_state_packed vga_text_buffer::move_cursor_right_n(vga_state_packed state_snapshot, const uint8_t count) noexcept
    {
        uint16_t pos{get_position(state_snapshot)};
        constexpr uint16_t max_pos{(base_row_max + vga_height) * vga_width - 1};
        pos = pos + cmp::min_cmov(static_cast<uint16_t>(max_pos - pos), static_cast<uint16_t>(count));
        const uint8_t abs_line{static_cast<uint8_t>(pos / vga_width)};
        const uint8_t old_base_row{static_cast<uint8_t>(state_snapshot >> layout::isolate_base_row)};
        constexpr uint8_t vga_height_m1{vga_height - 1};
        const uint8_t new_base_row{static_cast<uint8_t>(cmp::max_cmov(static_cast<uint8_t>(old_base_row + vga_height_m1), abs_line) - vga_height_m1)};

        if(old_base_row != new_base_row)
        {
            hardware_cursor::set_display_start(new_base_row * vga_width);
        }

        const uint8_t new_row{static_cast<uint8_t>(abs_line - new_base_row)};
        const uint8_t new_column{static_cast<uint8_t>(pos - (abs_line * vga_width))};
        state_snapshot &= layout::keep_color_n_reset;
        state_snapshot |= (static_cast<vga_state_packed>(new_base_row) << layout::isolate_base_row);
        state_snapshot |= (static_cast<vga_state_packed>(new_row) << layout::isolate_row);
        state_snapshot |= (static_cast<vga_state_packed>(new_column) << layout::isolate_column);
        if constexpr(types_t::wide) state_snapshot |= pos;
        return state_snapshot;
    }
}
