#include <stdint.h>
#include "kernel_timer.h"
#include "kernel_logger.h"

namespace
{
    kernel::logger* g_timer_logger{nullptr};

    volatile uint32_t g_timer_ticks{0};
    uint32_t g_timer_frequency{0};
}

namespace kernel
{
    void set_timer_logger(logger* log) noexcept { g_timer_logger = log; }

    void handle_timer_tick() noexcept
    {
        ++g_timer_ticks;
        if(g_timer_logger && g_timer_frequency != 0 && g_timer_ticks % g_timer_frequency == 0)
        {
            g_timer_logger->info() << "Timer ticks: " << g_timer_ticks
            << "\nUptime: " << uptime_seconds() << "s\n";
        }
    }

    void set_timer_frequency(uint32_t frequency) noexcept { g_timer_frequency = frequency; }

    uint32_t timer_ticks() noexcept { return g_timer_ticks; }
    uint32_t timer_frequency() noexcept { return g_timer_frequency; }
    uint32_t uptime_seconds() noexcept
    {
        if(g_timer_frequency == 0) return 0;
        return g_timer_ticks / g_timer_frequency;
    }
}