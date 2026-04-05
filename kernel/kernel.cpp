#include <stdint.h>
#include "terminal.h"

extern "C" [[noreturn]] void kernel_main()
{
    terminal screen{};
    screen.initialize();
    for(uint16_t i{0}; i < 25; ++i)
    {
        if(i == 24)
        {
            screen.write_string("24: Hello\n");
            screen.write_string("25: Scrolled");
            continue;
        }
        screen.put_char('\n');
    }
    screen.write_string("\n26\n");
    screen.write_string("27");

    while(true) asm volatile("hlt");
}