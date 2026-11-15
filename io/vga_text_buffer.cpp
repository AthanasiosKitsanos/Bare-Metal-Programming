#include "vga_text_buffer.h"
#include "hardware_cursor.h"
#include "cpu/features.h"
#include <immintrin.h>
#include "tools/builtin.h"

namespace
{
    [[gnu::target("sse2")]] [[gnu::regparm(3)]]
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

    [[gnu::target("avx2")]] [[gnu::regparm(3)]]
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

    [[gnu::regparm(3)]]
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


    using fill_fn = void(*)(volatile uint32_t* dst, uint16_t count, const uint32_t entry) noexcept [[gnu::regparm(3)]];
    struct fill_functions
    {
        fill_fn entries[3];

        constexpr fill_functions(): entries{fallback_fill, use_sse_2, use_avx_2}
        {}
    };
    constexpr fill_functions g_dispatch{};

    [[gnu::target("sse2")]] [[gnu::regparm(3)]]
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

    [[gnu::target("avx2")]] [[gnu::regparm(3)]]
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

    [[gnu::regparm(3)]]
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

    using fill_copy_fn = volatile uint32_t*(*)(const volatile uint32_t* source, volatile uint32_t* dst, uint16_t count) noexcept [[gnu::regparm(3)]];
    struct fill_copy_function
    {
        fill_copy_fn entries[3];

        constexpr fill_copy_function(): entries{fallback_copy, use_sse_2_copy, use_avx_2_copy}
        {}
    };
    constexpr fill_copy_function g_dispatch_cpy{};
}

namespace io
{
    [[gnu::noinline]] [[gnu::regparm(1)]] [[gnu::cold]]
    uint32_t vga_text_buffer::reset(state_t l_state) noexcept
    {
        static_assert(__has_unique_object_representations(state_t), "state_t reordered or changed, reset it to the previous member and order state");
        constexpr uint16_t copy_bytes{(vga_height - 1) * vga_width << 1};
        uint32_t s{builtin::convert_type<uint32_t>(l_state)};

        const volatile uint32_t* source{begin_32() + ((static_cast<uint8_t>(s >> 16) * vga_width) >> 1)};
        constexpr uint16_t dst_width{vga_width >> 1};
        volatile uint32_t* destination{begin_32() + dst_width};

        uint8_t idx{cpu::features::get()};
        destination = g_dispatch_cpy.entries[idx](source, destination, copy_bytes);

        constexpr uint8_t vga_height_m1{vga_height - 1};
        constexpr uint32_t values{static_cast<uint32_t>((1 << 16) | (vga_height_m1 << 8))};
        constexpr uint32_t mask{0xFF0000FF};
        s = static_cast<uint32_t>((s & mask) | values);

        constexpr uint8_t remaining_bytes{vga_width << 1};
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16 | entry)};
        g_dispatch.entries[idx](destination, remaining_bytes, entry_32);
        return s;
    }

    void vga_text_buffer::clear() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};
        constexpr uint16_t bytes{length};

        g_dispatch.entries[cpu::features::get()](begin_32(), bytes, entry_32);

        state.base_row = 0;
        state.column = 0;
        state.row = 0;

        hardware_cursor::set_display_start(0);
    }

    void vga_text_buffer::clear_row() noexcept
    {
        constexpr uint16_t entry{make_entry(' ', default_color)};
        constexpr uint32_t entry_32{(static_cast<uint32_t>(entry) << 16) | entry};
        constexpr uint8_t bytes{vga_width << 1};

        g_dispatch.entries[cpu::features::get()](cell_32(), bytes, entry_32);
    }

    [[gnu::regparm(1)]]
    void vga_text_buffer::put(char c) noexcept
    {
        *cell() = make_entry(c, state.active_color);
        move_forward();
    }

    void vga_text_buffer::remove_last_char() noexcept
    {
        move_backwards();
        *cell() = make_entry(' ', state.active_color);
    }

    void vga_text_buffer::move_forward() noexcept
    {
        state_t state_snapshot{state};
        ++state_snapshot.column;

        bool overflowed{state.column == vga_width};
        state_snapshot.column -= overflowed * vga_width;
        state_snapshot.row += overflowed;

        overflowed = (state_snapshot.row == vga_height);
        state_snapshot.base_row += overflowed;
        state_snapshot.row -= overflowed;

        if(overflowed)
        {
            if(state_snapshot.base_row > base_row_max) state_snapshot = builtin::convert_type<state_t>(reset(state_snapshot));
            else clear_row();
            hardware_cursor::set_display_start(state_snapshot.base_row * vga_width);
        }
        state = state_snapshot;
    }

    void vga_text_buffer::move_backwards() noexcept
    {
        bool column_at_end{state.column == 0};
        state.column = (state.column - 1) + column_at_end * vga_width;

        bool row_at_end{static_cast<bool>((state.row == 0) & (column_at_end))};
        state.base_row -= row_at_end;
        state.row = (state.row - column_at_end) + row_at_end * vga_height;
    }

    vga_text_buffer::state_t vga_text_buffer::move_to_next_line() noexcept
    {
        state_t state_snapshot{state};
        state_snapshot.column = 0;
        ++state_snapshot.row;
        bool overflowed{(state_snapshot.row == vga_height)};
        state_snapshot.base_row += overflowed;
        state_snapshot.row -= overflowed;

        if(overflowed)
        {
            if(state.base_row > base_row_max) state_snapshot = builtin::convert_type<state_t>(reset(state_snapshot));
            else clear_row();
            hardware_cursor::set_display_start(state_snapshot.base_row * vga_width);
        }
        return state_snapshot;
    }

    [[gnu::regparm(1)]]
    void vga_text_buffer::move_cursor_left_n(const uint8_t count) noexcept
    {
        int16_t abs_pos{static_cast<int16_t>(state.row * vga_width + state.column - count)};
        const uint8_t new_row{static_cast<uint8_t>(abs_pos / vga_width)};
        const uint8_t new_column{static_cast<uint8_t>(abs_pos - new_row * vga_width)};

        state.base_row -= static_cast<uint8_t>(state.row - new_row);
        state.row = new_row;
        state.column = new_column;
    }

    [[gnu::regparm(1)]]
    void vga_text_buffer::move_cursor_right_n(const uint8_t count) noexcept
    {
        int16_t abs_pos{static_cast<int16_t>(state.row * vga_width + state.column + count)};
        const uint8_t new_row{static_cast<uint8_t>(abs_pos / vga_width)};
        const uint8_t new_column{static_cast<uint8_t>(abs_pos - new_row * vga_width)};

        state.base_row -= static_cast<uint8_t>(state.row - new_row);
        state.row = new_row;
        state.column = new_column;
    }
}
