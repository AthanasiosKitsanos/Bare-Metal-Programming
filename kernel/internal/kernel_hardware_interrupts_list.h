#pragma once

#define HARDWARE_INTERRUPT_LIST \
    X(32, kernel, timer_interrupt, "Timer Interrupt", "IRQ0")   \
    X(33, driver, keyboard_interrupt, "Keyboard Interrupt", "IRQ1")
