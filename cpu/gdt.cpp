#include "gdt.h"

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
    
    constexpr size_t gdt_size{static_cast<size_t>(0x03)};
    alignas(8) constexpr uint64_t gdt_table[gdt_size] =
    {
        static_cast<uint64_t>(0x00),
        get_segment(0x00, 0x000FFFFF, code_access_byte, 0x0C),
        get_segment(0x00, 0x000FFFFF, data_access_byte, 0x0C)
    };

    static_assert(get_segment(0x00, 0x000FFFFF, code_access_byte, 0x0C) == 0x00CF9B000000FFFF);
    static_assert(get_segment(0x00, 0x000FFFFF, data_access_byte, 0x0C) == 0x00CF93000000FFFF);
}



namespace cpu::gdt
{
    void initialize() noexcept
    {

    }
}