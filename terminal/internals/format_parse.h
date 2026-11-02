#pragma once

#include <stddef.h>
#include "tools/fixed_string.h"

namespace terminal::format_parser
{
    struct str_slice
    {
        size_t base;
        size_t length;
    };

    template<size_t Slices>
    struct str_slice_vault
    {
        static_assert(Slices >= 1, "Vault must have at least one slice of text");
        str_slice slices[Slices];
    };

    template<size_t N>
    consteval size_t count_args_slots(const tools::fixed_string<N> str) noexcept
    {
        static_assert(N >= 2, "fixed string must be greater than 1, since \\0 counts as 1 character");
        size_t counter{0};
        for(size_t i{0}; i < (N - 2); ++i)
        {
            bool condition{(str.data[i] == '{')};
            condition &= (str.data[i + 1] == '}');
            if(condition)
            {
                ++counter;
                ++i;
            }
        }
        return counter;
    }

    template<tools::fixed_string Fmt>
    consteval str_slice_vault<count_args_slots(Fmt) + 1> slice_string() noexcept
    {
        return {};
    }
}
