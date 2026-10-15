#pragma once

#include <stdint.h>
#include <immintrin.h>
#include "tools/math.h"
#include "tools/builtin.h"
#include "tools/sequence.h"

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
    class array_of_shifts
    {
        private:
        // D = (sizeof(size_t)) x 8 - 1) - leading_zeros
        static constexpr size_t array_size{static_cast<size_t>(power::of_2(sizeof(T) << bits_per_byte_shift))};
        static constexpr size_t total_bits{static_cast<size_t>(sizeof(size_t) << bits_per_byte_shift)};
        static constexpr size_t total_bits_m1{total_bits - 1};
        
        uint8_t step[array_size];

        template<size_t N>
        [[gnu::always_inline]]
        static constexpr uint8_t compute_step(const size_t frames, const size_t d) noexcept
        {
            if constexpr(N == array_size - 1) return static_cast<uint8_t>(frames - d);
            else
            {
                return static_cast<uint8_t>((d - 1) & (static_cast<size_t>(1) << N));
            }
        }
        
        template<size_t ...I>
        [[gnu::always_inline]]
        constexpr array_of_shifts(const size_t frames, const size_t d, sequence::index_list<I...>) noexcept: step{compute_step<I>(frames, d)...}
        {}

        [[gnu::always_inline]]
        static constexpr size_t highest_power_of_2(const size_t frames) noexcept
        {
            return static_cast<size_t>(static_cast<size_t>(1) << (total_bits_m1 - builtin::leading_zeros(frames)));
        }

        template<size_t ...I>
        [[gnu::always_inline]]
        constexpr T space_impl(T inv, sequence::index_list<I...>) const noexcept
        {
            ((inv &= static_cast<T>(inv >> this->step[I])), ...);
            return inv;
        }

        public:

        [[gnu::always_inline]]
        constexpr array_of_shifts(const size_t frames) noexcept: array_of_shifts(frames, highest_power_of_2(frames), sequence::make_index_list<array_size>{})
        {}

        [[gnu::always_inline]]
        constexpr T space(T inv) const noexcept
        {
            return space_impl(inv, sequence::make_index_list<array_size>{});
        }
    };


    // Checking neighboring bits

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