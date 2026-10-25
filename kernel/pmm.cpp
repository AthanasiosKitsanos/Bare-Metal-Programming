#include "pmm.h"
#include "e820.h"
#include "cpu/features.h"
#include <immintrin.h>
#include "tools/cmp.h"
#include "tools/builtin.h"
#include "tools/math.h"
#include "internal/pmm_templates.h"
#include "terminal/output.h"

namespace
{

    struct bit_n_byte
    {
        size_t byte_index;
        uint8_t bit_index;

        [[gnu::always_inline]]
        bit_n_byte() noexcept = default;

        [[gnu::always_inline]]
        bit_n_byte(const size_t byte, const uint8_t bit) noexcept: byte_index{byte}, bit_index{bit}
        {}

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

    [[gnu::always_inline]]
    inline size_t frame_index(const uintptr_t address) noexcept { return address >> frame_shift; }

    [[gnu::always_inline]]
    inline uintptr_t frame_address(const size_t index) noexcept { return index << frame_shift; }

    [[gnu::always_inline]]
    inline bit_n_byte get_bit_n_byte(const size_t index) noexcept { return {(index >> bits_per_byte_shift), static_cast<uint8_t>(index & bit_mask)}; }

    [[gnu::always_inline]]
    inline bool is_frame_used(const bit_n_byte* const pair) noexcept
    {
        return (*(g_bitmap.start + pair->byte_index) & (1 << pair->bit_index)) != 0;
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

    // SIMD Methods
    constexpr uint8_t gpr_flag{0x00};
    constexpr uint8_t simd_flag{0x01};
    constexpr uint8_t avx2_flag{0x02};

    struct allocation_run
    {
        size_t start_frame{};
        size_t length{};
    };

    [[gnu::always_inline]]
    inline size_t run_found(const uint8_t* const ptr, const size_t length) noexcept
    {
        return (((ptr - g_bitmap.start) << bits_per_byte_shift) - length);
    }

    [[gnu::always_inline]]
    inline void contiguous_8_core_inline(allocation_run* const run, const size_t frames, const uint8_t* const end, const uint8_t** start) noexcept
    {
        const uint8_t* current{*start};
        size_t length{run->length};

        const array_of_shifts<uint8_t> shifts{frames};

        for(; current < end; ++current)
        {
            const uint8_t current_value{*current};

            if(current_value == 0x00)
            {
                length += byte_size;
                if(length >= frames)
                {
                    length -= byte_size;
                    run->start_frame = run_found(current, length);
                    break;
                }
            }
            else if(current_value == 0xFF) length ^= length;
            else
            {
                if((length + static_cast<size_t>(builtin::trailing_zeros(current_value))) >= frames)
                {
                    run->start_frame = run_found(current, length);
                    break;
                }

                if(frames <= 7)
                {
                    /*
                        Since we get here only if frames <= 7 (maximum amount of free frames in the middle)
                        and the inverted byte is not 0, means that the run here is completed.
                        So we don't really care if the length suddenly becomes huge (wrap around)
                        since we only care about the frames
                    */
                    uint8_t inverted_value{shifts.space(static_cast<uint8_t>(~current_value))};
                    if(inverted_value != 0)
                    {
                        length = -builtin::trailing_zeros(inverted_value);
                        run->start_frame = run_found(current, length);
                        break;
                    }

                }

                length = static_cast<size_t>(builtin::leading_zeros(current_value));
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
    }

    // IMPORTANT keep is sync with contiguous_8_core_inline right above
    [[gnu::noinline]] [[gnu::regparm(3)]]
    void contiguous_8_core(allocation_run* const run, const size_t frames, const uint8_t* const end, const uint8_t** start) noexcept
    {
        const uint8_t* current{*start};
        size_t length{run->length};

        const array_of_shifts<uint8_t> shifts{frames};

        for(; current < end; ++current)
        {
            const uint8_t current_value{*current};

            if(current_value == 0x00)
            {
                length += byte_size;
                if(length >= frames)
                {
                    length -= byte_size;
                    run->start_frame = run_found(current, length);
                    break;
                }
            }
            else if(current_value == 0xFF) length ^= length;
            else
            {
                if((length + static_cast<size_t>(builtin::trailing_zeros(current_value))) >= frames)
                {
                    run->start_frame = run_found(current, length);
                    break;
                }

                if(frames <= 7)
                {
                    /*
                        Since we get fhere only if frames <= 7 (maximum amount of free frames in the middle)
                        and the inverted byte is not 0, means that the run here is completed.
                        So we don't really care if the length suddenly becomes huge (wrap around)
                        since we only care about the frames
                    */
                    uint8_t inverted_value{shifts.space(static_cast<uint8_t>(~current_value))};
                    if(inverted_value != 0)
                    {
                        length = -builtin::trailing_zeros(inverted_value);
                        run->start_frame = run_found(current, length);
                        break;
                    }

                }

                length = static_cast<size_t>(builtin::leading_zeros(current_value));
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
    }

    constexpr size_t width_16{16};
    [[gnu::always_inline]] [[gnu::regparm(3)]]
    inline void contiguous_16_core_inline(allocation_run* const run, const size_t frames, const uint16_t* const end, const uint16_t** start) noexcept
    {
        const uint16_t* current{*start};
        size_t length{run->length};

        const array_of_shifts<uint16_t> shifts{frames};

        for(; current < end; ++current)
        {
            const uint16_t current_value{*current};

            if(current_value == 0x0000)
            {
                length += width_16;
                if(length >= frames)
                {
                    length -= width_16;
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }
            }
            else if(current_value == 0xFFFF) length ^= length;
            else
            {
                if((length + static_cast<size_t>(builtin::trailing_zeros(current_value))) >= frames)
                {
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }

                if(frames <= 15)
                {
                    /*
                        Since we get here only if frames <= 15 (maximum amount of free frames in the middle)
                        and the inverted byte is not 0, means that the run here is completed.
                        So we don't really care if the length suddenly becomes huge (wrap around)
                        since we only care about the frames
                    */
                    const uint16_t inverted_value{shifts.space(~current_value)};
                    if(inverted_value != 0)
                    {
                        length = -builtin::trailing_zeros(inverted_value);
                        run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                        break;
                    }

                }

                length = static_cast<size_t>(builtin::leading_zeros(current_value));
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
    }

    //IMPORTANT: Keep is sync with the contiguous_16_core_inline right above
    [[gnu::noinline]] [[gnu::regparm(3)]]
    void contiguous_16_core(allocation_run* const run, const size_t frames, const uint16_t* const end, const uint16_t** start) noexcept
    {
        const uint16_t* current{*start};
        size_t length{run->length};

        const array_of_shifts<uint16_t> shifts{frames};

        for(; current < end; ++current)
        {
            const uint16_t current_value{*current};

            if(current_value == 0x0000)
            {
                length += width_16;
                if(length >= frames)
                {
                    length -= width_16;
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }
            }
            else if(current_value == 0xFFFF) length ^= length;
            else
            {
                if((length + static_cast<size_t>(builtin::trailing_zeros(current_value))) >= frames)
                {
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }

                if(frames <= 15)
                {
                    /*
                        Since we get here only if frames <= 15 (maximum amount of free frames in the middle)
                        and the inverted byte is not 0, means that the run here is completed.
                        So we don't really care if the length suddenly becomes huge (wrap around)
                        since we only care about the frames
                    */
                    const uint16_t inverted_value{shifts.space(~current_value)};
                    if(inverted_value != 0)
                    {
                        length = -builtin::trailing_zeros(inverted_value);
                        run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                        break;
                    }

                }

                length = static_cast<size_t>(builtin::leading_zeros(current_value));
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
    }

    constexpr size_t width_32{32};
    [[gnu::regparm(3)]] [[gnu::always_inline]]
    inline void contiguous_32_core_inline(allocation_run* const run, const size_t frames, const uint32_t* const end, const uint32_t** start) noexcept
    {
        const uint32_t* current{*start};
        size_t length{run->length};

        const array_of_shifts<uint32_t> shifts{frames};

        for(; current < end; ++current)
        {
            const uint32_t current_value{*current};

            if(current_value == 0x00000000)
            {
                length += width_32;
                if(length >= frames)
                {
                    length -= width_32;
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }
            }
            else if(current_value == 0xFFFFFFFF) length ^= length;
            else
            {
                if((length + static_cast<size_t>(builtin::trailing_zeros(current_value))) >= frames)
                {
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }

                if(frames <= 31)
                {
                    /*
                        Since we get here only if frames <= 31 (maximum amount of free frames in the middle)
                        and the inverted byte is not 0, means that the run here is completed.
                        So we don't really care if the length suddenly becomes huge (wrap around)
                        since we only care about the frames
                    */
                    const uint32_t inverted_value{shifts.space(~current_value)};
                    if(inverted_value != 0)
                    {
                        length = -builtin::trailing_zeros(inverted_value);
                        run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                        break;
                    }

                }

                length = static_cast<size_t>(builtin::leading_zeros(current_value));
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
    }

    //IMPORTANT: Keep is sync with the contiguous_32_core_inline right above
    [[gnu::regparm(3)]] [[gnu::noinline]]
    void contiguous_32_core(allocation_run* const run, const size_t frames, const uint32_t* const end, const uint32_t** start) noexcept
    {
        const uint32_t* current{*start};
        size_t length{run->length};

        const array_of_shifts<uint32_t> shifts{frames};

        for(; current < end; ++current)
        {
            const uint32_t current_value{*current};

            if(current_value == 0x00000000)
            {
                length += width_32;
                if(length >= frames)
                {
                    length -= width_32;
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }
            }
            else if(current_value == 0xFFFFFFFF) length ^= length;
            else
            {
                if((length + static_cast<size_t>(builtin::trailing_zeros(current_value))) >= frames)
                {
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }

                if(frames <= 31)
                {
                    /*
                        Since we get here only if frames <= 31 (maximum amount of free frames in the middle)
                        and the inverted byte is not 0, means that the run here is completed.
                        So we don't really care if the length suddenly becomes huge (wrap around)
                        since we only care about the frames
                    */
                    const uint32_t inverted_value{shifts.space(~current_value)};
                    if(inverted_value != 0)
                    {
                        length = -builtin::trailing_zeros(inverted_value);
                        run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                        break;
                    }

                }

                length = static_cast<size_t>(builtin::leading_zeros(current_value));
            }
        }
        run->length = (current < end) ? frames : length;
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

    constexpr size_t width_128{128};
    [[gnu::target("sse2")]] [[gnu::always_inline]] [[gnu::regparm(3)]]
    inline void contiguous_sse2_core_inline(allocation_run* run, const size_t frames, const __m128i* const end, const __m128i** start) noexcept
    {
        size_t length{run->length};
        const __m128i* current{*start};
        const __m128i cmp{_mm_setzero_si128()};
        const __m128i all_ones{_mm_cmpeq_epi32(cmp, cmp)};

        for(; current < end; ++current)
        {
            const __m128i current_value{_mm_load_si128(current)};
            const __m128i cmp_result{_mm_cmpeq_epi32(current_value, cmp)};
            const uint16_t packed_result{static_cast<uint16_t>(_mm_movemask_epi8(cmp_result))};

            const __m128i all_ones_cmp_result{_mm_cmpeq_epi32(current_value, all_ones)};
            const uint16_t all_ones_packed{static_cast<uint16_t>(_mm_movemask_epi8(all_ones_cmp_result))};

            if(packed_result == 0xFFFF)
            {
                length += width_128;
                if(length >= frames)
                {
                    length -= width_128;
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }
            }
            else if(all_ones_packed == 0xFFFF) length ^= length;
            else
            {
                const uint32_t* lane_start{reinterpret_cast<const uint32_t*>(current)};
                constexpr uint8_t end_pos{sizeof(__m128i) >> 2};
                const uint32_t* const lane_end{lane_start + end_pos};
                run->length = length;
                contiguous_32_core_inline(run, frames, lane_end, &lane_start);
                length = run->length;
                if(length >= frames) break;
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
    }

    //IMPORTANT: Keep is sync with the contiguous_sse2_core_inline right above
    [[gnu::target("sse2")]] [[gnu::regparm(3)]] [[gnu::noinline]]
    void contiguous_sse2_core(allocation_run* run, const size_t frames, const __m128i* const end, const __m128i** start) noexcept
    {
        size_t length{run->length};
        const __m128i* current{*start};
        const __m128i cmp{_mm_setzero_si128()};
        const __m128i all_ones{_mm_cmpeq_epi32(cmp, cmp)};

        for(; current < end; ++current)
        {
            const __m128i current_value{_mm_load_si128(current)};
            const __m128i cmp_result{_mm_cmpeq_epi32(current_value, cmp)};
            const uint16_t packed_result{static_cast<uint16_t>(_mm_movemask_epi8(cmp_result))};

            const __m128i all_ones_cmp_result{_mm_cmpeq_epi32(current_value, all_ones)};
            const uint16_t all_ones_packed{static_cast<uint16_t>(_mm_movemask_epi8(all_ones_cmp_result))};

            if(packed_result == 0xFFFF)
            {
                length += width_128;
                if(length >= frames)
                {
                    length -= width_128;
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }
            }
            else if(all_ones_packed == 0xFFFF) length ^= length;
            else
            {
                const uint32_t* lane_start{reinterpret_cast<const uint32_t*>(current)};
                constexpr uint8_t end_pos{sizeof(__m128i) >> 2};
                const uint32_t* const lane_end{lane_start + end_pos};
                run->length = length;
                contiguous_32_core_inline(run, frames, lane_end, &lane_start);
                length = run->length;
                if(length >= frames) break;
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
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

    constexpr size_t width_256{256};
    [[gnu::target("avx2")]] [[gnu::regparm(3)]] [[gnu::always_inline]]
    inline void contiguous_avx2_core_inline(allocation_run* run, const size_t frames, const __m256i* const end, const __m256i** start) noexcept
    {
        size_t length{run->length};

        const __m256i* current{*start};
        const __m256i cmp{_mm256_setzero_si256()};
        const __m256i all_ones{_mm256_cmpeq_epi32(cmp, cmp)};

        for(; current < end; ++current)
        {
            const __m256i current_value{_mm256_load_si256(current)};
            const __m256i cmp_result{_mm256_cmpeq_epi32(current_value, cmp)};
            const uint32_t packed_result{static_cast<uint32_t>(_mm256_movemask_epi8(cmp_result))};

            const __m256i all_ones_cmp_result{_mm256_cmpeq_epi8(current_value, all_ones)};
            const uint32_t all_ones_packed{static_cast<uint32_t>(_mm256_movemask_epi8(all_ones_cmp_result))};

            if(packed_result == 0xFFFFFFFF)
            {
                length += width_256;
                if(length >= frames)
                {
                    length -= width_256;
                    run->start_frame = run_found(reinterpret_cast<const uint8_t*>(current), length);
                    break;
                }
            }
            else if(all_ones_packed == 0xFFFFFFFF) length ^= length;
            else
            {
                const __m128i* lane_start{reinterpret_cast<const __m128i*>(current)};
                constexpr uint8_t end_pos{sizeof(__m256i) >> 4};
                const __m128i* lane_end{lane_start + end_pos};
                run->length = length;
                contiguous_sse2_core_inline(run, frames, lane_end, &lane_start);
                length = run->length;
                if(length >= frames) break;
            }
        }
        run->length = (current < end) ? frames : length;
        *start = current;
    }

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
            entries[gpr_flag] = find_contiguous_frames_32;
            entries[simd_flag] = find_contiguous_frames_sse2;
            entries[avx2_flag] = find_contiguous_frames_avx2;
        }
    };

    constexpr simd_alloc_lut simd_lut{};

// Peeling methods for bulk setting frames as used
    constexpr uint8_t mask_2_byte{0x01};
    constexpr uint8_t mask_4_byte{0x03};
    constexpr uint8_t mask_16_byte{0x0F};
    constexpr uint8_t mask_32_byte{0x1F};

    [[gnu::regparm(2)]]
    void set_frames_used_32(uint8_t* start, const uint8_t* const end) noexcept
    {
        const uintptr_t unaligbed_end{reinterpret_cast<uintptr_t>(end)};

        const uintptr_t aligned_end_2_byte_address{(reinterpret_cast<uintptr_t>(unaligbed_end) & ~mask_2_byte)};
        const void* temp_end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_2_byte) & ~mask_2_byte), aligned_end_2_byte_address)};
        set_gpr_inline<uint8_t, set_bits::all_ones>(&start, reinterpret_cast<const uint8_t*>(temp_end));

        const uintptr_t aligned_end_4_byte_address{(unaligbed_end & ~mask_4_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_4_byte) & ~mask_4_byte), aligned_end_4_byte_address);
        set_gpr_inline<uint16_t, set_bits::all_ones>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(temp_end));

        set_gpr_inline<uint32_t, set_bits::all_ones>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(temp_end));

        // Fallback
        set_gpr<uint16_t, set_bits::all_ones>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(aligned_end_2_byte_address));

        set_gpr<uint8_t, set_bits::all_ones>(&start, end);
    }

    [[gnu::target("sse2")]] [[gnu::regparm(2)]]
    void set_frames_used_sse2(uint8_t* start, const uint8_t* const end) noexcept
    {
        const uintptr_t unaligbed_end{reinterpret_cast<uintptr_t>(end)};

        const uintptr_t aligned_end_2_byte_address{(unaligbed_end & ~mask_2_byte)};
        const void* temp_end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_2_byte) & ~mask_2_byte), aligned_end_2_byte_address)};
        set_gpr_inline<uint8_t, set_bits::all_ones>(&start, reinterpret_cast<const uint8_t*>(temp_end));

        const uintptr_t aligned_end_4_byte_address{(unaligbed_end & ~mask_4_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_4_byte) & ~mask_4_byte), aligned_end_4_byte_address);
        set_gpr_inline<uint16_t, set_bits::all_ones>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(temp_end));

        const uintptr_t aligned_end_16_byte_address{(unaligbed_end & ~mask_16_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_16_byte) & ~mask_16_byte), aligned_end_16_byte_address);
        set_gpr_inline<uint32_t, set_bits::all_ones>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(temp_end));

        set_sse2_inline<set_bits::all_ones>(reinterpret_cast<__m128i**>(&start), reinterpret_cast<const __m128i*>(aligned_end_16_byte_address));

        // Fallback
        set_gpr<uint32_t, set_bits::all_ones>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(aligned_end_4_byte_address));
        set_gpr<uint16_t, set_bits::all_ones>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(aligned_end_2_byte_address));
        set_gpr<uint8_t, set_bits::all_ones>(&start, end);
    }

    [[gnu::target("avx2")]] [[gnu::regparm(2)]]
    void set_frames_used_avx2(uint8_t* start, const uint8_t* const end) noexcept
    {
        const uintptr_t unaligbed_end{reinterpret_cast<uintptr_t>(end)};

        const uintptr_t aligned_end_2_byte_address{(unaligbed_end & ~mask_2_byte)};
        const void* temp_end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_2_byte) & ~mask_2_byte), aligned_end_2_byte_address)};
        set_gpr_inline<uint8_t, set_bits::all_ones>(&start, reinterpret_cast<const uint8_t*>(temp_end));

        const uintptr_t aligned_end_4_byte_address{(unaligbed_end & ~mask_4_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_4_byte) & ~mask_4_byte), aligned_end_4_byte_address);
        set_gpr_inline<uint16_t, set_bits::all_ones>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(temp_end));

        const uintptr_t aligned_end_16_byte_address{(unaligbed_end & ~mask_16_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_16_byte) & ~mask_16_byte), aligned_end_16_byte_address);
        set_gpr_inline<uint32_t, set_bits::all_ones>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(temp_end));

        const uintptr_t aligned_end_32_byte_address{(unaligbed_end & ~mask_32_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_32_byte) & ~mask_32_byte), aligned_end_32_byte_address);
        set_sse2_inline<set_bits::all_ones>(reinterpret_cast<__m128i**>(&start), reinterpret_cast<const __m128i*>(temp_end));

        set_avx2_inline<set_bits::all_ones>(reinterpret_cast<__m256i**>(&start), reinterpret_cast<const __m256i*>(aligned_end_32_byte_address));

        // Fallback
        set_sse2<set_bits::all_ones>(reinterpret_cast<__m128i**>(&start), reinterpret_cast<const __m128i*>(aligned_end_16_byte_address));
        set_gpr<uint32_t, set_bits::all_ones>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(aligned_end_4_byte_address));
        set_gpr<uint16_t, set_bits::all_ones>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(aligned_end_2_byte_address));
        set_gpr<uint8_t, set_bits::all_ones>(&start, end);
    }

// Peeling methods for bulk setting frames as free

    [[gnu::regparm(2)]]
    void set_frames_free_32(uint8_t* start, const uint8_t* const end) noexcept
    {
        constexpr uint8_t mask_2_byte{0x01};
        constexpr uint8_t mask_4_byte{0x03};

        const uintptr_t unaligbed_end{reinterpret_cast<uintptr_t>(end)};

        const uintptr_t aligned_end_2_byte_address{(reinterpret_cast<uintptr_t>(unaligbed_end) & ~mask_2_byte)};
        const void* temp_end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_2_byte) & ~mask_2_byte), aligned_end_2_byte_address)};
        set_gpr_inline<uint8_t, set_bits::all_zeros>(&start, reinterpret_cast<const uint8_t*>(temp_end));

        const uintptr_t aligned_end_4_byte_address{(unaligbed_end & ~mask_4_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_4_byte) & ~mask_4_byte), aligned_end_4_byte_address);
        set_gpr_inline<uint16_t, set_bits::all_zeros>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(temp_end));

        set_gpr_inline<uint32_t, set_bits::all_zeros>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(temp_end));

        // Fallback
        set_gpr<uint16_t, set_bits::all_zeros>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(aligned_end_2_byte_address));

        set_gpr<uint8_t, set_bits::all_zeros>(&start, end);
    }

    [[gnu::target("sse2")]] [[gnu::regparm(2)]]
    void set_frames_free_sse2(uint8_t* start, const uint8_t* const end) noexcept
    {
        constexpr uint8_t mask_2_byte{0x01};
        constexpr uint8_t mask_4_byte{0x03};
        constexpr uint8_t mask_16_byte{0x0F};

        const uintptr_t unaligbed_end{reinterpret_cast<uintptr_t>(end)};

        const uintptr_t aligned_end_2_byte_address{(unaligbed_end & ~mask_2_byte)};
        const void* temp_end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_2_byte) & ~mask_2_byte), aligned_end_2_byte_address)};
        set_gpr_inline<uint8_t, set_bits::all_zeros>(&start, reinterpret_cast<const uint8_t*>(temp_end));

        const uintptr_t aligned_end_4_byte_address{(unaligbed_end & ~mask_4_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_4_byte) & ~mask_4_byte), aligned_end_4_byte_address);
        set_gpr_inline<uint16_t, set_bits::all_zeros>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(temp_end));

        const uintptr_t aligned_end_16_byte_address{(unaligbed_end & ~mask_16_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_16_byte) & ~mask_16_byte), aligned_end_16_byte_address);
        set_gpr_inline<uint32_t, set_bits::all_zeros>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(temp_end));

        set_sse2_inline<set_bits::all_zeros>(reinterpret_cast<__m128i**>(&start), reinterpret_cast<const __m128i*>(aligned_end_16_byte_address));

        // Fallback
        set_gpr<uint32_t, set_bits::all_zeros>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(aligned_end_4_byte_address));
        set_gpr<uint16_t, set_bits::all_zeros>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(aligned_end_2_byte_address));
        set_gpr<uint8_t, set_bits::all_zeros>(&start, end);
    }

    [[gnu::target("avx2")]] [[gnu::regparm(2)]]
    void set_frames_free_avx2(uint8_t* start, const uint8_t* const end) noexcept
    {
        const uintptr_t unaligbed_end{reinterpret_cast<uintptr_t>(end)};

        const uintptr_t aligned_end_2_byte_address{(unaligbed_end & ~mask_2_byte)};
        const void* temp_end{get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_2_byte) & ~mask_2_byte), aligned_end_2_byte_address)};
        set_gpr_inline<uint8_t, set_bits::all_zeros>(&start, reinterpret_cast<const uint8_t*>(temp_end));

        const uintptr_t aligned_end_4_byte_address{(unaligbed_end & ~mask_4_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_4_byte) & ~mask_4_byte), aligned_end_4_byte_address);
        set_gpr_inline<uint16_t, set_bits::all_zeros>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(temp_end));

        const uintptr_t aligned_end_16_byte_address{(unaligbed_end & ~mask_16_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_16_byte) & ~mask_16_byte), aligned_end_16_byte_address);
        set_gpr_inline<uint32_t, set_bits::all_zeros>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(temp_end));

        const uintptr_t aligned_end_32_byte_address{(unaligbed_end & ~mask_32_byte)};
        temp_end = get_best_aligned_address(((reinterpret_cast<uintptr_t>(start) + mask_32_byte) & ~mask_32_byte), aligned_end_32_byte_address);
        set_sse2_inline<set_bits::all_zeros>(reinterpret_cast<__m128i**>(&start), reinterpret_cast<const __m128i*>(temp_end));

        set_avx2_inline<set_bits::all_zeros>(reinterpret_cast<__m256i**>(&start), reinterpret_cast<const __m256i*>(aligned_end_32_byte_address));

        // Fallback
        set_sse2<set_bits::all_zeros>(reinterpret_cast<__m128i**>(&start), reinterpret_cast<const __m128i*>(aligned_end_16_byte_address));
        set_gpr<uint32_t, set_bits::all_zeros>(reinterpret_cast<uint32_t**>(&start), reinterpret_cast<const uint32_t*>(aligned_end_4_byte_address));
        set_gpr<uint16_t, set_bits::all_zeros>(reinterpret_cast<uint16_t**>(&start), reinterpret_cast<const uint16_t*>(aligned_end_2_byte_address));
        set_gpr<uint8_t, set_bits::all_zeros>(&start, end);
    }

    using simd_set_methods = void(*)(uint8_t* start, const uint8_t* const end) noexcept [[gnu::regparm(2)]];
    constexpr uint8_t set_methods_size{3};
    template<::set_bits Set>
    struct simd_set_lut
    {
        simd_set_methods entries[set_methods_size];

        constexpr simd_set_lut(): entries{}
        {
            if constexpr(Set == set_bits::all_ones)
            {
                entries[gpr_flag] = set_frames_used_32;
                entries[simd_flag] = set_frames_used_sse2;
                entries[avx2_flag] = set_frames_used_avx2;
            }
            else
            {
                entries[gpr_flag] = set_frames_free_32;
                entries[simd_flag] = set_frames_free_sse2;
                entries[avx2_flag] = set_frames_free_avx2;
            }
        }

        [[gnu::always_inline]]
        constexpr simd_set_methods operator[](const size_t index) const noexcept
        {
            return entries[index];
        }
    };

    constexpr simd_set_lut<set_bits::all_ones> set_used_lut{};
    constexpr simd_set_lut<set_bits::all_zeros> set_free_lut{};

    // Precondition start < end, must be guarantied by caller
    [[gnu::regparm(2)]]
    void free_frame_range(const size_t start_frame, const size_t end_frame) noexcept
    {
        g_used_frames -= (end_frame - start_frame);
        const bit_n_byte inclusive_start{get_bit_n_byte(start_frame)};
        const bit_n_byte inclusive_end{get_bit_n_byte(end_frame - 1)};
        if(inclusive_start.byte_index == (inclusive_end.byte_index))
        {
            const uint8_t mask{static_cast<uint8_t>((0xFF >> (byte_size - inclusive_start.bit_index)) | (0xFF << (inclusive_end.bit_index + 1)))};
            g_bitmap.start[inclusive_start.byte_index] &= mask;
            return;
        }
        uint8_t* start_byte{&g_bitmap.start[inclusive_start.byte_index]};
        uint8_t* end_byte{&g_bitmap.start[inclusive_end.byte_index]};
        *start_byte &= static_cast<uint8_t>(0xFF >> (byte_size - inclusive_start.bit_index));
        set_free_lut[cpu::features::get()](++start_byte, end_byte);
        *end_byte &= static_cast<uint8_t>(0xFF << (inclusive_end.bit_index + 1));
    }

    struct reserved_vector
    {
        size_t base;
        size_t length;
    };

    constexpr size_t reserved_vector_size{2};
    constexpr size_t total_size_m1{(sizeof(reserved_vector) * reserved_vector_size) - 1};
    constexpr size_t align_at(power::highest_of_2(total_size_m1) << 1);
    alignas(align_at) reserved_vector g_reserved[reserved_vector_size];

    constexpr size_t cache_line_size{64};
    static_assert(align_at <= cache_line_size, "g_reserved must fit in one cache line");
}

namespace kernel::memory
{
    size_t pmm_total_frames() noexcept { return g_total_frames; }
    size_t pmm_used_frames() noexcept { return g_used_frames; }
    size_t pmm_free_frames() noexcept { return g_total_frames - g_used_frames; }

    [[gnu::regparm(3)]]
    void pmm_initialize(const e820_memory_map* map, const uintptr_t kernel_start, const uintptr_t kernel_end) noexcept
    {
        const e820_entry* const entries_end{map->entries + map->count};
        {
            uint64_t highest_address{0};
            for(const e820_entry* entry{map->entries}; entry < entries_end; ++entry)
            {
                if(entry->type == e820_memory_type::usable)
                {
                    const uint64_t current{entry->base + entry->length};
                    highest_address = cmp::max(highest_address, current);
                }
            }
            constexpr uintptr_t accessable_frames{(UINTPTR_MAX >> frame_shift) + 1};
            constexpr uint64_t s{UINT64_MAX};
            g_total_frames = static_cast<size_t>(cmp::min((highest_address >> frame_shift), static_cast<uint64_t>(accessable_frames)));
        }

        g_bitmap.start = reinterpret_cast<uint8_t*>(kernel_end);
        g_bitmap.end = g_bitmap.start + ((g_total_frames + 7) >> bits_per_byte_shift);
        g_bitmap.search_begin = g_bitmap.start;
        g_bitmap.search_end = g_bitmap.end;

        // Set bit of the bitmap to 1 (used)
        set_used_lut[cpu::features::get()](g_bitmap.start, g_bitmap.end);
        g_used_frames = g_total_frames;

        // g_reserved[0].base is 0, because .init_bss(kernel_entry.S), already assigns 0
        g_reserved[0].length = 1;
        constexpr size_t frame_size_m1{frame_size - 1};
        const size_t kernel_start_index{frame_index(kernel_start)};
        const size_t kernel_end_index{frame_index((reinterpret_cast<uintptr_t>(g_bitmap.end) + frame_size_m1))};
        g_reserved[1].base = kernel_start_index;
        g_reserved[1].length = kernel_end_index - kernel_start_index;

        const uint64_t promoted_total_frames{static_cast<uint64_t>(g_total_frames)};
        for(const e820_entry* entry{map->entries}; entry < entries_end; ++entry)
        {
            if(entry->type == e820_memory_type::usable)
            {
                const uint64_t base{entry->base};
                const uint64_t start_frame{cmp::min(((base + frame_size_m1) >> frame_shift), promoted_total_frames)};
                const uint64_t end_frame{cmp::min(((base + entry->length) >> frame_shift), promoted_total_frames)};

                if(start_frame < end_frame)
                {
                    size_t runner{static_cast<size_t>(start_frame)};
                    const size_t u_end{static_cast<size_t>(end_frame)};
                    for(size_t i{0}; i < reserved_vector_size; ++i)
                    {
                        const size_t reserved_start{g_reserved[i].base};
                        const size_t piece_end{cmp::min(reserved_start, u_end)};
                        if(runner < piece_end)
                        {
                            free_frame_range(runner, piece_end);
                        }
                        runner = cmp::max(runner, reserved_start + g_reserved[i].length);
                    }

                    if(runner < u_end) free_frame_range(runner, u_end);
                }
            }
        }
    }

    void* pmm_allocate_frame() noexcept
    {
        const uint8_t* current{g_bitmap.search_begin};
        for(; current < g_bitmap.search_end; ++current)
        {
            const uint8_t current_value{*current};
            if(current_value != 0xFF)
            {
                const size_t frame_index{static_cast<size_t>((current - g_bitmap.start) << bits_per_byte_shift) + builtin::bit_guard_tz(static_cast<uint8_t>(~current_value))};
                const bit_n_byte pair{get_bit_n_byte(frame_index)};
                set_frame_used(&pair);
                g_bitmap.search_begin = current + (current_value == 0xFF);
                return reinterpret_cast<void*>(frame_address(frame_index));
            }
        }

        if(current == g_bitmap.end)
        {
            g_bitmap.search_end = g_bitmap.search_begin;
            current = g_bitmap.start + g_bitmap.lower_limit.byte_index;
            for(; current < g_bitmap.search_end; ++current)
            {
                const uint8_t current_value{*current};
                if(current_value != 0xFF)
                {
                    const size_t frame_index{static_cast<size_t>((current - g_bitmap.start) << bits_per_byte_shift) + builtin::bit_guard_tz(static_cast<uint8_t>(~current_value))};
                    const bit_n_byte pair{get_bit_n_byte(frame_index)};
                    set_frame_used(&pair);
                    g_bitmap.search_begin = current + (current_value == 0xFF);
                    return reinterpret_cast<void*>(frame_address(frame_index));
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
        const bool is_addr_less{addr < g_bitmap.search_end};
        g_bitmap.search_begin = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(g_bitmap.search_begin) * !is_addr_less) + reinterpret_cast<uintptr_t>(addr) * is_addr_less);

        return pmm_result::success;
    }

    [[gnu::regparm(1)]]
    void* pmm_allocate_contiguous_frames(const size_t frames) noexcept
    {
        if(frames == 0) return nullptr;

        allocation_run run{};
        simd_lut.entries[cpu::features::get()](&run, frames);

        if(run.length < frames) return nullptr;

        const bit_n_byte start_byte{get_bit_n_byte(run.start_frame)};
        const bit_n_byte end_byte{get_bit_n_byte(run.start_frame + frames - 1)};

        uint8_t* set_start{g_bitmap.start + start_byte.byte_index};
        uint8_t* const set_end{g_bitmap.start + end_byte.byte_index};

        const uint8_t front_mask{static_cast<uint8_t>(0xFF << start_byte.bit_index)};
        const uint8_t end_mask{static_cast<uint8_t>(0xFF >> (bit_max_pos - end_byte.bit_index))};

        if(start_byte.byte_index == end_byte.byte_index) *set_start |= (front_mask & end_mask);
        else
        {
            *set_start |= front_mask;
            set_used_lut.entries[cpu::features::get()](++set_start, set_end);
            *set_end |= end_mask;
        }

        g_used_frames += frames;
        g_bitmap.search_begin = set_end + (*set_end == 0xFF);
        return reinterpret_cast<void*>(frame_address(run.start_frame));
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

        uint8_t* free_start{g_bitmap.start + start_byte.byte_index};
        uint8_t* const end{g_bitmap.start + end_byte.byte_index};

        const uint8_t front_mask{static_cast<uint8_t>(0xFF >> (byte_size - start_byte.bit_index))};
        const uint8_t back_mask{static_cast<uint8_t>(0xFF << (end_byte.bit_index + 1))};

        if(start_byte.byte_index == end_byte.byte_index) *free_start &= (front_mask | back_mask);
        else
        {
            *free_start &= front_mask;
            set_free_lut.entries[cpu::features::get()](free_start + 1, end);
            *end &= back_mask;
        }

        const bool is_free_start_less{free_start < g_bitmap.search_begin};
        g_bitmap.search_begin = reinterpret_cast<uint8_t*>((reinterpret_cast<uintptr_t>(g_bitmap.search_begin) * !is_free_start_less) + reinterpret_cast<uintptr_t>(free_start) * is_free_start_less);
        g_used_frames -= frames;
        return pmm_result::success;
    }

    namespace test
    {
        // Changes bitmap values without restoring them
        void allocate_8_frames() noexcept
        {
            *g_bitmap.start = 0x0F;
            g_bitmap.start[1] = 0xF0;
            const uint8_t* curr{g_bitmap.start};

            allocation_run run{};
            constexpr size_t frames{8};
            contiguous_8_core(&run, frames, (curr + 2), &curr);
            terminal::output out{};

            if(run.length >= frames) out << "Found\n";
            else out << "Not Found\n";

            const bit_n_byte start{get_bit_n_byte(run.start_frame)};
            const bit_n_byte end{get_bit_n_byte(run.start_frame + frames - 1)};
            out << "Frames: " << frames
            << "\nLength: " << run.length
            << "\nStart value: " << terminal::hex << *g_bitmap.start << terminal::dec
            << "\nByte index: " << start.byte_index
            << "\nBit index: " << start.bit_index;

            out << "\n\nEnd value: " << terminal::hex << g_bitmap.start[1] << terminal::dec
            << "\nByte end: " << end.byte_index
            << "\nBit end: " << end.bit_index
            << "\nCurrent Finished: " << static_cast<size_t>(curr - g_bitmap.start);
        }
    }
}
