#pragma once

#include <stdint.h>

struct hex32
{
    uint32_t value;
};

inline hex32 __attribute__((always_inline)) hex(uint32_t value) noexcept
{
    return hex32{value};
}