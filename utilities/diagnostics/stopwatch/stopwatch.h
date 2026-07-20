#pragma once

#include <stdint.h>

namespace diagnostics
{
    class stopwatch
    {
        uint32_t start;

        public:
        stopwatch() noexcept;
        ~stopwatch() noexcept;

        stopwatch(const stopwatch&) = delete;
        stopwatch& operator=(stopwatch&) = delete;
    };
}