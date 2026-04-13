#include "kernel_logger.h"

kernel_logger::kernel_logger(terminal* const t) noexcept: m_terminal(t) {}

void kernel_logger::set_error_color(const char* error_type, vga_color foreground, vga_color background) noexcept
{
    color_code temp{m_terminal->current_color_code()};
    m_terminal->set_color(foreground, background);
    *m_terminal << error_type;
    m_terminal->set_color_code(temp);
}