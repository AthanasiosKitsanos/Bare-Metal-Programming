#include "terminal_vga_text_buffer.h"
#include "vga/vga_hardware_cursor/terminal_vga_hardware_cursor.h"
#include "cpu_features/cpu_features.h"
#include <immintrin.h>

namespace
{
    constexpr uint8_t use_sse2_count{16};
    constexpr uint8_t use_avx_2_count{32};
    constexpr uint8_t fallback_fill_count{4};

    constexpr uint16_t b{(3840 >> 5)};
    [[gnu::always_inline]]
    inline void use_sse_2(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) noexcept
    {
        bytes >>= 4;
        __m128i blank{_mm_set1_epi32(static_cast<int>(entry))};
        __m128i* ptr{reinterpret_cast<__m128i*>(const_cast<uint32_t*>(dst))};
        const __m128i* const end{ptr + bytes};
        while(ptr < end)
        {
            _mm_store_si128(ptr, blank);
            ++ptr;
        }
    }

    [[gnu::always_inline]]
    inline void use_avx_2(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) noexcept
    {
        bytes >>= 5;
        __m256i blank{_mm256_set1_epi32(static_cast<int>(entry))};
        __m256i* ptr{reinterpret_cast<__m256i*>(const_cast<uint32_t*>(dst))};
        const __m256i* const end{ptr + bytes};
        while(ptr < end)
        {
            _mm256_store_si256(ptr, blank);
            ++ptr;
        }   
    }

    [[gnu::always_inline]]
    inline void fallback_fill(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) noexcept
    {
        bytes >>= 2;
        const volatile uint32_t* const end{dst + bytes};
        while(dst < end)
        {
            *dst = entry;
            ++dst;
        }
    }

    using fill_fn = void(*)(volatile uint32_t* dst, uint16_t count, const uint32_t entry);
    struct fill_functions
    {
        fill_fn entries[3];

        constexpr fill_functions(): entries{}
        {
            entries[0] = fallback_fill;
            entries[1] = use_sse_2;
            entries[2] = use_avx_2;
        }
    };
    constexpr fill_functions g_dispatch{};

    [[gnu::always_inline]]
    inline uint8_t get_index(const uint8_t has_sse2, const uint8_t has_avx) noexcept
    {
        return has_sse2 * (1 + has_avx);
    }
}

namespace terminal
{
    vga_text_buffer::vga_text_buffer() noexcept: base_row{0}, row{0}, column{0}, active_color{default_color}
    {}

    void vga_text_buffer::reset() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry)) << 16 | entry};
        constexpr uint16_t bytes{(vga_height - 1) * vga_width << 1};

        const cpu::features* f{cpu::features::get()};
        const uint8_t idx{get_index((f->simd_flags >> cpu::sse_2) & 1, (f->simd_flags >> cpu::avx_2) & 1)};

        constexpr uint8_t dest_width{vga_width >> 1};
        volatile uint32_t* destination{begin_32() + dest_width};

        // Need to find a way to use the g_fill_dispatch
        constexpr uint16_t end{(length >> 1)};
        const volatile uint32_t* buffer_end(begin_32() + end);

        for(volatile uint32_t* source{begin_32() + (base_row * dest_width)}; source < buffer_end; ++source)
        {
            *destination = *source;
            ++destination;
        }

        constexpr uint8_t vga_height_m1{vga_height - 1};
        base_row = 1;
        row = vga_height_m1;

        buffer_end = destination + dest_width;
        for(; destination < buffer_end; ++destination)
        {
            *destination = entry_32;
        }
    }

    void vga_text_buffer::clear() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};
        constexpr uint16_t bytes{length << 1};

        const cpu::features* f{cpu::features::get()};
        const uint8_t idx{get_index((f->simd_flags >> cpu::sse_2) & 1, (f->simd_flags >> cpu::avx_2) & 1)};
        g_dispatch.entries[idx](begin_32(), bytes, entry_32);

        base_row = 0;
        column = 0;
        row = 0;
    }

    void vga_text_buffer::clear_row() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};
        constexpr uint8_t bytes{vga_width << 1};

        const cpu::features* f{cpu::features::get()};
        const uint8_t idx{get_index((f->simd_flags >> cpu::sse_2) & 1, (f->simd_flags >> cpu::avx_2) & 1)};
        g_dispatch.entries[idx](cell_32(), bytes, entry_32);
    }

    void vga_text_buffer::put(char c) noexcept
    {
        *cell() = make_entry(c, active_color);
        move_forward();
    }

    void vga_text_buffer::remove_last_char() noexcept
    {
        move_backwards();
        *cell() = make_entry(' ', active_color);
    }

    void vga_text_buffer::move_forward() noexcept
    {
        ++column;

        bool overflowed{column == vga_width};
        column -= overflowed * vga_width;
        row += overflowed;

        overflowed = (row == vga_height);
        base_row += overflowed;
        row -= overflowed * vga_height;

        if(overflowed)
        {
            if(base_row > vga_height) reset();
            else clear_row();
            vga_hardware_cursor::set_display_start(base_row * vga_width);
        }
    }

    void vga_text_buffer::move_backwards() noexcept
    {
        bool column_at_end{column == 0};
        column = (column - 1) + column_at_end * vga_width;

        bool row_at_end = (row == 0) * (column_at_end);
        base_row -= row_at_end;
        row = (row - column_at_end) + row_at_end * vga_height;
    }
}