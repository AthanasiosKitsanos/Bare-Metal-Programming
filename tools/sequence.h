#pragma once

#include <stddef.h>

namespace sequence
{
    template<size_t ...I>
    struct index_list{};

    template<size_t N, size_t ...I>
    struct make_index_list_impl
    {
        using type = typename make_index_list_impl<N - 1, N - 1, I...>::type;
    };

    template<size_t ...I>
    struct make_index_list_impl<0, I...>
    {
        using type = index_list<I...>;
    };

    template<size_t N>
    using make_index_list = typename make_index_list_impl<N>::type;
}