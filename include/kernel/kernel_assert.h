#pragma once

#include <stdint.h>

#define KERNEL_ENABLE_DEBUG_ASSERTS

namespace kernel
{
    class logger;

    void set_assert_logger(logger* log) noexcept;
    [[noreturn]] void assert_failed(const char* expression, const char* file, uint32_t line) noexcept;
    [[noreturn]] void assert_failed_msg(const char* expression, const char* message, const char* file, uint32_t line) noexcept;
}

#define KERNEL_ASSERT(expression)                                                           \
do                                                                                          \
{                                                                                           \
    if(!(expression))                                                                       \
    {                                                                                       \
        ::kernel::assert_failed(#expression, __FILE__, static_cast<uint32_t>(__LINE__));    \
    }                                                                                       \
}while(false)

#define KERNEL_ASSERT_MSG(expression, message)                                                         \
do                                                                                                     \
{                                                                                                      \
    if(!(expression))                                                                                  \
    {                                                                                                  \
        ::kernel::assert_failed_msg(#expression, message, __FILE__, static_cast<uint32_t>(__LINE__));  \
    }                                                                                                  \
}while(false)

#ifdef KERNEL_ENABLE_DEBUG_ASSERTS
    #define KERNEL_DEBUG_ASSERT(expression) KERNEL_ASSERT(expression)
    #define KERNEL_DEBUG_ASSERT_MSG(expression, message) KERNEL_ASSERT_MSG(expression, message)
#else
    #define KERNEL_DEBUG_ASSERT(expression)
    #define KERNEL_DEBUG_ASSERT_MSG(expression, message)
#endif