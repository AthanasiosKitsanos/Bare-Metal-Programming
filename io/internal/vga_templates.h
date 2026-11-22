#pragma once

#include <stdint.h>
#include <stddef.h>
#include <immintrin.h>
#include "tools/arch.h"

namespace io::vga
{
    template<uint32_t E>
    [[gnu::target("sse2")]] [[REGPARM(2)]]
    void fill_sse_2(volatile uint32_t* dst, size_t bytes) noexcept
    {
        bytes >>= 4;
        const __m128i blank{_mm_set1_epi32(E)};
        __m128i* start{reinterpret_cast<__m128i*>(const_cast<uint32_t*>(dst))};
        const __m128i* const end{start + bytes};
        for(; start < end; ++start) _mm_store_si128(start, blank);
    }

    template<uint32_t E>
    [[gnu::target("avx2")]] [[REGPARM(2)]]
    void fill_avx_2(volatile uint32_t* dst, size_t bytes) noexcept
    {
        bytes >>= 5;
        const __m256i blank{_mm256_set1_epi32(E)};
        __m256i* start{reinterpret_cast<__m256i*>(const_cast<uint32_t*>(dst))};
        const __m256i* const end{start + bytes};
        for(; start < end; ++start) _mm256_store_si256(start, blank);
    }

    template<uint32_t E>

    [[REGPARM(2)]]
    void fill_32(volatile uint32_t* dst, size_t bytes) noexcept
    {
        bytes >>= 2;
        const volatile uint32_t* const end{dst + bytes};
        for(; dst < end; ++dst) *dst = E;
    }

    template<size_t B>
    [[gnu::target("sse2")]] [[REGPARM(2)]]
    volatile uint32_t* copy_sse_2(const volatile uint32_t* src, volatile uint32_t* dst) noexcept
    {
        constexpr size_t count{B >> 4};
        const __m128i* source{reinterpret_cast<__m128i*>(const_cast<uint32_t*>(src))};
        __m128i* destination{reinterpret_cast<__m128i*>(const_cast<uint32_t*>(dst))};
        const __m128i* const end{source + count};
        while(source < end)
        {
            _mm_store_si128(destination, _mm_load_si128(source));
            ++destination;
            ++source;
        }
        return reinterpret_cast<volatile uint32_t*>(destination);
    }

    template<size_t B>
    [[gnu::target("avx2")]] [[REGPARM(2)]]
    volatile uint32_t* copy_avx_2(const volatile uint32_t* src, volatile uint32_t* dst) noexcept
    {
        constexpr size_t count{B >> 5};
        const __m256i* source{reinterpret_cast<__m256i*>(const_cast<uint32_t*>(src))};
        __m256i * destination{reinterpret_cast<__m256i*>(const_cast<uint32_t*>(dst))};
        const __m256i* const end{source + count};
        while(source < end)
        {
            _mm256_store_si256(destination, _mm256_load_si256(source));
            ++source;
            ++destination;
        }
        return reinterpret_cast<volatile uint32_t*>(destination);
    }

    template<size_t B>
    [[REGPARM(2)]]
    volatile uint32_t* copy_32(const volatile uint32_t* src, volatile uint32_t* dst) noexcept
    {
        constexpr size_t count{B >> 2};
        const volatile uint32_t* const end{src + count};
        while(src < end)
        {
            *dst = *src;
            ++dst;
            ++src;
        }
        return dst;
    }

    template<uint32_t E>
    using fill_fn = void(*)(volatile uint32_t* dst, size_t step) noexcept [[REGPARM(2)]];

    template<uint32_t E>
    struct fill_fn_array
    {
        fill_fn<E> entries[3];

        constexpr fill_fn_array(): entries{fill_32<E>, fill_sse_2<E>, fill_avx_2<E>}
        {}

        [[gnu::always_inline]]
        constexpr fill_fn<E> operator[](const size_t index) const noexcept
        {
            return entries[index];
        }
    };

    template<size_t B>
    using copy_fn = volatile uint32_t*(*)(const volatile uint32_t* source, volatile uint32_t* dst) noexcept [[REGPARM(2)]];

    template<size_t B>
    struct copy_fn_array
    {
        copy_fn<B> entries[3];

        constexpr copy_fn_array(): entries{copy_32<B>, copy_sse_2<B>, copy_avx_2<B>}
        {}

        [[gnu::always_inline]]
        constexpr copy_fn<B> operator[](const size_t index) const noexcept
        {
            return entries[index];
        }
    };

    template<uint32_t E>
    constexpr fill_fn_array<E> g_fill_dispatch{};

    template<size_t E>
    constexpr copy_fn_array<E> g_copy_dispatch{};
}
