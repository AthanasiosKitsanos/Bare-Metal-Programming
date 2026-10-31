#include "main.h"

// enum class e820_memory_type: uint32_t
//     {
//         usable = 1,
//         reserved = 2,
//         acpi_reclaimable = 3,
//         acpi_nvs = 4,
//         bad_memory = 5
//     };

// constexpr const char* types[] =
// {
//     " ",
//     "Usable",
//     "Reserved",
//     "Acpi_Reclaimable",
//     "Acpi_nvs",
//     "Bad Memory"
// };

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

    {
        kernel::memory::e820_memory_map map{kernel::memory::get_e820_memory_map()};
        kernel::memory::pmm_initialize(&map, reinterpret_cast<uintptr_t>(&_kernel_start), reinterpret_cast<uintptr_t>(&_kernel_end));

        // for(const kernel::memory::e820_entry* entry{map.entries}; entry < map.entries + map.count; ++entry)
        // {
        //     out << terminal::hex
        //     << "Start: " << entry->base
        //     << " End: " << entry->base + entry->length << " Type: " << types[static_cast<uint32_t>(entry->type)]
        //     << '\n';
        // }

        // out << terminal::dec << "Total frames: " << kernel::memory::pmm_total_frames() << '\n';
    }

    out << "Total Frames: " << kernel::memory::pmm_total_frames()
    << "\nUsed Frames: " << kernel::memory::pmm_used_frames()
    << "\nFree Frames: " << kernel::memory::pmm_free_frames() << '\n';
    uint8_t* ptr{reinterpret_cast<uint8_t*>(kernel::memory::pmm_allocate_frame())};

    if(ptr) ++ptr;
    kernel::initialize_exceptions();
    drivers::initialize();

    asm volatile("sti");

    for(;;)
    {
        asm volatile("hlt");
    }
}
