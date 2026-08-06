#pragma once

#include <stdint.h>
#include "io/input/terminal_input.h"
#include "io/output/terminal_output.h"

namespace app
{   
    class alignas(64) shell
    {   
        terminal::input m_input;
        terminal::output m_output;
        uint8_t m_is_running;

        int8_t command_exists() const noexcept;
        void execute_command() noexcept;

        public:
            shell() noexcept;
            shell(const shell&) = delete;
            shell& operator=(const shell&) = delete;

            void run() noexcept;
            void clear() noexcept;
            void exit() noexcept;
            void flag() noexcept;
            void ticks() noexcept;
            void kernel_stack() noexcept;
            void interrupt_stack() noexcept;
    };
}