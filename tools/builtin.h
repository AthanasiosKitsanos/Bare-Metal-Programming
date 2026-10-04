#pragma once

#include <stddef.h>
#include <stdint.h>

namespace builtin
{
    namespace detail
    {
        template<typename T>
        constexpr bool is_supported_width() noexcept
        {
            constexpr uint8_t size{sizeof(T)};
            constexpr bool is_power_of_2{(size & (size - 1)) == 0};
            constexpr bool in_limit{size <= 8};
            return is_power_of_2 && in_limit;
        }
    }

    // Returns the leading zeros of a value
    // Use this if the caller first checks 
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr uint8_t leading_zeros(const T value) noexcept
    {
        constexpr bool is_supported{detail::is_supported_width<T>()};
        static_assert(is_supported, "Type size must be at least 1 byte to 8 bytes and a power of 2\n");

        if constexpr(sizeof(T) == sizeof(uint64_t))
        {
            constexpr uint8_t shift_left_64{64 - (sizeof(T) * 8)};
            return static_cast<uint8_t>(__builtin_clzll(static_cast<unsigned long long>(value) << shift_left_64));
        }
        else
        {
            constexpr uint8_t shift_left_32{32 - (sizeof(T) * 8)};
            return static_cast<uint8_t>(__builtin_clz(static_cast<unsigned int>(value) << shift_left_32));
        }
    }

    template<typename T>
    [[gnu::always_inline]]
    inline constexpr uint8_t bit_guard_lz(const T value) noexcept
    {
        constexpr bool is_supported{detail::is_supported_width<T>()};
        static_assert(is_supported, "Type size must be at least 1 byte to 8 bytes and a power of 2\n");

        if constexpr(sizeof(T) == sizeof(uint64_t))
        {
            return static_cast<uint8_t>((value != 0) ? __builtin_clzll(static_cast<unsigned long long>(value)) : (sizeof(T) << 3));
        }
        else if constexpr(sizeof(T) == sizeof(uint32_t))
        {
            return static_cast<uint8_t>((value != 0) ? __builtin_clz(static_cast<unsigned int>(value)) : (sizeof(uint32_t) << 3));
        }
        else
        {
            constexpr uint8_t shift{32 - (sizeof(T) << 3)};
        
            constexpr unsigned int mask{(1u << shift) - 1};
            return static_cast<uint8_t>(__builtin_clz((static_cast<unsigned int>(value) << shift) | mask));
        }
    }

    template<typename T>
    [[gnu::always_inline]]
    inline constexpr uint8_t trailing_zeros(const T value) noexcept
    {
        constexpr bool is_supported{detail::is_supported_width<T>()};
        static_assert(is_supported, "Type size must be at least 1 byte to 8 bytes and a power of 2\n");

        if constexpr(sizeof(T) == sizeof(uint64_t))
        {
            return static_cast<uint8_t>(__builtin_ctzll(static_cast<unsigned long long>(value)));
        }
        else return static_cast<uint8_t>(__builtin_ctz(static_cast<unsigned int>(value)));
    }

    template<typename T>
    [[gnu::always_inline]]
    inline constexpr uint8_t bit_guard_tz(const T value) noexcept
    {
        constexpr bool is_supported{detail::is_supported_width<T>()};
        static_assert(is_supported, "Type size must be at least 1 byte to 8 bytes and a power of 2\n");
        
        if constexpr(sizeof(T) == sizeof(uint64_t))
        {
            return static_cast<uint8_t>((value != 0) ? __builtin_ctzll(static_cast<unsigned long long>(value)) : (sizeof(uint64_t) << 3));
        }
        else if constexpr(sizeof(T) == sizeof(uint32_t))
        {
            return static_cast<uint8_t>((value != 0) ? __builtin_ctz(static_cast<unsigned int>(value)) : (sizeof(uint32_t) << 3));
        }
        else
        {
            constexpr uint8_t width_bits{sizeof(T) * 8};
            constexpr unsigned int mask{0x01 << width_bits};
            return static_cast<uint8_t>(__builtin_ctz((mask | static_cast<unsigned int>(value))));
        }
    }
}