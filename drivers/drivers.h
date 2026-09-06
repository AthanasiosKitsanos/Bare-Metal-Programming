#pragma once
#include "keyboard/keyboard.h"

namespace drivers
{
    [[gnu::always_inline]]
    inline void initialize() noexcept
    {
        driver::initialize_keyboard();
    }
}