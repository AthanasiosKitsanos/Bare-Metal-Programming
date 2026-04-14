#pragma once

#include "terminal.h"
#include <stdint.h>

namespace kernel
{
    class logger
    {
        // Private Members
        terminal* const m_terminal;

        // Private Methods
        void set_prefix_text_and_color(const char* text, vga_color foreground, vga_color background) noexcept;
        [[noreturn]] inline void __attribute__((always_inline)) halt_forever() const noexcept;
        
        public:
            // Constructor
            explicit logger(terminal* const) noexcept;
            ~logger() noexcept = default;

            // Public Methods
            [[noreturn]] void panic(const char* panic_message) noexcept;

            // Public Inline Methods
            inline terminal& __attribute__((always_inline)) error() noexcept
            {
                set_prefix_text_and_color("[ERROR]: ", vga_color::light_red, vga_color::black);
                return *m_terminal; 
            }

            inline terminal& __attribute__((always_inline)) warning() noexcept
            {
                set_prefix_text_and_color("[WARNING]: ", vga_color::yellow, vga_color::black);
                return *m_terminal; 
            }

            inline terminal& __attribute__((always_inline)) info() noexcept
            {
                set_prefix_text_and_color("[INFO]: ", vga_color::light_cyan, vga_color::black);
                return *m_terminal; 
            }

            inline terminal& __attribute__((always_inline)) debug() noexcept
            {
                set_prefix_text_and_color("[DEBUG]: ", vga_color::light_gray, vga_color::black);
                return *m_terminal; 
            }
    };
}