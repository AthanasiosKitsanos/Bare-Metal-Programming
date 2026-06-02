#include "memory/pmm/kernel_pmm.h"
#include "memory/e820/kernel_e820.h"

namespace
{
    uint8_t* g_bitmap{nullptr};
    size_t g_total_frames{0};
    size_t g_used_frames{0};

    constexpr size_t get_power_of_two(const size_t size) noexcept
    {
        size_t power_of_two{0};
        size_t number{2};
        while(number <= size)
        {
            number <<= 1;
            ++power_of_two;
        }
        return power_of_two;
    }

    constexpr size_t frame_size_bit_mask{get_power_of_two(kernel::memory::frame_size)};

    [[gnu::always_inline]]
    inline size_t frame_index(const uintptr_t address) noexcept { return address >> frame_size_bit_mask; }

    [[gnu::always_inline]]
    inline uintptr_t frame_address(const size_t index) noexcept { return index << frame_size_bit_mask; }

    constexpr uint8_t bit_size_byte{8};
    constexpr uint8_t bit_size_byte_mask{get_power_of_two(bit_size_byte)};
    constexpr uint8_t bit_mask{bit_size_byte - 1};
    
    struct bit_n_byte
    {
        size_t byte;
        uint8_t bit;
    };

    [[gnu::always_inline]]
    inline bit_n_byte get_bit_n_byte(const size_t index) noexcept { return {(index >> bit_size_byte_mask), (index & bit_mask)}; }

    [[gnu::always_inline]]
    inline void set_frame_used(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        *(g_bitmap + pair.byte) |= (1 << pair.bit);
        ++g_used_frames;
    }

    [[gnu::always_inline]]
    inline void set_frame_free(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        *(g_bitmap + pair.byte) &= ~(1 << pair.bit);
        --g_used_frames;
    }

    [[gnu::always_inline]]
    inline bool is_frame_used(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        return (*(g_bitmap + pair.byte) & (1 << pair.bit)) != 0;
    }

    [[gnu::always_inline]]
    inline uintptr_t max(const kernel::memory::e820_entry* entry) noexcept
    {
        return static_cast<uintptr_t>(entry->base + entry->length);
    }
}

namespace kernel::memory
{
    size_t pmm_total_frames() noexcept { return g_total_frames; }
    size_t pmm_used_frames() noexcept { return g_used_frames; }
    size_t pmm_free_frames() noexcept { return g_total_frames - g_used_frames; }

    void pmm_initialize(const e820_memory_map* map, const uintptr_t kernel_start, const uintptr_t kernel_end) noexcept
    {
        const e820_entry* const end{map->entries + map->count};
        {
            uintptr_t highest_address{0};
            {
                uintptr_t current{0};
                for(const e820_entry* start{map->entries}; start < end; ++start)
                {
                    current = max(start);
                    if(highest_address < current) highest_address = current;
                }
            }
            g_total_frames = (highest_address >> frame_size_bit_mask);
        }

        g_bitmap = reinterpret_cast<uint8_t*>(kernel_end);
        const uint8_t* const bitmap_end{g_bitmap + ((g_total_frames + 7) >> bit_size_byte_mask)};

        for(uint8_t* current{g_bitmap}; current < bitmap_end; ++current)
        {
            *current = 0xFF;
        }
        g_used_frames = g_total_frames;

        {
            size_t index{0};
            for(const e820_entry* current{map->entries}; current < end; ++current)
            {
                if(current->type == e820_memory_type::usable)
                {
                    index = frame_index(reinterpret_cast<uintptr_t>(current));
                    set_frame_free(index);
                }
            }
        
            for(uintptr_t current{kernel_start}; current < kernel_end; ++current)
            {
                index = frame_index(current);
                set_frame_used(index);
            }

            {
                const uintptr_t bitmap_end_address{reinterpret_cast<uintptr_t>(bitmap_end)};
                for(uintptr_t bitmap_current_address{kernel_end}; bitmap_current_address < bitmap_end_address; ++bitmap_current_address)
                {
                    index = frame_index(bitmap_current_address);
                    set_frame_used(index);
                }
            }
        }
    }

    uintptr_t pmm_allocate_frame() noexcept
    {
        const uint8_t* const bitmap_end{g_bitmap + ((g_total_frames + 7) >> bit_size_byte_mask)};
        uint8_t byte_index{0};
        for(const uint8_t* byte_ptr{g_bitmap}; byte_ptr < bitmap_end; ++byte_ptr)
        {
            if(*byte_ptr == 0xFF) continue;
            
        }
    }

    void pmm_free_frame(const uintptr_t address) noexcept
    {

    }
}