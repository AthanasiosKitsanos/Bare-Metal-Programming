#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

extern "C" uint32_t kernel_call;

extern "C" [[noreturn]] void kernel_main()
{
    cpu::gdt::initialize();
    
    kernel::initialize_pit(timer_frequency_hz);
    kernel::set_timer_frequency(timer_frequency_hz);
    terminal::output::initialize();
    terminal::output out{};
    // out << cpu::features::get();
    
    {
        kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};
        kernel::memory::pmm_initialize(&map, reinterpret_cast<uintptr_t>(&_kernel_start), reinterpret_cast<uintptr_t>(&_kernel_end));
    }
    
    kernel::initialize_exceptions();
    drivers::initialize();

    const size_t free_frames{kernel::memory::pmm_free_frames()};
    out << "Used Frames: " << kernel::memory::pmm_used_frames()
    << "\nFree Frames: " << free_frames << '\n';
    
    constexpr size_t frames{32371};
    const uint8_t* ptr{reinterpret_cast<const uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(frames))};
    if(!ptr) out << "Not enough frames\n";
    else
    {
        out << "Allocated: " << frames
        << "\nAddress: " << terminal::hex << reinterpret_cast<uintptr_t>(ptr) << '\n';
    }

    asm volatile("sti");

    for(;;) 
    {
        asm volatile("hlt");
    }
}