#pragma once

#include "terminal_vga_text_buffer.h"
#include "terminal_vga_hardware_cursor.h"
#include <stddef.h>
#include <stdint.h>

enum class integer_base: uint8_t
{
    dec,
    hex,
};

namespace terminal
{
    class output
    {
        // Private Members
        vga_text_buffer buffer;
        vga_hardware_cursor cursor;
        using output_manipulator = output& (*)(output&) noexcept;
        integer_base state{integer_base::dec};
        bool bool_alpha_enabled{false};

        // Private Methods
        void new_line() noexcept;
        void put_char_no_sync(char) noexcept;
        void write_string_no_sync(const char*) noexcept;

        size_t string_length(const char*) const noexcept;

        void write_unsigned_8_no_sync(uint8_t) noexcept;
        void write_signed_8_no_sync(int8_t) noexcept;
        void write_signed_16_no_sync(int16_t) noexcept;
        void write_unsigned_16_no_sync(uint16_t) noexcept;
        void write_unsigned_32_no_sync(uint32_t) noexcept;
        void write_signed_32_no_sync(int32_t) noexcept;
        void write_unsigned_64_no_sync(uint64_t) noexcept;
        void write_signed_64_no_sync(int64_t) noexcept;
        void write_pointer_no_sync(uintptr_t) noexcept;

        void write_hex_8_no_sync(uint8_t) noexcept;
        void write_hex_16_no_sync(uint16_t) noexcept;
        void write_hex_32_no_sync(uint32_t) noexcept;
        void write_hex_64_no_sync(uint64_t) noexcept;

        // Private Friend Methods
        friend output& dec(output&) noexcept;
        friend output& hex(output&) noexcept;
        friend output& bool_alpha(output&) noexcept;
        friend output& bool_no_alpha(output&) noexcept;

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
            output() noexcept = default;

            // Public methods
            void initialize() noexcept;
            void write(const char*, size_t) noexcept;
            void write_string(const char*) noexcept;

            vga_text_buffer* get_vga_buffer() noexcept;

            // Inline Public Methods
            inline void __attribute__((always_inline)) reset_color() noexcept { set_color_code(buffer.get_default_color_code()); }
            inline void __attribute__((always_inline)) set_color(vga_color foreground, vga_color background) noexcept { buffer.set_color(foreground, background); }
            inline void __attribute__((always_inline)) set_color_code(color_code color) noexcept { buffer.set_color_code(color); }
            inline color_code __attribute__((always_inline)) current_color_code() const noexcept { return buffer.current_color_code(); }

            inline void __attribute__((always_inline)) delete_last_char() noexcept
            {
                buffer.remove_last_char();
                sync_cursor();
            }

            inline bool __attribute__((always_inline)) in_default_color() const noexcept
            {
                return current_color_code() == buffer.get_default_color_code();
            }

            // Operators
            output& operator<<(const char) noexcept;
            output& operator<<(const char*) noexcept;
            output& operator<<(const uint8_t) noexcept;
            output& operator<<(const int8_t) noexcept;
            output& operator<<(const uint16_t) noexcept;
            output& operator<<(const int16_t) noexcept;
            output& operator<<(const uint32_t) noexcept;
            output& operator<<(const int32_t) noexcept;
            output& operator<<(const uint64_t) noexcept;
            output& operator<<(const int64_t) noexcept;
            output& operator<<(const bool) noexcept;
            output& operator<<(const void*) noexcept;
            output& operator<<(output_manipulator) noexcept;

            // Templates
            template<size_t N>
            output& operator<<(const char (&text)[N]) noexcept
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

            template<size_t N>
            output& operator<<(const uint8_t (&value)[N]) noexcept
            {
                const uint8_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const uint8_t* curr{value}; curr < end; ++curr) write_unsigned_8_no_sync(*curr);
                        break;
                    case integer_base::hex:
                        for(const uint8_t* curr{value}; curr < end; ++curr) write_hex_8_no_sync(*curr);
                        break;
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const int8_t (&value)[N]) noexcept
            {
                const int8_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const int8_t* curr{value}; curr < end; ++curr) write_signed_8_no_sync(*curr);
                        break;
                    case integer_base::hex:
                    {
                        int8_t temp{0};
                        for(const int8_t* curr{value}; curr < end; ++curr)
                        {
                            temp = *curr;
                            if(temp < 0)
                            {
                                put_char_no_sync('-');
                                write_hex_8_no_sync(static_cast<uint8_t>(0) - static_cast<uint8_t>(temp));
                                continue;
                            }
                            write_hex_8_no_sync(static_cast<uint8_t>(temp));
                        }
                        break;
                    }
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const uint16_t (&value)[N]) noexcept
            {
                const uint16_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const uint16_t* curr{value}; curr < end; ++curr) write_unsigned_16_no_sync(*curr);
                        break;
                    case integer_base::hex:
                        for(const uint16_t* curr{value}; curr < end; ++curr) write_hex_16_no_sync(*curr);
                        break;
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const int16_t (&value)[N]) noexcept
            {
                const int16_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const int16_t* curr{value}; curr < end; ++curr) write_signed_16_no_sync(*curr);
                        break;
                    case integer_base::hex:
                    {
                        int16_t temp{0};
                        for(const int16_t* curr{value}; curr < end; ++curr)
                        {
                            temp = *curr;
                            if(temp < 0)
                            {
                                put_char_no_sync('-');
                                write_hex_16_no_sync(static_cast<uint16_t>(0) - static_cast<uint16_t>(temp));
                                continue;
                            }
                            write_hex_16_no_sync(static_cast<uint16_t>(temp));
                        }
                        break;
                    }
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const uint32_t (&value)[N]) noexcept
            {
                const uint32_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const uint32_t* curr{value}; curr < end; ++curr) write_unsigned_32_no_sync(*curr);
                        break;
                    case integer_base::hex:
                        for(const uint32_t* curr{value}; curr < end; ++curr) write_hex_32_no_sync(*curr);
                        break;
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const int32_t (&value)[N]) noexcept
            {
                const int32_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const int32_t* curr{value}; curr < end; ++curr) write_signed_32_no_sync(*curr);
                        break;
                    case integer_base::hex:
                    {
                        int32_t temp{0};
                        for(const int32_t* curr{value}; curr < end; ++curr)
                        {
                            temp = *curr;
                            if(temp < 0)
                            {
                                put_char_no_sync('-');
                                write_hex_32_no_sync(static_cast<uint32_t>(0) - static_cast<uint32_t>(temp));
                                continue;
                            }
                            write_hex_32_no_sync(static_cast<uint32_t>(temp));
                        }
                        break;
                    }
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const uint64_t (&value)[N]) noexcept
            {
                const uint64_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const uint64_t* curr{value}; curr < end; ++curr) write_unsigned_64_no_sync(*curr);
                        break;
                    case integer_base::hex:
                        for(const uint64_t* curr{value}; curr < end; ++curr) write_hex_64_no_sync(*curr);
                        break;
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const int64_t (&value)[N]) noexcept
            {
                const int64_t* const end{value + N};
                switch(state)
                {
                    case integer_base::dec:
                        for(const int64_t* curr{value}; curr < end; ++curr) write_signed_64_no_sync(*curr);
                        break;
                    case integer_base::hex:
                    {
                        int64_t temp{0};
                        for(const int64_t* curr{value}; curr < end; ++curr)
                        {
                            temp = *curr;
                            if(temp < 0)
                            {
                                put_char_no_sync('-');
                                write_hex_64_no_sync(static_cast<uint64_t>(0) - static_cast<uint64_t>(temp));
                                continue;
                            }
                            write_hex_64_no_sync(static_cast<uint64_t>(temp));
                        }
                        break;
                    }
                }
                sync_cursor();
                return *this;
            }

            template<size_t N>
            output& operator<<(const bool (&array)[N]) noexcept
            {
                const bool* const end{array + N};
                for(const bool* curr{array}; curr < end; ++curr)
                {
                    if(bool_alpha_enabled) write_string_no_sync(static_cast<const char*>(*curr ? "true" : "false"));
                    else put_char_no_sync('0' + *curr);
                }
                sync_cursor();
                return *this;
            }
    };

    // Free Functions
    output& dec(output&) noexcept;
    output& hex(output&) noexcept;
    output& bool_alpha(output&) noexcept;
    output& bool_no_alpha(output&) noexcept;
}