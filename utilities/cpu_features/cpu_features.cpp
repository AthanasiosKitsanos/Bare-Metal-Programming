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
        
    }
}