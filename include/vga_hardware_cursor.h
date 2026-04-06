#pragma once

#include <stddef.h>
#include <stdint.h>

class vga_hardware_cursor
{
    static constexpr uint16_t command_port{0x3D4};
    static constexpr uint16_t data_port{0x3D5};

    void write_register(uint8_t index, uint8_t value) noexcept;
    uint8_t read_register(uint8_t index) noexcept;

    public:
        void enable(uint8_t start = 14, uint8_t = 15) noexcept;
        inline __attribute__((always_inline)) void disable() noexcept { write_register(0x0A, 0x20); }
        void set_position(size_t position) noexcept;
};