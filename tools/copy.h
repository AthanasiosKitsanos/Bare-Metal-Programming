#pragma once

#include <stdint.h>
#include <immintrin.h>

namespace tools
{

    template<typename T>
    [[gnu::noinline]] [[gnu::regparm(3)]]
    void copy_data(const T* source, T* destination, const size_t length) noexcept
    {
        const T* const end{source + length};
        for(; source < end; ++source)
        {
            *destination = *source;
            ++destination;
        }
    }
    
    template<typename T>
    [[gnu::always_inline]]
    inline void copy_data_inline(const T* source, T* destination, const size_t length) noexcept
    {
        const T* const end{source + length};
        for(; source < end; ++source)
        {
            *destination = *source;
            ++destination;
        }
    }
    
    [[gnu::noinline]] [[gnu::target("sse2")]] [[gnu::regparm(3)]]
    void copy_sse2(const __m128i* source, __m128i* destination, const size_t length) noexcept
    {
        __m128i value{_mm_setzero_si128()};
        const __m128i* const end{source +length};
        for(; source < end; ++source)
        {
            value = _mm_load_si128(source);
            _mm_store_si128(destination, value);
            ++destination;
        }
    }
    
    [[gnu::always_inline]] [[gnu::target("sse2")]]
    void copy_sse2_inline(const __m128i* source, __m128i* destination, const size_t length) noexcept
    {
        __m128i value{_mm_setzero_si128()};
        const __m128i* const end{source +length};
        for(; source < end; ++source)
        {
            value = _mm_load_si128(source);
            _mm_store_si128(destination, value);
            ++destination;
        }
    }

    [[gnu::noinline]] [[gnu::target("avx2")]] [[gnu::regparm(3)]]
    void copy_avx2(const __m256i* source, __m256i* destination, const size_t length) noexcept
    {
        __m256i value{_mm256_setzero_si256()};
        const __m256i* const end{source +length};
        for(; source < end; ++source)
        {
            value = _mm256_load_si256(source);
            _mm256_store_si256(destination, value);
            ++destination;
        }
    }
    
    [[gnu::always_inline]] [[gnu::target("avx2")]]
    void copy_avx2_inline(const __m256i* source, __m256i* destination, const size_t length) noexcept
    {
        __m256i value{_mm256_setzero_si256()};
        const __m256i* const end{source +length};
        for(; source < end; ++source)
        {
            value = _mm256_load_si256(source);
            _mm256_store_si256(destination, value);
            ++destination;
        }
    }
}