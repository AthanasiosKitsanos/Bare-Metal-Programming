#include <stdint.h>
#include "io/output/terminal_output.h"
#include "logger/kernel_logger.h"
#include "exceptions/kernel_exceptions.h"
#include "timer/kernel_timer.h"
#include "pit/kernel_pit.h"
#include "keyboard/keyboard.h"
#include "internal/kernel_interrupt_guard.h"
#include "memory/e820/kernel_e820.h"
#include "memory/pmm/kernel_pmm.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint8_t _kernel_start;
extern "C" uint8_t _kernel_end;

const uintptr_t kernel_start = reinterpret_cast<uintptr_t>(&_kernel_start);
const uintptr_t kernel_end = reinterpret_cast<uintptr_t>(&_kernel_end);

extern "C" [[noreturn]] void kernel_main()
{
    // Test pmm
    {
        kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};
        kernel::memory::pmm_initialize(&map, kernel_start, kernel_end);
    }

    terminal::output console{};
    kernel::logger logger{&console};
    console.initialize();

    kernel::set_exception_logger(&logger);
    kernel::initialize_exceptions();

    kernel::set_timer_logger(&logger);

    if(!kernel::initialize_pit(timer_frequency_hz))
    {
        logger.panic("Failed to initialize PIT");
    }
    kernel::set_timer_frequency(timer_frequency_hz);

    if(!driver::initialize_keyboard())
    {
        logger.warning() << "Failed to synchronize keyboard\n";
    }
    

    console << "Used frames: " << kernel::memory::pmm_used_frames();

    uintptr_t address_allocated{kernel::memory::pmm_allocate_frame()};
    console << "\nUsed frames: " << kernel::memory::pmm_used_frames() << "\n\n";

    kernel::memory::pmm_free_frame(address_allocated, &console);
    console << "\nUsed frames: " << kernel::memory::pmm_used_frames() << '\n';
    
    asm volatile("sti");

    for(;;)
    {
        asm volatile("hlt");
    }
}