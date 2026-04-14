#include <stdint.h>
#include "terminal.h"
#include "logger.h"

extern "C" [[noreturn]] void kernel_main()
{
    kernel::terminal screen{};
    kernel::logger logger{&screen};
    screen.initialize();
    logger.error() << "Something went wrong\n";
    logger.warning() << "Warning Testing\n";
    logger.info() << "Info Testing\n";
    screen << "Check if color is reset back to white\n";
    logger.debug() << "Debug Testing\n";
    
    logger.panic("End of program!");
}