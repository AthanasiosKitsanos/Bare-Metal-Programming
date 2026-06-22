#pragma once

#include <stdint.h>

namespace cpu::features
{
    typedef uint8_t simd_flags;
    extern "C" simd_flags mm_flag;

    [[gnu::always_inline]]
    inline static simd_flags get() noexcept { return mm_flag; }
}