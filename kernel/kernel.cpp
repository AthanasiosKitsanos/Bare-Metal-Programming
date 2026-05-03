#include <stdint.h>
#include "terminal.h"
#include "kernel_logger.h"
#include "kernel_assert.h"
#include "kernel_exceptions.h"
#include "kernel_timer.h"
#include "kernel_pit.h"
#include "keyboard.h"

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

    logger.info() << "Setting frequency to: " << timer_frequency_hz << '\n';
    if(!kernel::initialize_pit(timer_frequency_hz))
    {
        logger.panic("Failed to initialize PIT");
    }
    kernel::set_timer_frequency(timer_frequency_hz);
    console << "Frequency set\n";

    logger.info() << "Setting Keyboard logger\n";
    kernel::set_keyboard_logger(&logger);
    console << "Keyboard Logger set\n";

    asm volatile("sti");

    for(;;) asm volatile("hlt");
}