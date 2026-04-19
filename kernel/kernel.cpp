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
 
    KERNEL_DEBUG_ASSERT(console.in_default_color());
    console.set_color_code(yellow_color);
    const color_code previous_color(console.current_color_code());
    logger.info() << "Hello from yellow world!\n";

    KERNEL_DEBUG_ASSERT_MSG(console.current_color_code() == previous_color, "Logger did not restore the previous color!");
    console.reset_color();
    KERNEL_DEBUG_ASSERT(console.in_default_color());
    int32_t array_32[] = { 1, 2 };
    int64_t array_64[] = { 3, 4 };

    console << array_32 << '\n' << array_64 << '\n';
    logger.info() << "Program Terminated Succesfully!\n";
    while(true) asm volatile("cli; hlt");
}