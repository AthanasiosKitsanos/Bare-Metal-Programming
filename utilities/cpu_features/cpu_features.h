#pragma once

#include <stdint.h>

namespace cpu
{
    typedef uint8_t simd_flag;

    class features
    {
        static simd_flag ymm_flag;

        public:
            static void detect() noexcept;
            static void init() noexcept;

            [[gnu::always_inline]]
            inline static simd_flag get() noexcept { return  ymm_flag; }
    };
}