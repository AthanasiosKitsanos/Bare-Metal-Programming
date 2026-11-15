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
        void set_prefix_text_and_color(const char* text, io::vga_color foreground, io::vga_color background) noexcept;

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
                set_prefix_text_and_color("[ERROR]: ", io::vga_color::light_red, io::vga_color::black);
                return m_terminal;
            }

            [[gnu::always_inline]]
            inline terminal::output& warning() noexcept
            {
                set_prefix_text_and_color("[WARNING]: ", io::vga_color::yellow, io::vga_color::black);
                return m_terminal;
            }

            [[gnu::always_inline]]
            inline terminal::output& info() noexcept
            {
                set_prefix_text_and_color("[INFO]: ", io::vga_color::light_cyan, io::vga_color::black);
                return m_terminal;
            }

            [[gnu::always_inline]]
            inline terminal::output& debug() noexcept
            {
                set_prefix_text_and_color("[DEBUG]: ", io::vga_color::light_gray, io::vga_color::black);
                return m_terminal;
            }
    };
}
