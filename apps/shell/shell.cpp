#include <stdint.h>

#include "shell.h"
#include "internal/shell_commands_list.h"
#include "cpu/features.h"
#include "kernel/timer.h"

extern "C" uint32_t _kernel_stack_top;
extern "C" uint32_t _kernel_stack_bottom;
extern "C" uint32_t _interrupt_stack_top;
extern "C" uint32_t _interrupt_stack_bottom;

namespace kernel::memory::test
{
    extern void allocate_8_frames() noexcept;
}

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

    constexpr uint8_t command_list_size{7};
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

    [[gnu::regparm(1)]]
    void execute_clear(app::shell* shell) noexcept { shell->clear(); }

    [[gnu::regparm(1)]]
    void execute_exit(app::shell* shell) noexcept { shell->exit(); }

    [[gnu::regparm(1)]]
    void execute_flag(app::shell* shell) noexcept { shell->flag(); }

    [[gnu::regparm(1)]]
    void execute_ticks(app::shell* shell) noexcept { shell->ticks(); }

    [[gnu::regparm(1)]]
    void execute_interrupt_stack(app::shell* shell) noexcept { shell->interrupt_stack(); }

    [[gnu::regparm(1)]]
    void execute_kernel_stack(app::shell* shell) noexcept { shell->kernel_stack(); }

    [[gnu::regparm(1)]]
    void execute_pmm_test(app::shell* shell) noexcept { shell->test_allocation(); }

    using command_list_functions = void(*)(app::shell*) noexcept [[gnu::regparm(1)]];
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
    shell::shell() noexcept: m_input{}, m_output{}, m_is_running{true}
    {}

    [[gnu::regparm(1)]]
    int8_t shell::command_exists() const noexcept
    {
        uint8_t left{0};
        uint8_t right{command_list_size};
        int8_t mid{0};
        int8_t result{0};
        while(left < right)
        {
            mid = left + (right - left) / 2;
            result = str_compare(m_input.read_buffer(), g_command_list.entries[mid]);
            if(result == 0) return mid;
            else if (result > 0) left = mid + 1;
            else right = mid;
        }
        return -1;
    }

    [[gnu::regparm(1)]]
    void shell::execute_command() noexcept
    {
        int8_t index{command_exists()};
        if(index != -1)
        {
            g_command_functions.entries[index](this);
            return;
        }
        m_output << "Command not found\n";
    }

    [[gnu::regparm(1)]]
    void shell::run() noexcept
    {
        while(m_is_running == 1)
        {
            m_output << "my_OS:> ";
            m_input.start();
            execute_command();
            m_input.reset();
        }
    }

    [[gnu::regparm(1)]]
    void shell::clear() noexcept
    {
        m_input.reset();
        m_output.clear();
    }

    [[gnu::regparm(1)]]
    void shell::exit() noexcept
    {
        m_is_running = 0;
        m_output << "Program terminated\n";
    }

    [[gnu::regparm(1)]]
    void shell::flag() noexcept { m_output << "mm_flag: " << cpu::features::get() << '\n'; }

    [[gnu::regparm(1)]]
    void shell::ticks() noexcept
    {
        m_output << "Ticks: " << kernel::timer_ticks() << '\n';
    }

    [[gnu::regparm(1)]]
    void shell::interrupt_stack() noexcept
    {
        const uintptr_t interrupt_stack{reinterpret_cast<uintptr_t>(&_interrupt_stack_top) - reinterpret_cast<uintptr_t>(&_interrupt_stack_bottom)};
        m_output << interrupt_stack << " bytes\n";
    }

    [[gnu::regparm(1)]]
    void shell::kernel_stack() noexcept
    {
        const uintptr_t kernel_stack{reinterpret_cast<uintptr_t>(&_kernel_stack_top) - reinterpret_cast<uintptr_t>(&_kernel_stack_bottom)};
        m_output << kernel_stack << " bytes\n";
    }

    [[gnu::regparm(1)]]
    void shell::test_allocation() noexcept
    {
        kernel::memory::test::allocate_8_frames();
    }
}
