#pragma once

#include <stdint.h>

extern "C" uint8_t _mm_flag;

namespace cpu::features
{
    [[gnu::always_inline]]
    inline uint8_t get() noexcept { return _mm_flag; }
}