#pragma once

#include <stdint.h>
#include "io/output/terminal_output.h"

namespace kernel
{
    class logger
    {
        // Private Members
        terminal::output m_terminal;

        // Private Methods
        void set_prefix_text_and_color(const char* text, vga_color foreground, vga_color background) noexcept;
        [[noreturn]] inline void __attribute__((always_inline)) halt_forever() const noexcept;
        
        public:
            // Constructor
            constexpr logger() noexcept = default;
            ~logger() noexcept = default;

            logger(const logger&) = delete;
            logger& operator=(const logger&) = delete;

            // Public Methods
            [[noreturn]] void panic(const char* panic_message) noexcept;

            // Public Inline Methods
            inline terminal::output& __attribute__((always_inline)) error() noexcept
            {
                set_prefix_text_and_color("[ERROR]: ", vga_color::light_red, vga_color::black);
                return m_terminal; 
            }

            inline terminal::output& __attribute__((always_inline)) warning() noexcept
            {
                set_prefix_text_and_color("[WARNING]: ", vga_color::yellow, vga_color::black);
                return m_terminal; 
            }

            inline terminal::output& __attribute__((always_inline)) info() noexcept
            {
                set_prefix_text_and_color("[INFO]: ", vga_color::light_cyan, vga_color::black);
                return m_terminal; 
            }

            inline terminal::output& __attribute__((always_inline)) debug() noexcept
            {
                set_prefix_text_and_color("[DEBUG]: ", vga_color::light_gray, vga_color::black);
                return m_terminal; 
            }
    };
}