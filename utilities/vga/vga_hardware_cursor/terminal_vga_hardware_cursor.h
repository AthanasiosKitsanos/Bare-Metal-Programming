#pragma once

#include <stddef.h>
#include <stdint.h>

namespace terminal
{
    class vga_hardware_cursor
    {
        static constexpr uint16_t command_port{0x3D4};
        static constexpr uint16_t data_port{0x3D5};

        [[gnu::regparm(1)]]
        static uint8_t read_register(uint8_t index) noexcept;

        [[gnu::regparm(2)]]
        static void write_register(uint8_t index, uint8_t value) noexcept;

        public:
            static void enable(uint8_t start = 14, uint8_t end = 15) noexcept;

            [[gnu::regparm(1)]]
            static void set_position(uint16_t position) noexcept;

            [[gnu::regparm(1)]]
            static void set_display_start(const uint16_t position) noexcept;
            
            [[gnu::always_inline]]
            inline static void disable() noexcept { write_register(0x0A, 0x20); }
    };
}