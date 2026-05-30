#pragma once

#include <stdint.h>

namespace kernel
{
    [[nodiscard]] bool initialize_pit(uint32_t frequency) noexcept;
}