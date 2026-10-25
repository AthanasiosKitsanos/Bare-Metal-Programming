#include "main.h"

constexpr uint32_t timer_frequency_hz{100};

extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;

extern "C" uint32_t kernel_call;

// enum class e820_memory_type: uint32_t
//     {
//         usable = 1,
//         reserved = 2,
//         acpi_reclaimable = 3,
//         acpi_nvs = 4,
//         bad_memory = 5
//     };

constexpr const char* types[] =
{
    " ",
    "Usable",
    "Reserved",
    "Acpi_Reclaimable",
    "Acpi_nvs",
    "Bad Memory"
};

constexpr uint64_t sqr_64() noexcept
{
    uint64_t result{2};
    for(uint8_t i{0}; i < 64; ++i)
    {
        result *= 2;
    }
    return ((result - 1) >> 12);
}

constexpr uint64_t result{sqr_64()};

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
        out << "Map Count: " << map.count << '\n';
        // const uintptr_t kernel_start{reinterpret_cast<uintptr_t>(&_kernel_start)};
        // const uintptr_t kernel_end{reinterpret_cast<uintptr_t>(&_kernel_end)};

        // for(const kernel::memory::e820_entry* entry{map.entries}; entry < map.entries + map.count; ++entry)
        // {
        //     out << terminal::hex
        //     << "Type: " << types[static_cast<uint32_t>(entry->type)]
        //     << "\nStart: " << entry->base
        //     << " End: " << entry->base + entry->length;
        //     if(kernel_start >= entry->base && kernel_start < (entry->base + entry->length))
        //     {
        //         out << "\nKernel Start: " << kernel_start;
        //     }
        //     if(kernel_end >= entry->base && kernel_end < (entry->base + entry->length))
        //     {
        //         out << "\nKernel End: " << kernel_end;
        //     }
        //     out << "\n\n";
        // }
    }
 
    // out << "Total Frames: " << kernel::memory::pmm_total_frames()
    // << "\nUsed Frames: " << kernel::memory::pmm_used_frames()
    // << "\nFree Frames: " << kernel::memory::pmm_free_frames() << '\n';
    kernel::initialize_exceptions();
    drivers::initialize();

    asm volatile("sti");

    for(;;) 
    {
        asm volatile("hlt");
    }
}