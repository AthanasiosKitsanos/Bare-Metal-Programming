#pragma once

#include "io/input/terminal_input.h"

namespace terminal
{
    class output;
}

namespace app
{
    class shell
    {
        terminal::input m_input;
        terminal::output* const m_output;
        bool m_command_ready;

        public:
            explicit shell(terminal::output* scr) noexcept;
    };
}