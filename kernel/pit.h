#pragma once

#include <stdint.h>

namespace kernel
{
    void initialize_pit(uint32_t frequency) noexcept;
}