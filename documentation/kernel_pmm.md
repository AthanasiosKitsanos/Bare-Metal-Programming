# `kernel_pmm.cpp` — Documentation

## File Purpose

Implements the **Physical Memory Manager (PMM)**: a bitmap-based physical memory allocator. Each bit of the bitmap corresponds to one memory frame of 4096 bytes. Bit encoding convention: **bit = 0 means free frame, bit = 1 means occupied**.

The file provides:
- Bitmap initialization on top of the e820 memory map (`pmm_initialize`).
- Allocation/deallocation of a single frame (`pmm_allocate_frame` / `pmm_free_frame`).
- Allocation/deallocation of **contiguous** frames (`pmm_allocate_contiguous_frames` / `pmm_free_contiguous_frames`), needed for DMA buffers or structures that require physical contiguity.
- Statistics (`pmm_total_frames`, `pmm_used_frames`, `pmm_free_frames`).

## Includes

- `memory/pmm/kernel_pmm.h`: public API declarations, `frame_size = 4096`, `pmm_result`.
- `memory/e820/kernel_e820.h`: `e820_entry`, `e820_memory_map`, `e820_memory_type`.

## Internal structures (anonymous namespace)

### `bit_n_byte`

```cpp
struct bit_n_byte { size_t byte_index; uint8_t bit_index; };
```

Represents a bit position inside the bitmap, split into a byte index (`byte_index`) and a bit position within that byte (`bit_index`, 0–7). It also defines `operator<` for lexicographic comparison of two positions — used to check whether a requested address lies *below* the bitmap's lower allowed bound (`lower_limit`).

### `bitmap`

```cpp
struct bitmap
{
    uint8_t* start;
    const uint8_t* search_begin;
    const uint8_t* end;
    bit_n_byte lower_limit;
};
```

- `start`: start of the bitmap array in memory (placed right after the end of the kernel, `kernel_end`).
- `search_begin`: **optimization cursor** — the point from which the next free-frame search begins. It advances as frames get allocated, so the beginning of the bitmap is never rescanned every time. It moves backward when a frame is freed at a lower address than it currently points to.
- `end`: end of the bitmap array.
- `lower_limit`: the lowest valid bit index — protects against freeing frames that belong to the bitmap itself or to non-usable memory below it.

### Global state

```cpp
size_t g_used_frames{0};
size_t g_total_frames{0};
bitmap g_bitmap{};
```

Translation-unit-local globals, accessible only within this file — they hide the PMM's internal state from the outside world (encapsulation via anonymous namespace instead of a class).

## Helper `constexpr`/`inline` functions

- **`get_power_of_two(size)`** *(constexpr)*: computes, at compile time, the \\(\log_2\\) of a size (e.g. 4096 → 12). Used to produce `frame_size_bit_mask`, so address↔frame-index conversions are done via bit shifts (`>>`) instead of division.
- **`frame_index(address)`**: `address >> frame_size_bit_mask` — gives the frame number from a physical address.
- **`frame_address(index)`**: the inverse; `index << frame_size_bit_mask`.
- **`get_bit_n_byte(index)`**: converts a frame number into a `{byte_index, bit_index}` pair inside the bitmap, via `index >> 3` (byte) and `index & 7` (bit).
- **`is_frame_used(pair)`**: checks whether the corresponding bit is 1.
- **`set_frame_used(pair)` / `set_frame_free(pair)`**: set/clear the bit for a **single** frame and update `g_used_frames`.
- **`max(entry)`**: returns `base + length` of an e820 entry — i.e. the (exclusive) end address of the region.
- **`leading_zeros` / `trailing_zeros`**: wrappers around GCC's `__builtin_clz`/`__builtin_ctz` built-ins, specialized for `uint8_t` values (one bitmap byte).

## Finding "buried" free runs inside a byte

### `find_buried_run_packed(value)` *(constexpr)*

Takes a byte value (8 bits, where 1 = occupied) and searches, **exclusively over bits 1 through 6** (the middle bits, excluding the edges 0 and 7), for the longest contiguous run of zero bits (i.e. free frames) that is fully "buried" inside the byte — not at the edges, since the edges are already covered by the separate `trailing_zeros`/`leading_zeros` checks. It returns a packed byte: the top 4 bits are the `position`, the bottom 4 bits are the `length` of the best run, or `0` if no run of length > 1 exists.

The computation is **deliberately branchless** inside the loop, via multiplication by boolean conditions instead of `if`, even though this is a `constexpr` compile-time function (it never runs as runtime code). The code comment says so explicitly: it's done this way **purely for practice**, not because compile-time performance requires it.

### `buried_zeros_lut` (Look-Up Table)

```cpp
constexpr byte_lut buried_zeros_lut{};
```

A 256-entry table, fully populated at compile time by calling `find_buried_run_packed` for every possible byte value (0–255). At runtime, searching for a buried run inside a byte becomes a **single table lookup**, O(1), instead of a loop. A classic example of trading space for speed, appropriate for hot-path kernel code.

## Masks for bulk bit updates

- **`front_byte_mask_used(bit_pos)`** = `0xFF << bit_pos`: a mask covering from position `bit_pos` to the end of the byte (useful when a range starts inside a byte).
- **`back_byte_mask_used(bit_end_pos)`** = `0xFF >> (7 - bit_end_pos)`: a mask covering from the start of the byte to position `bit_end_pos` (useful when a range ends inside a byte).
- **`set_frames_in_byte_used` / `set_frames_in_byte_free`**: apply a mask to a bitmap byte (`|=` for allocation, `&=` for deallocation) and update `g_used_frames` by the number of frames (`frames`) that changed.
- **`mark_whole_byte_used` / `mark_whole_byte_free`**: fast path for when an entire byte (8 frames) falls fully within the requested range — write `0xFF`/`0x00` directly with no mask.
- **`front_byte_free_mask` / `back_byte_free_mask`**: the corresponding masks but for the AND logic used when freeing (bits outside the range must stay untouched, so the mask has ones outside the range and zeros inside it — the inverse logic of the allocation masks).

## Public API (namespace `kernel::memory`)

### `pmm_total_frames()`, `pmm_used_frames()`, `pmm_free_frames()`

Simple getters over the globals. `pmm_free_frames()` is computed as `total - used` rather than keeping a separate counter — avoids data duplication (which could get out of sync).

### `pmm_initialize(map, kernel_start, kernel_end)`

Marked `[[gnu::regparm(3)]]` (all three arguments pass through registers instead of the stack, reducing call overhead).

Steps:
1. **Find the highest address** referenced anywhere in the e820 map (even in non-usable regions), to determine how many bits the bitmap needs in total (`g_total_frames`).
2. **Place the bitmap** right after `kernel_end` — so the bitmap naturally "sits" in physical memory right after the kernel image, with no need for a separate allocator (chicken-and-egg problem: the PMM can't allocate memory for itself before it's initialized).
3. **Initially fill the bitmap with all bits set to 1** (everything occupied), in three passes for speed:
   - Byte-by-byte until 4-byte alignment is reached.
   - Bulk-write `0xFFFFFFFF` in 32-bit words (4 bytes at a time) for the aligned portion.
   - Byte-by-byte for any remainder at the end.

   This is a classic "head/body/tail" pattern for fast, alignment-aware memory filling, similar in spirit to the VGA buffer's SIMD dispatch functions, but here operating at the level of 32-bit words instead of SIMD registers.
4. **Record `search_begin`/`lower_limit`** at the end of the bitmap itself, so no future search or free operation can ever "hit" frames before this point (that's where the kernel + the bitmap itself live).
5. **Walk the e820 map**: for every entry of type `usable`, mark the corresponding frames as **free** (`set_frame_free`), in 4096-byte steps.
6. **Re-cover the kernel's own region**: because step 5 may have (incorrectly) freed frames that the kernel actually occupies (since e820 has no idea where the kernel was loaded inside a `usable` region), the final step walks from `kernel_start` to the end of the bitmap and re-marks (`set_frame_used`) any frame not already occupied.

### `pmm_allocate_frame()`

Searches, starting at `search_begin`, for the first byte that isn't `0xFF` (i.e. has at least one free bit). Within that byte, checks bit by bit with `is_frame_used` until it finds the first free one. Marks it used, updates `search_begin` (advancing it by one byte if the current byte just became fully occupied), and returns the frame's physical address. Returns `nullptr` if no free frame is found — the same unified failure convention used throughout the PMM/heap, instead of a separate error enum.

### `pmm_free_frame(address)`

Marked `[[gnu::regparm(1)]]`. Returns a `pmm_result` (`success`, `failed`, `lb_deny`, `hb_deny`):
- `hb_deny`: the address is beyond the total frame count.
- `lb_deny`: the address is below `lower_limit` (would corrupt the kernel or the bitmap itself).
- `failed`: the frame was already free (double-free protection).
- `success`: the frame was freed normally; additionally, if the newly-freed position is *before* the current `search_begin`, `search_begin` is stepped back (branchless, via multiplication by a boolean condition) so the next allocation finds it immediately.

### `pmm_allocate_contiguous_frames(frames)`

Marked `[[gnu::regparm(1)]]`. Searches for a **contiguous** run of `frames` free bits, walking the bitmap byte by byte from `search_begin`:
- If a byte is `0x00` (all free), the run extends by 8.
- If it's `0xFF` (all occupied), the run resets to zero (the streak breaks).
- On a mixed byte, it first checks the trailing zeros (continuation of the previous run), then looks for a "buried" run inside the byte via the `buried_zeros_lut` LUT, and finally the leading zeros (potential start of a new run continuing into the next byte).

Once enough length is found, it applies the masks (front/middle/back, as described above) to mark the whole range as occupied in O(bytes) instead of O(frames) individual bit updates. Finally, `search_begin` is updated.

### `pmm_free_contiguous_frames(address, frames)`

Marked `[[gnu::regparm(2)]]`. The "reverse" of `pmm_allocate_contiguous_frames`: converts the start address to a `bit_n_byte`, checks bounds (`hb_deny` if beyond the total, `lb_deny` if below `lower_limit`), computes the last bit (`end_byte`), and applies the same symmetric front/middle/back logic — but instead of `|=` (set) it uses `&=` with the "inverted" free masks (set on don't-care bits, clear on the bits inside the range) to zero out all bits in the range without touching bits outside it. Returns `pmm_result::zero_frames` if asked to free zero frames. Also updates `search_begin` if it needs to step back, same as `pmm_free_frame`.

## Design notes

- **No dynamic allocation** is used anywhere; the bitmap "sits" at a statically computed address in physical memory.
- All critical functions are `[[gnu::always_inline]]` within the anonymous namespace — no real function exists in the final binary for them, just inline code at the call site.
- The "one mask per byte" strategy instead of "one bit at a time" in all bulk operations (contiguous allocate/free) drastically reduces the instruction count when many contiguous frames are requested.
- The `search_begin` optimization turns the average-case search from O(n) into near-O(1) when memory isn't fragmented, since it never needs to rescan already-full bytes.
