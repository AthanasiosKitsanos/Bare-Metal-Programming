#pragma once

#include <stdint.h>
#include "terminal/output.h"

namespace kernel
{
    class logger
    {
        // Private Members
        terminal::output m_terminal;

        // Private Methods
        void set_prefix_text_and_color(const char* text, vga_color foreground, vga_color background) noexcept;
        
        public:
            // Constructor
            constexpr logger() noexcept = default;
            ~logger() noexcept = default;

            logger(const logger&) = delete;
            logger& operator=(const logger&) = delete;

            // Public Methods
            [[noreturn]] void panic(const char* panic_message) noexcept;

            // Public Inline Methods
            [[gnu::always_inline]]
            inline terminal::output& error() noexcept
            {
                set_prefix_text_and_color("[ERROR]: ", vga_color::light_red, vga_color::black);
                return m_terminal; 
            }

            [[gnu::always_inline]]
            inline terminal::output& warning() noexcept
            {
                set_prefix_text_and_color("[WARNING]: ", vga_color::yellow, vga_color::black);
                return m_terminal; 
            }

            [[gnu::always_inline]]
            inline terminal::output& info() noexcept
            {
                set_prefix_text_and_color("[INFO]: ", vga_color::light_cyan, vga_color::black);
                return m_terminal; 
            }

            [[gnu::always_inline]]
            inline terminal::output& debug() noexcept
            {
                set_prefix_text_and_color("[DEBUG]: ", vga_color::light_gray, vga_color::black);
                return m_terminal; 
            }
    };
}