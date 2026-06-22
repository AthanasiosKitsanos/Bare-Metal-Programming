#include "terminal_vga_text_buffer.h"
#include "vga/vga_hardware_cursor/terminal_vga_hardware_cursor.h"
#include "cpu_features/cpu_features.h"
#include <immintrin.h>

namespace
{
    [[gnu::target("sse2")]]
    void use_sse_2(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) noexcept
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

    [[gnu::target("avx2")]]
    void use_avx_2(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) noexcept
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

    void fallback_fill(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) noexcept
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

        constexpr fill_functions(): entries{fallback_fill, use_sse_2, use_avx_2}
        {}
    };
    constexpr fill_functions g_dispatch{};

    [[gnu::target("sse2")]]
    volatile uint32_t* use_sse_2_copy(const volatile uint32_t* src, volatile uint32_t* dst, uint16_t count) noexcept
    {
        count >>= 4;
        const __m128i* source{reinterpret_cast<__m128i*>(const_cast<uint32_t*>(src))};
        __m128i* destination{reinterpret_cast<__m128i*>(const_cast<uint32_t*>(dst))};
        const __m128i* const end{source + count};
        while(source < end)
        {
            _mm_store_si128(destination, _mm_load_si128(source));
            ++destination;
            ++source;
        }
        return reinterpret_cast<volatile uint32_t*>(destination);
    }

    [[gnu::target("avx2")]]
    volatile uint32_t* use_avx_2_copy(const volatile uint32_t* src, volatile uint32_t* dst, uint16_t count) noexcept
    {
        count >>= 5;
        const __m256i* source{reinterpret_cast<__m256i*>(const_cast<uint32_t*>(src))};
        __m256i * destination{reinterpret_cast<__m256i*>(const_cast<uint32_t*>(dst))};
        const __m256i* const end{source + count};
        while(source < end)
        {
            _mm256_store_si256(destination, _mm256_load_si256(source));
            ++source;
            ++destination;
        }
        return reinterpret_cast<volatile uint32_t*>(destination);
    }

    volatile uint32_t* fallback_copy(const volatile uint32_t* src, volatile uint32_t* dst, uint16_t count) noexcept
    {
        count >>= 2;
        const volatile uint32_t* const end{src + count};
        while(src < end)
        {
            *dst = *src;
            ++dst;
            ++src;
        }
        return dst;
    }

    using fill_copy_fn = volatile uint32_t*(*)(const volatile uint32_t* source, volatile uint32_t* dst, uint16_t count);
    struct fill_copy_function
    {
        fill_copy_fn entries[3];

        constexpr fill_copy_function(): entries{fallback_copy, use_sse_2_copy, use_avx_2_copy}
        {}
    };
    constexpr fill_copy_function g_dispatch_cpy{};
}

namespace terminal
{
    vga_text_buffer::vga_text_buffer() noexcept: base_row{0}, row{0}, column{0}, active_color{default_color}
    {}

    void vga_text_buffer::reset() noexcept
    {
        constexpr uint16_t copy_bytes{(vga_height - 1) * vga_width << 1};

        const volatile uint32_t* source{begin_32() + ((base_row * vga_width) >> 1)};
        constexpr uint16_t dst_width{vga_width >> 1};
        volatile uint32_t* destination{begin_32() + dst_width};

        uint8_t idx{cpu::features::get()};
        destination = g_dispatch_cpy.entries[idx](source, destination, copy_bytes);

        constexpr uint8_t vga_height_m1{vga_height - 1};
        base_row = 1;
        row = vga_height_m1;

        constexpr uint8_t remaining_bytes{vga_width << 1};
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16 | entry)};
        g_dispatch.entries[idx](destination, remaining_bytes, entry_32);
    }

    void vga_text_buffer::clear() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};

        g_dispatch.entries[cpu::features::get()](begin_32(), length, entry_32);

        base_row = 0;
        column = 0;
        row = 0;
    }

    void vga_text_buffer::clear_row() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};
        constexpr uint8_t bytes{vga_width << 1};

        g_dispatch.entries[cpu::features::get()](cell_32(), bytes, entry_32);
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

    void vga_text_buffer::move_to_next_line() noexcept
    {
        column = 0;
        ++row;
        bool overflowed{(row == vga_height)};
        base_row += overflowed;
        row -= overflowed * vga_height;

        if(overflowed)
        {
            if(base_row > vga_height) reset();
            else clear_row();
            vga_hardware_cursor::set_display_start(base_row * vga_width);
        }
    }
}