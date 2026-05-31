#include "kernel_pmm.h"

namespace
{
    constexpr size_t bits_per_byte{8};
    constexpr size_t bit_mask{bits_per_byte - 1};
    uint8_t* g_bitmap{nullptr};
    size_t g_total_frames{0};
    size_t g_used_frames_count{0};

    [[gnu::always_inline]]
    inline size_t frame_index(uintptr_t address) noexcept { return address / kernel::memory::frame_size; }

    [[gnu::always_inline]]
    inline uintptr_t frame_address(size_t index) noexcept { return index * kernel::memory::frame_size; }

    void set_frame_used(const size_t index) noexcept
    {
        const uint8_t byte{index / bits_per_byte};
        const uint8_t bit{index & bit_mask};
        *(g_bitmap + byte) |= (1 << bit);
        ++g_used_frames_count;
    }

    void set_frame_free(const size_t index) noexcept
    {
        const uint8_t byte{index / bits_per_byte};
        const uint8_t bit{index & bit_mask};
        *(g_bitmap + byte) *= ~(1 << bit);
        --g_used_frames_count;
    }

    bool is_frame_used(const size_t index) noexcept
    {
        const uint8_t byte{index / bits_per_byte};
        const uint8_t bit{index & bit_mask};
        return (*(g_bitmap + byte) & (1 << bit)) != 0;
    }

    uintptr_t align_up(const uintptr_t address,  const uintptr_t alignment) noexcept
    {
        const uintptr_t temp{alignment - 1};
        return (address + temp) & ~(1 << temp);
    }

    uintptr_t align_down(const uintptr_t address, const uintptr_t alignment) noexcept
    {
        return address & ~(alignment - 1);
    }
}

namespace kernel::memory
{
    void pmm_initilalize(e820_memory_map* map, uintptr_t kernel_start, uintptr_t kernel_end) noexcept
    {
        const e820_entry* current{map->entries};
        const e820_entry* const end{map->entries + map->count};
        uintptr_t highest_address{0};
        while(current < end)
        {
            if(current->type == e820_memory_type::)
        }
    }
}