#include "shell.h"
#include "io/output/terminal_output.h"
#include "internal/shell_commands_list.h"
#include <stdint.h>

namespace
{
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

    constexpr uint8_t command_list_size{3};
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

    [[gnu::always_inline]]
    inline void execute_clear(app::shell* shell) noexcept { shell->clear(); }

    [[gnu::always_inline]]
    inline void execute_exit(app::shell* shell) noexcept { shell->exit(); }
    
    [[gnu::always_inline]]
    inline void execute_peek(app::shell* shell) noexcept { shell->peek(); }

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
    shell::shell(terminal::output* out) noexcept: shell_hot{terminal::input{out}, 1}, shell_cold{out}
    {}

    int8_t shell::command_exists() const noexcept
    {
        uint8_t left{0};
        uint8_t right{command_list_size};
        int8_t mid{0};
        int8_t result{0};
        while(left < right)
        {
            mid = left + (right - left) / 2;
            result = str_compare(shell_hot.m_input.read_buffer(), g_command_list.entries[mid]);
            if(result == 0) return mid;
            else if (result > 0) left = mid + 1;
            else right = mid;
        }
        return -1;
    }

    void shell::execute_command() noexcept
    {
        int8_t index{command_exists()};
        if(index != -1)
        {
            g_command_functions.entries[index](this);
            return;
        }
        *shell_cold.m_output << "Command not found\n";
    }

    void shell::run() noexcept
    {
        while(shell_hot.m_is_running == 1)
        {
            *shell_cold.m_output << "my_OS:> ";
            shell_hot.m_input.start();
            execute_command();
            shell_hot.m_input.reset();
        }
    }

    void shell::clear() noexcept
    {
        shell_hot.m_input.reset();
        shell_cold.m_output->clear();
    }

    void shell::exit() noexcept
    {
        shell_hot.m_is_running = 0;
        *shell_cold.m_output << "Program terminated\n";
    }

    void shell::peek() noexcept { *shell_cold.m_output << "There is nothing to peek!\n"; }
}