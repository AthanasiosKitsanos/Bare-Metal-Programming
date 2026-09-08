#include "memory/pmm/kernel_pmm.h"
#include "memory/e820/kernel_e820.h"
#include "cpu/features.h"
#include <immintrin.h>
#include "io/output/terminal_output.h"

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
        const uint8_t* end{nullptr};
        const uint8_t* search_begin{nullptr};
        const uint8_t* search_end{nullptr};
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
    
    template<typename T>
    [[gnu::always_inline]]
    inline uint8_t leading_zeros(const T value) noexcept
    {
        constexpr uint8_t shift
        {
            sizeof(T) == sizeof(uint32_t) ? 0 :
            sizeof(T) == sizeof(uint16_t) ? 16 : 24
        };
        return static_cast<uint8_t>(__builtin_clz(static_cast<unsigned int>(value) << shift));
    }

    template<typename T>
    [[gnu::always_inline]]
    inline uint8_t trailing_zeros(const T value) noexcept
    {
        return static_cast<uint8_t>(__builtin_ctz(static_cast<unsigned int>(value)));
    }

    [[gnu::always_inline]]
    inline uint8_t safe_trailing_zeros(const uint8_t value) noexcept
    {
        return (value !=0) ? trailing_zeros(value) : bit_size_byte;
    }

    [[gnu::always_inline]]
    inline uint8_t safe_leading_zeros(const uint8_t value) noexcept
    {
        return (value != 0) ? leading_zeros(value) : bit_size_byte; 
    }
    
    struct run
    {
        uint8_t position{0};
        uint8_t length{0};
    };

    // This is for finding one free bit without the need to use loops
    constexpr uint8_t dedicated_1_frame_lut(const uint8_t value) noexcept
    {
        uint8_t position{0};
        for(uint8_t i{0}; i < bit_size_byte; ++i)
        {
            if((value & (1 << i)) == 0)
            {
                position = i;
                break;
            }
        }
        return position;
    }

    constexpr size_t dedicated_1_frame_lut_size{255};
    struct dedicated_lut_frame
    {
        uint8_t entries[dedicated_1_frame_lut_size];

        constexpr dedicated_lut_frame(): entries{}
        {
            for(uint8_t i{0}; i < dedicated_1_frame_lut_size; ++i)
            {
                entries[i] = dedicated_1_frame_lut(i);
            }
        }
    };

    constexpr dedicated_lut_frame dedicated_lut{};
    
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
    
    constexpr uint8_t fill_mask{0xFF};
    constexpr uint8_t bit_max_pos{0x07};
    
    [[gnu::always_inline]]
    inline uint8_t front_byte_mask_used(const uint8_t bit_pos) noexcept
    {
        return fill_mask << bit_pos;
    }
    
    [[gnu::always_inline]]
    inline uint8_t back_byte_mask_used(const uint8_t bit_end_pos) noexcept
    {
        return fill_mask >> (bit_max_pos - bit_end_pos);
    }
    
    [[gnu::always_inline]]
    inline void set_frames_in_byte_used(const size_t index, const uint8_t mask, const size_t frames) noexcept
    {
        *(g_bitmap.start + index) |= mask;
        g_used_frames += frames;
    }

    [[gnu::always_inline]]
    inline void mark_whole_byte_used(const size_t index) noexcept
    {
        *(g_bitmap.start + index) = 0xFF;
        g_used_frames += bit_size_byte;
    }
    
    [[gnu::always_inline]]
    inline uint8_t front_byte_free_mask(const size_t index) noexcept
    {
        return fill_mask >> (bit_size_byte - index);
    }

    [[gnu::always_inline]]
    inline uint8_t back_byte_free_mask(const size_t index) noexcept
    {
        return fill_mask << (index + 1);
    }
    
    [[gnu::always_inline]]
    inline void mark_whole_byte_free(const size_t index) noexcept
    {
        *(g_bitmap.start + index) = 0x00;
        g_used_frames -= bit_size_byte;
    }

    [[gnu::always_inline]]
    inline void set_frames_in_byte_free(const size_t index, const uint8_t mask, const size_t frames) noexcept
    {
        *(g_bitmap.start + index) &= mask;
        g_used_frames -= frames;
    }

    // SIMD Methods
    struct allocation_run
    {
        bit_n_byte start_index{};
        size_t length{};
    };

    [[gnu::always_inline]] [[gnu::regparm(3)]]
    inline void contiguous_8_core_inline(allocation_run* const run, const size_t frames, const uint8_t* const end, const uint8_t** start) noexcept
    {
        const uint8_t* current{*start};
        uint8_t current_value{0};
        bool is_first_run{false};

        for(; current < end; ++current)
        {
            current_value = *current;
            is_first_run = (run->length == 0);
            run->start_index.byte_index = (run->start_index.byte_index * !is_first_run) + (static_cast<size_t>(current - g_bitmap.start) * is_first_run);
            run->start_index.bit_index *= !is_first_run;

            if(current_value == 0x00)
            {
                run->length += 8;
                if(run->length >= frames) break;
            }
            else if(current_value == 0xFF) run->length ^= run->length;
            else
            {
                run->length += trailing_zeros(current_value);
                if(run->length >= frames) break;

                const uint8_t pos_n_length{*(buried_zeros_lut.entries + current_value)};

                run->start_index.byte_index = static_cast<size_t>(current - g_bitmap.start);
                run->start_index.bit_index = static_cast<uint8_t>(pos_n_length >> 4);
                run->length = (pos_n_length & 0x0F);
                if(run->length >= frames) break;

                run->length = leading_zeros(current_value);
                run->start_index.bit_index = (bit_size_byte - run->length);
                if(run->length >= frames) break;
            }
        }
        *start = current;
    }

    // IMPORTANT keep is sync with contiguous_8_core_inline right above
    [[gnu::noinline]] [[gnu::regparm(3)]]
    void contiguous_8_core(allocation_run* const run, const size_t frames, const uint8_t* const end, const uint8_t** start) noexcept
    {
        const uint8_t* current{*start};
        uint8_t current_value{0};
        bool is_first_run{false};

        for(; current < end; ++current)
        {
            current_value = *current;
            is_first_run = (run->length == 0);
            run->start_index.byte_index = (run->start_index.byte_index * !is_first_run) + (static_cast<size_t>(current - g_bitmap.start) * is_first_run);
            run->start_index.bit_index *= !is_first_run;

            if(current_value == 0x00)
            {
                run->length += 8;
                if(run->length >= frames) break;
            }
            else if(current_value == 0xFF) run->length ^= run->length;
            else
            {
                run->length += trailing_zeros(current_value);
                if(run->length >= frames) break;

                const uint8_t pos_n_length{*(buried_zeros_lut.entries + current_value)};

                run->start_index.byte_index = static_cast<size_t>(current - g_bitmap.start);
                run->start_index.bit_index = static_cast<uint8_t>(pos_n_length >> 4);
                run->length = (pos_n_length & 0x0F);
                if(run->length >= frames) break;

                run->length = leading_zeros(current_value);
                run->start_index.bit_index = (bit_size_byte - run->length);
                if(run->length >= frames) break;
            }
        }
        *start = current;
    }

    [[gnu::always_inline]] [[gnu::regparm(3)]]
    inline void contiguous_16_core_inline(allocation_run* const run, const size_t frames, const uint16_t* const end, const uint16_t** start) noexcept
    {
        const uint16_t* current{*start};

        uint16_t current_value{0};
        bool is_first_run{false};
        bool greater_equal{false};

        for(; current < end; ++current)
        {
            current_value = *current;
            is_first_run = (run->length == 0);
            run->start_index.byte_index = (run->start_index.byte_index * !is_first_run) + (static_cast<size_t>((reinterpret_cast<const uint8_t*>(current) - g_bitmap.start) * is_first_run));
            run->start_index.bit_index *= !is_first_run;

            if(current_value == 0x0000)
            {
                run->length += 16;
                if(run->length >= frames) return;
            }
            else if(current_value == 0xFFFF) run->length ^= run->length;
            else
            {
                run->length += trailing_zeros(current_value);
                if(run->length >= frames) return;

                uint8_t byte_n_bit_pos{0};
                uintptr_t absolute_address{static_cast<uintptr_t>(reinterpret_cast<const uint8_t*>(current) - g_bitmap.start)};
                {
                    uint8_t pos_n_length{0};
                    uint8_t temp_lut_value{0};
                    for(uint8_t right_shift{0}; right_shift < 16; right_shift += 8)
                    {
                        temp_lut_value = *(buried_zeros_lut.entries + static_cast<uint8_t>(current_value >> right_shift));
                        greater_equal = ((pos_n_length & 0x0F) >= ((temp_lut_value & 0x0F)));
                        pos_n_length = (pos_n_length * greater_equal) + (temp_lut_value * !greater_equal);
                        byte_n_bit_pos = static_cast<uint8_t>((byte_n_bit_pos * greater_equal) + (((right_shift << 1) | (pos_n_length >> 4)) * !greater_equal));
                    }

                    const uint8_t byte0_leading{safe_leading_zeros(static_cast<uint8_t>(current_value))};
                    const uint8_t length_sum{static_cast<uint8_t>(byte0_leading + safe_trailing_zeros(static_cast<uint8_t>(current_value >> 8)))};

                    uint8_t temp_length{static_cast<uint8_t>(pos_n_length & 0x0F)};

                    greater_equal = (temp_length >= length_sum);
                    temp_length = (temp_length * greater_equal) + (length_sum * !greater_equal);
                    byte_n_bit_pos = (byte_n_bit_pos * greater_equal) + (static_cast<uint8_t>(0x00 | (bit_size_byte - byte0_leading)) * !greater_equal);

                    run->length = temp_length;
                    run->start_index.byte_index = static_cast<size_t>(absolute_address + (byte_n_bit_pos >> 4));
                    run->start_index.bit_index = static_cast<uint8_t>(byte_n_bit_pos & 0x0F);
                    if(run->length >= frames) return;
                }


                constexpr uint8_t word_bits{16};
                const uint8_t l_zeros{leading_zeros(current_value)};
                greater_equal = (run->length >= l_zeros);
                run->length = (run->length * greater_equal) + (l_zeros * !greater_equal);

                const uint8_t bit_index{static_cast<uint8_t>(word_bits - l_zeros)};
                byte_n_bit_pos = static_cast<uint8_t>((bit_index >> bit_size_byte_mask) << 4) | (bit_index & bit_mask);
                run->start_index.byte_index = static_cast<size_t>((run->start_index.byte_index * greater_equal) + ((absolute_address + (byte_n_bit_pos >> 4)) * !greater_equal));
                run->start_index.bit_index = (run->start_index.bit_index * greater_equal) + ((byte_n_bit_pos & 0x0F) * !greater_equal);
                if(run->length >= frames) return;
            }
        }
        *start = current;
    }

    //IMPORTANT: Keep is sync with the contiguous_16_core_inline right above
    [[gnu::noinline]] [[gnu::regparm(3)]]
    void contiguous_16_core(allocation_run* const run, const size_t frames, const uint16_t* const end, const uint16_t** start) noexcept
    {
        const uint16_t* current{*start};

        uint16_t current_value{0};
        bool is_first_run{false};
        bool greater_equal{false};

        for(; current < end; ++current)
        {
            current_value = *current;
            is_first_run = (run->length == 0);
            run->start_index.byte_index = (run->start_index.byte_index * !is_first_run) + (static_cast<size_t>((reinterpret_cast<const uint8_t*>(current) - g_bitmap.start) * is_first_run));
            run->start_index.bit_index *= !is_first_run;

            if(current_value == 0x0000)
            {
                run->length += 16;
                if(run->length >= frames) return;
            }
            else if(current_value == 0xFFFF) run->length ^= run->length;
            else
            {
                run->length += trailing_zeros(current_value);
                if(run->length >= frames) return;

                uint8_t byte_n_bit_pos{0};
                uintptr_t absolute_address{static_cast<uintptr_t>(reinterpret_cast<const uint8_t*>(current) - g_bitmap.start)};
                {
                    uint8_t pos_n_length{0};
                    uint8_t temp_lut_value{0};
                    for(uint8_t right_shift{0}; right_shift < 16; right_shift += 8)
                    {
                        temp_lut_value = *(buried_zeros_lut.entries + static_cast<uint8_t>(current_value >> right_shift));
                        greater_equal = ((pos_n_length & 0x0F) >= ((temp_lut_value & 0x0F)));
                        pos_n_length = (pos_n_length * greater_equal) + (temp_lut_value * !greater_equal);
                        byte_n_bit_pos = static_cast<uint8_t>((byte_n_bit_pos * greater_equal) + (((right_shift << 1) | (pos_n_length >> 4)) * !greater_equal));
                    }

                    const uint8_t byte0_leading{safe_leading_zeros(static_cast<uint8_t>(current_value))};
                    const uint8_t length_sum{static_cast<uint8_t>(byte0_leading + safe_trailing_zeros(static_cast<uint8_t>(current_value >> 8)))};

                    uint8_t temp_length{static_cast<uint8_t>(pos_n_length & 0x0F)};

                    greater_equal = (temp_length >= length_sum);
                    temp_length = (temp_length * greater_equal) + (length_sum * !greater_equal);
                    byte_n_bit_pos = (byte_n_bit_pos * greater_equal) + (static_cast<uint8_t>(0x00 | (bit_size_byte - byte0_leading)) * !greater_equal);

                    run->length = temp_length;
                    run->start_index.byte_index = static_cast<size_t>(absolute_address + (byte_n_bit_pos >> 4));
                    run->start_index.bit_index = static_cast<uint8_t>(byte_n_bit_pos & 0x0F);
                    if(run->length >= frames) return;
                }


                constexpr uint8_t word_bits{16};
                const uint8_t l_zeros{leading_zeros(current_value)};
                greater_equal = (run->length >= l_zeros);
                run->length = (run->length * greater_equal) + (l_zeros * !greater_equal);

                const uint8_t bit_index{static_cast<uint8_t>(word_bits - l_zeros)};
                byte_n_bit_pos = static_cast<uint8_t>((bit_index >> bit_size_byte_mask) << 4) | (bit_index & bit_mask);
                run->start_index.byte_index = static_cast<size_t>((run->start_index.byte_index * greater_equal) + ((absolute_address + (byte_n_bit_pos >> 4)) * !greater_equal));
                run->start_index.bit_index = (run->start_index.bit_index * greater_equal) + ((byte_n_bit_pos & 0x0F) * !greater_equal);
                if(run->length >= frames) return;
            }
        }
        *start = current;
    }

    [[gnu::regparm(3)]] [[gnu::always_inline]]
    inline void contiguous_32_core_inline(allocation_run* const run, const size_t frames, const uint32_t* const end, const uint32_t** start) noexcept
    {
        const uint32_t* current{*start};

        uint32_t current_value{0};
        bool is_first_run{false};
        bool greater_equal{false};

        for(; current < end; ++current)
        {
            current_value = *current;
            is_first_run = (run->length == 0);
            run->start_index.byte_index = (run->start_index.byte_index * !is_first_run) + (static_cast<size_t>((reinterpret_cast<const uint8_t*>(current) - g_bitmap.start) * is_first_run));
            run->start_index.bit_index *= !is_first_run;

            if(current_value == 0x00000000)
            {
                run->length += 32;
                if(run->length >= frames) return;
            }
            else if(current_value == 0xFFFFFFFF) run->length ^= run->length;
            else
            {
                run->length += trailing_zeros(current_value);
                if(run->length >= frames) return;

                uint8_t byte_n_bit_pos{0};
                uintptr_t absolute_address{static_cast<uintptr_t>(reinterpret_cast<const uint8_t*>(current) - g_bitmap.start)};
                {
                    uint8_t pos_n_length{0};
                    {
                        uint8_t temp_lut_value{0};
                        for(uint8_t right_shift{0}; right_shift < 32; right_shift += 8)
                        {
                            temp_lut_value = *(buried_zeros_lut.entries + static_cast<uint8_t>(current_value >> right_shift));
                            greater_equal = ((pos_n_length & 0x0F) >= ((temp_lut_value & 0x0F)));
                            pos_n_length = (pos_n_length * greater_equal) + (temp_lut_value * !greater_equal);
                            byte_n_bit_pos = static_cast<uint8_t>((byte_n_bit_pos * greater_equal) + (((right_shift << 1) | (pos_n_length >> 4)) * !greater_equal));
                        }
                    }

                    {
                        uint8_t byte0_leading{0};
                        uint8_t byte1_trailing{0};
                        uint8_t byte1_leading{0};
                        uint8_t byte2_trailing{0};
                        uint8_t byte2_leading{0};
                        uint8_t byte3_trailing{0};

                        uint8_t value{static_cast<uint8_t>(current_value)};
                        byte0_leading = safe_leading_zeros(value);

                        value = static_cast<uint8_t>(current_value >> 8);
                        byte1_trailing = safe_trailing_zeros(value);
                        byte1_leading = safe_leading_zeros(value);

                        value = static_cast<uint8_t>(current_value >> 16);
                        byte2_trailing = safe_trailing_zeros(value);
                        byte2_leading = safe_leading_zeros(value);

                        value = static_cast<uint8_t>(current_value >> 24);
                        byte3_trailing = safe_trailing_zeros(value);

                        const uint8_t length_sums[] =
                        {
                            static_cast<uint8_t>(byte0_leading + byte1_trailing),
                            static_cast<uint8_t>(byte1_leading + byte2_trailing),
                            static_cast<uint8_t>(byte2_leading + byte3_trailing)
                        };

                        const uint8_t packed_positions[] =
                        {
                            static_cast<uint8_t>(0x00 | (bit_size_byte - byte0_leading)),
                            static_cast<uint8_t>(0x10 | (bit_size_byte - byte1_leading)),
                            static_cast<uint8_t>(0x20 | (bit_size_byte - byte2_leading))
                        };

                        uint8_t length_sum{0};
                        uint8_t temp_length{static_cast<uint8_t>(pos_n_length & 0x0F)};

                        for(uint8_t i{0}; i < 3; ++i)
                        {
                            length_sum = *(length_sums + i);
                            greater_equal = (temp_length >= length_sum);
                            temp_length = (temp_length * greater_equal) + (length_sum * !greater_equal);
                            byte_n_bit_pos = (byte_n_bit_pos * greater_equal) + (*(packed_positions + i) * !greater_equal);
                        }

                        run->length = temp_length;
                        run->start_index.byte_index = static_cast<size_t>(absolute_address + (byte_n_bit_pos >> 4));
                        run->start_index.bit_index = static_cast<uint8_t>(byte_n_bit_pos & 0x0F);
                        if(run->length >= frames) return;
                    }
                }


                constexpr uint8_t word_bits{32};
                const uint8_t l_zeros{leading_zeros(current_value)};
                greater_equal = (run->length >= l_zeros);
                run->length = (run->length * greater_equal) + (l_zeros * !greater_equal);

                const uint8_t bit_index{static_cast<uint8_t>(word_bits - l_zeros)};
                 
                byte_n_bit_pos = static_cast<uint8_t>((bit_index >> bit_size_byte_mask) << 4) | (bit_index & bit_mask);
                
                run->start_index.byte_index = static_cast<size_t>((run->start_index.byte_index * greater_equal) + ((absolute_address + (byte_n_bit_pos >> 4)) * !greater_equal));
                run->start_index.bit_index = (run->start_index.bit_index * greater_equal) + ((byte_n_bit_pos & 0x0F) * !greater_equal);
                if(run->length >= frames) return;
            }
        }
        *start = current;
    }

    //IMPORTANT: Keep is sync with the contiguous_32_core_inline right above
    [[gnu::regparm(3)]] [[gnu::noinline]]
    void contiguous_32_core(allocation_run* const run, const size_t frames, const uint32_t* const end, const uint32_t** start) noexcept
    {
        const uint32_t* current{*start};

        uint32_t current_value{0};
        bool is_first_run{false};
        bool greater_equal{false};

        for(; current < end; ++current)
        {
            current_value = *current;
            is_first_run = (run->length == 0);
            run->start_index.byte_index = (run->start_index.byte_index * !is_first_run) + (static_cast<size_t>((reinterpret_cast<const uint8_t*>(current) - g_bitmap.start) * is_first_run));
            run->start_index.bit_index *= !is_first_run;

            if(current_value == 0x00000000)
            {
                run->length += 32;
                if(run->length >= frames) return;
            }
            else if(current_value == 0xFFFFFFFF) run->length ^= run->length;
            else
            {
                run->length += trailing_zeros(current_value);
                if(run->length >= frames) return;

                uint8_t byte_n_bit_pos{0};
                uintptr_t absolute_address{static_cast<uintptr_t>(reinterpret_cast<const uint8_t*>(current) - g_bitmap.start)};
                {
                    uint8_t pos_n_length{0};
                    {
                        uint8_t temp_lut_value{0};
                        for(uint8_t right_shift{0}; right_shift < 32; right_shift += 8)
                        {
                            temp_lut_value = *(buried_zeros_lut.entries + static_cast<uint8_t>(current_value >> right_shift));
                            greater_equal = ((pos_n_length & 0x0F) >= ((temp_lut_value & 0x0F)));
                            pos_n_length = (pos_n_length * greater_equal) + (temp_lut_value * !greater_equal);
                            byte_n_bit_pos = static_cast<uint8_t>((byte_n_bit_pos * greater_equal) + (((right_shift << 1) | (pos_n_length >> 4)) * !greater_equal));
                        }
                    }

                    {
                        uint8_t byte0_leading{0};
                        uint8_t byte1_trailing{0};
                        uint8_t byte1_leading{0};
                        uint8_t byte2_trailing{0};
                        uint8_t byte2_leading{0};
                        uint8_t byte3_trailing{0};

                        uint8_t value{static_cast<uint8_t>(current_value)};
                        byte0_leading = safe_leading_zeros(value);

                        value = static_cast<uint8_t>(current_value >> 8);
                        byte1_trailing = safe_trailing_zeros(value);
                        byte1_leading = safe_leading_zeros(value);

                        value = static_cast<uint8_t>(current_value >> 16);
                        byte2_trailing = safe_trailing_zeros(value);
                        byte2_leading = safe_leading_zeros(value);

                        value = static_cast<uint8_t>(current_value >> 24);
                        byte3_trailing = safe_trailing_zeros(value);

                        const uint8_t length_sums[] =
                        {
                            static_cast<uint8_t>(byte0_leading + byte1_trailing),
                            static_cast<uint8_t>(byte1_leading + byte2_trailing),
                            static_cast<uint8_t>(byte2_leading + byte3_trailing)
                        };

                        const uint8_t packed_positions[] =
                        {
                            static_cast<uint8_t>(0x00 | (bit_size_byte - byte0_leading)),
                            static_cast<uint8_t>(0x10 | (bit_size_byte - byte1_leading)),
                            static_cast<uint8_t>(0x20 | (bit_size_byte - byte2_leading))
                        };

                        uint8_t length_sum{0};
                        uint8_t temp_length{static_cast<uint8_t>(pos_n_length & 0x0F)};

                        for(uint8_t i{0}; i < 3; ++i)
                        {
                            length_sum = *(length_sums + i);
                            greater_equal = (temp_length >= length_sum);
                            temp_length = (temp_length * greater_equal) + (length_sum * !greater_equal);
                            byte_n_bit_pos = (byte_n_bit_pos * greater_equal) + (*(packed_positions + i) * !greater_equal);
                        }

                        run->length = temp_length;
                        run->start_index.byte_index = static_cast<size_t>(absolute_address + (byte_n_bit_pos >> 4));
                        run->start_index.bit_index = static_cast<uint8_t>(byte_n_bit_pos & 0x0F);
                        if(run->length >= frames) return;
                    }
                }


                constexpr uint8_t word_bits{32};
                const uint8_t l_zeros{leading_zeros(current_value)};
                greater_equal = (run->length >= l_zeros);
                run->length = (run->length * greater_equal) + (l_zeros * !greater_equal);

                const uint8_t bit_index{static_cast<uint8_t>(word_bits - l_zeros)};
                 
                byte_n_bit_pos = static_cast<uint8_t>((bit_index >> bit_size_byte_mask) << 4) | (bit_index & bit_mask);
                
                run->start_index.byte_index = static_cast<size_t>((run->start_index.byte_index * greater_equal) + ((absolute_address + (byte_n_bit_pos >> 4)) * !greater_equal));
                run->start_index.bit_index = (run->start_index.bit_index * greater_equal) + ((byte_n_bit_pos & 0x0F) * !greater_equal);
                if(run->length >= frames) return;
            }
        }
        *start = current;
    }

    [[gnu::always_inline]] [[gnu::regparm(2)]]
    inline const void* get_best_aligned_address(const uintptr_t addr_1, const uintptr_t addr_2) noexcept
    {
        bool addr_1_le{addr_1 <= addr_2};
        return reinterpret_cast<void*>((addr_1 * addr_1_le) + (addr_2 * !addr_1_le));
    }

    [[gnu::always_inline]] [[gnu::regparm(2)]]
    inline const void* sweep_32(allocation_run* run, const size_t frames) noexcept
    {
        const uint8_t* current{g_bitmap.search_begin};

        constexpr uintptr_t mask_2_byte{0x01};
        constexpr uintptr_t mask_4_byte{0x03};
        const uintptr_t unaligned_end{reinterpret_cast<uintptr_t>(g_bitmap.search_end)};

        const uintptr_t aligned_end_2_address{unaligned_end & ~mask_2_byte};
        const void* end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_2_byte) & ~mask_2_byte), aligned_end_2_address)};
        contiguous_8_core_inline(run, frames, reinterpret_cast<const uint8_t*>(end), &current);
        if(run->length >= frames) return current;

        const uintptr_t aligned_end_4_address{unaligned_end & ~mask_4_byte};
        end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_4_byte) & ~mask_4_byte), aligned_end_4_address);
        contiguous_16_core_inline(run, frames, reinterpret_cast<const uint16_t*>(end), reinterpret_cast<const uint16_t**>(&current));
        if(run->length >= frames) return current;
        
        contiguous_32_core_inline(run, frames, reinterpret_cast<const uint32_t*>(aligned_end_4_address), reinterpret_cast<const uint32_t**>(&current));
        if(run->length >= frames) return current;

        // Fallback
        contiguous_16_core(run, frames, reinterpret_cast<const uint16_t*>(aligned_end_2_address), reinterpret_cast<const uint16_t**>(&current));
        if(run->length >= frames) return current;

        contiguous_8_core(run, frames, g_bitmap.search_end, reinterpret_cast<const uint8_t**>(&current));
        return current;
    }

    [[gnu::regparm(2)]]
    void find_contiguous_frames_32(allocation_run* const run, const size_t frames) noexcept
    {
        if(sweep_32(run, frames) == g_bitmap.end)
        {
            run->length ^= run->length;
            g_bitmap.search_end = g_bitmap.search_begin;
            g_bitmap.search_begin = (g_bitmap.start + g_bitmap.lower_limit.byte_index);
            sweep_32(run, frames);
            g_bitmap.search_end = g_bitmap.end;
        }
    }

    [[gnu::target("sse2")]] [[gnu::always_inline]] [[gnu::regparm(3)]]
    inline void contiguous_sse2_core_inline(allocation_run* run, const size_t frames, const __m128i* const end, const __m128i** start) noexcept
    {

    }

    //IMPORTANT: Keep is sync with the contiguous_sse2_core_inline right above
    [[gnu::target("sse2")]] [[gnu::regparm(3)]] [[gnu::noinline]]
    void contiguous_sse2_core(allocation_run* run, const size_t frames, const __m128i* const end, const __m128i** start) noexcept
    {

    }

    [[gnu::target("sse2")]] [[gnu::always_inline]] [[gnu::regparm(2)]]
    inline const uint8_t* sweep_sse_2(allocation_run* const run, const size_t frames) noexcept
    {
        const uint8_t* current{g_bitmap.search_begin};

        constexpr uintptr_t mask_2_byte{0x01};
        constexpr uintptr_t mask_4_byte{0x03};
        constexpr uintptr_t mask_16_byte{0x0F};
        const uintptr_t unaligned_end{reinterpret_cast<uintptr_t>(g_bitmap.search_end)};

        const uintptr_t aligned_end_2_address{unaligned_end & ~mask_2_byte};
        const void* end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_2_byte) & ~mask_2_byte), aligned_end_2_address)};
        contiguous_8_core_inline(run, frames, reinterpret_cast<const uint8_t*>(end), &current);
        if(run->length >= frames) return current;

        const uintptr_t aligned_end_4_address{unaligned_end & ~mask_4_byte};
        end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_4_byte) & ~mask_4_byte), aligned_end_4_address);
        contiguous_16_core_inline(run, frames, reinterpret_cast<const uint16_t*>(end), reinterpret_cast<const uint16_t**>(&current));
        if(run->length >= frames) return current;
        
        const uintptr_t aligned_end_16_address{unaligned_end & ~mask_16_byte};
        end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_16_byte) & ~mask_16_byte), aligned_end_16_address);
        contiguous_32_core_inline(run, frames, reinterpret_cast<const uint32_t*>(end), reinterpret_cast<const uint32_t**>(&current));
        if(run->length >= frames) return current;

        contiguous_sse2_core_inline(run, frames, reinterpret_cast<const __m128i*>(aligned_end_16_address), reinterpret_cast<const __m128i**>(&current));
        if(run->length >= frames) return current;

        // Fallback
        contiguous_32_core(run, frames, reinterpret_cast<const uint32_t*>(aligned_end_4_address), reinterpret_cast<const uint32_t**>(&current));
        if(run->length >= frames) return current;

        contiguous_16_core(run, frames, reinterpret_cast<const uint16_t*>(aligned_end_2_address), reinterpret_cast<const uint16_t**>(&current));
        if(run->length >= frames) return current;

        contiguous_8_core(run, frames, g_bitmap.search_end, reinterpret_cast<const uint8_t**>(&current));
        return current;
    }

    
    [[gnu::target("sse2")]] [[gnu::regparm(2)]]
    void find_contiguous_frames_sse2(allocation_run* const run, const size_t frames) noexcept
    {
        if(sweep_sse_2(run, frames) == g_bitmap.end)
        {
            run->length ^= run->length;
            g_bitmap.search_end = g_bitmap.search_begin;
            g_bitmap.search_begin = (g_bitmap.start + g_bitmap.lower_limit.byte_index);
            sweep_sse_2(run, frames);
            g_bitmap.search_end = g_bitmap.end;
        }
    }

    [[gnu::target("avx2")]] [[gnu::regparm(3)]] [[gnu::always_inline]]
    inline void contiguous_avx2_core_inline(allocation_run* run, const size_t frames, const __m256i* const end, const __m256i** start) noexcept
    {

    }

    //IMPORTANT: Keep is sync with the contiguous_avx2_core_inline right above
    // [[gnu::target("avx2")]] [[gnu::regparm(3)]] [[gnu::noinline]]
    // void contiguous_avx2_core(allocation_run* run, const size_t frames, const __m256i* const end, const __m256i** start) noexcept
    // {

    // }

    [[gnu::target("avx2")]] [[gnu::always_inline]] [[gnu::regparm(2)]]
    inline const uint8_t* sweep_avx_2(allocation_run* const run, const size_t frames) noexcept
    {
        const uint8_t* current{g_bitmap.search_begin};

        constexpr uintptr_t mask_2_byte{0x01};
        constexpr uintptr_t mask_4_byte{0x03};
        constexpr uintptr_t mask_16_byte{0x0F};
        constexpr uintptr_t mask_32_byte{0x1F};
        const uintptr_t unaligned_end{reinterpret_cast<uintptr_t>(g_bitmap.search_end)};

        const uintptr_t aligned_end_2_address{unaligned_end & ~mask_2_byte};
        const void* end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_2_byte) & ~mask_2_byte), aligned_end_2_address)};
        contiguous_8_core_inline(run, frames, reinterpret_cast<const uint8_t*>(end), &current);
        if(run->length >= frames) return current;

        const uintptr_t aligned_end_4_address{unaligned_end & ~mask_4_byte};
        end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_4_byte) & ~mask_4_byte), aligned_end_4_address);
        contiguous_16_core_inline(run, frames, reinterpret_cast<const uint16_t*>(end), reinterpret_cast<const uint16_t**>(&current));
        if(run->length >= frames) return current;
        
        const uintptr_t aligned_end_16_address{unaligned_end & ~mask_16_byte};
        end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_16_byte) & ~mask_16_byte), aligned_end_16_address);
        contiguous_32_core_inline(run, frames, reinterpret_cast<const uint32_t*>(end), reinterpret_cast<const uint32_t**>(&current));
        if(run->length >= frames) return current;

        const uintptr_t aligned_end_32_address{unaligned_end & ~mask_32_byte};
        end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(current) + mask_32_byte) & ~mask_32_byte), aligned_end_32_address);
        contiguous_sse2_core_inline(run, frames, reinterpret_cast<const __m128i*>(end), reinterpret_cast<const __m128i**>(&current));
        if(run->length >= frames) return current;

        contiguous_avx2_core_inline(run, frames, reinterpret_cast<const __m256i*>(aligned_end_32_address), reinterpret_cast<const __m256i**>(&current));
        if(run->length >= frames) return current;

        // Fallback
        contiguous_sse2_core(run, frames, reinterpret_cast<const __m128i*>(aligned_end_16_address), reinterpret_cast<const __m128i**>(&current));
        if(run->length >= frames) return current;

        contiguous_32_core(run, frames, reinterpret_cast<const uint32_t*>(aligned_end_4_address), reinterpret_cast<const uint32_t**>(&current));
        if(run->length >= frames) return current;

        contiguous_16_core(run, frames, reinterpret_cast<const uint16_t*>(aligned_end_2_address), reinterpret_cast<const uint16_t**>(&current));
        if(run->length >= frames) return current;

        contiguous_8_core(run, frames, g_bitmap.search_end, reinterpret_cast<const uint8_t**>(&current));
        return current;
    }

    [[gnu::target("avx2")]] [[gnu::regparm(2)]]
    void find_contiguous_frames_avx2(allocation_run* const run, const size_t frames) noexcept
    {
        if(sweep_avx_2(run, frames) == g_bitmap.end)
        {
            run->length ^= run->length;
            g_bitmap.search_end = g_bitmap.search_begin;
            g_bitmap.search_begin = (g_bitmap.start + g_bitmap.lower_limit.byte_index);
            sweep_avx_2(run, frames);
            g_bitmap.search_end = g_bitmap.end;
        }
    }

    using simd_alloc_methods = void(*)(allocation_run* const, const size_t) noexcept [[gnu::regparm(2)]];

    constexpr uint8_t simd_methods_size{3};
    struct simd_alloc_lut
    {
        simd_alloc_methods entries[simd_methods_size];
        
        constexpr simd_alloc_lut() noexcept: entries{}
        {
            entries[0] = find_contiguous_frames_32;
            entries[1] = find_contiguous_frames_sse2;
            entries[2] = find_contiguous_frames_avx2;
        }
    };

    constexpr simd_alloc_lut simd_lut{};
}

namespace kernel::memory
{
    size_t pmm_total_frames() noexcept { return g_total_frames; }
    size_t pmm_used_frames() noexcept { return g_used_frames; }
    size_t pmm_free_frames() noexcept { return g_total_frames - g_used_frames; }
    
    [[gnu::regparm(2)]]
    void pmm_initialize(const e820_memory_map* map, const uintptr_t kernel_end) noexcept
    {
        const e820_entry* const entry_end{map->entries + map->count};
        {
            uintptr_t highest_address{0};
            uintptr_t current{0};
            {
                bool greater{false};
                for(const e820_entry* start{map->entries}; start < entry_end; ++start)
                {
                    current = max(start);
                    greater = (highest_address > current);
                    highest_address = (highest_address * greater) + (current * !greater);
                }
            }
            g_total_frames = (highest_address >> frame_size_bit_mask);
        }

        g_bitmap.start = reinterpret_cast<uint8_t*>(kernel_end);
        g_bitmap.end = g_bitmap.start + ((g_total_frames + 7) >> bit_size_byte_mask);
        g_bitmap.search_end = g_bitmap.end;
        
        uint8_t* current{g_bitmap.start};
        constexpr uintptr_t alignment_mask{0x3};
        constexpr uintptr_t alignment_mask_not{~alignment_mask};
        const uint8_t* bitmap_end{reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(current) + alignment_mask) & alignment_mask_not)};
        for(; current < bitmap_end; ++current)
        {
            *current = 0xFF;
        }
        
        {
            uint32_t* current_32{reinterpret_cast<uint32_t*>(current)};
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
        
        for(index = 0; index < g_bitmap.lower_limit.byte_index; ++index)
        {
            mark_whole_byte_used(index);
        }

        set_frames_in_byte_used(index, back_byte_mask_used(g_bitmap.lower_limit.bit_index), g_bitmap.lower_limit.bit_index + 1);
    }

    void* pmm_allocate_frame() noexcept
    {
        bit_n_byte pair{};
        const uint8_t* current{g_bitmap.search_begin};
        for(; current < g_bitmap.search_end; ++current)
        {
            if(*current != 0xFF)
            {
                pair.byte_index = static_cast<size_t>(current - g_bitmap.start);
                pair.bit_index = dedicated_lut.entries[*current];
                set_frame_used(&pair);
                g_bitmap.search_begin = current + (*current == 0xFF);
                return reinterpret_cast<void*>(frame_address((pair.byte_index << bit_size_byte_mask) + pair.bit_index));
            } 
        }
        
        if(current == g_bitmap.end)
        {
            g_bitmap.search_end = g_bitmap.search_begin;
            current = g_bitmap.start + g_bitmap.lower_limit.byte_index;
            for(; current < g_bitmap.search_end; ++current)
            {
                if(*current != 0xFF)
                {
                    pair.byte_index = static_cast<size_t>(current - g_bitmap.start);
                    pair.bit_index = dedicated_lut.entries[*current];
                    set_frame_used(&pair);
                    g_bitmap.search_begin = current + (*current == 0xFF);
                    g_bitmap.search_end = g_bitmap.end;
                    return reinterpret_cast<void*>(frame_address((pair.byte_index << bit_size_byte_mask) + pair.bit_index));
                }
            }
            g_bitmap.search_end = g_bitmap.end;
        }
        return nullptr;
    }

    [[gnu::regparm(1)]]
    pmm_result pmm_free_frame(const void* address) noexcept
    {
        size_t index{frame_index(reinterpret_cast<uintptr_t>(address))};
        
        if(index >= g_total_frames) return pmm_result::hb_deny;

        const bit_n_byte bitmap_frame_pos{get_bit_n_byte(index)};

        if(bitmap_frame_pos < g_bitmap.lower_limit) return pmm_result::lb_deny;

        if(!is_frame_used(&bitmap_frame_pos)) return pmm_result::failed;
        set_frame_free(&bitmap_frame_pos);

        const uint8_t* const addr{g_bitmap.start + bitmap_frame_pos.byte_index};
        g_bitmap.search_begin -= ((g_bitmap.search_begin - addr) * (addr < g_bitmap.search_begin));
        
        return pmm_result::success;
    }

    [[gnu::regparm(1)]]
    void* pmm_allocate_contiguous_frames(const size_t frames) noexcept
    {
        if(frames == 0) return nullptr;

        allocation_run run{};
        // simd_lut.entries[0](&run, frames);
        simd_lut.entries[cpu::features::get()](&run, frames);

        if(run.length < frames) return nullptr;

        const uintptr_t start_address{(run.start_index.byte_index << bit_size_byte_mask) + run.start_index.bit_index};
        const bit_n_byte end_byte{get_bit_n_byte(start_address + frames - 1)};
        if(run.start_index.byte_index == end_byte.byte_index)
        {
            const uint8_t mask{static_cast<uint8_t>(front_byte_mask_used(run.start_index.bit_index) & back_byte_mask_used(end_byte.bit_index))};
            set_frames_in_byte_used(run.start_index.byte_index, mask, frames);
        }
        else
        {
            set_frames_in_byte_used(run.start_index.byte_index, front_byte_mask_used(run.start_index.bit_index), bit_max_pos - run.start_index.bit_index + 1);
            for(size_t start{run.start_index.byte_index + 1}; start < end_byte.byte_index; ++start)
            {
                mark_whole_byte_used(start);
            }
            set_frames_in_byte_used(end_byte.byte_index, back_byte_mask_used(end_byte.bit_index), end_byte.bit_index + 1);
        }
        const uint8_t* temp_end{g_bitmap.start + end_byte.byte_index};
        g_bitmap.search_begin = temp_end + (*temp_end == 0xFF);
        return reinterpret_cast<void*>(frame_address(start_address));
    }

    [[gnu::regparm(2)]]
    pmm_result pmm_free_contiguous_frames(const void* address, const size_t frames) noexcept
    {
        if(frames == 0) return pmm_result::zero_frames;

        const size_t index{frame_index(reinterpret_cast<uintptr_t>(address))};
        if(index >= g_total_frames) return pmm_result::hb_deny;

        const bit_n_byte start_byte{get_bit_n_byte(index)};

        if(start_byte < g_bitmap.lower_limit) return pmm_result::lb_deny;

        const bit_n_byte end_byte{get_bit_n_byte(index + frames - 1)};

        if(start_byte.byte_index == end_byte.byte_index)
        {
            const uint8_t mask{static_cast<uint8_t>(front_byte_free_mask(start_byte.bit_index) | static_cast<uint8_t>(back_byte_free_mask(end_byte.bit_index)))};
            set_frames_in_byte_free(start_byte.byte_index, mask, frames);
        }
        else
        {
            set_frames_in_byte_free(start_byte.byte_index, front_byte_free_mask(start_byte.bit_index), bit_max_pos - start_byte.bit_index + 1);
            for(size_t start{start_byte.byte_index + 1}; start < end_byte.byte_index; ++start)
            {
                mark_whole_byte_free(start);
            }
            set_frames_in_byte_free(end_byte.byte_index, back_byte_free_mask(end_byte.bit_index), end_byte.bit_index + 1);
        }

        const uint8_t* const addr{g_bitmap.start + start_byte.byte_index};
        g_bitmap.search_begin -= ((g_bitmap.search_begin - addr) * (addr < g_bitmap.search_begin));

        return pmm_result::success;
    }
}