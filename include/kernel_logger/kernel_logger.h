#pragma once

#include "terminal.h"

class kernel_logger
{
    terminal* const m_terminal;

    public:
        kernel_logger(terminal* const) noexcept;
        ~kernel_logger() noexcept = default;

        inline void __attribute__((always_inline)) print_error(const char* text) noexcept { *m_terminal << text; }
};