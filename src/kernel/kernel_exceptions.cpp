#include "kernel_idt.h"
#include "kernel_logger.h"
#include "kernel_exceptions.h"
#include "kernel_interrupt_frame.h"
#include "kernel_pic.h"
#include "kernel_timer.h"
#include "keyboard.h"
#include <stdint.h>
#include "kernel_cpu_interrupts.h"
#include "kernel_hardware_interrupts.h"

namespace
{
    // Selectors and Attributes
    constexpr uint16_t interrupt_vector_count{256};
    constexpr uint16_t kernel_code_selector{0x08};
    constexpr uint8_t interrupt_gate_attributes{0x8E};
    constexpr uint8_t cpu_exception_count{32};
    constexpr uint8_t irq_base{32};
    constexpr uint8_t irq_max{47};

    using exception_handler_ptr = void (*)() noexcept;

    struct exception_descriptor
    {
        uint8_t vector;
        exception_handler_ptr stub;
        const char* name;
        const char* mnemonic;
    };

    using interrupt_handler = void (*)(kernel::interrupt_frame*) noexcept;

    interrupt_handler g_interrupt_handlers[interrupt_vector_count]{};

    kernel::logger* g_exception_logger{nullptr};

    #define X(vector, name, title, mnemonic) extern "C" void isr_##vector() noexcept;
        
    CPU_INTERRUPT_LIST
    #undef X

    #define X(vector, name_space, name, title, mnemonic) extern "C" void irq_##vector() noexcept;
        
    HARDWARE_INTERRUPT_LIST
    #undef X

    #define X(vector, name, title, mnemonic)    \
        constexpr exception_descriptor name##_desc{vector, isr_##vector, title, mnemonic};
    CPU_INTERRUPT_LIST
    #undef X

    #define X(vector, name_space, name, title, mnemonic)    \
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

    [[noreturn]] void handle_exception(const char* name, const char* mnemonic, kernel::interrupt_frame* frame) noexcept
    {
        if(!g_exception_logger) halt_forever();
        g_exception_logger->error() << "CPU exception: " << name << ' ' << mnemonic
        << kernel::hex << "\nEIP:" << frame->eip << "\nEFLAGS:" << frame->eflags << "\nError Code:" << frame->error_code
        << "\nEAX:" << frame->eax << "     ECX:" << frame->ecx << "\nEDX:" << frame->edx << " EBX:" << frame->ebx
        << "\nESP:" << frame->esp << " EBP:" << frame->ebp << "\nESI:" << frame->esi << "  EDI:" << frame->edi
        << "\nVector:" << frame->vector << '\n';
        g_exception_logger->panic("Unhandled CPU exception");
    }

    [[noreturn]] void handle_cpu_exception(kernel::interrupt_frame* frame) noexcept
    {
        switch(frame->vector)
        {
            #define X(vector, name, title, mnemonic)    \
                case vector:    \
                    handle_exception(title, mnemonic, frame);   \
                    break;
            CPU_INTERRUPT_LIST
            #undef X

            default:
                if(g_exception_logger) g_exception_logger->warning() << "No handler registered in vector " << frame->vector;
                halt_forever();
        }
    }

    void default_interrupt_handler(kernel::interrupt_frame* frame) noexcept
    {
        if(!frame) halt_forever();
        const uint32_t vector{frame->vector};
        if(g_exception_logger)
        {
            g_exception_logger->error() << "Unhandled interrupt vector: " << vector << "\nError Code: " << frame->error_code
            << "\nEIP: " << kernel::hex << frame->eip << '\n' << kernel::dec;
        }
        if(vector >= irq_base && vector <= irq_max) return;
        halt_forever();
    }

    void initialize_interrupt_handlers_table() noexcept
    {
        for(uint32_t vector{0}; vector < interrupt_vector_count; ++vector)
        {
            *(g_interrupt_handlers + vector) = default_interrupt_handler;
        }

        #define X(vector, name, title, mnemonic)    \
            *(g_interrupt_handlers + vector) = handle_cpu_exception;
        CPU_INTERRUPT_LIST
        #undef X
        
        #define X(vector, name_space, name, title, mnemonic)    \
            *(g_interrupt_handlers + vector) = name_space::handle_##name;
        HARDWARE_INTERRUPT_LIST
        #undef X

    }
}

extern "C" void interrupt_dispatcher(kernel::interrupt_frame* frame) noexcept
{
    uint32_t vector{frame->vector};
    g_interrupt_handlers[vector](frame);
    if(vector >=irq_base && vector <= irq_max) kernel::send_eoi(static_cast<uint8_t>(vector - irq_base));
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
        #undef X

        #define X(vector, name_scace, name, title, mnemonic)    \
            install_exception(name##_desc);

        HARDWARE_INTERRUPT_LIST
        #undef X
        
        initialize_interrupt_handlers_table();

        kernel::mask_all_except_timer_and_keyboard();

        load_idt();
    }
}