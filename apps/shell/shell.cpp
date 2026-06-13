#include "shell.h"
#include "io/output/terminal_output.h"
#include "keyboard/keyboard.h"
#include "internal/kernel_interrupt_guard.h"
#include "internal/shell_commands_list.h"
#include "internal/shell_control_input_handlers.h"
#include "internal/shell_navigation_handlers.h"
#include <stdint.h>

namespace
{
    [[gnu::always_inline]]
    inline uint8_t get_control_map_index(const driver::keyboard::keyboard_key key) noexcept
    {
        constexpr uint8_t index_mask{0x03};
        return static_cast<uint8_t>((static_cast<uint16_t>(key) - 1) & index_mask);
    }

    int8_t str_compare(const char* comparer, const char* other) noexcept
    {
        char c{*comparer};
        char o{*other};
        while(c != '\0' && c == o)
        {
            c = *(++comparer);
            o = *(++other);
        }
        return static_cast<int8_t>(c) - static_cast<int8_t>(o);
    }

    constexpr uint8_t command_list_size{2};
    struct command_list
    {
        const char* entries[command_list_size];

        constexpr command_list(): entries{}
        {
            #define X(index, command)  \
                entries[index] = command;
            COMMAND_LIST
            #undef X
        }
    };
    constexpr command_list g_command_list{};

    void execute_clear(app::shell* shell) noexcept { shell->clear_output(); }

    void execute_peek(app::shell* shell) noexcept
    {
        shell->peek_command();
    }

    using command_list_functions = void(*)(app::shell*) noexcept;
    struct command_functions
    {
        command_list_functions entries[command_list_size];

        constexpr command_functions(): entries{}
        {
            #define X(index, command)   \
                entries[index] = execute_##command;
            COMMAND_FUNCTIONS
            #undef X
        }
    };
    constexpr command_functions g_command_functions{};
}

namespace app
{
    shell::shell(terminal::output* scr) noexcept: m_input{}, m_output(scr), m_command_ready{false}, m_is_running{true}
    {}

    void shell::run() noexcept
    {
        driver::keyboard::keyboard_event event{};
        char c{'\0'};
        while(m_is_running)
        {
            *m_output << "my_OS:> ";

            while(!m_command_ready)
            {
                while(!driver::keyboard::poll_keyboard_event(&event))
                {
                    asm volatile("hlt");
                }
                if(driver::keyboard::try_translate_text_event(&event, &c))
                {
                    if(m_input.add_character(c))
                    {
                        *m_output << c;
                        if(!m_input.is_buffer_synched())
                        {
                            m_output->print_string_no_sync(m_input.get_cursor());
                            m_output->move_cursor_to_left_pos(m_input.cursor_to_data_end());
                            m_output->call_cursor_sync();
                        }
                    }
                }
                else if(driver::keyboard::is_control_input_candidate_event(&event))
                {
                    control_key_dispatch(event.key);
                }
                else if(driver::keyboard::is_navigation_input_candidate_event(&event))
                {
                    navigation_key_dispatch(event.key);
                }
            }
            execute_command();
            m_input.reset_buffer();
            m_command_ready = false;
        }
    }

    void shell::handle_escape() noexcept
    {
        m_output->move_cursor_to_right_pos(m_input.cursor_to_data_end());
        for(uint8_t steps{m_input.get_input_count()}; steps > 0; --steps)
        {
            m_output->delete_last_char_no_sync();
        }
        m_output->call_cursor_sync();
        m_input.reset_buffer();
    }
    
    void shell::handle_backspace() noexcept
    {
        if(m_input.delete_character())
        {
            m_output->delete_last_char_no_sync();
            if(!m_input.is_buffer_synched())
            {
                m_output->print_string_no_sync(m_input.get_cursor());
                m_output->move_to_next_no_sync();
                m_output->delete_last_char_no_sync();
                m_output->move_cursor_to_left_pos(m_input.cursor_to_data_end());
            }
            m_output->call_cursor_sync();
        }
    }
    
    void shell::handle_tab() noexcept
    {
        constexpr const char* spaces{"    "};
        if(m_input.add_string(spaces))
        {
            *m_output << spaces;
            if(!m_input.is_buffer_synched())
            {
                m_output->print_string_no_sync(m_input.get_cursor());
                m_output->move_cursor_to_left_pos(m_input.cursor_to_data_end());
                m_output->call_cursor_sync();
            }
        }
    }
    
    void shell::handle_enter() noexcept
    {
        if(!m_input.is_empty())
        {
            if(!m_input.is_buffer_synched()) m_input.go_to_last_printable_input();
        }
        *m_output << '\n';
        m_command_ready = true;
    }
    
    void shell::control_key_dispatch(const driver::keyboard::keyboard_key key) noexcept
    {
        switch(key)
        {
            #define X(key)  \
                case driver::keyboard::keyboard_key::key:   \
                    handle_##key(); \
                return;
            CONTROL_INPUT_HANDLERS
            #undef X

            default:
                return;
        }
    }

    void shell::handle_home() noexcept
    {
        const uint8_t steps{m_input.get_cursor_position()};
        m_input.move_cursor_to_buffer_begin();
        m_output->move_cursor_to_left_pos(steps);
        m_output->call_cursor_sync();
    }

    void shell::handle_arrow_up() noexcept
    {
        return;
    }

    void shell::handle_page_up() noexcept
    {
        return;
    }

    void shell::handle_arrow_left() noexcept
    {
        if(m_input.move_cursor_left()) m_output->move_to_previous();
    }

    void shell::handle_arrow_right() noexcept
    {
        if(m_input.move_cursor_right()) m_output->move_to_next();
    }

    void shell::handle_end() noexcept
    {
        const uint8_t steps{m_input.cursor_to_data_end()};
        m_input.move_cursor_to_buffer_end();
        m_output->move_cursor_to_right_pos(steps);
        m_output->call_cursor_sync();
    }

    void shell::handle_arrow_down() noexcept
    {
        return;
    }

    void shell::handle_page_down() noexcept
    {
        return;
    }

    void shell::navigation_key_dispatch(const driver::keyboard::keyboard_key key) noexcept
    {
        switch(key)
        {
            #define X(key)  \
                case driver::keyboard::keyboard_key::key:   \
                    handle_##key(); \
                    return;
            NAVIGATION_HANDLERS
            #undef X

            default:
                return;
        }
    }

    int8_t shell::command_exists(const char* const command) const noexcept
    {
        uint8_t left{0};
        uint8_t right{command_list_size};
        uint8_t mid{0};
        int8_t c_result{0};
        while(left < right)
        {
            mid = (right - left) / 2;
            c_result = str_compare(command, g_command_list.entries[mid]);
            if(c_result == 0) return mid;
            else if(c_result < 0) right = mid;
            else left = mid + 1;
        }
        return -1;
    }

    void shell::execute_command() noexcept
    {
        const int8_t index{command_exists(m_input.read_buffer())};
        if(index < 0)
        {
            *m_output << "Command not found\n";
            return;
        }
        g_command_functions.entries[index](this);
    }

    [[gnu::always_inline]]
    inline void shell::clear_input() noexcept { m_input.reset_buffer(); }

    [[gnu::always_inline]]
    inline void shell::clear_output() noexcept { m_output->clear(); }

    [[gnu::always_inline]]
    inline void shell::peek_command() noexcept
    {
        *m_output << "Nothing to peek in here!\n";
    }
}