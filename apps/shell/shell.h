#pragma once

#include "io/input/terminal_input.h"

namespace terminal
{
    class output;
}

namespace driver::keyboard
{
    enum class keyboard_key: uint16_t;
}

namespace app
{
    class shell
    {
        terminal::input m_input;
        terminal::output* const m_output;
        bool m_command_ready;
        bool m_is_running;

        void handle_escape() noexcept;
        void handle_backspace() noexcept;
        void handle_tab() noexcept;
        void handle_enter() noexcept;
        void control_key_dispatch(const driver::keyboard::keyboard_key key) noexcept;

        void handle_home() noexcept;
        void handle_arrow_up() noexcept;
        void handle_page_up() noexcept;
        void handle_arrow_left() noexcept;
        void handle_arrow_right() noexcept;
        void handle_end() noexcept;
        void handle_arrow_down() noexcept;
        void handle_page_down() noexcept;
        void navigation_key_dispatch(const driver::keyboard::keyboard_key key) noexcept;

        public:
            explicit shell(terminal::output* scr) noexcept;

            void run() noexcept;
    };
}