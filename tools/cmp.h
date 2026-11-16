#pragma once

#include <stddef.h>
#include <stdint.h>

namespace cmp
{
    // Returns the maximum value without creating a branch.
    // Comparison must be excecuted between same types
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr T max(const T value_1, const T value_2) noexcept
    {
        const T result{static_cast<T>(-(value_1 >= value_2))};
        return (value_1 & result) | (value_2 & ~result);
    }

     // Returns the minimum value without creating a branch
     // Comparison must be excecuted between same types
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr T min(const T value_1, const T value_2) noexcept
    {
        const T result{static_cast<T>(-(value_1 < value_2))};
        return (value_1 & result) | (value_2 & ~result);
    }

    // Returns the maximum value with the result of conditional move or branch
    // Comparison must be excecuted between same types
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr T max_cmov(const T value_1, const T value_2) noexcept
    {
        return (value_1 >= value_2) ? value_1 : value_2;
    }

     // Returns the minimum value with the result of conditional move or branch
     // Comparison must be excecuted between same types
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr T min_cmov(const T value_1, const T value_2) noexcept
    {
        return (value_1 < value_2) ? value_1 : value_2;
    }

    static_assert(min<int8_t>(-1, -2) == -2);
    static_assert(min_cmov<int8_t>(-1, -2) == -2);
    static_assert(max<int8_t>(-1, -2) == -1);
    static_assert(max_cmov<int8_t>(-1, -2) == -1);
}
