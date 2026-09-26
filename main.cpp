#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

extern "C" uint32_t kernel_call;

constexpr const char* types[] =
{
    "Nothing",
    "Usable",
    "Reserved",
    "acpi_reclaimable",
    "acpi_nvs",
    "bad_memory"
};

extern "C" [[noreturn]] void kernel_main()
{
    kernel::initialize_pit(timer_frequency_hz);
    kernel::set_timer_frequency(timer_frequency_hz);
    terminal::output::initialize();
    terminal::output out{};
    
    {
        kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};
        out << terminal::hex;
        uint8_t count{1};
        uintptr_t kernel_start{reinterpret_cast<uintptr_t>(&_kernel_start)};
        uintptr_t kernel_end{reinterpret_cast<uintptr_t>(&_kernel_end)};
        uintptr_t entry_size{0};
        for(const kernel::memory::e820_entry* entry{map.entries}; entry < map.entries + map.count; ++entry)
        {
            entry_size = entry->base + entry->length;
            out << count++ << ". entry_base: " << terminal::hex << entry->base
            << " entry_end: " << entry_size
            << " type: " << types[static_cast<uint8_t>(entry->type)]
            << "\nstorage: " << terminal::dec << entry->length << "\n\n";
            if(kernel_start >= entry->base && kernel_start < entry_size)
            {
                out << "Kernel is here\n";
            }
        }
        kernel::memory::pmm_initialize(&map, reinterpret_cast<uintptr_t>(&_kernel_end));
    }
    
    kernel::initialize_exceptions();
    drivers::initialize();

    // struct something
    // {
    //     uint8_t* ptr;
    //     size_t frames;

    //     something(): ptr{nullptr}, frames{0}
    //     {}

    //     something(const size_t f): ptr{nullptr}, frames{f}
    //     {}
    // };

    // out << "Total Frames: " << kernel::memory::pmm_total_frames()
    // << "\nFrames used before 1st allocation: " << kernel::memory::pmm_used_frames();
    // something first{};

    // first.ptr = reinterpret_cast<uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(32480));
    // if(first.ptr)
    // {
    //     first.frames = 32480;
    //     out << "\nAllocated: " << first.frames;
    // }

    // out << "\nFrames used after 1st allocation: " << kernel::memory::pmm_used_frames();
        
    // something second{kernel::memory::pmm_free_frames()};
    // second.ptr = reinterpret_cast<uint8_t*>(kernel::memory::pmm_allocate_contiguous_frames(second.frames));
    // if(second.ptr) out << "\nAllocated: " << second.frames;
    
    // out << "\nFrames used after 2nd allocation: " << kernel::memory::pmm_used_frames() << '\n';

    // app::shell shell{};
    
    asm volatile("sti");

    // shell.run();

    for(;;) 
    {
        asm volatile("hlt");
    }
}