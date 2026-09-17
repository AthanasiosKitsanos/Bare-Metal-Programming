#include <stdint.h>
#include <immintrin.h>

enum class set_bits: uint8_t
{
    all_zeros = 0x00,
    all_ones = 0x01
};

template<typename T, ::set_bits Set>
[[gnu::always_inline]] [[gnu::regparm(2)]]
inline void set_inline(T** start, const T* const end) noexcept
{
    constexpr uint32_t all_ones{UINT32_MAX};
    constexpr T value{static_cast<T>(all_ones) * static_cast<T>(Set)};
    
    T* current{*start};
    for(; current < end; ++current) *current = value;
    *start = current;
}

template<typename T, ::set_bits Set>
[[gnu::noinline]] [[gnu::regparm(2)]]
void set(T** start, const T* const end) noexcept
{
    constexpr uint32_t all_ones{UINT32_MAX};
    constexpr T value{static_cast<T>(all_ones) * static_cast<T>(Set)};
    
    T* current{*start};
    
    for(; current < end; ++current) *current = value;
    *start = current;
}

[[gnu::always_inline]] [[gnu::target("sse2")]]
inline __m128i make_zero(const __m128i*) noexcept { return _mm_setzero_si128(); }

[[gnu::always_inline]] [[gnu::target("sse2")]]
inline __m128i make_all_ones(const __m128i*) noexcept
{
    const __m128i zero{_mm_setzero_si128()};
    return _mm_cmpeq_epi32(zero, zero);
}

[[gnu::always_inline]] [[gnu::target("sse2")]]
inline void store(__m128i* const ptr, const __m128i value) noexcept { _mm_store_si128(ptr, value); }

[[gnu::always_inline]] [[gnu::target("avx2")]]
inline __m256i make_zero(const __m256i*) noexcept { return _mm256_setzero_si256(); }

[[gnu::always_inline]] [[gnu::target("avx2")]]
inline __m256i make_all_ones(const __m256i*) noexcept
{
    const __m256i zero{_mm256_setzero_si256()};
    return _mm256_cmpeq_epi32(zero, zero);
}

[[gnu::always_inline]] [[gnu::target("avx2")]]
inline void store(__m256i* const ptr, const __m256i value) noexcept { _mm256_store_si256(ptr, value); }

template<typename T, ::set_bits Set>
[[gnu::always_inline]] [[gnu::regparm(2)]] [[gnu::target("sse2", "avx2")]]
inline void set_extend_inline(T** start, const T* const end) noexcept
{
    T* current{*start};

    T value;
    if constexpr(Set == set_bits::all_ones) value = make_all_ones(current);
    else value = make_zero(current);

    for(; current < end; ++current) store(current, value);
    *start = current;
}

template<typename T, ::set_bits Set>
[[gnu::noinline]] [[gnu::regparm(2)]] [[gnu::target("sse2", "avx2")]]
void set_extend(T** start, const T* const end) noexcept
{
    T* current{*start};

    T value;
    if constexpr(Set == set_bits::all_ones) value = make_all_ones(current);
    else value = make_zero(current);

    for(; current < end; ++current) store(current, value);
    *start = current;
}