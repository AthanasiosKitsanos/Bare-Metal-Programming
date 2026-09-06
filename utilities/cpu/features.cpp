#include "features.h"

namespace cpu::features
{
    extern "C" simd_flags mm_flag{0};

    simd_flags get() noexcept { return mm_flag; }
}