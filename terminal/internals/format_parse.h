#pragma once

#include <stddef.h>
#include <stdint.h>
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
        static_assert(N >= 2, "count_args_slots: fixed string must be greater than 1, since \\0 counts as 1 character");
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
    struct format_layout;

    template<tools::fixed_string Fmt>
    consteval str_slice_vault<format_layout<Fmt>::slices_count> slice_string() noexcept
    {
        static_assert(Fmt.size >= 2, "slice_string: fixed string must be greater than 1, since \\0 counts as 1 character");
        str_slice_vault<format_layout<Fmt>::slices_count> s_vault{};
        size_t slice{0};
        size_t slice_base{0};

        constexpr size_t fmt_end{Fmt.size - 2};
        for(size_t i{0}; i < fmt_end; ++i)
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
        s_vault[slice] = str_slice{slice_base, Fmt.size - slice_base - 1};
        return s_vault;
    }

    template<tools::fixed_string Fmt>
    struct format_layout
    {
        // Slots and slice_count are not depended from slices and slice_string
        static constexpr size_t placeholders_count{count_args_slots(Fmt)};
        static constexpr size_t slices_count{placeholders_count + 1};
        static constexpr str_slice_vault<slices_count> slices{slice_string<Fmt>()};

        [[gnu::always_inline]]
        constexpr inline const str_slice operator[](const size_t index) const noexcept
        {
            return slices[index];
        }
    };

    template<tools::fixed_string Fmt, size_t I>
    struct sub_string
    {
        public:
            using layout = format_layout<Fmt>;
            static_assert(I < layout::slices_count, "sub_string: index template parameter out of bounds");
            static constexpr size_t base{layout::slices[I].base};
            static constexpr size_t sub_size{layout::slices[I].length};

        private:
            consteval static tools::fixed_string<sub_size + 1> create_sub_string() noexcept
            {
                constexpr size_t temp_size{sub_size + 1};
                char buffer[temp_size]{}; // Initializing a char array with brackets, zeros the array '\0'
                for(size_t j{0}; j < temp_size - 1; ++j)
                {
                    buffer[j] = Fmt.data[base + j];
                }
                return buffer;
            }

        public:
            static constexpr tools::fixed_string<sub_size + 1> string{create_sub_string()};
    };


    constexpr tools::fixed_string str{"ab{}cd"};
    using sub_1 = sub_string<str, 0>;
    static_assert(sub_1::sub_size == 2);
    static_assert(sub_1::string.data[0] == 'a' && sub_1::string.data[1] == 'b');

    using sub_2 = sub_string<str, 1>;
    static_assert(sub_2::sub_size == 2);
    static_assert(sub_2::string.data[0] == 'c' && sub_2::string.data[1] == 'd');
}
