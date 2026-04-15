#pragma once

#include <stdint.h>

namespace kernel
{
    class logger;

    void set_assert_logger(logger* log) noexcept;
    [[noreturn]] void assert_failed(const char* expression, const char* file, uint32_t line) noexcept;
}

#define KERNEL_ASSERT(expression)                                                               \
    do                                                                                          \
    {                                                                                           \
        if(!(expression))                                                                       \
        {                                                                                       \
            ::kernel::assert_failed(#expression, __FILE__, static_cast<uint32_t>(__LINE__));    \
        }                                                                                       \
    }while(false)
