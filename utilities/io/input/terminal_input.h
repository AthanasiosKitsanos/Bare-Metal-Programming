#pragma once

#include <stdint.h>

namespace terminal
{
    class input
    {
        static constexpr uint8_t command_capacity{128};
        char command_buffer[command_capacity + 1];
        char* writable_data;
        char* data_end;
        bool command_ready;
    };
}