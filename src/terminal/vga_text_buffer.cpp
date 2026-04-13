#include "vga_text_buffer.h"

vga_text_buffer::vga_text_buffer() noexcept: begin(reinterpret_cast<volatile uint16_t*>(vga_address)), end(begin + vga_width * vga_height), current(begin), color(make_color(white, black)) {}


void vga_text_buffer::clear() noexcept
{
    current = begin;
    for(; current < end; ++current) *current = make_entry(' ', color);
    current = begin;
}

void vga_text_buffer::put(char c) noexcept
{
    if(current == end) return;
    *current = make_entry(c, color);
    ++current;
}

void vga_text_buffer::move_to_line_start() noexcept
{
    if(current == end) --current;
    const size_t row{static_cast<size_t>(current - begin) / vga_width};
    current = begin + row * vga_width;
}

void vga_text_buffer::move_to_next_line() noexcept
{
    const size_t row{(static_cast<size_t>(current - begin) / vga_width) + 1};
    current = begin + row * vga_width;
}

void vga_text_buffer::scroll() noexcept
{
    current = begin;
    volatile uint16_t* source{current + vga_width};
    for(; source < end; ++source)
    {
        *current = *source;
        ++current;
    }
    for(; current < end; ++current) *current = make_entry(' ', color);
    current -= vga_width;
}