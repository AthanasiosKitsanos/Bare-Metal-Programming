#include "kernel_logger.h"

namespace kernel
{
    // Constructor
    logger::logger(terminal* const t) noexcept: m_terminal(t) {}

    // Private Methods
    void logger::set_prefix_text_and_color(const char* error_type, vga_color foreground, vga_color background) noexcept
    {
        color_code temp{m_terminal->current_color_code()};
        m_terminal->set_color(foreground, background);
        *m_terminal << error_type;
        m_terminal->set_color_code(temp);
    }

    [[noreturn]] void logger::halt_forever() const noexcept
    {
        while(true) asm volatile("cli; hlt");
    }

    // Public Methods
    [[noreturn]] void logger::panic(const char* panic_message) noexcept
    {
        m_terminal->set_color(vga_color::white, vga_color::red);
        *m_terminal << "[PANIC]: " << panic_message << '\n';
        halt_forever();
    }
}