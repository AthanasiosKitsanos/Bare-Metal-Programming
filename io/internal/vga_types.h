#pragma once

#include <stdint.h>
#include <stddef.h>

namespace io::types_t
{
    template<size_t N>
    struct nothing{};

    template<bool B, typename T, typename U>
    struct pick
    {
        using type = T;
    };

    template<typename T, typename U>
    struct pick<false, T, U>
    {
        using type = U;
    };

    constexpr bool wide{sizeof(uintptr_t) == 8};
    constexpr uint32_t align_32{4};
    constexpr uint64_t align_64{8
    };
    // If compiled with a 64-bit targer, the struct will have a size of 8 bytes
    // and an alignment of 8, else it gets a size of 4 and alignment of 4
    // This is made in such way to load the struct inside a register.
    // In case of 8 bytes in size, never change the bits 48 to 63.
    struct alignas(align_32) state_32
    {
        uint8_t column;
        uint8_t row;
        uint8_t base_row;
        uint8_t active_color;
    };

    struct alignas(align_64) state_64
    {
        uint16_t index;
        uint8_t column;
        uint8_t row;
        uint8_t base_row;
        uint8_t active_color;
        uint16_t reserved;
    };

    using state_t = pick<wide, state_64, state_32>::type;

    static_assert((sizeof(state_32) == 4), "Keep state_t size at 4 or 8 bytes.");
    static_assert((alignof(state_32) == align_32), "Keep state_t aligned at 4 or 8.");
    static_assert(__has_unique_object_representations(state_32), "Introduced padding. Remove it.");

    static_assert((sizeof(state_64) == 8), "Keep state_t size at 4 or 8 bytes.");
    static_assert((alignof(state_64) == align_64), "Keep state_t aligned at 4 or 8.");
    static_assert(__has_unique_object_representations(state_32), "Introduced padding. Remove it.");

    template<size_t T>
    struct rules;

    template<>
    struct rules<4>
    {
        static constexpr uint32_t step{0x01u};
        static constexpr uint32_t inc_row{0x100u};
        static constexpr uint32_t inc_base_row{0x10000u};
        static constexpr uint32_t reset_column{~0xFFu};
        static constexpr uint32_t inc_base_row_dec_row{inc_base_row - inc_row};
        static constexpr uint32_t keep_color_n_reset{0xFF000000u};
        static constexpr uint8_t isolate_color{16};
        static constexpr uint8_t isolate_column{0};
        static constexpr uint8_t isolate_row{8};
        static constexpr uint8_t isolate_base_row{16};
    };

    template<>
    struct rules<8>
    {
        static constexpr uint64_t step{0x010001ull};
        static constexpr uint64_t inc_row{0x1000000ull};
        static constexpr uint64_t inc_base_row{0x0000000100000000ull};
        static constexpr uint64_t reset_column{~0xFF0000ull};
        static constexpr uint64_t inc_base_row_dec_row{inc_base_row - inc_row};
        static constexpr uint64_t keep_color_n_reset{0xFFFFFF0000000000ull};
        static constexpr uint8_t isolate_color{32};
        static constexpr uint8_t isolate_column{16};
        static constexpr uint8_t isolate_row{24};
        static constexpr uint8_t isolate_base_row{32};
    };

}

using layout = io::types_t::rules<sizeof(uintptr_t)>;
using vga_state_packed = io::types_t::pick<io::types_t::wide, uint64_t, uint32_t>::type;
