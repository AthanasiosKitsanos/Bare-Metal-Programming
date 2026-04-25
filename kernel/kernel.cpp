#include <stdint.h>
#include "terminal.h"
#include "kernel_logger.h"
#include "kernel_assert.h"
#include "kernel_exceptions.h"

extern "C" [[noreturn]] void kernel_main()
{
    kernel::terminal console{};
    kernel::logger logger{&console};
    console.initialize();
    kernel::set_assert_logger(&logger);
    kernel::set_exception_logger(&logger);
    kernel::initialize_exceptions();
    
    asm volatile("ud2");
    while(true) asm volatile("cli; hlt");
}