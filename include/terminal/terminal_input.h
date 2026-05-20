#pragma once

#include <stdint.h>

namespace terminal
{
    class input_line
    {
        static constexpr uint8_t capacity{128};
        char buffer[capacity + 1];
        uint8_t length;
        bool completed;

        public:
            constexpr input_line() noexcept: buffer{'\0'}, length{0}, completed{false} {}
    };
}