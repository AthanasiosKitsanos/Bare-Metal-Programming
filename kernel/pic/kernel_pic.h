#pragma once

#include <stdint.h>
#include "terminal_io_registers.h"

namespace kernel
{
    void pic_remap(uint8_t offset_1, uint8_t offset_2) noexcept;
    void send_eoi(uint8_t vector) noexcept;
    void mask_all_except_timer_and_keyboard() noexcept;
    
    inline void __attribute__((always_inline)) io_wait() noexcept { terminal::outb(0x80, 0); }
}