#include "cpu_features.h"
#include <stdint.h>

namespace cpu
{
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

        asm volatile
        (
            "cpuid":
            "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
            : "a"(7), "c"(0)
        );
        const uint8_t has_avx2{static_cast<uint8_t>(ebx >> 5) & 1};
        
        f.ymm_flag = (has_sse2 * (1 + has_avx2));
        return f;
    }
}