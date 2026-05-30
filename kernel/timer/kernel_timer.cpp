#include <stdint.h>
#include "kernel_timer.h"
#include "logger/kernel_logger.h"
#include "assert/kernel_assert.h"
#include "internal/kernel_interrupt_frame.h"

// #define KERNEL_DEBUG

namespace
{
    constexpr uint32_t milliseconds_per_second{1000};

    kernel::logger* g_timer_logger{nullptr};

    volatile uint32_t g_timer_ticks{0};
    uint32_t g_timer_frequency{0};
}

namespace kernel
{
    void set_timer_logger(logger* log) noexcept { g_timer_logger = log; }

    void handle_timer_interrupt(interrupt_frame* frame) noexcept
    {
        static_cast<void>(frame);
        ++g_timer_ticks;
        #ifdef KERNEL_DEBUG
            if(g_timer_logger && g_timer_frequency != 0 && g_timer_ticks % g_timer_frequency == 0)
            {
                g_timer_logger->info() << "Timer ticks: " << g_timer_ticks
                << "\nUptime: " << uptime_seconds() << "s\n";
            }
        #endif
    }

    void set_timer_frequency(uint32_t frequency) noexcept { g_timer_frequency = frequency; }

    uint32_t timer_ticks() noexcept { return g_timer_ticks; }
    uint32_t timer_frequency() noexcept { return g_timer_frequency; }
    uint32_t uptime_seconds() noexcept
    {
        const uint32_t frequency{g_timer_frequency};
        if(frequency == 0) return 0;
        return g_timer_ticks / frequency;
    }

    void sleep_ticks(uint32_t ticks) noexcept
    {
        const uint32_t start{g_timer_ticks};
        while((g_timer_ticks - start) < ticks) asm volatile("hlt");
    }

    void sleep_ms(uint32_t ms) noexcept
    {
        const uint32_t frequency{g_timer_frequency};
        if(frequency == 0 || ms == 0) return;

        const uint32_t whole_seconds{ms / milliseconds_per_second};
        const uint32_t remaining_milliseconds{ms % milliseconds_per_second};

        if(whole_seconds > UINT32_MAX / frequency)
        {
            sleep_ticks(UINT32_MAX);
            return;
        }

        const uint32_t whole_second_ticks{whole_seconds * frequency};
        const uint32_t remaining_ticks{(remaining_milliseconds * frequency + milliseconds_per_second - 1) / milliseconds_per_second};

        if(whole_second_ticks > UINT32_MAX - remaining_ticks)
        {
            sleep_ticks(UINT32_MAX);
            return;
        }

        sleep_ticks(whole_second_ticks + remaining_ticks);
    }
}