#include "terminal_output.h"

namespace terminal
{
    // Private Methods
    void output::new_line() noexcept
    {
        buffer.move_to_next_line();
        if(buffer.at_buffer_end()) buffer.scroll();
    }

    size_t output::string_length(const char* text) const noexcept
    {
        size_t length{0};
        for(; *text != '\0'; ++text) ++length;
        return length;
    }

    void output::write_string_no_sync(const char* text) noexcept
    {
        for(char c{*text}; c != '\0'; c = *text)
        {
            put_char_no_sync(c);
            ++text;
        }
    }

    void output::put_char_no_sync(char c) noexcept
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
                if(buffer.at_buffer_end()) buffer.scroll();
        }
    }

    void output::write_pointer_no_sync(uintptr_t value) noexcept
    {
        put_hex_prefix();
        constexpr size_t total_nibbles{sizeof(uintptr_t) * 2};
        size_t shift{0};
        uint8_t nibble{0};
        for(size_t i{0}; i < total_nibbles; ++i)
        {
            shift = (total_nibbles - 1 - i) * 4;
            nibble = static_cast<uint8_t>((value >> shift) & static_cast<uintptr_t>(0x0F));
            put_char_no_sync(hex_digit(nibble));
        }
    }

    void output::write_signed_8_no_sync(int8_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_8_no_sync(static_cast<uint8_t>(0) - static_cast<uint8_t>(value));
            return;
        }
        write_unsigned_32_no_sync(static_cast<uint8_t>(value));
    }

    void output::write_unsigned_8_no_sync(uint8_t value) noexcept
    {
        constexpr uint8_t count{3};
        char digits[count];
        char* end{digits + count};
        char* current{end};
        do
        {
            --current;
            *current = static_cast<char>('0' + (value % 10));
            value /= 10;
        }while(value != 0);
        for(; current < end; ++current) put_char_no_sync(*current);
    }

    void output::write_signed_16_no_sync(int16_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_16_no_sync(static_cast<uint16_t>(0) - static_cast<uint16_t>(value));
            return;
        }
        write_unsigned_16_no_sync(static_cast<uint16_t>(value));
    }

    void output::write_unsigned_16_no_sync(uint16_t value) noexcept
    {
        constexpr uint16_t count{5};
        char digits[count];
        char* end{digits + count};
        char* current{end};
        do
        {
            --current;
            *current = static_cast<char>('0' + (value % 10));
            value /= 10;
        }while(value != 0);
        for(; current < end; ++current) put_char_no_sync(*current);
    }

    void output::write_signed_32_no_sync(int32_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_32_no_sync(static_cast<uint32_t>(0) - static_cast<uint32_t>(value));
            return;
        }
        write_unsigned_32_no_sync(static_cast<uint32_t>(value));
    }

    void output::write_unsigned_32_no_sync(uint32_t value) noexcept
    {
        constexpr uint16_t count{10};
        char digits[count];
        char* end{digits + count};
        char* current{end};
        do
        {
            --current;
            *current = static_cast<char>('0' + (value % 10));
            value /= 10;
        }while(value != 0);
        for(; current < end; ++current) put_char_no_sync(*current);
    }

    void output::write_signed_64_no_sync(int64_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_64_no_sync(static_cast<uint64_t>(0) - static_cast<uint64_t>(value));
            return;
        }
        write_unsigned_64_no_sync(static_cast<uint64_t>(value));
    }

    void output::write_unsigned_64_no_sync(uint64_t value) noexcept
    {
        constexpr uint16_t count{20};
        char digits[count];
        char* end{digits + count};
        char* current{end};
        do
        {
            --current;
            *current = static_cast<char>('0' + (value % 10));
            value /= 10;
        }while(value != 0);
        for(; current < end; ++current) put_char_no_sync(*current);
    }

    void output::write_hex_8_no_sync(uint8_t value) noexcept
    {
        put_hex_prefix();
        if(value == 0)
        {
            put_char_no_sync('0');
            return;
        }
        bool started{false};
        uint8_t nibble{0};
        constexpr int initial_shift{sizeof(uint8_t) * 8 - 4};
        int shift{initial_shift};
        for(; shift >= 0; shift -= 4)
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

    void output::write_hex_16_no_sync(uint16_t value) noexcept
    {
        put_hex_prefix();
        if(value == 0)
        {
            put_char_no_sync('0');
            return;
        }
        bool started{false};
        uint8_t nibble{0};
        constexpr int initial_shift{sizeof(uint16_t) * 8 - 4};
        int shift{initial_shift};
        for(; shift >= 0; shift -= 4)
        {
            nibble = static_cast<uint16_t>((value >> shift) & 0x0F);
            if(!started)
            {
                if(nibble == 0) continue;
                started = true;
            }
            put_char_no_sync(hex_digit(nibble));
        }
    }

    void output::write_hex_32_no_sync(uint32_t value) noexcept
        {
            put_hex_prefix();
            if(value == 0)
            {
                put_char_no_sync('0');
                return;
            }
            bool started{false};
            uint8_t nibble{0};
            constexpr int initial_shift{sizeof(uint32_t) * 8 - 4};
            int shift{initial_shift};
            for(; shift >= 0; shift -= 4)
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

        void output::write_hex_64_no_sync(uint64_t value) noexcept
        {
            put_hex_prefix();
            if(value == 0)
            {
                put_char_no_sync('0');
                return;
            }
            bool started{false};
            uint8_t nibble{0};
            constexpr int initial_shift{sizeof(uint64_t) * 8 - 4};
            int shift{initial_shift};
            for(; shift >= 0; shift -= 4)
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

    // Public Methods
    void output::initialize() noexcept
    {
        buffer.clear();
        cursor.enable();
        sync_cursor();
    }

    void output::write(const char* data, size_t size) noexcept
    {
        const char* const end_of_data{data + size};
        for(; data < end_of_data; ++data) put_char_no_sync(*data);
    }

    void output::write_string(const char* text) noexcept
    {
        write(text, string_length(text));
    }

    // Operators
    output& output::operator<<(const char c) noexcept
    {
        put_char_no_sync(c);
        sync_cursor();
        return *this;
    }

    output& output::operator<<(const char* text) noexcept
    {
        write_string_no_sync(text);
        sync_cursor();
        return *this;
    }

    output& output::operator<<(uint8_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_unsigned_8_no_sync(value);
                break;
            case integer_base::hex:
                write_hex_8_no_sync(value);
                break;   
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(int8_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_signed_8_no_sync(value);
                break;
            case integer_base::hex:
                if(value < 0)
                {
                    put_char_no_sync('-');
                    write_hex_8_no_sync(static_cast<uint8_t>(0) - static_cast<uint8_t>(value));
                    break;
                }
                write_hex_8_no_sync(static_cast<uint8_t>(value));
                break;
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(uint16_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_unsigned_16_no_sync(value);
                break;
            case integer_base::hex:
                write_hex_16_no_sync(value);
                break;   
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(int16_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_signed_16_no_sync(value);
                break;
            case integer_base::hex:
                if(value < 0)
                {
                    put_char_no_sync('-');
                    write_hex_16_no_sync(static_cast<uint16_t>(0) - static_cast<uint16_t>(value));
                    break;
                }
                write_hex_16_no_sync(static_cast<uint16_t>(value));
                break;
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(uint32_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_unsigned_32_no_sync(value);
                break;
            case integer_base::hex:
                write_hex_32_no_sync(value);
                break;   
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(int32_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_signed_32_no_sync(value);
                break;
            case integer_base::hex:
                if(value < 0)
                {
                    put_char_no_sync('-');
                    write_hex_32_no_sync(static_cast<uint32_t>(0) - static_cast<uint32_t>(value));
                    break;
                }
                write_hex_32_no_sync(static_cast<uint32_t>(value));
                break;
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(uint64_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_unsigned_64_no_sync(value);
                break;
            case integer_base::hex:
                write_hex_64_no_sync(value);
                break;   
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(int64_t value) noexcept
    {
        switch(state)
        {
            case integer_base::dec:
                write_signed_64_no_sync(value);
                break;
            case integer_base::hex:
                if(value < 0)
                {
                    put_char_no_sync('-');
                    write_hex_64_no_sync(static_cast<uint64_t>(0) - static_cast<uint64_t>(value));
                    break;
                }
                write_hex_64_no_sync(static_cast<uint64_t>(value));
                break;
        }
        sync_cursor();
        return *this;
    }

    output& output::operator<<(const bool value) noexcept
    {
        if(bool_alpha_enabled) write_string_no_sync(static_cast<const char*>(value ? "true" : "false"));
        else put_char_no_sync(static_cast<char>('0' + value));
        sync_cursor();
        return *this;
    }

    output& output::operator<<(const void* ptr) noexcept
    {
        write_pointer_no_sync(reinterpret_cast<uintptr_t>(ptr));
        sync_cursor();
        return *this;
    }

    output& output::operator<<(output_manipulator manipulator) noexcept
    {
        return manipulator(*this);
    }

    // Free Methods
    output& dec(output& out) noexcept
    {
        out.state = integer_base::dec;
        return out;
    }

    output& hex(output& out) noexcept
    {
        out.state = integer_base::hex;
        return out;
    }

    output& bool_alpha(output& out) noexcept
    {
        out.bool_alpha_enabled = true;
        return out;
    }

    output& bool_no_alpha(output& out) noexcept
    {
        out.bool_alpha_enabled = false;
        return out;
    }
}