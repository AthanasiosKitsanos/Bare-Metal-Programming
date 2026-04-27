#include "kernel_idt.h"
#include "kernel_logger.h"
#include "kernel_exceptions.h"
#include "kernel_interrupt_frame.h"

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
        uint8_t vector;
        exception_handler_ptr stub;
        const char* name;
        const char* mnemonic;
    };

    kernel::logger* g_exception_logger{nullptr};

    [[noreturn]] void halt_forever() noexcept
    {
        for(;;) asm volatile("cli; hlt");
    }

    [[noreturn]] void handle_exception(const char* name, const char* mnemonic, const kernel::interrupt_frame* frame) noexcept
    {
        if(!g_exception_logger) halt_forever();
        g_exception_logger->error() << "CPU exception: " << name << ' ' << mnemonic
        << kernel::hex << "\nEIP:" << frame->eip << "\nEFLAGS:" << frame->eflags << "\nError Code:" << frame->error_code
        << "\nEAX:" << frame->eax << "     ECX:" << frame->ecx << "\nEDX:" << frame->edx << " EBX:" << frame->ebx
        << "\nESP:" << frame->esp << " EBP:" << frame->ebp << "\nESI:" << frame->esi << "  EDI:" << frame->edi
        << "\nVector:" << frame->vector << '\n';
        g_exception_logger->panic("Unhandled CPU exception");
    }

    inline void __attribute__((always_inline)) install_exception(const exception_descriptor& ex) noexcept
    {
        kernel::set_interrupt_gate(ex.vector, reinterpret_cast<uint32_t>(ex.stub), kernel_code_selector, interrupt_gate_attributes);
    }

    extern "C" void invalid_opcode_stub() noexcept;
    extern "C" void divide_error_stub() noexcept;

    constexpr exception_descriptor opcode_error{invalid_opcode_vector, invalid_opcode_stub, "Invalid Opcode", "#UD"};
    constexpr exception_descriptor divide_error{divide_error_vector, divide_error_stub, "Divide Error", "#DE"};

    // Opcode Exception Handler
    [[noreturn]] void invalid_opcode_exception_handler(kernel::interrupt_frame* frame) noexcept
    {
        handle_exception(opcode_error.name, opcode_error.mnemonic, frame);
    }

    // Divide Error Handler
    [[noreturn]] void divide_error_exception_handler(kernel::interrupt_frame* frame) noexcept
    {
        handle_exception(divide_error.name, divide_error.mnemonic, frame);
    }
}

extern "C" void interrupt_dispatcher(kernel::interrupt_frame* frame) noexcept
{
    switch(frame->vector)
    {
        case 0:
            divide_error_exception_handler(frame);
        case 6:
            invalid_opcode_exception_handler(frame);
        default:
            halt_forever();
    }
}

namespace kernel
{
    void set_exception_logger(logger* log) noexcept { g_exception_logger = log; }

    void initialize_exceptions() noexcept
    {
        initialize_idt();
        install_exception(opcode_error);
        install_exception(divide_error);
        load_idt();
    }
}