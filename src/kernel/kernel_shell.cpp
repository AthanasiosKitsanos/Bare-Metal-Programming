#include "kernel_shell.h"
#include <stdint.h>
#include "kernel_control_input_table.h"
#include "keyboard.h"

namespace
{
    driver::keyboard_event g_k_event{};

    void escape_handler(kernel::shell* s, char) noexcept {}
    void backspace_handler(kernel::shell* s, char) noexcept {}
    void tab_handler(kernel::shell* s, char) noexcept {}

    void enter_handler(kernel::shell* s, char c) noexcept
    {
        if(!s->is_empty())
        {
            s->submit();
            *(s->get_console()) << '\n' << s->command();
            s->reset();
        }
        *(s->get_console()) << '\n';
    }

    using control_func_ptr = void(*)(kernel::shell*, char c) noexcept;

    constexpr uint8_t table_size{4};
    struct control_func_table
    {
        control_func_ptr entries[table_size];

        constexpr control_func_table() noexcept: entries{}
        {
            #define X(func_name, index) \
                entries[index] = func_name##_handler;
            CONTROL_KEY_MAPPING
            #undef X
        }
    };

    constexpr control_func_table g_control_table{};
}

namespace kernel
{
    void shell::reset() noexcept
    {
        current_data = command_buffer;
        *current_data = '\0';
        command_ready = false;
    }

    bool shell::push_character(char c) noexcept
    {
        if(command_ready || current_data >= (command_buffer + command_capacity)) return false;
        *current_data = c;
        ++current_data;
        return true;
    }

    bool shell::backspace() noexcept
    {
        if(command_ready || command_size() == 0) return false;
        --current_data;
        return true;
    }

    void shell::handle_input() noexcept
    {
        char c{'\0'};
        while(driver::poll_keyboard_event(&g_k_event))
        {
            if(driver::try_translate_text_event(&g_k_event, &c))
            {
                if(push_character(c)) console << c;
                continue;
            }
            if(driver::is_control_input_candidate_event(&g_k_event))
            {
                g_control_table.entries[static_cast<uint8_t>(g_k_event.key)  ](this, c);
            }
        }
    }
}