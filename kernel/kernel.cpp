#include <stdint.h>
#include "terminal.h"
#include "kernel_logger.h"
#include "kernel_assert.h"

constexpr color_code yellow_color{static_cast<color_code>(vga_color::yellow) | (static_cast<color_code>(vga_color::black) << 4)};

extern "C" [[noreturn]] void kernel_main()
{
    kernel::terminal console{};
    kernel::logger logger{&console};
    console.initialize();
    kernel::set_assert_logger(&logger);
 
    
    
    logger.info() << "Program Terminated Succesfully!\n";
    while(true) asm volatile("cli; hlt");
}