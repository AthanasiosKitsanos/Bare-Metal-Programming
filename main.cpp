#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;
extern "C" uint32_t _kernel_stack_top;
extern "C" uint32_t _kernel_stack_bottom;
extern "C" uint32_t _interrupt_stack_top;
extern "C" uint32_t _interrupt_stack_bottom;

extern "C" [[noreturn]] void kernel_main()
{
    const uintptr_t kernel_start{reinterpret_cast<uintptr_t>(&_kernel_start)};
    const uintptr_t kernel_end{reinterpret_cast<uintptr_t>(&_kernel_end)};

    const uintptr_t kernel_stack_top{reinterpret_cast<uintptr_t>(&_kernel_stack_top)};
    const uintptr_t kernel_stack_bottom{reinterpret_cast<uintptr_t>(&_kernel_stack_bottom)};
    const uintptr_t interrupt_stack_top{reinterpret_cast<uintptr_t>(&_interrupt_stack_top)};
    const uintptr_t interrupt_stack_bottom{reinterpret_cast<uintptr_t>(&_interrupt_stack_bottom)};

    kernel::initialize_pit(timer_frequency_hz);
    kernel::set_timer_frequency(timer_frequency_hz);
    terminal::output::initialize();

    kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};
    kernel::memory::pmm_initialize(&map, kernel_start, kernel_end);
    
    kernel::initialize_exceptions();
    
    driver::initialize_keyboard();

    {
        terminal::output out{};
        terminal::input in{};
        out << "Kernel stack size: " << kernel_stack_top - kernel_stack_bottom
        << "\nInterrupt stack size: " << interrupt_stack_top - interrupt_stack_bottom << '\n';
    }

    app::shell shell{};
    
    asm volatile("sti");

    shell.run();

    for(;;)
    {
        asm volatile("hlt");
    }
}