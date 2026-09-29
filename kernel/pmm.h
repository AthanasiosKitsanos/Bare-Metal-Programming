#pragma once

#include <stdint.h>
#include <stddef.h>

namespace kernel::memory
{
    constexpr size_t frame_size{4096};

    struct e820_memory_map;

    enum class pmm_result: uint8_t
    {
        success = 0x00,
        failed = 0x01,
        lb_deny = 0x02,
        hb_deny = 0x03,
        zero_frames = 0x04
    };

    [[gnu::regparm(2)]]
    void pmm_initialize(const e820_memory_map* map, const uintptr_t kenrel_end) noexcept;

    void* pmm_allocate_frame() noexcept;

    [[gnu::regparm(1)]]
    void* pmm_allocate_contiguous_frames(const size_t frames) noexcept;

    [[gnu::regparm(2)]]
    pmm_result pmm_free_contiguous_frames(const void* address, const size_t frames) noexcept;

    [[gnu::regparm(1)]]
    pmm_result pmm_free_frame(const void*) noexcept;

    size_t pmm_total_frames() noexcept;
    size_t pmm_used_frames() noexcept;
    size_t pmm_free_frames() noexcept;

    // Testing
    
    namespace test
    {
        enum class set: uint8_t
        {
            all_zero = 0x00,
            all_ones = 0x01,
            all_random = 0x03
        };
        
        template<set SET>
        struct test_bitmap
        {
            uint8_t entries[frame_size];
            constexpr test_bitmap(): entries{}
            {
                if constexpr(SET == set::all_zero)
                {
                    for(size_t i{0}; i < frame_size; ++i) entries[i] = 0x00;
                }
                else if constexpr(SET == set::all_ones)
                {
                    for(size_t i{0}; i < frame_size; ++i) entries[i] = 0xFF;
                }
                else
                {
                    uint8_t value{0x00};
                    for(size_t i{0}; i < frame_size; ++i)
                    {
                        entries[i] = value;
                        ++value;
                    }
                }
            }
        };
        
        size_t core_8(const size_t frames, const uint8_t* end, const uint8_t** const start) noexcept;

        template<set SET>
        inline constexpr test_bitmap<SET> map{};

        template<set SET>
        constexpr const uint8_t* entries()
        {
            return map<SET>.entries;
        }

        template<set SET>
        size_t test_map(const size_t frames) noexcept
        {
            const uint8_t* ptr{entries<SET>()};
            return core_8(frames, ptr + frame_size, &ptr);
        }
    }
}