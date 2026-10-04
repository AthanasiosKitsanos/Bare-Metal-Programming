#include "logger.h"

namespace
{
    [[noreturn]] void halt_forever() noexcept
    {
        while(true) asm volatile("cli; hlt");
    }
}

namespace kernel
{
    // Private Methods
    void logger::set_prefix_text_and_color(const char* error_type, vga_color foreground, vga_color background) noexcept
    {
        color_code temp{m_terminal.current_color_code()};
        m_terminal.set_color(foreground, background);
        m_terminal << error_type;
        m_terminal.set_color_code(temp);
    }

    // Public Methods
    [[noreturn]] void logger::panic(const char* panic_message) noexcept
    {
        m_terminal.set_color(vga_color::white, vga_color::red);
        m_terminal << "[PANIC]: " << panic_message << '\n';
        halt_forever();
    }
}