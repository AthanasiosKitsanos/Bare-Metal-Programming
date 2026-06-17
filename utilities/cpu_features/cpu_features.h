#pragma once

#include <stdint.h>

namespace cpu
{
    enum flags: uint8_t
    {
        sse_2 = ( 1 << 0),
        avx = ( 1 << 1),
        avx_2 = ( 1 << 2)
    };

    struct features
    {
        uint8_t simd_flags;

        static features detect() noexcept;
        static const features* get() noexcept;
    };
}