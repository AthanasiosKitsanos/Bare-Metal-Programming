#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

extern "C" [[noreturn]] void kernel_main()
{
    
    kernel::initialize_pit(timer_frequency_hz);
    kernel::set_timer_frequency(timer_frequency_hz);
    terminal::output::initialize();
    
    kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};

    {
        const uintptr_t kernel_start{reinterpret_cast<uintptr_t>(&_kernel_start)};
        const uintptr_t kernel_end{reinterpret_cast<uintptr_t>(&_kernel_end)};
        kernel::memory::pmm_initialize(&map, kernel_start, kernel_end);
    }
    
    kernel::initialize_exceptions();
    
    driver::initialize_keyboard();

    app::shell shell{};
    
    asm volatile("sti");

    shell.run();

    for(;;)
    {
        asm volatile("hlt");
    }
}