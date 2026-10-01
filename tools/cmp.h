#pragma once

namespace tools
{
    template<typename T>
    [[gnu::always_inline]]
    inline constexpr T max(const T value_1, const T value_2) noexcept
    {
        return (value_1 >= value_2) ? value_1 : value_2;
    }
}