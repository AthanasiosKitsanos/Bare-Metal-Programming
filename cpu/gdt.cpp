#include "gdt.h"
#include <stdint.h>
#include <stddef.h>

namespace
{
    constexpr uint8_t code_access_byte{0x9B};
    constexpr uint8_t data_access_byte{0x93};
    
    constexpr uint64_t get_segment(const uint32_t base, const uint32_t limit, const uint8_t access, const uint8_t flags) noexcept
    {
        uint64_t value{static_cast<uint64_t>(0x00)};
        value |= static_cast<uint64_t>(base >> 24) << 56;
        value |= static_cast<uint64_t>(flags & 0x0F) << 52;
        value |= static_cast<uint64_t>(static_cast<uint8_t>(limit >> 16) & 0x0F) << 48;
        value |= static_cast<uint64_t>(access) << 40;
        value |= static_cast<uint64_t>((base & 0x00FFFFFF)) << 16;
        return value |= static_cast<uint64_t>(static_cast<uint16_t>(limit));
    }
    
    alignas(8) constexpr uint64_t gdt_table[] =
    {
        static_cast<uint64_t>(0x00),
        get_segment(0x00, 0x000FFFFF, code_access_byte, 0x0C),
        get_segment(0x00, 0x000FFFFF, data_access_byte, 0x0C)
    };

    static_assert(get_segment(0x00, 0x000FFFFF, code_access_byte, 0x0C) == 0x00CF9B000000FFFF);
    static_assert(get_segment(0x00, 0x000FFFFF, data_access_byte, 0x0C) == 0x00CF93000000FFFF);
    
    constexpr uint16_t zero_seg{static_cast<size_t>(0x00)};
    constexpr uint16_t code_seg{static_cast<size_t>(0x08)};
    constexpr uint16_t data_seg{static_cast<size_t>(0x10)};

    struct [[gnu::packed]] descriptor
    {
        uint16_t limit;
        const void* base;

        constexpr descriptor(const uint16_t l, const void* b) noexcept: limit{l}, base{b}
        {}
    };

    constexpr descriptor g_desc{static_cast<uint16_t>(sizeof(gdt_table) - 1), gdt_table};

    static_assert(sizeof(descriptor) == 6);
    static_assert(offsetof(descriptor, descriptor::base) == 2);
}

namespace cpu::gdt
{
    void initialize() noexcept
    {
        asm volatile
        (
            "lgdt %[gdtr]\n\t"
            "movw %w[data], %%ds\n\t"
            "movw %w[data], %%es\n\t"
            "movw %w[data], %%fs\n\t"
            "movw %w[data], %%gs\n\t"
            "movw %w[data], %%ss\n\t"
            "ljmp %[code], $1f\n\t"
            "1:"
            : // No ouptput
            : [gdtr] "m"(g_desc), [data] "r"(data_seg), [code] "i"(code_seg)
            : "memory", "cc"
        );
    }
}