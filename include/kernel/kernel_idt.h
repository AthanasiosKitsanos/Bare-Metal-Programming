#pragma once

#include <stdint.h>

namespace kernel
{
    struct [[gnu::packed]] idt_entry
    {
        uint16_t offset_low;
        uint16_t selector;
        uint8_t zero{0};
        uint8_t type_attributes;
        uint16_t offset_high;
    };

    struct [[gnu::packed]] idtr_descriptor
    {
        uint16_t limit;
        uint32_t base;
    };

    static_assert(sizeof(idt_entry) == 8, "idt_entry must be exactly 8 bytes");
    static_assert(sizeof(idtr_descriptor) == 6, "idtr_descriptor must be exactly 6 bytes");

    void initialize_idt() noexcept;
    void set_interrupt_gate(uint8_t vector, uint32_t handler_address, uint16_t selector, uint8_t type_attributes) noexcept;
    void load_idt() noexcept;
}