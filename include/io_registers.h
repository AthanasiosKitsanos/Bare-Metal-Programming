#pragma once

#include <stdint.h>

inline __attribute__((always_inline)) void outb(uint16_t port, uint8_t value) noexcept
{
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

inline __attribute__((always_inline)) uint8_t inb(uint16_t port) noexcept
{
    uint8_t value{0};
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}