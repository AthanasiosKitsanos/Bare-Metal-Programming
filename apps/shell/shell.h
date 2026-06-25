#pragma once

#include <stdint.h>
#include "io/input/terminal_input.h"

namespace terminal
{
    class output;
}

namespace app
{   
    class alignas(64) shell
    {   
        terminal::input m_input;
        terminal::output* const m_output;
        uint8_t m_is_running;

        int8_t command_exists() const noexcept;
        void execute_command() noexcept;

        public:
            explicit shell(terminal::output* out) noexcept;

            void run() noexcept;
            void clear() noexcept;
            void exit() noexcept;
            void peek() noexcept;
            void flag() noexcept;
    };
}