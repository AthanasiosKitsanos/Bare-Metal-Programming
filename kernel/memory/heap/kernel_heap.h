#pragma once

#include <stdint.h>

namespace kernel::memory
{
    struct alignas(8) block_header
    {
        block_header* next;
        uint32_t flags;
        uint32_t size;
    };

    void heap_initialize(void* heap_start, uint32_t heap_size) noexcept;
    void* kmalloc(uint32_t requested_size) noexcept;
    void kfree(void* ptr) noexcept;
}