#include <stdint.h>
#include <immintrin.h>

enum class set_bits: uint8_t
{
    all_zeros = 0x00,
    all_ones = 0x01
};

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