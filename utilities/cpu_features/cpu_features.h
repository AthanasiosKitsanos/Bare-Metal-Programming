#pragma once

#include <stdint.h>

namespace cpu
{
    typedef uint8_t simd_flag;

    struct features
    {
        simd_flag ymm_flag;

        static features detect() noexcept;

        [[gnu::always_inline]]
        inline static features get() noexcept
        {
            static features cached{detect()};
            return cached;
        }
    };
}