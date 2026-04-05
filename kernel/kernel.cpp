#include <stdint.h>
#include "../include/terminal.h"

extern "C" [[noreturn]] void kernel_main()
{
    while(true) asm volatile("hlt");
}