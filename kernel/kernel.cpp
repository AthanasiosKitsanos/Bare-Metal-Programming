#include <stdint.h>
#include "terminal.h"

extern "C" [[noreturn]] void kernel_main()
{
    terminal screen{};
    screen.initialize();

    screen << "Hex zero: " << hex(0) << '\n'
    << "Hex Small: " << hex(0x2A) << '\n'
    << "Hex full: " << hex(0xDEADBEEF) << '\n'
    << "Null pointer: " << static_cast<const void*>(nullptr) << '\n'
    << "VGA pointer: " << reinterpret_cast<const void*>(0xB8000);

    while(true) asm volatile("hlt");
}