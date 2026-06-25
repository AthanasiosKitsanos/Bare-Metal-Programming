#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint8_t _kernel_start;
extern "C" uint8_t _kernel_end;

extern "C" [[noreturn]] void kernel_main()
{
    const uintptr_t kernel_start = reinterpret_cast<uintptr_t>(&_kernel_start);
    const uintptr_t kernel_end = reinterpret_cast<uintptr_t>(&_kernel_end);
    
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

    // console << "Size of vga_buffer: " << sizeof(terminal::vga_text_buffer)
    // << "\nSize of vga_cursor: " << sizeof(terminal::vga_hardware_cursor)
    // << "\nSize of terminal::output: " << sizeof(terminal::output)
    // << "\nSize of Terminal::input: " << sizeof(terminal::input)
    // << "\nSize of Shell: " << sizeof(app::shell)
    // << "\nSize of shell_hot: " << sizeof(app::hot)
    // << "\nSize of shell_cold: " << sizeof(app::cold)
    // << "\nSize of keyboard_event: " << sizeof(driver::keyboard::keyboard_event)
    // << "\nmm flag: " << cpu::features::get();
    
    app::shell shell{&console};
    
    asm volatile("sti");

    shell.run();

    for(;;)
    {
        asm volatile("hlt");
    }
}