#pragma once

#include "terminal.h"
#include <stdint.h>

class kernel_logger
{
    terminal* const m_terminal;
    
    public:
        kernel_logger(terminal* const) noexcept;
        ~kernel_logger() noexcept = default;

        inline terminal& __attribute__((always_inline)) error() noexcept { return *m_terminal << "[ERROR]: ";}
        inline terminal& __attribute__((always_inline)) warning() noexcept {return *m_terminal << "[WARNING]: "; }
        inline terminal& __attribute__((always_inline)) info() noexcept { return *m_terminal << "[INFO]: "; }
        inline terminal& __attribute__((always_inline)) debug() noexcept { return *m_terminal << "[DEBUG]: "; }
};