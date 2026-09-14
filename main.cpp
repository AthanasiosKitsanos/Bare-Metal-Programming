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
    
    const uint8_t* first_allocation{nullptr};
    const uint8_t* second_allocation{nullptr};
    terminal::output out{};
    const size_t frames{kernel::memory::pmm_free_frames()};
    out << "Free Frames before first allication: " << frames << '\n';
    {
        diagnostics::stopwatch watch{};
        first_allocation = reinterpret_cast<const uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(32480));
        if(!first_allocation) out << "Failed to do first allocation\n";
    }
    const size_t frames_after{kernel::memory::pmm_free_frames()};
    if(frames_after < frames)
    {
        out << "Free frames after first allocation: " << frames_after << '\n';
    }
    else out << "Free frames did not chagne: " << frames_after << '\n';

    {
        diagnostics::stopwatch watch{};
        second_allocation = reinterpret_cast<const uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(141));
        if(!second_allocation) out << "Failed to do second allocation\n";
    }

    if(first_allocation)
    {
        diagnostics::stopwatch watch{};
        kernel::memory::pmm_free_contiguous_frames(first_allocation, 32480);
    }
    if(second_allocation)
    {
        diagnostics::stopwatch watch{};
        kernel::memory::pmm_free_contiguous_frames(second_allocation, 141);
    }

    // app::shell shell{};
    
    asm volatile("sti");

    // shell.run();

    for(;;) 
    {
        asm volatile("hlt");
    }
}