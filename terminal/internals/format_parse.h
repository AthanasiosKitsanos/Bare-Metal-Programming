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

        [[gnu::always_inline]]
        constexpr inline str_slice& operator[](const size_t index) noexcept
        {
            return slices[index];
        }

        [[gnu::always_inline]]
        constexpr inline const str_slice operator[](const size_t index) const noexcept
        {
            return slices[index];
        }
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

    template<size_t VS>
    consteval size_t get_valid_vault_size(const tools::fixed_string<VS> str) noexcept
    {
        return count_args_slots(str) + 1;
    }

    template<tools::fixed_string Fmt>
    consteval str_slice_vault<get_valid_vault_size(Fmt)> slice_string() noexcept
    {
        constexpr size_t fmt_size{sizeof(Fmt.data)};
        static_assert(fmt_size >= 2, "fixed string must be greater than 1, since \\0 counts as 1 character");

        constexpr size_t vault_size{get_valid_vault_size(Fmt)};
        str_slice_vault<vault_size> s_vault{};
        size_t slice{0};
        size_t slice_base{0};
        for(size_t i{0}; i < (fmt_size - 2); ++i)
        {
            bool condition{(Fmt.data[i] == '{')};
            condition &= (Fmt.data[i + 1] == '}');
            if(condition)
            {
                s_vault[slice] = str_slice{slice_base, i - slice_base};
                ++slice;
                ++i;
                slice_base = i + 1;
            }
        }
        s_vault[slice] = str_slice{slice_base, fmt_size - slice_base - 1};
        return s_vault;
    }
}
