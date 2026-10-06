#pragma once

#include <stdint.h>

namespace trait_of_types
{
    template<typename T, T v>
    struct integral_constant
    {
        static constexpr T value = v;
    };

    using false_type = integral_constant<bool, false>;
    using true_type = integral_constant<bool, true>;

    template<typename T, typename U>
    struct is_same: false_type{};

    template<typename T>
    struct is_same<T, T>: true_type{};

    template<typename T, typename U>
    inline constexpr bool is_same_v = is_same<T, U>::value;
}