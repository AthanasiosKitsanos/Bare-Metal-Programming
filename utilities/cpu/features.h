#pragma once

#include <stdint.h>

namespace cpu::features
{
    typedef uint8_t simd_flags;

    simd_flags get() noexcept;
}