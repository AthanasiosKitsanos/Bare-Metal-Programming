#pragma once

#include <stddef.h>
#include "sequence.h"

namespace tools
{
    template<size_t N>
    class fixed_string
    {
        static_assert(N >= 1, "String must have at least space for a termination character (\\0)");
        template<size_t ...I>
        [[gnu::always_inline]]
        constexpr explicit fixed_string(const char (&source)[N], sequence::index_list<I...>) noexcept: data{source[I]...}
        {}

        public:
            static constexpr size_t size{N};
            char data[size];

            //  No explicit: template parameter initialized through copy
            [[gnu::always_inline]]
            constexpr inline fixed_string(const char (&source)[N]) noexcept: fixed_string(source, sequence::make_index_list<N>{})
            {}
    };
}
