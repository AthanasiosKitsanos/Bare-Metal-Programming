#pragma once

#include <stdint.h>
#include "terminal/input.h"
#include "terminal/output.h"

namespace app
{
    class alignas(64) shell
    {
        terminal::input m_input;
        terminal::output m_output;
        uint8_t m_is_running;

        [[gnu::regparm(1)]]
        int8_t command_exists() const noexcept;

        [[gnu::regparm(1)]]
        void execute_command() noexcept;

        public:
            shell() noexcept;
            shell(const shell&) = delete;
            shell& operator=(const shell&) = delete;

            [[gnu::regparm(1)]]
            void run() noexcept;

            [[gnu::regparm(1)]]
            void clear() noexcept;

            [[gnu::regparm(1)]]
            void exit() noexcept;

            [[gnu::regparm(1)]]
            void flag() noexcept;

            [[gnu::regparm(1)]]
            void ticks() noexcept;

            [[gnu::regparm(1)]]
            void kernel_stack() noexcept;

            [[gnu::regparm(1)]]
            void interrupt_stack() noexcept;

            [[gnu::regparm(1)]]
            void test_allocation() noexcept;
    };
}
