#include "kernel_idt.h"
#include <stddef.h>

namespace
{
    constexpr size_t total_entries{256};
    static kernel::idt_entry idt_entry_table[total_entries]; // 0 - 255
}

namespace kernel
{
    void initialize_idt() noexcept
    {
        const idt_entry* const end{idt_entry_table + total_entries};
        for(idt_entry* curr{idt_entry_table}; curr < end; ++curr)
        {
            *curr = idt_entry{};
        }
    }

    void set_interrupt_gate(uint8_t vector, uint32_t handler_address, uint16_t selector, uint8_t type_attributes) noexcept
    {
        idt_entry* const entry{idt_entry_table + vector};
        entry->offset_low = static_cast<uint16_t>(handler_address & 0xFFFFu);
        entry->selector = selector;
        entry->zero = 0;
        entry->type_attributes = type_attributes;
        entry->offset_high = static_cast<uint16_t>((handler_address >> 16) & 0xFFFFu);
    }

    void load_idt() noexcept
    {
        const idtr_descriptor descriptor
        {
            static_cast<uint16_t>(sizeof(idt_entry_table) - 1),
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(idt_entry_table))
        };

        asm volatile("lidt %0" : : "m"(descriptor) : "memory");
    }
}