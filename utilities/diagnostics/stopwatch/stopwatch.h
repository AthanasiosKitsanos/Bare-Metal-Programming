#pragma once

#include <stdint.h>
#include "kernel/timer/kernel_timer.h"

namespace diagnostics
{
    class stopwatch
    {
        uint32_t _start;

        public:
        explicit stopwatch() noexcept: _start{kernel::timer_ticks()} {}
        ~stopwatch() noexcept
        {
            uint32_t end{kernel::timer_ticks()};
            uint32_t time_elapsed{end - _start};
        }
    };
}