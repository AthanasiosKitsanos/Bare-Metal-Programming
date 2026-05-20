#include <stdint.h>
#include "terminal_output.h"
#include "kernel_logger.h"
#include "kernel_assert.h"
#include "kernel_exceptions.h"
#include "kernel_timer.h"
#include "kernel_pit.h"
#include "keyboard.h"
#include "kernel_interrupt_guard.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" [[noreturn]] void kernel_main()
{
    terminal::output console{};
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

    if(!driver::initialize_keyboard())
    {
        logger.warning() << "Failed to synchronize keyboard\n";
    }
    
    asm volatile("sti");

    for(;;)
    {
        driver::keyboard_event event{};
        char character{'\0'};
        while(driver::poll_keyboard_event(&event))
        {
            if(driver::try_translate_text_event(&event, &character))
            {
                console << character;
            }
        }

        kernel::disable_interrupts();

        if(!driver::has_pending_keyboard_event())
        {
            kernel::enable_interrupt_and_halt();
        }
        else
        {
            kernel::enable_interrupts();
        }
    }
}