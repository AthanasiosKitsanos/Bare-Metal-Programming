#include "shell.h"
#include "io/output/terminal_output.h"
#include "keyboard/keyboard.h"
#include "internal/kernel_interrupt_guard.h"
#include "internal/shell_commands_list.h"

namespace
{
    [[gnu::always_inline]]
    inline void control_key_dispatch(const driver::keyboard::keyboard_key key) noexcept
    {
        
    }
}

namespace app
{
    shell::shell(terminal::output* scr) noexcept: m_input{}, m_output(scr), m_command_ready{false}, is_running{true}
    {}

    void shell::run() noexcept
    {
        driver::keyboard::keyboard_event event{};
        char c{'\0'};
        while(is_running)
        {
            *m_output << "my_OS:> ";

            while(!m_command_ready)
            {
                if(!driver::keyboard::poll_keyboard_event(&event))
                {
                    asm volatile("hlt");
                    continue;
                }
                if(driver::keyboard::try_translate_text_event(&event, &c))
                {
                    if(m_input.add_character(c)) *m_output << c;
                }
                else if(driver::keyboard::is_control_input_candidate_event(&event))
                {
                    switch(event.key)
                    {
                        case driver::keyboard::keyboard_key::enter:

                    }
                }
                else if(driver::keyboard::is_navigation_input_candidate_event(&event))
                {

                }
            }
            m_input.reset_buffer();
            m_command_ready = false;
        }
    }

    void shell::handle_enter() noexcept
    {
        
    }

    void shell::handle_backspace() noexcept
    {

    }

    void shell::handle_tab() noexcept
    {

    }

    void shell::handle_escape() noexcept
    {

    }
}