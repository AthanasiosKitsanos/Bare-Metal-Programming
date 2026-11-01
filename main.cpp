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

constexpr const char* pmm_res[] =
{
        "success",
        "failed",
        "reserved",
        "out of range",
        "zero_frames"
};

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
        out << terminal::hex << "Kernel_end: " << reinterpret_cast<uintptr_t>(&_kernel_end) << '\n';
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

    using namespace kernel::memory;

    const uint8_t* ptr1{reinterpret_cast<uint8_t*>(pmm_allocate_frame())};
    if(ptr1) out << "1. ptr1: " << terminal::hex << reinterpret_cast<uintptr_t>(ptr1) << " Used Frames: " << terminal::dec << pmm_used_frames() << '\n';

    const uint8_t* ptr2{reinterpret_cast<uint8_t*>(pmm_allocate_frame())};
    if(ptr2) out << "2. ptr2: " << terminal::hex << reinterpret_cast<uintptr_t>(ptr2) << " Used Frames: " << terminal::dec << pmm_used_frames() << '\n';

    pmm_result res{pmm_free_frame(ptr1)};
    out << "3. ptr1 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_frame(reinterpret_cast<uint8_t*>(0x1000));
    out << "4. 0x1000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_frame(reinterpret_cast<uint8_t*>(0x5000));
    out << "5. 0x5000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_frame(reinterpret_cast<uint8_t*>(0x0));
    out << "6. 0x0 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_frame(reinterpret_cast<uint8_t*>(0x100000));
    out << "7. 0x100000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_frame(reinterpret_cast<uint8_t*>(0x107000));
    out << "8. 0x107000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_frame(reinterpret_cast<uint8_t*>(0x108000));
    out << "9. 0x108000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_frame(reinterpret_cast<uint8_t*>(0x7FE0000));
    out << "10. 0x7FE0000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    const uint8_t* ptr3{reinterpret_cast<uint8_t*>(pmm_allocate_frame())};
    if(ptr2) out << "11. ptr3: " << terminal::hex << reinterpret_cast<uintptr_t>(ptr3) << " Used Frames: " << terminal::dec << pmm_used_frames() << '\n';

    out << "Used Frames Before try 12: " << pmm_used_frames() << '\n';
    const uint8_t* ptr4{reinterpret_cast<uint8_t*>(pmm_allocate_contiguous_frames(4))};
    if(ptr4) out << "\n12. ptr4: " << terminal::hex << reinterpret_cast<uintptr_t>(ptr4) << " Used Frames: " << terminal::dec << pmm_used_frames() << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0x3000), 4);
    out << "13. 0x3000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0x3000), 4);
    out << "14. 0x3000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0x1000), 3);
    out << "15. 0x1000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0x1000), 0);
    out << "16. 0x1000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0xFF000), 2);
    out << "17. 0xFF000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0xFA000), 20);
    out << "18. 0xFA000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0x7FDA000), 10);
    out << "19. 0x7FDA000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0x7FDA000), 6);
    out << "20. 0x7FDA000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

    res = pmm_free_contiguous_frames(reinterpret_cast<uint8_t*>(0x1000), 2);
    out << "21. 0x1000 Result: " << pmm_res[static_cast<uint8_t>(res)] << '\n';

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
