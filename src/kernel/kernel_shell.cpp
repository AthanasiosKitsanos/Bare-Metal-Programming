#include "kernel_shell.h"
#include <stdint.h>
#include "kernel_control_input_table.h"
#include "keyboard.h"
#include "kernel_interrupt_guard.h"
#include "terminal_output.h"

namespace
{
    driver::keyboard_event g_k_event{};

    using control_func_ptr = void(*)(kernel::shell*) noexcept;

    constexpr uint8_t table_size{4};
    struct control_func_table
    {
        control_func_ptr entries[table_size];

        constexpr control_func_table() noexcept: entries{}
        {
            #define X(func_name, index) \
                entries[index] = kernel::func_name##_handler;
            CONTROL_KEY_MAPPING
            #undef X
        }
    };

    constexpr control_func_table g_control_table{};

    int16_t string_compare(const char* from, const char* to) noexcept
    {
        while(*from != '\0' && *from == *to)
        {
            ++from;
            ++to;
        }
        return static_cast<int16_t>(*from) - static_cast<int16_t>(*to);
    }

    [[gnu::always_inline]]
    inline uint8_t control_key_index(const uint8_t key_code) noexcept
    {
        constexpr uint8_t index_mask{0x03};
        return ((key_code - 1) & index_mask);
    }

    void wait_for_keyboard_event() noexcept
    {
        kernel::disable_interrupts();
        if(!driver::has_pending_keyboard_event()) kernel::enable_interrupt_and_halt();
        else kernel::enable_interrupts();
    }
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

    void shell::read_command() noexcept
    {
        char c{'\0'};
        while(!command_ready)
        {
            while(driver::poll_keyboard_event(&g_k_event))
            {
                if(driver::try_translate_text_event(&g_k_event, &c))
                {
                    if(push_character(c)) *(console) << c;
                    continue;
                }
                if(driver::is_control_input_candidate_event(&g_k_event))
                {
                    g_control_table.entries[control_key_index(g_k_event.key_code)](this);
                    continue;
                }
                if(driver::is_navigation_input_candidate_event(&g_k_event))
                {
                    // g_navigation_table.entries
                    // continue;
                }
            }

            if(!command_ready) wait_for_keyboard_event();
        }
        if(string_compare(command_buffer, "clear") == 0) console->clear();
        reset();
    }

    // Control Friend Fucntions
    void escape_handler(shell* s) noexcept
    {
        while(s->current_data > s->command_buffer)
        {
            --s->current_data;
            s->console->delete_last_char_no_sync();
        }
        *s->current_data = '\0';
        s->console->call_cursor_sync();
        s->command_ready = true;
    }

    void backspace_handler(shell* s) noexcept { if(s->backspace()) s->console->delete_last_char_sync(); }
    
    void tab_handler(shell* s) noexcept
    {
        constexpr uint8_t tab_spaces{4};
        for(uint8_t i{0}; i < tab_spaces; ++i)
        {
            if(s->push_character(' ')) *s->console << ' ';
        }
    }

    void enter_handler(kernel::shell* s) noexcept
    {
        if(!s->is_empty())
        {
            *(s->console) << '\n' << s->command_buffer;
        }
        s->submit();
        *(s->console) << '\n';
    }

    // Navigation Friend Functions

}