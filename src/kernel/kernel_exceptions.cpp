#include "kernel_exceptions.h"
#include "kernel_logger.h"
#include "kernel_idt.h"
#include <stdint.h>

namespace
{
    // Vectors
    constexpr uint8_t invalid_opcode_vector{6};
    constexpr uint8_t divide_error_vector{0};

    // Selectors and Attributes
    constexpr uint16_t kernel_code_selector{0x08};
    constexpr uint8_t interrupt_gate_attributes{0x8E};

    using exception_handler_ptr = void (*)() noexcept;

    struct exception_descriptor
    {
        exception_handler_ptr stub;
        const char* name;
        const char* mnemonic;
        uint8_t vector;
    };

    kernel::logger* g_exception_logger{nullptr};

    [[noreturn]] void halt_forever() noexcept
    {
        for(;;) asm volatile("cli; hlt");
    }

    #define VECTOR_TO_STRING(expression)
    [[noreturn]] void handle_exception(const char* name, const char* mnemonic, uint8_t vector) noexcept
    {
        if(!g_exception_logger) halt_forever();
        g_exception_logger->error() << "CPU exception: " << name << " (" << mnemonic << " vector " << vector << ")\n" ;
        
        g_exception_logger->panic("Unhandled CPU exception");
    }
}



// Stub Declarations
extern "C" void invalid_opcode_stub() noexcept;
extern "C" void divide_error_stub() noexcept;

// Opcode Exception Handler
constexpr exception_descriptor opcode_error{invalid_opcode_stub, "Invalid Opcode", "#UD", invalid_opcode_vector};
extern "C" [[noreturn]] void invalid_opcode_exception_handler() noexcept
{
    handle_exception(opcode_error.name, opcode_error.mnemonic, opcode_error.vector);
}

// Divide Error Handler
constexpr exception_descriptor divide_error{divide_error_stub, "Divide Exception", "#DE", divide_error_vector};
extern "C" [[noreturn]] void divide_error_exception_handler() noexcept
{
    handle_exception(divide_error.name, divide_error.mnemonic, divide_error.vector);
}

namespace kernel
{
    void set_exception_logger(logger* log) noexcept { g_exception_logger = log; }

    void initialize_exceptions() noexcept
    {
        initialize_idt();
        set_interrupt_gate(opcode_error.vector, reinterpret_cast<uint32_t>(opcode_error.stub), kernel_code_selector, interrupt_gate_attributes);
        set_interrupt_gate(divide_error.vector, reinterpret_cast<uint32_t>(divide_error.stub), kernel_code_selector, interrupt_gate_attributes);
        load_idt();
    }
}