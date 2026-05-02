#include "kernel_idt.h"
#include "kernel_logger.h"
#include "kernel_exceptions.h"
#include "kernel_interrupt_frame.h"
#include "kernel_pic.h"
#include "kernel_timer.h"
#include <stdint.h>

#define CPU_INTERRUPT_LIST  \
    X(0, divide_error, "Divide Error", "#DE")   \
    X(6, invalid_opcode, "Invalid Opcode", "#UD")   \
    X(13, general_protection_fault, "General Protection Fault", "#GP")   \
    X(14, page_fault, "Page Fault", "#PF")  \

#define HARDWARE_INTERRUPT_LIST \
    X(32, timer_interrupt, "Timer Interrupt", "IRQ0")

namespace
{
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

    #define X(vector, name, title, mnemonic) extern "C" void isr_##vector() noexcept;
        
    CPU_INTERRUPT_LIST
    #undef X

    #define X(vector, name, title, mnemonic) extern "C" void irq_##vector() noexcept;
        
    HARDWARE_INTERRUPT_LIST
    #undef X

    #define X(vector, name, title, mnemonic)    \
        constexpr exception_descriptor name##_desc{vector, isr_##vector, title, mnemonic};
    CPU_INTERRUPT_LIST
    #undef X

    #define X(vector, name, title, mnemonic)    \
        constexpr exception_descriptor name##_desc{vector, irq_##vector, title, mnemonic};
    HARDWARE_INTERRUPT_LIST
    #undef X
    

    inline void __attribute__((always_inline)) install_exception(const exception_descriptor& ex) noexcept
    {
        kernel::set_interrupt_gate(ex.vector, reinterpret_cast<uint32_t>(ex.stub), kernel_code_selector, interrupt_gate_attributes);
    }

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

    void handle_hardware_interrupt(const kernel::interrupt_frame* frame) noexcept
    {
        kernel::handle_timer_tick();
        kernel::send_eoi(frame->vector);
        return;
    }
}

extern "C" void interrupt_dispatcher(kernel::interrupt_frame* frame) noexcept
{
    switch(frame->vector)
    {
        #define X(vector, name, title, mnemonic)    \
            case vector:    \
                handle_exception(title, mnemonic, frame); \
                break;
            
        CPU_INTERRUPT_LIST
        #undef X

        #define X(vector, name, title, mnemonic)    \
            case vector:    \
                handle_hardware_interrupt(frame);   \
                break;
        
        HARDWARE_INTERRUPT_LIST
        #undef X

        default:
            halt_forever();
            break;
    }
}

namespace kernel
{
    void set_exception_logger(logger* log) noexcept { g_exception_logger = log; }

    void initialize_exceptions() noexcept
    {
        initialize_idt();

        constexpr uint8_t master_offset{32};
        constexpr uint8_t slave_offset{40};
        pic_remap(master_offset, slave_offset);

        #define X(vector, name, title, mnemonic)    \
            install_exception(name##_desc);

        CPU_INTERRUPT_LIST
        HARDWARE_INTERRUPT_LIST
        #undef X

        kernel::mask_all_except_timer();

        load_idt();
    }
}