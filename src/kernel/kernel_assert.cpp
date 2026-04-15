#include "kernel_assert.h"
#include "kernel_logger.h"

namespace
{
    kernel::logger* g_assert_logger{nullptr};

    [[noreturn]] void halt_forever() noexcept
    {
        while(true) asm volatile("cli; hlt");
    }
}

namespace kernel
{
    void set_assert_logger(logger* log) noexcept { g_assert_logger = log; }

    [[noreturn]] void assert_failed(const char* expression, const char* file, uint32_t line)
    {
        if(!g_assert_logger) halt_forever();
        g_assert_logger->error() << "Assertion failed: " << expression << '\n';
        g_assert_logger->error() << "File: " << file << '\n';
        g_assert_logger->error() << "Line: " << line << '\n';
        g_assert_logger->panic("Kernel assertion failed");
    }
}