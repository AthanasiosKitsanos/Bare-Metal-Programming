#include <stdint.h>
#include "terminal.h"

extern "C" [[noreturn]] void kernel_main()
{
    terminal screen{};
    screen.initialize();

    screen << hex << uint32_t{0xDEADBEEF};
    while(true) asm volatile("hlt");
}