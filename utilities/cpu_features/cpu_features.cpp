#include "cpu_features.h"
#include <stdint.h>

namespace cpu
{
    const features* features::get() noexcept
    {
        static features cached{detect()};
        return &cached;
    }

    features features::detect() noexcept
    {
        features f{};
        uint32_t eax, ebx, ecx, edx;
        asm volatile
        (
            "cpuid":
            "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(1), "c"(0)
        );
        const uint8_t has_sse2{static_cast<uint8_t>(edx >> 26) & 1};
        const uint8_t has_avx{static_cast<uint8_t>(ecx >> 28) & 1};

        asm volatile
        (
            "cpuid":
            "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(7), "c"(0)
        );
        const uint8_t has_avx2{static_cast<uint8_t>(ebx >> 5) & 1};
        
        f.simd_flags |= ((has_sse2 << sse_2) | (has_avx << avx) | (has_avx2 << avx_2));
        return f;
    }
}