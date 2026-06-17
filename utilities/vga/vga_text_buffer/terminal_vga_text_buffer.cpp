#include "terminal_vga_text_buffer.h"
#include "vga/vga_hardware_cursor/terminal_vga_hardware_cursor.h"

namespace terminal
{
    vga_text_buffer::vga_text_buffer() noexcept: base_row{0}, row{0}, column{0}, active_color{default_color}
    {}

    void vga_text_buffer::reset() noexcept
    {
        constexpr uint8_t dest_width{vga_width >> 1};
        volatile uint32_t* destination{begin_32() + dest_width};

        constexpr uint16_t end{(length >> 1)};
        const volatile uint32_t* buffer_end(begin_32() + end);

        for(volatile uint32_t* source{begin_32() + (base_row * dest_width)}; source < buffer_end; ++source)
        {
            *destination = *source;
            ++destination;
        }

        constexpr uint8_t vga_height_m1{vga_height - 1};
        base_row = 1;
        row = vga_height_m1;

        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry)) << 16 | entry};
        buffer_end = destination + dest_width;
        for(; destination < buffer_end; ++destination)
        {
            *destination = entry_32;
        }
    }

    void vga_text_buffer::clear() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};
        constexpr uint16_t t_length{length >> 1};
        const volatile uint32_t* const end{begin_32() + t_length};
        for(volatile uint32_t* ptr{begin_32()}; ptr < end; ++ptr)
        {
            *ptr = entry_32;
        }
        base_row = 0;
        column = 0;
        row = 0;
    }

    void vga_text_buffer::put(char c) noexcept
    {
        *cell() = make_entry(c, active_color);
        move_forward();
    }

    void vga_text_buffer::remove_last_char() noexcept
    {
        move_backwards();
        *cell() = make_entry(' ', active_color);
    }

    void vga_text_buffer::move_forward() noexcept
    {
        ++column;

        bool overflowed{column == vga_width};
        column -= overflowed * vga_width;
        row += overflowed;

        overflowed = (row == vga_height);
        base_row += overflowed;
        row -= overflowed * vga_height;

        if(overflowed)
        {
            if(base_row > vga_height) reset();
            else clear_row();
            vga_hardware_cursor::set_display_start(base_row * vga_width);
        }
    }

    void vga_text_buffer::move_backwards() noexcept
    {
        bool column_at_end{column == 0};
        column = (column - 1) + column_at_end * vga_width;

        bool row_at_end = (row == 0) * (column_at_end);
        base_row -= row_at_end;
        row = (row - column_at_end) + row_at_end * vga_height;
    }
}