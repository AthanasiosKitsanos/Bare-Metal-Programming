#pragma once

#include <stdint.h>

namespace cpu
{
    enum flags: uint8_t
    {
        sse_2 = 0,
        avx = 1,
        avx_2 = 2
    };

    struct features
    {
        uint8_t simd_flags;

        static features detect() noexcept;

        [[gnu::always_inline]]
        inline static const features* get() noexcept
        {
            static features cached{detect()};
            return &cached;
        }
    };
}