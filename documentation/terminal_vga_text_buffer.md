# `terminal_vga_text_buffer.cpp` — Documentation

## File Purpose

Implements the logic of the **VGA text-mode buffer**: filling, copying (for scrolling), placing characters, and moving the cursor within the 80×25 VGA text grid. The centerpiece of this file is the **SIMD dispatch table**: the same operation (fill or copy) can be performed by either a scalar fallback, SSE2 (128-bit), or AVX2 (256-bit) implementation — and the choice is made dynamically at runtime, depending on what the CPU actually supports.

## Includes

- `terminal_vga_text_buffer.h`: declares the `vga_text_buffer` class, its private helper methods (`begin_32`, `cell_32`, `make_entry`, etc.), and the dimension constants (`vga_width = 80`, `vga_height = 25`).
- `vga/vga_hardware_cursor/terminal_vga_hardware_cursor.h`: for updating the hardware scroll register (`set_display_start`).
- `cpu/features.h`: for `cpu::features::get()`, the index telling which SIMD level the current CPU supports.
- `<immintrin.h>`: the SSE2/AVX2 intrinsics (`_mm_set1_epi32`, `_mm256_store_si256`, etc.).

## The three "fill" implementations

All share the same signature: `void(volatile uint32_t* dst, uint16_t bytes, const uint32_t entry) [[gnu::regparm(3)]]`. `entry` is a 32-bit value containing **two** VGA text cells (character + color) packed together, so that each 32-bit write fills two consecutive screen positions at once.

### `use_sse_2` — `[[gnu::target("sse2")]]`

Converts `entry` into a 128-bit SSE register (`__m128i`) with four repetitions of the same 32-bit value (`_mm_set1_epi32`), i.e. 8 VGA cells per write. `bytes >>= 4` converts the byte length into a count of 16-byte (128-bit) writes. Writes with `_mm_store_si128` — an **aligned** write, requiring the destination pointer to be a multiple of 16 bytes.

### `use_avx_2` — `[[gnu::target("avx2")]]`

Same logic but with 256-bit registers (`__m256i`, `_mm256_set1_epi32`, `_mm256_store_si256`), i.e. 16 VGA cells per write. `bytes >>= 5` corresponds to 32-byte (256-bit) writes.

### `fallback_fill`

A simple scalar loop (`bytes >>= 2`, i.e. 4-byte writes, writing one `uint32_t` at a time) — used when the CPU supports neither SSE2 nor AVX2 (rare on modern hardware, but necessary for correctness/portability).

### Why `[[gnu::target("...")]]` exists

Because the project-wide compile flags **don't** include `-mavx2` globally (that would cause VEX-encoded instructions to be emitted before the AVX hardware is initialized at boot, leading to a `#UD` fault), `[[gnu::target("avx2")]]` allows **these specific functions** to be compiled with AVX2 instructions, without affecting the rest of the kernel's code. The same applies to the `sse2` target, even though SSE2 is baseline on every x86-64/i686 CPU with an FPU; it's kept explicit here for clarity and consistency.

## `g_dispatch` — The fill dispatch table

```cpp
using fill_fn = void(*)(volatile uint32_t*, uint16_t, const uint32_t) [[gnu::regparm(3)]];
struct fill_functions { fill_fn entries[3]; constexpr fill_functions(): entries{fallback_fill, use_sse_2, use_avx_2} {} };
constexpr fill_functions g_dispatch{};
```

An array of function pointers, populated at **compile time** (`constexpr`), in the order `{fallback, sse2, avx2}`. `cpu::features::get()` returns a 0/1/2 index matching exactly this order, so choosing the SIMD implementation reduces to a **single array lookup (O(1))** instead of a chain of `if`/`switch` statements — avoiding branch misprediction on every call.

## The three "copy" implementations

Same philosophy as fill, but for copying data from a source to a destination (used during scrolling):

- **`use_sse_2_copy`** / **`use_avx_2_copy`**: `_mm_load_si128`/`_mm256_load_si256` from the source, followed by `_mm_store_si128`/`_mm256_store_si256` to the destination. Both return the **updated destination pointer** (`destination`) at the end, so the caller knows where the write ended.
- **`fallback_copy`**: a scalar loop, one `uint32_t` at a time.

The corresponding dispatch table is `g_dispatch_cpy` (`fill_copy_function`), with the same `{fallback, sse2, avx2}` structure.

## Public methods of `vga_text_buffer` (namespace `terminal`)

### `reset()`

Called when the "viewport" of the circular buffer reaches the maximum allowed `base_row` (`base_row_max = 179`). It copies the last `vga_height - 1` lines back to the **start** of the physical VGA memory (via `g_dispatch_cpy`) and resets `base_row` to `1`. This implements the **double-buffer ring scheme**: instead of moving data on every line (scrolling), the buffer simply advances a base pointer (`base_row`) within a larger virtual region, and only when that region's edge is reached does it copy the visible portion back to the start — a rare, "expensive" operation instead of one happening on every new line. After copying, it clears the new last line (`g_dispatch.entries[idx]`).

### `clear()`

Fills the **entire** buffer (`length` bytes) with the default blank cell (space character, `default_color`), resets `base_row`, `row`, `column`, and resets the hardware scroll register (`vga_hardware_cursor::set_display_start(0)`). Used by the shell's `clear` command and during initialization.

### `clear_row()` — `[[gnu::regparm(1)]]`

Fills only a single row (`vga_width << 1` bytes) at position `cell_32()` — used when a new line is added at the bottom of the visible window, without needing a full `reset()`.

### `put(c)` — `[[gnu::regparm(2)]]`

Writes a character at the current cursor position (`make_entry(c, active_color)`) and advances the position with `move_forward()`.

### `remove_last_char()` — `[[gnu::regparm(1)]]`

Steps back with `move_backwards()` and writes a blank at the new position — implements backspace.

### `move_forward()` — `[[gnu::regparm(1)]]`

Advances `column`, with **branchless "carry" logic**:

```cpp
++column;
bool overflowed{column == vga_width};
column -= overflowed * vga_width;
row += overflowed;

overflowed = (row == vga_height);
base_row += overflowed;
row -= overflowed;
```

Instead of nested `if` statements, the overflow from column into row, and from row into "new line at the bottom of the screen", is computed via boolean → arithmetic conversion (`true`/`false` → `1`/`0`) multiplied by the overflow step. If a row overflow occurs, it checks whether a full `reset()` is needed (reached the edge of the circular buffer) or a simple `clear_row()` suffices, and updates the hardware scroll register so the screen visually "scrolls".

### `move_backwards()` — `[[gnu::regparm(1)]]`

The symmetric counterpart of `move_forward`, with the same branchless philosophy, for moving backward (backspace at start of line, unwinding into a previous line).

### `move_to_next_line()` — `[[gnu::regparm(1)]]`

Used for `'\n'`: resets the column and advances the row, with the same overflow/reset/clear_row logic as `move_forward`.

### `move_cursor_left_n(count)` / `move_cursor_right_n(count)` — `[[gnu::regparm(2)]]`

Convert the current position into an "absolute" position within the visible screen (`row * vga_width + column`), subtract/add `count`, and recompute `row`/`column` via integer division/remainder arithmetic (`/` and remainder subtraction), updating `base_row` appropriately if the row changed. Used by `terminal::input` for left/right arrow keys and Home/End.

## Design notes

- The **SIMD dispatch index** (`cpu::features::get()`) is computed once during boot (CPU feature detection) and reused on every fill/copy call — CPUID is never re-checked on every character.
- The choice of `volatile uint32_t*` for the pointers reflects the fact that VGA memory (`0xB8000`) is memory-mapped I/O; `volatile` prevents the compiler from eliminating (optimizing away) or reordering writes that theoretically "have no effect" on the program, when in reality they have a visible effect on the screen.
- All geometric transitions (row/column overflow) are deliberately branchless, in line with the project's principle that hot-path kernel code should avoid `if`-based control flow wherever it can be avoided.
