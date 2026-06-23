#pragma once

#include <stdint.h>
#include "io/input/terminal_input.h"

namespace terminal
{
    class output;
}

namespace app
{
    struct alignas(64) hot
    {
        terminal::input m_input;
        terminal::output* const m_output;
        uint8_t m_is_running;
    };
    struct cold
    {
        terminal::output* const m_output;
    };
    
    class shell
    {   
        hot shell_hot;
        cold shell_cold;

        int8_t command_exists() const noexcept;
        void execute_command() noexcept;

        public:
            explicit shell(terminal::output* out) noexcept;

            void run() noexcept;
            void clear() noexcept;
            void exit() noexcept;
            void peek() noexcept;
    };
}