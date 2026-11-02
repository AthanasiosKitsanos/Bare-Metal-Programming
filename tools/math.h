#pragma once

#include <stddef.h>
#include "builtin.h"
namespace power
{
    // returns -1 if parameter is 0 or is not a power of 2
    template<typename T>
    inline constexpr int8_t of_2(const T value) noexcept
    {
        if((value <= 0) || ((value & (value - 1)) != 0)) return static_cast<int8_t>(-1);
        if constexpr(sizeof(T) == sizeof(uint64_t))
        {
            return static_cast<int8_t>(__builtin_ctzll(static_cast<unsigned long long>(value)));
        }
        else return static_cast<int8_t>(__builtin_ctz(static_cast<unsigned int>(value)));
    }

    template<typename T>
    [[gnu::always_inline]]
    static constexpr size_t highest_of_2(const T value) noexcept
    {
        constexpr size_t total_bits_m1{(sizeof(size_t) << 3) - 1};
        return (static_cast<size_t>(1) << (total_bits_m1 - builtin::leading_zeros(value)));
    }
}