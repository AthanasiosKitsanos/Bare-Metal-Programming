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
        bool is_running;

        void handle_enter() noexcept;
        void handle_backspace() noexcept;
        void handle_tab() noexcept;
        void handle_escape() noexcept;
        void control_key_dispatch() noexcept;

        public:
            explicit shell(terminal::output* scr) noexcept;

            void run() noexcept;
    };
}