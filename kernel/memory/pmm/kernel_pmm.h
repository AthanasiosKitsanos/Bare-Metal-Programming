#pragma once

#include <stdint.h>
#include <stddef.h>
#include "memory/e820/kernel_e820.h"

namespace kernel::memory
{
    constexpr size_t frame_size{4096};

    void pmm_initilalize(const e820_memory_map* map, uintptr_t kernel_start, uintptr_t kernel_end) noexcept;
    uintptr_t pmm_allocate_frame() noexcept;
    void pmm_free_frame(uintptr_t address) noexcept;

    size_t pmm_total_frames() noexcept;
    size_t pmm_used_frames() noexcept;
    size_t pmm_unused_frames() noexcept;
}