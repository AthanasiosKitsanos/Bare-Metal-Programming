#include "kernel_shell.h"
#include <stdint.h>
#include "kernel_control_input_table.h"
#include "keyboard.h"
#include "kernel_interrupt_guard.h"
#include "terminal_output.h"
#include "kernel_command_functions.h"

namespace
{
    driver::keyboard_event g_k_event{};

    using control_func_ptr = void(*)(kernel::shell*) noexcept;

    // Control Table Struct
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

    [[gnu::always_inline]]
    inline uint8_t control_key_index(const uint8_t key_code) noexcept
    {
        constexpr uint8_t index_mask{0x03};
        return ((key_code - 1) & index_mask);
    }

    using nav_func_ptr = void (*)(kernel::shell*) noexcept;

    // Navigation Table Struct
    constexpr uint8_t nav_table_size{16};
    struct nav_func_table
    {
        nav_func_ptr entries[nav_table_size];
        
        constexpr nav_func_table(): entries{}
        {
            #define X(func_name, index) \
                entries[index] = kernel::func_name##_handler;
            NAVIGATION_KEY_TABLE
            #undef X
        }
    };

    constexpr nav_func_table g_navigation_func_table{};

    [[gnu::always_inline]]
    inline uint8_t navigation_key_index(const uint8_t key_code) noexcept
    {
        constexpr uint8_t mask{0x0F};
        return (key_code & mask);
    }

    int16_t string_compare(const char* from, const char* to) noexcept
    {
        while(*from != '\0' && *from == *to)
        {
            ++from;
            ++to;
        }
        return static_cast<int16_t>(*from) - static_cast<int16_t>(*to);
    }

    void wait_for_keyboard_event() noexcept
    {
        kernel::disable_interrupts();
        if(!driver::has_pending_keyboard_event()) kernel::enable_interrupt_and_halt();
        else kernel::enable_interrupts();
    }

    using command_function_ptr = void(*)(terminal::output*) noexcept;
    struct command_map
    {
        const char* command;
        command_function_ptr function;
    };

    [[gnu::always_inline]]
    inline void clear_hadnler(terminal::output* console) noexcept { console->clear();}


    constexpr uint8_t command_table_size{1};
    struct command_map_table
    {
        command_map entries[command_table_size];

        constexpr command_map_table(): entries{}
        {
            #define X(index, command, func)    \
                entries[index] = {command, func##_hadnler};
            COMMAND_MAPPING
            #undef X
        }
    };
    constexpr command_map_table command_table{};

    void execute_command(const char* command, terminal::output* out) noexcept
    {
        const command_map* left{command_table.entries};
        const command_map* right{command_table.entries + command_table_size};
        const command_map* mid{nullptr};
        uint16_t result{0};
        while(left < right)
        {
            mid = left + ((right - left) / 2);
            result = string_compare(command, mid->command);
            if(result == 0)
            {
                mid->function(out);
                return;
            }
            else if(result < 0) left = mid + 1;
            else right = mid;
        }
        if(out) *out << "Command not found\n";
    }
}

namespace kernel
{
    void shell::reset() noexcept
    {
        writable_data = command_buffer;
        data_end = command_buffer;
        *writable_data = '\0';
        command_ready = false;
    }

    bool shell::push_character(char c) noexcept
    {
        if(command_ready || data_end >= command_buffer + command_capacity) return false;
        *writable_data = c;
        ++writable_data;
        ++data_end;
        return true;
    }

    bool shell::backspace() noexcept
    {
        if(command_ready || command_size() == 0) return false;
        --writable_data;
        --data_end;
        *writable_data= ' ';
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
                }
                else if(driver::is_control_input_candidate_event(&g_k_event))
                {
                    g_control_table.entries[control_key_index(g_k_event.key_code)](this);
                    continue;
                }
                else if(driver::is_navigation_input_candidate_event(&g_k_event))
                {
                    g_navigation_func_table.entries[navigation_key_index(g_k_event.key_code)](this);
                }
            }

            if(!command_ready) wait_for_keyboard_event();
        }
        // if(string_compare(command_buffer, "clear") == 0) console->clear();
        execute_command(command_buffer, console);
        reset();
    }

    // Control Friend Fucntions
    void escape_handler(shell* s) noexcept
    {
        while(s->writable_data > s->command_buffer)
        {
            --s->writable_data;
            s->console->delete_last_char_no_sync();
        }
        s->data_end = s->writable_data;
        *s->writable_data = '\0';
        s->console->call_cursor_sync();
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
        s->submit();
        if(!s->is_empty()) *(s->console) << '\n' << s->command_buffer;
        *(s->console) << '\n';
    }

    // Navigation Friend Functions
    void arrow_down_handler(shell* s) noexcept
    {
        return;
    }

    void page_down_handler(shell* s) noexcept
    {
        return;
    }

    void home_handler(shell* s) noexcept
    {
        return;
    }

    void arrow_up_handler(shell* s) noexcept
    {
        return;
    }

    void page_up_handler(shell* s) noexcept
    {
        return;
    }

    void arrow_left_handler(shell* s) noexcept
    {
        if(s->move_arrow_left()) s->console->move_cursor_left();
    }

    void arrow_right_handler(shell* s) noexcept
    {
        if(s->move_arrow_right()) s->console->move_cursor_right();
    }

    void end_handler(shell* s) noexcept
    {
        return;
    }
}