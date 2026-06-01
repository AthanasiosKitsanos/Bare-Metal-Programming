#include "memory/pmm/kernel_pmm.h"
#include "memory/e820/kernel_e820.h"

namespace
{
    uint8_t* g_bitmap{nullptr};
    size_t g_total_frames{0};
    size_t g_used_frames{0};

    constexpr size_t get_times_two_is_powered(size_t size) noexcept
    {
        size_t power{0};
        while(size > 1)
        {
            size = (size >> 1);
            ++power;
        }
        return power;
    }

    constexpr size_t frame_size_bit_mask{get_times_two_is_powered(kernel::memory::frame_size)};

    [[gnu::always_inline]]
    inline size_t frame_index(const uintptr_t address) noexcept { return address >> frame_size_bit_mask; }

    [[gnu::always_inline]]
    inline uintptr_t frame_address(const size_t index) noexcept { return index << frame_size_bit_mask; }

    constexpr uint8_t bit_size_byte{8};
    constexpr uint8_t bit_size_byte_mask{get_times_two_is_powered(bit_size_byte)};
    constexpr uint8_t bit_mask{bit_size_byte - 1};
    
    struct bit_n_byte
    {
        uint8_t byte;
        uint8_t bit;
    };

    [[gnu::always_inline]]
    inline bit_n_byte get_bit_n_byte(const size_t index) noexcept
    {
        return {(index >> bit_size_byte_mask), (index & bit_mask)};
    }

    [[gnu::always_inline]]
    void set_frame_used(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        *(g_bitmap + pair.byte) |= (1 << pair.bit);
    }

    void set_frame_free(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        *(g_bitmap + pair.byte) &= ~(1 << pair.bit);
        --g_used_frames;
    }

    bool is_frame_used(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        return (*(g_bitmap + pair.byte) & (1 << pair.bit)) != 0;
        ++g_used_frames;
    }

    uintptr_t max(const kernel::memory::e820_entry* entry) noexcept
    {
        return static_cast<uintptr_t>(entry->base + entry->length);
    }
}

namespace kernel::memory
{
    void pmm_initialize(const e820_memory_map* map, const uintptr_t kernel_start, const uintptr_t kernel_end) noexcept
    {
        uintptr_t highest_address{0};
        {
            const e820_entry* const end{map->entries + map->count};
            uintptr_t current{0};
            for(const e820_entry* start{map->entries}; start < end; ++start)
            {
                current = max(start);
                if(highest_address < current) highest_address = current;
            }
        }
    }
}