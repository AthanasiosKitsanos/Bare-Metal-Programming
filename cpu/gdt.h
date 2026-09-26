#pragma once

#include <stdint.h>

namespace cpu::gdt
{
    constexpr uint16_t zero_seg{static_cast<size_t>(0x00)};
    constexpr uint16_t code_seg{static_cast<size_t>(0x08)};
    constexpr uint16_t data_seg{static_cast<size_t>(0x10)};

    struct [[gnu::packed]] descriptor
    {
        uint16_t length;
        uint32_t base;
    };

    void initialize() noexcept;
}