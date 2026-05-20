#include "kernel_assert.h"
#include "kernel_logger.h"

namespace
{
    kernel::logger* g_assert_logger{nullptr};

    [[noreturn]] void halt_forever() noexcept
    {
        while(true) asm volatile("cli; hlt");
    }

    terminal::output& build_assert_message(const char* expression, const char* file, uint32_t line) noexcept
    {
        if(!g_assert_logger) halt_forever();
        return g_assert_logger->error() << "Assertion failed: " << expression << "\nFile: " << file << "\nLine: " << line << '\n';
    }
}

namespace kernel
{
    constexpr const char* panic_message{"Kernel assertion failed"};
    void set_assert_logger(logger* log) noexcept { g_assert_logger = log; }

    [[noreturn]] void assert_failed(const char* expression, const char* file, uint32_t line) noexcept
    {
        build_assert_message(expression, file, line);
        g_assert_logger->panic(panic_message);
    }

    [[noreturn]] void assert_failed_msg(const char* expression, const char* message, const char* file, uint32_t line) noexcept
    {
        build_assert_message(expression, file, line) << "Message: " << message << '\n';
        g_assert_logger->panic(panic_message);
    }
}