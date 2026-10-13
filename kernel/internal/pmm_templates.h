#pragma once

#include <stdint.h>
#include <immintrin.h>
#include "tools/math.h"

namespace
{
    constexpr size_t frame_size{4096};
    constexpr int8_t frame_shift{power::of_2(::frame_size)};
    constexpr uint8_t byte_size{8};
    constexpr uint8_t bits_per_byte_shift{power::of_2(::byte_size)};
    constexpr uint8_t bit_mask{::byte_size - 1};

    enum class set_bits: uint8_t
    {
        
        all_zeros = 0x00,
        all_ones = 0x01
    };

    template<typename T>
    struct shifts_array
    {
        static constexpr size_t size{static_cast<size_t>(power::of_2(sizeof(T) << ::bits_per_byte_shift))};
        
        template<size_t N>
        [[gnu::always_inline]]
        static constexpr size_t degree_before(const size_t frames) noexcept
        {
            if constexpr(N == 0) return 1;
            else
            {
                return static_cast<size_t>(degree_before<N - 1>(frames) + compute_step<N - 1>(frames));
            }
        }
        
        template<size_t N>
        [[gnu::always_inline]]
        static constexpr uint8_t compute_step(const size_t frames) noexcept
        {
            if constexpr(N == (size - 1))
            {           
                return static_cast<uint8_t>(frames - degree_before<N>(frames));
            }
            else
            {
                constexpr size_t limit{static_cast<size_t>(0x02 << N)};
                return static_cast<uint8_t>((frames >= limit) << N);
            }
        }
        
        
        uint8_t step[size];
        
        constexpr shifts_array(const size_t frames): step{}
        {
            if constexpr(size == power::of_2(sizeof(uint8_t) << ::bits_per_byte_shift))
            {
                step[0] = static_cast<uint8_t>(frames >= 2);
                step[1] = static_cast<uint8_t>((frames >= 4) << 1);
                step[2] = static_cast<uint8_t>(frames - (step[0] + step[1] + 1));
            }
            else if constexpr(size == power::of_2(sizeof(uint16_t) << ::bits_per_byte_shift))
            {
                step[0] = static_cast<uint8_t>(frames >= 2);
                step[1] = static_cast<uint8_t>((frames >= 4) << 1);
                step[2] = static_cast<uint8_t>((frames >= 8) << 2);
                step[3] = static_cast<uint8_t>(frames - (step[0] + step[1] + step[2] + 1));
            }
            else if constexpr(size == power::of_2(sizeof(uint32_t) << ::bits_per_byte_shift))
            {
                step[0] = static_cast<uint8_t>(frames >= 2);
                step[1] = static_cast<uint8_t>((frames >= 4) << 1);
                step[2] = static_cast<uint8_t>((frames >= 8) << 2);
                step[3] = static_cast<uint8_t>((frames >= 16) << 3);
                step[4] = static_cast<uint8_t>(frames - (step[0] + step[1] + step[2] + step[3] + 1));
            }
            else if constexpr(size == power::of_2(sizeof(uint64_t) << ::bits_per_byte_shift))
            {
                step[0] = static_cast<uint8_t>(frames >= 2);
                step[1] = static_cast<uint8_t>((frames >= 4) << 1);
                step[2] = static_cast<uint8_t>((frames >= 8) << 2);
                step[3] = static_cast<uint8_t>((frames >= 16) << 3);
                step[4] = static_cast<uint8_t>((frames >= 32) << 4);
                step[5] = static_cast<uint8_t>(frames - (step[0] + step[1] + step[2] + step[3] + step[4] + 1));
            }
        }
    };

    template<typename T>
    [[gnu::always_inline]]
    inline ::shifts_array<T> calculate_shifts(const size_t frames) noexcept
    {
        if constexpr(N == sizeof(uint8_t) << 3)
        {
            return shift_array<N>
            {
                (frames >= 2),
                ((frames >= 4)),
                frames + 
            };
        }
    } 


    // General Purpose Registers
    template<typename T, ::set_bits Set>
    [[gnu::always_inline]]
    inline void set_gpr_inline(T** start, const T* const end) noexcept
    {
        constexpr uint64_t all_ones{UINT64_MAX};
        constexpr T value{static_cast<T>(all_ones) * static_cast<T>(Set)};
        
        T* current{*start};
        for(; current < end; ++current) *current = value;
        *start = current;
    }

    template<typename T, ::set_bits Set>
    [[gnu::noinline]] [[gnu::regparm(2)]]
    void set_gpr(T** start, const T* const end) noexcept
    {
        constexpr uint32_t all_ones{UINT32_MAX};
        constexpr T value{static_cast<T>(all_ones) * static_cast<T>(Set)};
        
        T* current{*start};
        
        for(; current < end; ++current) *current = value;
        *start = current;
    }

    // SIMD and AVX Templates
    template<::set_bits Set>
    [[gnu::always_inline]] [[gnu::target("sse2")]]  
    inline void set_sse2_inline(__m128i** start, const __m128i* const end) noexcept
    {
        __m128i* current{*start};
        __m128i value;
        if constexpr(Set == set_bits::all_ones)
        {
            __m128i temp{_mm_setzero_si128()};
            value = _mm_cmpeq_epi32(temp, temp);
        }
        else value = _mm_setzero_si128();

        for(; current < end; ++current) _mm_store_si128(current, value);
        *start = current;
    }

    template<::set_bits Set>
    [[gnu::noinline]] [[gnu::target("sse2")]] [[gnu::regparm(2)]]
    void set_sse2(__m128i** start, const __m128i* const end) noexcept
    {
        __m128i* current{*start};
        __m128i value;
        if constexpr(Set == set_bits::all_ones)
        {
            __m128i temp{_mm_setzero_si128()};
            value = _mm_cmpeq_epi32(temp, temp);
        }
        else value = _mm_setzero_si128();

        for(; current < end; ++current) _mm_store_si128(current, value);
        *start = current;
    }

    template<::set_bits Set>
    [[gnu::always_inline]] [[gnu::target("avx2")]]
    inline void set_avx2_inline(__m256i** start, const __m256i* const end) noexcept
    {
        __m256i* current{*start};
        __m256i value;
        if constexpr(Set == set_bits::all_ones)
        {
            __m256i temp{_mm256_setzero_si256()};
            value = _mm256_cmpeq_epi32(temp, temp);
        }
        else value = _mm256_setzero_si256();

        for(; current < end; ++current) _mm256_store_si256(current, value);
        *start = current;
    }

    template<::set_bits Set>
    [[gnu::noinline]] [[gnu::target("avx2")]] [[gnu::regparm(2)]]
    void set_avx2(__m256i** start, const __m256i* const end) noexcept
    {
        __m256i* current{*start};
        __m256i value;
        if constexpr(Set == set_bits::all_ones)
        {
            __m256i temp{_mm256_setzero_si256()};
            value = _mm256_cmpeq_epi32(temp, temp);
        }
        else value = _mm256_setzero_si256();

        for(; current < end; ++current) _mm256_store_si256(current, value);
        *start = current;
    }
}