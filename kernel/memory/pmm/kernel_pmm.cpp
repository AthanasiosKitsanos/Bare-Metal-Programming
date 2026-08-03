#include "memory/pmm/kernel_pmm.h"
#include "memory/e820/kernel_e820.h"

namespace
{
    struct bit_n_byte
    {
        size_t byte_index;
        uint8_t bit_index;

        [[gnu::always_inline]]
        inline bool operator<(const bit_n_byte& other) const noexcept
        {
            return byte_index < other.byte_index || (byte_index == other.byte_index && bit_index < other.bit_index);
        }
    };

    struct bitmap
    {
        uint8_t* start{nullptr};
        const uint8_t* search_begin{nullptr};
        const uint8_t* end{nullptr};
        bit_n_byte lower_limit;
    };

    size_t g_used_frames{0};
    size_t g_total_frames{0};
    bitmap g_bitmap{};

    constexpr size_t get_power_of_two(const size_t size) noexcept
    {
        size_t power_of_two{0};
        size_t entries{2};
        while(entries <= size)
        {
            entries <<= 1;
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

    [[gnu::always_inline]]
    inline bit_n_byte get_bit_n_byte(const size_t index) noexcept { return {(index >> bit_size_byte_mask), static_cast<uint8_t>(index & bit_mask)}; }

    [[gnu::always_inline]]
    inline bool is_frame_used(const bit_n_byte* const pair) noexcept
    {
        return (*(g_bitmap.start + pair->byte_index) & (1 << pair->bit_index)) != 0;
    }

    [[gnu::always_inline]]
    inline uintptr_t max(const kernel::memory::e820_entry* entry) noexcept
    {
        return static_cast<uintptr_t>(entry->base + entry->length);
    }

    [[gnu::always_inline]]
    inline void set_frame_used(const bit_n_byte* const pair) noexcept
    {
        *(g_bitmap.start + pair->byte_index) |= (1 << pair->bit_index);
        ++g_used_frames;
    }

    [[gnu::always_inline]]
    inline void set_frame_free(const bit_n_byte* const pair) noexcept
    {
        *(g_bitmap.start + pair->byte_index) &= ~(1 << pair->bit_index);
        --g_used_frames;
    }

    [[gnu::always_inline]]
    inline uint8_t leading_zeros(const uint8_t value) noexcept
    {
        return static_cast<uint8_t>(__builtin_clz(static_cast<unsigned int>(value) << 24));
    }

    [[gnu::always_inline]]
    inline uint8_t trailing_zeros(const uint8_t value) noexcept
    {
        return static_cast<uint8_t>(__builtin_ctz(static_cast<unsigned int>(value)));
    }

    struct run
    {
        uint8_t position{0};
        uint8_t length{0};
    };

    // There is no need for branchless calculations. Those are done for practicing
    constexpr uint8_t find_buried_run_packed(const uint8_t value) noexcept
    {
        run current_run{};
        run best_run{};
        bool is_first_run{false};
        bool is_zero_bit{false};
        bool is_current_better{false};

        for(uint8_t i{1}; i < 7; ++i)
        {
            is_first_run = (current_run.length == 0);
            is_zero_bit = ((value & (1 << i)) == 0);
            current_run.position = (current_run.position * !is_first_run) + (i * is_first_run);
            current_run.length += is_zero_bit;

            is_current_better = (current_run.length > best_run.length);
            best_run.position = (best_run.position * !is_current_better) + (current_run.position * is_current_better);
            best_run.length = (best_run.length * !is_current_better) + (current_run.length * is_current_better);
            current_run.length *= is_zero_bit;
        }
        return static_cast<uint8_t>((best_run.length > 1) * ((best_run.position << 4) | best_run.length));
    }

    constexpr size_t byte_array_size{256};
    struct byte_lut
    {
        uint8_t entries[byte_array_size];
        
        constexpr byte_lut() noexcept: entries{}
        {
            for(size_t i{0}; i < byte_array_size; ++i)
            {
                *(entries + i) = find_buried_run_packed(static_cast<uint8_t>(i));
            }
        }
    };

    constexpr byte_lut buried_zeros_lut{};
}

namespace kernel::memory
{
    size_t pmm_total_frames() noexcept { return g_total_frames; }
    size_t pmm_used_frames() noexcept { return g_used_frames; }
    size_t pmm_free_frames() noexcept { return g_total_frames - g_used_frames; }

    [[gnu::regparm(3)]]
    void pmm_initialize(const e820_memory_map* map, const uintptr_t kernel_start, const uintptr_t kernel_end) noexcept
    {
        const e820_entry* const entry_end{map->entries + map->count};
        {
            uintptr_t highest_address{0};
            uintptr_t current{0};
            {
                for(const e820_entry* start{map->entries}; start < entry_end; ++start)
                {
                    current = max(start);
                    if(highest_address < current) highest_address = current;
                }
            }
            g_total_frames = (highest_address >> frame_size_bit_mask);
        }

        g_bitmap.start = reinterpret_cast<uint8_t*>(kernel_end);
        g_bitmap.end = g_bitmap.start + ((g_total_frames + 7) >> bit_size_byte_mask);

        uint8_t* current{g_bitmap.start};
        constexpr uintptr_t alignment_mask{0x3};
        for(; current < g_bitmap.end && ((reinterpret_cast<uintptr_t>(current) & alignment_mask) != 0); ++current)
        {
            *current = 0xFF;
        }

        {
            uint32_t* current_32{reinterpret_cast<uint32_t*>(current)};
            constexpr uintptr_t alignment_mask_not{~alignment_mask};
            uint32_t* const current_32_end{reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(g_bitmap.end) & alignment_mask_not)};
            for(; current_32 < current_32_end; ++current_32)
            {
                *current_32 = 0xFFFFFFFF;
            }
            current = reinterpret_cast<uint8_t*>(current_32);
        }

        for(; current < g_bitmap.end; ++current)
        {
            *current = 0xFF;
        }
        g_used_frames = g_total_frames;

        size_t index{frame_index(reinterpret_cast<uintptr_t>(g_bitmap.end))};
        bit_n_byte pair{get_bit_n_byte(index)};
        g_bitmap.search_begin = g_bitmap.start + pair.byte_index;
        g_bitmap.lower_limit = pair;

        for(const e820_entry* current{map->entries}; current < entry_end; ++current)
        {
            if(current->type == e820_memory_type::usable)
            {
                const uint64_t end{max(current)};
                for(uint64_t start{current->base}; start < end; start += frame_size)
                {
                    index = frame_index(static_cast<uintptr_t>(start));
                    pair = get_bit_n_byte(index);
                    set_frame_free(&pair);
                }
            }
        }

        const size_t last_frame{(g_bitmap.lower_limit.byte_index << bit_size_byte_mask) + g_bitmap.lower_limit.bit_index};
        for(index = frame_index(kernel_start); index <= last_frame; ++index)
        {
            pair = get_bit_n_byte(index);
            if(!is_frame_used(&pair))
            {
                set_frame_used(&pair);
            }
        }
    }

    [[gnu::regparm(1)]]
    pmm_result pmm_allocate_frame(uintptr_t* const address) noexcept
    {
        for(const uint8_t* current{g_bitmap.search_begin}; current < g_bitmap.end; ++current)
        {
            if(*current != 0xFF)
            {
                bit_n_byte pair{static_cast<size_t>(current - g_bitmap.start), 0x00};
                do
                {
                    if(!is_frame_used(&pair))
                    {
                        set_frame_used(&pair);
                        *address = frame_address((pair.byte_index << bit_size_byte_mask) + pair.bit_index);
                        g_bitmap.search_begin = current + (*current == 0xFF);
                        return pmm_result::success;
                    }
                    ++pair.bit_index;
                }while(pair.bit_index < bit_size_byte);
            }
        }
        return pmm_result::failed;
    }

    [[gnu::regparm(2)]]
    pmm_result pmm_find_contiguous_free_frames(const size_t frames, uintptr_t* const address) noexcept
    {
        size_t run_length{0};
        bit_n_byte run_start_index{};

        uint8_t current_value{0};
        bool is_first_run{false};
        for(const uint8_t* current{g_bitmap.search_begin}; current < g_bitmap.end; ++current)
        {
            current_value = *current;
            is_first_run = (run_length == 0);
            run_start_index.byte_index = (run_start_index.byte_index * !is_first_run) + (static_cast<size_t>((current - g_bitmap.start) * is_first_run));
            run_start_index.bit_index *= !is_first_run;
            if(current_value == 0x00)
            {
                run_length += 8;
                if(run_length >= frames)
                {
                    size_t frame_start{(run_start_index.byte_index << bit_size_byte_mask) + run_start_index.bit_index};
                    bit_n_byte end{get_bit_n_byte(frame_start + frames - 1)};
                    
                }
            }
            else if(current_value == 0xFF) run_length = 0;
            else
            {
                run_length += trailing_zeros(current_value);
                if(run_length >= frames)
                {
                    
                }

                const uint8_t pos_n_length{*(buried_zeros_lut.entries + current_value)};
                run_start_index.byte_index = static_cast<size_t>(current - g_bitmap.start);
                run_start_index.bit_index = static_cast<uint8_t>(pos_n_length >> 4);
                run_length = (pos_n_length & 0x0F);

                if(run_length >= frames)
                {
                    
                }

                run_length = leading_zeros(current_value);
                run_start_index.bit_index = (bit_size_byte - run_length);
                if(run_length >= frames)
                {
                    
                }
            }
        }

        return pmm_result::failed;
    }

    [[gnu::regparm(1)]]
    pmm_result pmm_free_frame(const uintptr_t address) noexcept
    {
        size_t index{frame_index(address)};
        
        if(index >= g_total_frames) return pmm_result::hb_deny;

        const bit_n_byte bitmap_frame_pos{get_bit_n_byte(index)};

        if(bitmap_frame_pos < g_bitmap.lower_limit) return pmm_result::lb_deny;

        if(!is_frame_used(&bitmap_frame_pos)) return pmm_result::failed;
        set_frame_free(&bitmap_frame_pos);

        const uint8_t* const addr{g_bitmap.start + bitmap_frame_pos.byte_index};
        g_bitmap.search_begin -= ((g_bitmap.search_begin - addr) * (addr < g_bitmap.search_begin));
        
        return pmm_result::success;
    }
}