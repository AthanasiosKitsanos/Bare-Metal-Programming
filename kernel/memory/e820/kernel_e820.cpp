#include "kernel_e820.h"

namespace
{
    constexpr uintptr_t e820_count_address{0x500};
    constexpr uintptr_t e820_entries_address{0x502};
}

namespace kernel::memory
{
    e820_memory_map get_e820_memory_map() noexcept
    {
        const uint16_t count{*reinterpret_cast<volatile const uint16_t*>(e820_count_address)};
        const e820_entry* entries{reinterpret_cast<const e820_entry*>(e820_entries_address)};
        return {entries, count};
    }
}