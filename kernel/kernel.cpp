#include <stdint.h>
#include "terminal.h"

extern "C" [[noreturn]] void kernel_main()
{
    terminal screen{};
    screen.initialize();
    screen << "ABC\rZ";
    while(true) asm volatile("hlt");
}