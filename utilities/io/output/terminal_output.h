#pragma once

#include "vga/vga_text_buffer/terminal_vga_text_buffer.h"
#include "vga/vga_hardware_cursor/terminal_vga_hardware_cursor.h"
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
        inline static vga_text_buffer buffer;
        using output_manipulator = output& (*)(output&) noexcept [[gnu::regparm(1)]];
        integer_base state{integer_base::dec};
        bool bool_alpha_enabled{false};

        // Private Methods
        [[gnu::regparm(1)]]
        void new_line() noexcept;

        [[gnu::regparm(2)]]
        void write_string_no_sync(const char*) noexcept;

        [[gnu::regparm(2)]]
        void write_signed_8_no_sync(int8_t) noexcept;

        [[gnu::regparm(2)]]
        void write_signed_16_no_sync(int16_t) noexcept;

        [[gnu::regparm(2)]]
        void write_signed_32_no_sync(int32_t) noexcept;

        [[gnu::regparm(3)]]
        void write_signed_64_no_sync(int64_t) noexcept;

        [[gnu::regparm(2)]]
        void write_pointer_no_sync(uintptr_t) noexcept;

        template<typename T>
        void write_unsigned_no_sync(T value) noexcept
        {
            constexpr uint8_t max_digits
            {
                sizeof(T) == 1 ? 3 :
                sizeof(T) == 2 ? 5 :
                sizeof(T) == 4 ? 10 : 20
            };

            char digits[max_digits];
            char* const end{digits + max_digits};
            char* current{end};
            do
            {
                --current;
                *current = static_cast<char>('0' + (value - (value / 10) * 10));
                value /= 10;
            }while(value != 0);
            for(; current < end; ++current) put_char_no_sync(*current);
        }

        [[gnu::always_inline]]
        inline void put_char_no_sync(const char c) noexcept
        {
            switch(c)
            {
                case '\r':
                    line_start();
                    break;
                case '\n':
                    new_line();
                    break;
                default:
                    buffer.put(c);
                    
            }
        }

        [[gnu::always_inline]]
        inline static void sync_cursor() noexcept { vga_hardware_cursor::set_position(buffer.get_position()); }

        [[gnu::always_inline]]
        inline void line_start() noexcept { buffer.line_start(); }

        [[gnu::regparm(2)]]
        void write_hex_8_no_sync(uint8_t) noexcept;

        [[gnu::regparm(2)]]
        void write_hex_16_no_sync(uint16_t) noexcept;

        [[gnu::regparm(2)]]
        void write_hex_32_no_sync(uint32_t) noexcept;

        [[gnu::regparm(3)]]
        void write_hex_64_no_sync(uint64_t) noexcept;

        // Private Friend Methods
        friend output& dec(output&) noexcept [[gnu::regparm(1)]];
        friend output& hex(output&) noexcept [[gnu::regparm(1)]];
        friend output& bool_alpha(output&) noexcept [[gnu::regparm(1)]];
        friend output& bool_no_alpha(output&) noexcept [[gnu::regparm(1)]];

        // Inline Methods
        [[gnu::always_inline]]
        inline void put_hex_prefix() noexcept
        {
            put_char_no_sync('0');
            put_char_no_sync('x');
        }
        
        public:
            // Constructor
            constexpr output() noexcept = default;

            // Public methods
            static void initialize() noexcept;

            [[gnu::always_inline]]
            inline void move_cursor_left_n(const uint8_t count) noexcept { buffer.move_cursor_left_n(count); }
            
            [[gnu::always_inline]]
            inline void move_cursor_right_n(const uint8_t count) noexcept { buffer.move_cursor_right_n(count); }

            // Inline Public Methods
            [[gnu::always_inline]]
            inline void reset_color() noexcept { set_color_code(buffer.get_default_color_code()); }

            [[gnu::always_inline]]
            inline void set_color(vga_color foreground, vga_color background) noexcept { buffer.set_color(foreground, background); }

            [[gnu::always_inline]]
            inline void set_color_code(color_code color) noexcept { buffer.set_color_code(color); }

            [[gnu::always_inline]]
            inline color_code current_color_code() const noexcept { return buffer.current_color_code(); }

            [[gnu::always_inline]]
            inline void delete_last_char_no_sync() noexcept { buffer.remove_last_char(); }

            [[gnu::always_inline]]
            inline bool in_default_color() const noexcept { return current_color_code() == buffer.get_default_color_code(); }

            [[gnu::always_inline]]
            inline void print_string_no_sync(const char* string) noexcept { write_string_no_sync(string); }

            [[gnu::always_inline]]
            inline void call_cursor_sync() noexcept { sync_cursor(); }

            [[gnu::always_inline]]
            inline void go_backwards() noexcept { buffer.move_backwards(); }

            [[gnu::always_inline]]
            inline void go_forward() noexcept { buffer.move_forward(); }

            [[gnu::always_inline]]
            inline void print_char_no_sync(char c) noexcept { put_char_no_sync(c); }

            [[gnu::always_inline]]
            inline void clear() noexcept { buffer.clear(); }

            // Operators
            [[gnu::regparm(2)]]
            output& operator<<(const char) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const char*) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const uint8_t) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const int8_t) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const uint16_t) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const int16_t) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const uint32_t) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const int32_t) noexcept;

            [[gnu::regparm(3)]]
            output& operator<<(const uint64_t) noexcept;

            [[gnu::regparm(3)]]
            output& operator<<(const int64_t) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const bool) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(const void*) noexcept;

            [[gnu::regparm(2)]]
            output& operator<<(output_manipulator) noexcept;

            // Templates
            template<size_t N>
            output& operator<<(const char (&text)[N]) noexcept
            {
                write_string_no_sync(text);
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
                        for(const uint8_t* curr{value}; curr < end; ++curr) write_unsigned_no_sync(*curr);
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
                        for(const uint16_t* curr{value}; curr < end; ++curr) write_unsigned_no_sync(*curr);
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
                        for(const uint32_t* curr{value}; curr < end; ++curr) write_unsigned_no_sync(*curr);
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
                        for(const uint64_t* curr{value}; curr < end; ++curr) write_unsigned_no_sync(*curr);
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
    [[gnu::regparm(1)]]
    output& dec(output&) noexcept;

    [[gnu::regparm(1)]]
    output& hex(output&) noexcept;

    [[gnu::regparm(1)]]
    output& bool_alpha(output&) noexcept;

    [[gnu::regparm(1)]]
    output& bool_no_alpha(output&) noexcept;
}