#include "stopwatch.h"
#include "kernel/timer/kernel_timer.h"
#include "io/output/terminal_output.h"

namespace diagnostics
{
    stopwatch::stopwatch() noexcept: start{kernel::timer_ticks()}
    {}

    stopwatch::~stopwatch() noexcept
    {
        const uint32_t time_diff{kernel::timer_ticks() - start};
        const uint32_t frequency{kernel::timer_frequency()};

        if(frequency != 0)
        {
            constexpr uint32_t ms_per_sec{1000};
            const uint32_t whole_seconds{time_diff / frequency};
            const uint32_t remaining_ticks{time_diff - (whole_seconds * frequency)};

            uint32_t elapsed_ms{0};
            if(whole_seconds > UINT32_MAX / ms_per_sec) elapsed_ms = UINT32_MAX;
            else
            {
                const uint32_t whole_seconds_ms{whole_seconds * ms_per_sec};
                const uint32_t remaining_ms{(remaining_ticks * ms_per_sec) / frequency};
                if(whole_seconds_ms > UINT32_MAX - remaining_ms)
                {
                    elapsed_ms = UINT32_MAX;
                }
                else
                {
                    elapsed_ms = whole_seconds_ms + remaining_ms;
                }
            }

            terminal::output console{};
            console << "Time elapsed: " << elapsed_ms << "ms\n";
        }
    }
}