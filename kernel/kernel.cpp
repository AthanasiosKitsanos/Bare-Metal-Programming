#include <stdint.h>
#include "terminal.h"

extern "C" [[noreturn]] void kernel_main()
{
    terminal screen{};
    screen.initialize();

    screen << "Char test: " << 'A' << '\n'
    << "Unsigned test: " << static_cast<uint32_t>(1234567890u) << '\n'
    << "Signed test: " << static_cast<int32_t>(-12345) << '\n'
    << "Zero test: " << static_cast<uint32_t>(0) << '\n'
    << "Min int32 test: " << static_cast<int32_t>(-2147483647 - 1) << '\n'
    << "CR test: ABC" << '\r' << 'Z' << '\n'
    << "Positive signed test: " << static_cast<int32_t>(5) << '\n';

    while(true) asm volatile("hlt");
}