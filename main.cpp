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
    
    // terminal::output out{};
    // out << kernel::memory::pmm_free_frames() << '\n';
    // const uint8_t* ptr_1{reinterpret_cast<uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(32480))};
    // if(!ptr_1) out << "Failed to allocate ptr 1\n";
    // else out << "Allocated ptr 1'\n" << "Free Frames " << kernel::memory::pmm_free_frames() << '\n';

    // const uint8_t* ptr_2{reinterpret_cast<uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(144))};
    // if(!ptr_2) out << "Failed to allocate ptr 2\n";
    // else out << "Allocated ptr 2'\n";

    // if(kernel::memory::pmm_free_contiguous_frames(ptr_1, 32480) == kernel::memory::pmm_result::success)
    // {
    //     out << "Deallocated ptr 1\n";
    // }
    // else out << "Failed to deallocate ptr 1\n";

    // out << kernel::memory::pmm_free_frames() << '\n';

    // if(kernel::memory::pmm_free_contiguous_frames(ptr_2, 144) == kernel::memory::pmm_result::success)
    // {
    //     out << "Deallocated ptr 2\n";
    // }
    // else out << "Failed to deallocate ptr 2\n";

    // out << kernel::memory::pmm_free_frames() << '\n'; 

    app::shell shell{};
    
    asm volatile("sti");

    shell.run();

    for(;;)
    {
        asm volatile("hlt");
    }
}