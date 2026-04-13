#pragma once

#include "terminal.h"
#include <stdint.h>

class kernel_logger
{
    // Private Members
    terminal* const m_terminal;

    // Privte Methods
    void set_error_color(const char* text, vga_color foreground, vga_color background) noexcept;
    
    public:
        explicit kernel_logger(terminal* const) noexcept;
        ~kernel_logger() noexcept = default;

        inline terminal& __attribute__((always_inline)) error() noexcept
        {
            set_error_color("[ERROR]: ", vga_color::light_red, vga_color::black);
            return *m_terminal; 
        }

        inline terminal& __attribute__((always_inline)) warning() noexcept
        {
            set_error_color("[WARNING]: ", vga_color::yellow, vga_color::black);
            return *m_terminal; 
        }

        inline terminal& __attribute__((always_inline)) info() noexcept
        {
            set_error_color("[INFO]: ", vga_color::light_cyan, vga_color::black);
            return *m_terminal; 
        }

        inline terminal& __attribute__((always_inline)) debug() noexcept
        {
            set_error_color("[DEBUG]: ", vga_color::light_gray, vga_color::black);
            return *m_terminal; 
        }
};