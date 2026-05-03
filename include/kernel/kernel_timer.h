#pragma once

#include <stdint.h>

namespace kernel
{
    class logger;

    void set_timer_logger(logger* log) noexcept;
    
    void set_timer_frequency(uint32_t frequency) noexcept;
    uint32_t timer_frequency() noexcept;

    void handle_timer_interrupt() noexcept;
    uint32_t timer_ticks() noexcept;
    uint32_t uptime_seconds() noexcept;
    void sleep_ticks(uint32_t ticks) noexcept;
    void sleep_ms(uint32_t ms) noexcept;
}