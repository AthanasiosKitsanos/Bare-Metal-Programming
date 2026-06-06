#pragma once

#include <stdint.h>
#include <stddef.h>

namespace terminal
{
    class output;
}

namespace kernel::memory
{
    constexpr size_t frame_size{4096};

    struct e820_memory_map;

    void pmm_initialize(const e820_memory_map* map, const uintptr_t kernel_start, const uintptr_t kernel_end) noexcept;
    uintptr_t pmm_allocate_frame() noexcept;
    void pmm_free_frame(const uintptr_t, terminal::output* const output) noexcept;

    size_t pmm_total_frames() noexcept;
    size_t pmm_used_frames() noexcept;
    size_t pmm_free_frames() noexcept;
}