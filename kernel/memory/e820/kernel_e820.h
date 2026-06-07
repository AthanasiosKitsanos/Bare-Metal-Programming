#pragma once

#include <stdint.h>

namespace kernel::memory
{
    enum class e820_memory_type: uint32_t
    {
        usable = 1,
        reserved = 2,
        acpi_reclaimable = 3,
        acpi_nvs = 4,
        bad_memory = 5
    };

    struct [[gnu::packed]] e820_entry
    {
        uint64_t base;
        uint64_t length;
        e820_memory_type type;    
    };
    static_assert(sizeof(e820_entry) == 20);

    struct e820_memory_map
    {
        const e820_entry* entries;
        uint16_t count;
    };

    e820_memory_map get_e820_memory_map() noexcept;
}