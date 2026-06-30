#include "terminal_output.h"

namespace
{
    constexpr char table[] =  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    [[gnu::always_inline]]
    inline char hex_digit(uint8_t nibble) noexcept { return *(table + nibble); }
}

namespace terminal
{
    // Private Methods
    void output::new_line() noexcept
    {
        buffer.move_to_next_line();
    }

    void output::write_string_no_sync(const char* text) noexcept
    {
        for(char c{*text}; c != '\0'; c = *text)
        {
            put_char_no_sync(c);
            ++text;
        }
    }

    void output::write_pointer_no_sync(uintptr_t value) noexcept
    {
        put_hex_prefix();
        constexpr size_t total_nibbles_m1{sizeof(uintptr_t) * 2 - 1};
        const int leading_zeros{__builtin_clz(value) >> 2};

        const int starting_shift{(static_cast<int>(total_nibbles_m1) - leading_zeros) * 4};
        for(int shift{starting_shift}; shift >= 0; shift -= 4)
        {
            put_char_no_sync(hex_digit((value >> shift) & 0x0F));
        }
    }

    void output::write_signed_8_no_sync(int8_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_no_sync(static_cast<uint8_t>(0) - static_cast<uint8_t>(value));
            return;
        }
        write_unsigned_no_sync(static_cast<uint8_t>(value));
    }

    void output::write_signed_16_no_sync(int16_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_no_sync(static_cast<uint16_t>(0) - static_cast<uint16_t>(value));
            return;
        }
        write_unsigned_no_sync(static_cast<uint16_t>(value));
    }

    void output::write_signed_32_no_sync(int32_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_no_sync(static_cast<uint32_t>(0) - static_cast<uint32_t>(value));
            return;
        }
        write_unsigned_no_sync(static_cast<uint32_t>(value));
    }

    void output::write_signed_64_no_sync(int64_t value) noexcept
    {
        if(value < 0)
        {
            put_char_no_sync('-');
            write_unsigned_no_sync(static_cast<uint64_t>(0) - static_cast<uint64_t>(value));
            return;
        }
        write_unsigned_no_sync(static_cast<uint64_t>(value));
    }

    void output::write_hex_8_no_sync(uint8_t value) noexcept
    {
        put_hex_prefix();
        if(value == 0)
        {
            put_char_no_sync('0');
            return;
        }

        constexpr int total_nibbles_m1{sizeof(uint8_t) * 2 - 1};
        const int leading_zeros{(__builtin_clz(value) - 24) >> 2};

        const int starting_shift{(total_nibbles_m1 - leading_zeros) * 4};
        for(int shift{starting_shift}; shift >= 0; shift -= 4)
        {
            put_char_no_sync(hex_digit((value >> shift) & 0x0F));
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
        
        constexpr int total_nibbles_m1{sizeof(uint16_t) * 2 - 1};
        const int leading_zeros{(__builtin_clz(value) - 16) >> 2};

        const int starting_shift{(total_nibbles_m1 - leading_zeros) * 4};
        for(int shift{starting_shift}; shift >= 0; shift -= 4)
        {
            put_char_no_sync(hex_digit((value >> shift) & 0x0F));
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

        constexpr int total_nibbles_m1{sizeof(uint32_t) * 2 - 1};
        const int leading_zeros{__builtin_clz(value) >> 2};

        const int starting_shift{(total_nibbles_m1 - leading_zeros) * 4};
        for(int shift{starting_shift}; shift >= 0; shift -= 4)
        {
            put_char_no_sync(hex_digit((value >> shift) & 0x0F));
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

        constexpr int total_nibbles_m1{sizeof(uint64_t) * 2 - 1};
        const int leading_zeros{__builtin_clzll(value) >> 2};

        const int starting_shift{(total_nibbles_m1 - leading_zeros) * 4};
        for(int shift{starting_shift}; shift >= 0; shift -= 4)
        {
            put_char_no_sync(hex_digit((value >> shift) & 0x0F));
        }
    }

    // Public Methods
    void output::initialize() noexcept
    {
        buffer.clear();
        vga_hardware_cursor::enable();
        sync_cursor();
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
                write_unsigned_no_sync(value);
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
                write_unsigned_no_sync(value);
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
                write_unsigned_no_sync(value);
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
                write_unsigned_no_sync(value);
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