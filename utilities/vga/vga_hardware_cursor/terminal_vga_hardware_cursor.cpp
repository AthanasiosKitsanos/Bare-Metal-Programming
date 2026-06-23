#include "terminal_vga_hardware_cursor.h"
#include "internals/terminal_io_registers.h"

namespace
{
    constexpr uint8_t cursor_register_low{0x0F};
    constexpr uint8_t cursor_register_high{0x0E};
    constexpr uint8_t display_register_low{0x0D};
    constexpr uint8_t display_register_high{0x0C};
}

namespace terminal
{
    void vga_hardware_cursor::write_register(uint8_t index, uint8_t value) noexcept
    {
        outb(command_port, index);
        outb(data_port, value);
    }

    uint8_t vga_hardware_cursor::read_register(uint8_t index) noexcept
    {
        outb(command_port, index);
        return inb(data_port);
    }

    void vga_hardware_cursor::enable(uint8_t start, uint8_t end) noexcept
    {
        uint8_t cursor_start{static_cast<uint8_t>((read_register(0x0A) & 0xC0) | (start & 0x1F))};
        write_register(0x0A, cursor_start);

        uint8_t cursor_end{static_cast<uint8_t>((read_register(0x0B) & 0xE0) | (end & 0x1F))};
        write_register(0x0B, cursor_end);
    }

    void vga_hardware_cursor::set_position(const uint16_t position) noexcept
    {
        write_register(cursor_register_low, static_cast<uint8_t>(position & 0x00FF));
        write_register(cursor_register_high, static_cast<uint8_t>((position >> 8) & 0x00FF));
    }

    void vga_hardware_cursor::set_display_start(const uint16_t position) noexcept
    {
        write_register(display_register_low, static_cast<uint8_t>(position & 0x00FF));
        write_register(display_register_high, static_cast<uint8_t>((position >> 8) & 0x00FF));
    }
}