#include <stdint.h>
#include "terminal.h"
#include "kernel_logger.h"
#include "kernel_assert.h"
#include "kernel_exceptions.h"
#include "kernel_timer.h"
#include "kernel_pit.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" [[noreturn]] void kernel_main()
{
    kernel::terminal console{};
    kernel::logger logger{&console};
    console.initialize();

    kernel::set_assert_logger(&logger);
    kernel::set_exception_logger(&logger);
    kernel::initialize_exceptions();
    kernel::set_timer_logger(&logger);

    if(!kernel::initialize_pit(timer_frequency_hz))
    {
        logger.panic("Failed to initialize PIT");
    }
    kernel::set_timer_frequency(timer_frequency_hz);

    logger.info() << "Timer frequency: " << kernel::timer_frequency() << " Hz\n"
    << "Activating Interrupts\n";

    asm volatile("sti");

    logger.info() << "Before sleep ticks: " <<  kernel::timer_ticks() << '\n';
    // kernel::sleep_ticks(300);
    kernel::sleep_ms(3000);
    logger.info() << "After sleep ticks: " << kernel::timer_ticks() << '\n';

    for(;;) asm volatile("hlt");
}