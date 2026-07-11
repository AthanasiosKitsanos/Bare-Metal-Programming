#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint8_t _kernel_start;
extern "C" uint8_t _kernel_end;

extern "C" [[noreturn]] void kernel_main()
{
    const uintptr_t kernel_start{reinterpret_cast<uintptr_t>(&_kernel_start)};
    const uintptr_t kernel_end{reinterpret_cast<uintptr_t>(&_kernel_end)};
    
    {
        kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};
        kernel::memory::pmm_initialize(&map, kernel_start, kernel_end);
    }

    terminal::output::initialize();
    kernel::initialize_exceptions();

    kernel::initialize_pit(timer_frequency_hz);
    kernel::set_timer_frequency(timer_frequency_hz);

    driver::initialize_keyboard();
    
    uintptr_t heap_start{0};
    kernel::memory::pmm_allocate_frame(&heap_start);
    kernel::memory::heap_initialize(reinterpret_cast<void*>(heap_start), kernel::memory::frame_size);

    app::shell shell{};
    
    asm volatile("sti");

    shell.run();

    for(;;)
    {
        asm volatile("hlt");
    }
}