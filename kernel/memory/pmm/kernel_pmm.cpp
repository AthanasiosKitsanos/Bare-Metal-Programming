#include "memory/pmm/kernel_pmm.h"
#include "memory/e820/kernel_e820.h"

namespace
{
    struct bitmap
    {
        uint8_t* start{nullptr};
        const uint8_t* end{nullptr};
    };

    size_t g_used_frames{0};
    size_t g_total_frames{0};
    bitmap g_bitmap{};

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
    inline bit_n_byte get_bit_n_byte(const size_t index) noexcept { return {(index >> bit_size_byte_mask), static_cast<uint8_t>(index & bit_mask)}; }

    [[gnu::always_inline]]
    inline bool is_frame_used(const bit_n_byte* const pair) noexcept
    {
        return (*(g_bitmap.start + pair->byte) & (1 << pair->bit)) != 0;
    }

    [[gnu::always_inline]]
    inline uintptr_t max(const kernel::memory::e820_entry* entry) noexcept
    {
        return static_cast<uintptr_t>(entry->base + entry->length);
    }

    [[gnu::always_inline]]
    inline void set_frame_used(const bit_n_byte* const pair) noexcept
    {
        *(g_bitmap.start + pair->byte) |= (1 << pair->bit);
        ++g_used_frames;
    }

    [[gnu::always_inline]]
    inline void set_frame_free(const bit_n_byte* const pair) noexcept
    {
        *(g_bitmap.start + pair->byte) &= ~(1 << pair->bit);
        --g_used_frames;
    }
}

namespace kernel::memory
{
    size_t pmm_total_frames() noexcept { return (g_bitmap.end - g_bitmap.start) << 3; }
    size_t pmm_used_frames() noexcept { return g_used_frames; }
    size_t pmm_free_frames() noexcept { return pmm_total_frames() - g_used_frames; }

    void pmm_initialize(const e820_memory_map* map, const uintptr_t kernel_start, const uintptr_t kernel_end) noexcept
    {
        const e820_entry* const end{map->entries + map->count};
        {
            uintptr_t highest_address{0};
            uintptr_t current{0};
            {
                for(const e820_entry* start{map->entries}; start < end; ++start)
                {
                    current = max(start);
                    if(highest_address < current) highest_address = current;
                }
            }
            g_total_frames = (highest_address >> frame_size_bit_mask);
        }

        g_bitmap.start = reinterpret_cast<uint8_t*>(kernel_end);
        g_bitmap.end = g_bitmap.start + ((g_total_frames + 7) >> bit_size_byte_mask);

        for(uint8_t* current{g_bitmap.start}; current < g_bitmap.end; ++current)
        {
            *current = 0xFF;
        }
        g_used_frames = g_total_frames;

        {
            bit_n_byte pair{};
            size_t index{0};
            for(const e820_entry* current{map->entries}; current < end; ++current)
            {
                if(current->type == e820_memory_type::usable)
                {
                    const uint64_t end{current->base + current->length};
                    for(uint64_t start{current->base}; start < end; start += frame_size)
                    {
                        index = frame_index(static_cast<uintptr_t>(start));
                        pair = get_bit_n_byte(index);
                        set_frame_free(&pair);
                    }
                }
            }

            for(uintptr_t current{kernel_start}; current < kernel_end; current += frame_size)
            {
                index = frame_index(current);
                pair = get_bit_n_byte(index);
                set_frame_used(&pair);
            }

            for(uint8_t* current{g_bitmap.start}; current < g_bitmap.end; current += frame_size)
            {
                index = frame_index(reinterpret_cast<uintptr_t>(current));
                pair = get_bit_n_byte(index);
                set_frame_used(&pair);
            }
        }
    }

    // Needs more refactoring
    uintptr_t pmm_allocate_frame() noexcept
    {
        uint8_t temp_cpy{0};
        for(const uint8_t* current{g_bitmap.start}; current < g_bitmap.end; ++current)
        {
            temp_cpy = *current;
            if(temp_cpy != 0xFF)
            {
                constexpr uint8_t right_shift{0x01};
                const uint8_t bit_limit{(0x01) << bit_mask};
                for(uint8_t bit_pos{0x01}; bit_pos < bit_limit; bit_pos <<= right_shift)
                {
                    if((temp_cpy & bit_pos) == 0) break;
                }
                return frame_address(current - g_bitmap.start);
            }
        }
        return 0;
    }

    void pmm_free_frame(const uintptr_t address) noexcept
    {
        static_cast<void>(address);
        return;
    }
}