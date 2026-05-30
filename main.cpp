#include <stdint.h>
#include "io/output/terminal_output.h"
#include "logger/kernel_logger.h"
#include "exceptions/kernel_exceptions.h"
#include "timer/kernel_timer.h"
#include "pit/kernel_pit.h"
#include "keyboard/keyboard.h"
#include "internal/kernel_interrupt_guard.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" [[noreturn]] void kernel_main()
{
    terminal::output console{};
    kernel::logger logger{&console};
    console.initialize();

    kernel::set_exception_logger(&logger);
    kernel::initialize_exceptions();

    kernel::set_timer_logger(&logger);

    if(!kernel::initialize_pit(timer_frequency_hz))
    {
        logger.panic("Failed to initialize PIT");
    }
    kernel::set_timer_frequency(timer_frequency_hz);

    if(!driver::initialize_keyboard())
    {
        logger.warning() << "Failed to synchronize keyboard\n";
    }
    
    asm volatile("sti");

    for(;;)
    {

    }
}