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
    
    uintptr_t heap_start{0};
    kernel::memory::pmm_allocate_frame(&heap_start);
    kernel::memory::heap_initialize(reinterpret_cast<void*>(heap_start), kernel::memory::frame_size);

    using namespace kernel::memory;

    void* x{kmalloc(64)};
    void* y{kmalloc(64)};

    logger.info() << "x: " << x << '\n';
    logger.info() << "y: " << y << '\n';

    kfree(y);
    kfree(x);
    void* z{kmalloc(140)};
    logger.info() << "z: " << z << '\n';

    app::shell shell{&console};
    
    asm volatile("sti");

    shell.run();

    for(;;)
    {
        asm volatile("hlt");
    }
}