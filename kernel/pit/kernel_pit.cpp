#include "kernel_pit.h"
#include "internals/terminal_io_registers.h"

namespace
{
    constexpr uint32_t input_frequency{1193182};
    constexpr uint32_t min_divisor{1};
    constexpr uint32_t max_divisor{0xFFFF};
    constexpr uint32_t min_supported_frequency{20};
    constexpr uint32_t max_supported_frequency{100};

    constexpr uint16_t channel_0_data_port{0x40};
    constexpr uint16_t command_port{0x43};

    constexpr uint8_t channel_0_lobyte_hibyte_mode_3_binary{0x36};
}

namespace kernel
{
    bool initialize_pit(uint32_t frequency) noexcept
    {
        if(frequency < min_supported_frequency || frequency > max_supported_frequency) return false;

        const uint32_t divisor_32{(input_frequency + frequency / 2) / frequency};
        if(divisor_32 < min_divisor || divisor_32 > max_divisor) return false;
        const uint16_t divisor{static_cast<uint16_t>(divisor_32)};

        terminal::outb(command_port, channel_0_lobyte_hibyte_mode_3_binary);
        terminal::outb(channel_0_data_port, static_cast<uint8_t>(divisor & 0x00FF));
        terminal::outb(channel_0_data_port, static_cast<uint8_t>((divisor >> 8) & 0x00FF));
        
        return true;
    }
}