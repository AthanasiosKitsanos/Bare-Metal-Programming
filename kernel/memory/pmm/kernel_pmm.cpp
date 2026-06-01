#include "memory/pmm/kernel_pmm.h"
#include "memory/e820/kernel_e820.h"

namespace
{
    uint8_t* g_bitmap{nullptr};
    size_t g_total_frames{0};
    size_t g_used_frames{0};

    constexpr size_t get_times_powered_by_two(size_t size) noexcept
    {
        size_t power{0};
        while(size > 1)
        {
            size = (size >> 1);
            ++power;
        }
        return power;
    }

    constexpr size_t frame_size_bit_mask{get_times_powered_by_two(kernel::memory::frame_size)};

    [[gnu::always_inline]]
    inline size_t frame_index(const uintptr_t address) noexcept { return address >> frame_size_bit_mask; }

    [[gnu::always_inline]]
    inline uintptr_t frame_address(const size_t index) noexcept { return index << frame_size_bit_mask; }

    constexpr uint8_t bit_size_byte{8};
    constexpr uint8_t bit_size_byte_mask{get_times_powered_by_two(bit_size_byte)};
    constexpr uint8_t bit_mask{bit_size_byte - 1};
    
    struct bit_n_byte
    {
        uint8_t byte;
        uint8_t bit;
    };

    [[gnu::always_inline]]
    inline bit_n_byte get_bit_n_byte(const size_t index) noexcept
    {
        return {(index >> bit_size_byte_mask), (index & bit_mask)};
    }

    [[gnu::always_inline]]
    void set_frame_used(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        *(g_bitmap + pair.byte) |= (1 << pair.bit);
    }

    void set_frame_free(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        *(g_bitmap + pair.byte) &= ~(1 << pair.bit);
    }

    bool is_frame_used(const size_t index) noexcept
    {
        const bit_n_byte pair{get_bit_n_byte(index)};
        return (*(g_bitmap + pair.byte) & (1 << pair.bit)) != 0;
    }
}

namespace kernel::memory
{

}