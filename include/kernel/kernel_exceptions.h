#pragma once

namespace kernel
{
    class logger;

    void set_exception_logger(logger* log) noexcept;
    void initialize_exceptions() noexcept;
}