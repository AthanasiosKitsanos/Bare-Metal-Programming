#include "kernel_exceptions.h"
#include "kernel_logger.h"
#include "kernel_idt.h"
#include <stdint.h>

namespace
{
    // Vectors
    constexpr uint8_t invalid_opcode_vector{6};
    constexpr uint8_t division_by_zero{0};

    constexpr uint16_t kernel_code_selector{0x08};
    constexpr uint8_t interrupt_gate_attributes{0x8E};

    kernel::logger* g_exception_logger{nullptr};

    [[noreturn]] void halt_forever() noexcept
    {
         for(;;) asm volatile("cli; hlt");
    }
}

extern "C" void invalid_opcode_stub() noexcept;
extern "C" [[noreturn]]  void invalid_opcode_exception_handler() noexcept
{
    if(!g_exception_logger) halt_forever();
    g_exception_logger->error() << "CPU exception: Invalid Opcode (#UD, vector 6)\n";
    g_exception_logger->panic("Unhandled CPU exception");
}

extern "C" void divide_error_stub() noexcept;
extern "C" [[noreturn]] void divide_error_exception_handler() noexcept
{
    if(!g_exception_logger) halt_forever();
    g_exception_logger->error() << "CPU exception: Divide Exception (#DE, vector 0)\n";
    g_exception_logger->panic("Unhandled CPU exception");
}

namespace kernel
{
    void set_exception_logger(logger* log) noexcept { g_exception_logger = log; }

    void initialize_exceptions() noexcept
    {
        initialize_idt();
        set_interrupt_gate(division_by_zero, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(divide_error_stub)), kernel_code_selector, interrupt_gate_attributes);
        set_interrupt_gate(invalid_opcode_vector, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(invalid_opcode_stub)), kernel_code_selector, interrupt_gate_attributes);
        load_idt();
    }
}