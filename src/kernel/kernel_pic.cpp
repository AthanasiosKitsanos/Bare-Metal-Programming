#include "kernel_pic.h"
#include <stdint.h>

namespace
{
    constexpr uint16_t master_command{0x20};
    constexpr uint16_t master_data{0x21};
    constexpr uint16_t slave_command{0xA0};
    constexpr uint16_t slave_data{0xA1};
    
    constexpr uint8_t enable{0x11};
    constexpr uint8_t master_bit{0x04};
    constexpr uint8_t slave_bit{0x02};
    constexpr uint8_t x86_mode{0x01};
    constexpr uint8_t eoi_command{0x20};
}

namespace kernel
{
    void pic_remap(uint8_t offset_1, uint8_t offset_2) noexcept
    {
        uint8_t master_mask{inb(master_data)};
        uint8_t slave_mask{inb(slave_data)};

        outb(master_command, enable);
        io_wait();
        outb(slave_command, enable);
        io_wait();

        outb(master_data, offset_1);
        io_wait();
        outb(slave_data, offset_2);
        io_wait();

        outb(master_data, master_bit);
        io_wait();
        outb(slave_data, slave_bit);
        io_wait();

        outb(master_data, x86_mode);
        io_wait();
        outb(slave_data, x86_mode);
        io_wait();

        outb(master_data, master_mask);
        io_wait();
        outb(slave_data, slave_mask);
        io_wait();
    }

    void send_eoi(uint8_t vector) noexcept
    {
        if(vector > 39) outb(slave_command, eoi_command);
        outb(master_command, eoi_command);
    }

    void mask_all_except_timer() noexcept
    {
        outb(master_data, 0xFE);
        io_wait();
        outb(slave_data, 0xFF);
        io_wait();
    }
}