#include <stdint.h>
#include "terminal.h"
#include "kernel_logger.h"
#include "kernel_assert.h"

constexpr color_code yellow_color{static_cast<color_code>(vga_color::yellow) | (static_cast<color_code>(vga_color::black) << 4)};
constexpr color_code previous_color{yellow_color};

extern "C" [[noreturn]] void kernel_main()
{
    kernel::terminal console{};
    kernel::logger logger{&console};
    console.initialize();
    kernel::set_assert_logger(&logger);

    KERNEL_ASSERT(console.in_default_color());
    console.set_color_code(yellow_color);
    logger.info() << "Hello from yellow world!\n";

    KERNEL_ASSERT_MSG(console.current_color_code() == previous_color, "Logger did not restore the previous color!");
    console.reset_color();
    KERNEL_ASSERT(console.in_default_color());

    console << "Program Terminated Succesfully!\n";
    while(true) asm volatile("cli; hlt");
}