#include <stdint.h>
#include "terminal.h"
#include "kernel_logger.h"

extern "C" [[noreturn]] void kernel_main()
{
    terminal screen{};
    kernel_logger logger{&screen};
    screen.initialize();
    logger.print_error("Error");
    
    while(true) asm volatile("hlt");
}