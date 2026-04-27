#pragma once

#include <stdint.h>

namespace kernel
{
    struct [[gnu::packed]] interrupt_frame
    {
        uint32_t eax;
        uint32_t ecx;
        uint32_t edx;
        uint32_t ebx;
        uint32_t esp;
        uint32_t ebp;
        uint32_t esi;
        uint32_t edi;
        uint32_t vector;
        uint32_t error_code;
        uint32_t eip;
        uint32_t cs;
        uint32_t eflags;
    };

    static_assert(sizeof(interrupt_frame) == 52);
}