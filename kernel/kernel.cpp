#include <stdint.h>
#include "terminal.h"
#include "kernel_logger.h"
#include "kernel_assert.h"

extern "C" [[noreturn]] void kernel_main()
{
    kernel::terminal screen{};
    kernel::logger logger{&screen};
    screen.initialize();
    
    kernel::set_assert_logger(&logger);
    int value{0};
    KERNEL_ASSERT(value < 0);
    logger.panic("End of Program!");
}