#pragma once

namespace cmp
{
    // Returns the maximum value
    // Values must be the same type
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr T max(const T value_1, const T value_2) noexcept
    {
        return (value_1 >= value_2) ? value_1 : value_2;
    }
    
    // Returns the minimum value
    // Values must be the same type
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr T min(const T value_1, const T value_2) noexcept
    {
        return (value_1 < value_2) ? value_1 : value_2;
    }
}