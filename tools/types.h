#pragma once

#include <stdint.h>

namespace types
{
    template<typename T, T v>
    struct integral_constant
    {
        static constexpr T value = v;
    };

    using false_type = integral_constant<bool, false>;
    using true_type = integral_constant<bool, true>;

    template<typename T, typename U>
    struct equal: false_type{};

    template<typename T>
    struct equal<T, T>: true_type{};
}
