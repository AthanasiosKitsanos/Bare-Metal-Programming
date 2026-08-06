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
        hb_deny = 0x03
    };

    [[gnu::regparm(3)]]
    void pmm_initialize(const e820_memory_map*, const uintptr_t, const uintptr_t) noexcept;

    void* pmm_allocate_frame() noexcept;

    [[gnu::regparm(1)]]
    void* pmm_find_contiguous_free_frames(const size_t frames) noexcept;

    [[gnu::regparm(1)]]
    pmm_result pmm_free_frame(const uintptr_t) noexcept;

    size_t pmm_total_frames() noexcept;
    size_t pmm_used_frames() noexcept;
    size_t pmm_free_frames() noexcept;
}