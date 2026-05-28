#pragma once

#include <stdint.h>

namespace kernel
{
    [[gnu::always_inline]]
    inline uint32_t read_eflags() noexcept
    {
        uint32_t flags{0};
        asm volatile("pushfl; popl %0" : "=r"(flags) : : "memory", "cc");
        return flags;
    }

    [[gnu::always_inline]]
    inline void disable_interrupts() noexcept { asm volatile("cli" : : : "memory", "cc"); }

    [[gnu::always_inline]]
    inline void enable_interrupts() noexcept { asm volatile("sti" : : : "memory", "cc"); }

    [[gnu::always_inline]]
    inline void enable_interrupt_and_halt() noexcept
    {
        asm volatile("sti; hlt");
    }

    class interrupt_guard
    {
        static constexpr uint32_t interrupt_flag{1u << 9};
        bool was_enabled;

        public:
            interrupt_guard() noexcept: was_enabled((read_eflags() & interrupt_flag) != 0)
            {
                disable_interrupts();
            }

            ~interrupt_guard() noexcept
            {
                if(was_enabled) enable_interrupts();
            }

            interrupt_guard(const interrupt_guard&) = delete;
            interrupt_guard& operator=(const interrupt_guard&) = delete;
    };
}