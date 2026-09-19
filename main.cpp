#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

extern "C" [[noreturn]] void kernel_main()
{
    kernel::initialize_pit(timer_frequency_hz);
    kernel::set_timer_frequency(timer_frequency_hz);
    terminal::output::initialize();
    
    {
        kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};
        kernel::memory::pmm_initialize(&map, reinterpret_cast<uintptr_t>(&_kernel_end));
    }
    
    kernel::initialize_exceptions();
    drivers::initialize();

    terminal::output out{};

    struct something
    {
        uint8_t* ptr;
        size_t frames;

        something(): ptr{nullptr}, frames{0}
        {}

        something(const size_t f): ptr{nullptr}, frames{f}
        {}
    };

    something first{};

    first.ptr = reinterpret_cast<uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(32480));
    if(first.ptr)
    {
        first.frames = 32480;
        out << first.frames << '\n';
    }
        
    something second{kernel::memory::pmm_free_frames()};
    second.ptr = reinterpret_cast<uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(second.frames));
    if(second.ptr) out << "Second: " << second.frames << '\n';
    
    // app::shell shell{};
    
    asm volatile("sti");

    // shell.run();

    for(;;) 
    {
        asm volatile("hlt");
    }
}