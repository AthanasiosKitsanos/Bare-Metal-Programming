#include <stdint.h>
#include "tools/copy.h"
#include <immintrin.h>

extern "C" uint32_t _kernel_source_image;
extern "C" uint32_t _kernel_start;
extern "C" uint32_t _bss_start;

[[gnu::regparm(3)]]
void copy_32(const uint32_t* source, uint32_t* dest, size_t length) noexcept
{
    tools::copy_data_inline<uint32_t>(source, dest, length);
}

[[gnu::target("sse2")]] [[gnu::regparm(3)]]
void copy_sse2(const uint32_t* source, uint32_t* dest, size_t length) noexcept
{
    length >>= 2;
    tools::copy_sse2_inline(reinterpret_cast<const __m128i*>(source), reinterpret_cast<__m128i*>(dest), length);
}

[[gnu::target("avx2")]] [[gnu::regparm(3)]]
void copy_avx2(const uint32_t* source, uint32_t* dest, size_t length) noexcept
{
    length >>= 3;
    tools::copy_avx2_inline(reinterpret_cast<const __m256i*>(source), reinterpret_cast<__m256i*>(dest), length);
}

constexpr size_t size{3};
using copy_kernel_method = void(*)(const uint32_t* source, uint32_t* dest, size_t length) noexcept [[gnu::regparm(3)]];
struct copy_table
{
    copy_kernel_method entries[size];
    constexpr copy_table(): entries{}
    {
        entries[0] = copy_32;
        entries[1] = copy_sse2;
        entries[2] = copy_avx2;
    }
};

constexpr copy_table c_table{};

extern "C" [[gnu::regparm(1)]] void move_kernel(const uint8_t flag) noexcept
{
    const uint32_t* source{reinterpret_cast<uint32_t*>(&_kernel_source_image)};
    uint32_t* destination{reinterpret_cast<uint32_t*>(&_kernel_start)};
    const size_t length{static_cast<size_t>(reinterpret_cast<uint32_t*>(&_bss_start) - destination)};
    c_table.entries[flag](source, destination, length);
}