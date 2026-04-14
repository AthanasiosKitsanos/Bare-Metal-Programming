#pragma once

#include "vga_text_buffer.h"
#include "vga_hardware_cursor.h"
#include <stddef.h>
#include <stdint.h>

enum class integer_base: uint8_t
{
    dec,
    hex,
};

namespace kernel
{
    class terminal
    {
        // Private Members
        vga_text_buffer buffer;
        vga_hardware_cursor cursor;
        using terminal_manipulator = terminal& (*)(terminal&) noexcept;
        integer_base state{integer_base::dec};
        bool bool_alpha_enabled{false};

        // Private Methods
        void new_line() noexcept;
        size_t string_length(const char*) const noexcept;
        void write_unsigned_no_sync(uint32_t) noexcept;
        void write_signed_no_sync(int32_t) noexcept;
        void write_pointer_no_sync(uintptr_t) noexcept;
        void put_char_no_sync(char) noexcept;
        void write_string_no_sync(const char*) noexcept;

        template<typename T>
        void write_hex_no_sync(T value) noexcept
        {
            put_hex_prefix();
            if(value == 0)
            {
                put_char_no_sync('0');
                return;
            }
            bool started{false};
            uint8_t nibble{0};
            for(int shift{28}; shift >= 0; shift -= 4)
            {
                nibble = static_cast<uint8_t>((value >> shift) & 0x0F);
                if(!started)
                {
                    if(nibble == 0) continue;
                    started = true;
                }
                put_char_no_sync(hex_digit(nibble));
            }
        }

        // Private Friend Methods
        friend terminal& dec(terminal&) noexcept;
        friend terminal& hex(terminal&) noexcept;
        friend terminal& bool_alpha(terminal&) noexcept;
        friend terminal& bool_no_alpha(terminal&) noexcept;

        // Inline Methods
        inline char __attribute__((always_inline)) hex_digit(uint8_t nibble) const noexcept
        {
            return (nibble < 10) ? static_cast<char>('0' + nibble) : static_cast<char>('A' + (nibble - 10));
        }

        inline void __attribute__((always_inline)) put_hex_prefix() noexcept
        {
            put_char_no_sync('0');
            put_char_no_sync('x');
        }

        inline void __attribute__((always_inline)) line_start() noexcept { buffer.move_to_line_start(); }
        inline void __attribute__((always_inline)) sync_cursor() noexcept { cursor.set_position(buffer.cursor_position()); }
        
        public:
            // Constructor
            terminal() noexcept = default;

            // Public methods
            void initialize() noexcept;
            void write(const char*, size_t) noexcept;
            void write_string(const char*) noexcept;

            // Inline Public Methods
            inline void __attribute__((always_inline)) set_color(vga_color foreground, vga_color background) noexcept { buffer.set_color(foreground, background); }
            inline void __attribute__((always_inline)) set_color_code(color_code color) noexcept { buffer.set_color_code(color); }
            inline color_code __attribute__((always_inline)) current_color_code() const noexcept { return buffer.current_color_code(); }

            // Operators
            terminal& operator<<(const char) noexcept;
            terminal& operator<<(const char*) noexcept;
            terminal& operator<<(const uint32_t) noexcept;
            terminal& operator<<(const int32_t) noexcept;
            terminal& operator<<(const bool) noexcept;
            terminal& operator<<(const void*) noexcept;
            terminal& operator<<(terminal_manipulator) noexcept;

            template<size_t N>
            terminal& operator<<(const char (&text)[N]) noexcept
            {
                size_t length{0};
                for(const char* curr{text}; length < N && *curr != '\0'; ++length)
                {
                    ++curr;
                }
                write(text, length);
                sync_cursor();
                return *this;
            }
    };

    terminal& dec(terminal&) noexcept;
    terminal& hex(terminal&) noexcept;
    terminal& bool_alpha(terminal&) noexcept;
    terminal& bool_no_alpha(terminal&) noexcept;
}