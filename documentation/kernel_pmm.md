# `kernel_pmm.cpp` — Documentation

## File Purpose

Implements the **Physical Memory Manager (PMM)**: a bitmap-based physical memory allocator. Each bit of the bitmap corresponds to one memory frame of 4096 bytes. Bit encoding convention: **bit = 0 means free frame, bit = 1 means occupied**.

The file provides:
- Bitmap initialization on top of the e820 memory map (`pmm_initialize`).
- Allocation/deallocation of a single frame (`pmm_allocate_frame` / `pmm_free_frame`).
- Allocation/deallocation of **contiguous** frames (`pmm_allocate_contiguous_frames` / `pmm_free_contiguous_frames`), needed for DMA buffers or structures that require physical contiguity. Both the free-run *search* and the bulk *marking* of a found run are runtime-dispatched across three CPU tiers (scalar GPR, SSE2, AVX2), selected once via `cpu::features::get()`.
- Statistics (`pmm_total_frames`, `pmm_used_frames`, `pmm_free_frames`).

## Includes

- `memory/pmm/kernel_pmm.h`: public API declarations, `frame_size = 4096`, `pmm_result`.
- `memory/e820/kernel_e820.h`: `e820_entry`, `e820_memory_map`, `e820_memory_type`.
- `cpu/features.h`: `cpu::features::get()` — runtime CPU capability detection, used to select the GPR/SSE2/AVX2 tier at both allocation and free time.
- `<immintrin.h>`: SSE2/AVX2 intrinsics.
- `io/output/terminal_output.h`: kernel console output (diagnostics).
- `pmm_templates.tpp` (via `#include`, mid-file, inside the same anonymous namespace): the generic fill-value templates used by the bulk marking machinery. See its own section below.

## Internal structures (anonymous namespace)

### `bit_n_byte`

```cpp
struct bit_n_byte { size_t byte_index; uint8_t bit_index; };
```

Represents a bit position inside the bitmap, split into a byte index (`byte_index`) and a bit position within that byte (`bit_index`, 0–7). Defines `operator<` for lexicographic comparison of two positions — used to check whether a requested address lies *below* the bitmap's lower allowed bound (`lower_limit`).

### `bitmap`

```cpp
struct bitmap
{
    uint8_t* start;
    const uint8_t* end;
    const uint8_t* search_begin;
    const uint8_t* search_end;
    bit_n_byte lower_limit;
};
```

- `start`: start of the bitmap array in memory (placed right after `kernel_end`). Non-`const`, since this is the only field ever written through directly.
- `end`: end of the bitmap array.
- `search_begin`: **optimization cursor** — the point from which the next free-frame search begins. Advances as frames get allocated; steps back (branchless) when a frame is freed at a lower address.
- `search_end`: the current upper bound for a search pass. Normally equal to `end`; temporarily narrowed to `search_begin` during a wrap-around retry (see `find_contiguous_frames_*` below).
- `lower_limit`: the lowest valid bit index — protects against freeing frames that belong to the bitmap itself or to non-usable memory below it.

### `allocation_run`

```cpp
struct allocation_run { bit_n_byte start_index{}; size_t length{}; };
```

Accumulator used by every scanning core (`contiguous_N_core`, `contiguous_sse2_core`, `contiguous_avx2_core_inline`): tracks the start position and current length, in frames, of the free run found so far as the bitmap is walked.

### Global state

```cpp
size_t g_used_frames{0};
size_t g_total_frames{0};
bitmap g_bitmap{};
```

Translation-unit-local globals — encapsulated via the anonymous namespace instead of a class.

## Helper `constexpr`/`inline` functions

- **`get_power_of_two(size)`** *(constexpr)*: computes, at compile time, the log₂ of a size (4096 → 12), producing `frame_size_bit_mask` so address↔frame-index conversions use bit shifts instead of division.
- **`frame_index(address)`** / **`frame_address(index)`**: address ↔ frame-number conversion via `>>`/`<<`.
- **`get_bit_n_byte(index)`**: splits a frame number into `{byte_index, bit_index}` via `index >> 3` and `index & 7`.
- **`is_frame_used(pair)`**, **`set_frame_used(pair)`**, **`set_frame_free(pair)`**: single-frame bit test/set/clear, updating `g_used_frames`.
- **`max(entry)`**: `base + length` of an e820 entry (its exclusive end address).
- **`leading_zeros<T>` / `trailing_zeros<T>`**: template wrappers around `__builtin_clz`/`__builtin_ctz`, generic over `uint8_t`/`uint16_t`/`uint32_t` via a compile-time shift correction (`leading_zeros`) so the same builtin works uniformly across widths.
- **`safe_leading_zeros` / `safe_trailing_zeros`**: `uint8_t`-only guards returning `8` for a zero input, since `__builtin_clz`/`__builtin_ctz` are undefined behavior on zero.

## Finding "buried" free runs inside a byte

### `dedicated_1_frame_lut` / `dedicated_lut`

A 255-entry compile-time table mapping a byte value to the position of its first zero bit. Used exclusively by `pmm_allocate_frame` (single-frame allocation) as an O(1) alternative to a bit-by-bit scan.

### `find_buried_run_packed(value)` *(constexpr)* / `buried_zeros_lut`

Searches bits 1–6 of a byte (excluding the edges, which are handled separately by `trailing_zeros`/`leading_zeros`) for the longest fully-buried run of zero bits. Returns a packed byte (`position << 4 | length`). Computed branchlessly inside the loop purely for practice (it is `constexpr`, never runtime code). `buried_zeros_lut` pre-computes this for all 256 byte values, turning the runtime lookup into O(1).

## Masks for single-byte partial-range updates

- **`front_byte_mask_used(bit_pos)`** = `0xFF << bit_pos`, **`back_byte_mask_used(bit_end_pos)`** = `0xFF >> (7 - bit_end_pos)`: masks for the first/last byte of an allocation range.
- **`front_byte_free_mask` / `back_byte_free_mask`**: the inverted counterparts used when freeing (AND logic).
- **`set_frames_in_byte_used` / `set_frames_in_byte_free`**: apply a mask (`|=`/`&=`) to one bitmap byte and adjust `g_used_frames`.
- **`mark_whole_byte_used` / `mark_whole_byte_free`**: fast path writing `0xFF`/`0x00` directly to a fully-covered byte. (Retained as utilities; the multi-byte middle section of a contiguous allocation/free no longer uses a byte-by-byte loop — see the bulk-fill section below.)

## Tiered SIMD free-run scanning

The search for a contiguous free run is dispatched across three tiers, each built as a chain of progressively wider cores that align the scan pointer up to the next tier's natural alignment before handing off, with a scalar fallback chain for any tail that a wider core couldn't fully consume.

### Cores: `contiguous_8/16/32_core` (+ `_inline`), `contiguous_sse2_core` (+ `_inline`), `contiguous_avx2_core_inline`

Each core walks its width (`uint8_t`/`uint16_t`/`uint32_t`/`__m128i`/`__m256i`) over `[start, end)`, extending `run->length` on an all-zero unit, resetting it on an all-ones unit, and — on a mixed unit — falling through to `trailing_zeros`/`buried_zeros_lut`/`leading_zeros` (for GPR widths) or to the next-narrower core (for SIMD widths: `contiguous_sse2_core_inline` delegates a mixed 128-bit lane to `contiguous_32_core_inline`; `contiguous_avx2_core_inline` delegates a mixed 256-bit lane to `contiguous_sse2_core_inline`). Every `_inline` core has a byte-for-byte `noinline` twin, kept manually in sync (marked `IMPORTANT: keep in sync` in comments), used only as the tail fallback after the main aligned pass.

SIMD cores compare the loaded vector against both zero and an all-ones sentinel (produced via self-compare, `_mm_cmpeq_epi32`/`_mm256_cmpeq_epi32` against a zeroed register, never `_mm_set1_epi8` — this guarantees a register-only instruction with no memory load, independent of optimization level) and pack the byte-wise comparison result via `_mm_movemask_epi8`/`_mm256_movemask_epi8`.

### `get_best_aligned_address(addr_1, addr_2)`

Branchless min of two addresses (`(addr_1 * (addr_1<=addr_2)) + (addr_2 * (addr_1>addr_2))`), used to compute, at each alignment step, whichever comes first: the next natural alignment boundary, or the actual end of the search range (so a short search range never gets over-aligned past its own end).

### `sweep_32` / `sweep_sse_2` / `sweep_avx_2`

Each is the top-level driver for its tier: it aligns the cursor up through 2→4[→16[→32]] bytes using `contiguous_N_core_inline`, hands off the bulk of the range to the tier's main core (`contiguous_32_core_inline` / `contiguous_sse2_core_inline` / `contiguous_avx2_core_inline`), and — only if the requested length still isn't satisfied — falls back through the `noinline` cores of that tier and every narrower tier, down to `contiguous_8_core`, covering the unaligned tail at the far end of the range.

### `find_contiguous_frames_32/sse2/avx2`

Calls the corresponding `sweep_*`. If the sweep reaches `g_bitmap.end` without satisfying the request, it resets the run, narrows `search_end` to the old `search_begin`, rewinds `search_begin` to `lower_limit`, and retries — a wrap-around pass over the region already scanned in previous allocations.

### `simd_alloc_lut`

```cpp
using simd_alloc_methods = void(*)(allocation_run* const, const size_t) noexcept [[gnu::regparm(2)]];
```

A `constexpr`-constructed, 3-entry function-pointer table (`gpr_flag=0`, `simd_flag=1`, `avx2_flag=2`), indexed by `cpu::features::get()` at every call to `pmm_allocate_contiguous_frames`.

## Bulk fill: `pmm_templates.tpp`

A separate file, `#include`d mid-file directly inside `kernel_pmm.cpp`'s anonymous namespace (not a standalone translation unit — it has no include guard, since it has exactly one, known include site, and no namespace of its own, since it is meant to be textually absorbed into the surrounding one). It replaces the old byte-by-byte middle loop of a contiguous allocate/free with an alignment-peeling bulk fill, generic over both the value written (all-ones vs. all-zeros) and the register width.

### `set_bits`

```cpp
enum class set_bits: uint8_t { all_zeros = 0x00, all_ones = 0x01 };
```

### `set_gpr_inline<T, Set>` / `set_gpr<T, Set>`

```cpp
template<typename T, set_bits Set>
inline void set_gpr_inline(T** start, const T* const end) noexcept;
```

`Set` is a **non-type template parameter**, not a runtime `bool` — this is what lets the fill value resolve as a genuine constant expression regardless of optimization level (a `constexpr` local computed from an ordinary function parameter would not qualify; a named parameter is never itself a constant expression). The fill value is computed branchlessly: `static_cast<T>(UINT64_MAX) * static_cast<T>(Set)` — truncating an all-bits-one 64-bit constant to `T`'s width yields the correct all-ones pattern for any width, and multiplying by `0`/`1` selects zero or all-ones with no branch. Instantiated at `T = uint8_t/uint16_t/uint32_t`; both `T` and `Set` are always given explicitly at the call site (both parameters are non-deducible or intentionally not relied upon for deduction).

### `set_sse2_inline<Set>` / `set_sse2<Set>`, `set_avx2_inline<Set>` / `set_avx2<Set>`

Each fixed to one vector width (`__m128i` / `__m256i`) with its own minimal target attribute (`[[gnu::target("sse2")]]` / `[[gnu::target("avx2")]]`). Internally, `if constexpr(Set == set_bits::all_ones)` selects a self-compare (`_mm_cmpeq_epi32`/`_mm256_cmpeq_epi32` against zero) or `_mm_setzero_si128`/`_mm256_setzero_si256`; the non-selected branch is discarded at compile time, so there is no runtime branch inside the fill loop.

**Design note — why width-specific templates instead of one generic template:** an earlier version used a single `template<typename T, set_bits Set>` covering both `__m128i` and `__m256i`, with `if constexpr(sizeof(T)==...)` picking the intrinsic family. GCC's `always_inline` requires a callee's declared `[[gnu::target(...)]]` set to be a subset of (or equal to) the caller's, checked against the *declared* set — not against which branches a given instantiation actually executes. That unified template therefore had to be declared `target("sse2","avx2")`, which — because target attributes act like whole-function `-m` flags, enabling the auto-vectorizer along with everything else — let the compiler auto-vectorize the plain scalar tail-fill loops of the `sse2`-only orchestrator into real 256-bit AVX2 instructions (confirmed via disassembly: `vpcmpeqd %ymm0`/`vmovdqu %ymm0` inside a function meant to run on SSE2-only hardware). Splitting into one template per width, each with only the target it actually needs, closed that leak; a callee needing only `{sse2}` no longer forces its `{sse2}`-only caller to widen its own declared target set.

## Bulk-fill orchestrators and dispatch

### `set_frames_used_32/sse2/avx2`, `set_frames_free_32/sse2/avx2`

Six functions, one per (tier × used-or-free), all sharing the signature `void(uint8_t*, const uint8_t* const)`. Each peels the range up through 2→4 byte alignment (word/dword) via `set_gpr_inline`, then — for the `sse2`/`avx2` tiers — further up to 16 (and, for `avx2`, 32) byte alignment via the wider inline templates, fills the aligned bulk with the tier's own inline SIMD/GPR fill, and falls back through progressively narrower `noinline` fills (`set_sse2`, then `set_gpr<uint32_t>`/`<uint16_t>`/`<uint8_t>`, tier-dependent) for whatever tail remains beyond the last fully-aligned boundary. The `_free_` variants are structurally identical to their `_used_` counterparts, differing only in passing `set_bits::all_zeros` instead of `set_bits::all_ones` to every templated call.

### `simd_set_lut<Set>`

```cpp
template<set_bits Set>
struct simd_set_lut
{
    simd_set_methods entries[set_methods_size];
    constexpr simd_set_lut(): entries{}
    {
        if constexpr(Set == set_bits::all_ones) { /* wire the three *_used_* functions */ }
        else { /* wire the three *_free_* functions */ }
    }
};

constexpr simd_set_lut<set_bits::all_ones>  set_used_lut{};
constexpr simd_set_lut<set_bits::all_zeros> set_free_lut{};
```

A `constexpr` class template, non-type-parameterized on `Set`, producing two independent dispatch tables at compile time. Declared **after** all six orchestrator functions (a template's `if constexpr` still requires every name it mentions — selected branch or not — to already be visible at the point the template itself is defined; only the branch's *evaluation*, not its *name lookup*, is deferred/discarded per instantiation).

## Public API (namespace `kernel::memory`)

### `pmm_total_frames()`, `pmm_used_frames()`, `pmm_free_frames()`

Simple getters; `pmm_free_frames()` is `total - used`, avoiding a duplicated, sync-prone counter.

### `pmm_initialize(map, kernel_end)`

1. Scans the e820 map for the highest referenced address to size the bitmap (`g_total_frames`).
2. Places the bitmap directly after `kernel_end` (no separate allocator needed for the bitmap itself).
3. Fills the whole bitmap with `0xFF` (all occupied) via a byte→dword→byte head/body/tail pass.
4. Records `search_begin`/`search_end`/`lower_limit` at the end of the bitmap.
5. Walks `usable` e820 entries, marking their frames free.
6. Re-marks as used any frame from `kernel_end` up to `lower_limit` that step 5 may have incorrectly freed (e820 has no notion of where the kernel image itself sits inside a usable region).

### `pmm_allocate_frame()`

Scans from `search_begin` for the first non-`0xFF` byte, uses `dedicated_lut` to find the first free bit within it, marks it used, advances `search_begin`, and wraps around to `lower_limit` once if the first pass reaches `end` without success. Returns `nullptr` on total exhaustion.

### `pmm_free_frame(address)`

Returns `pmm_result::hb_deny`/`lb_deny`/`failed`/`success` per the same bounds/double-free checks as before; steps `search_begin` back branchlessly if the freed frame precedes it.

### `pmm_allocate_contiguous_frames(frames)`

Dispatches the search via `simd_lut.entries[cpu::features::get()]`. If the found run is long enough: for a single-byte range, ORs a combined front∧end mask directly; for a multi-byte range, ORs the front byte's partial mask, dispatches the **middle** bytes through `set_used_lut.entries[cpu::features::get()]` (the alignment-peeling bulk fill described above, replacing the former byte-by-byte loop), then ORs the end byte's partial mask. Updates `g_used_frames` and `search_begin` once, unconditionally, after the branch (an earlier version double-incremented `g_used_frames` by also adding it inside the multi-byte branch — fixed).

### `pmm_free_contiguous_frames(address, frames)`

The mirror image: bounds-checks, computes `start_byte`/`end_byte`, ANDs the front/end partial masks, and — for a multi-byte range — dispatches the middle bytes through `set_free_lut.entries[cpu::features::get()]`. Returns `pmm_result::zero_frames` for a zero-length request.

## Design notes

- **No dynamic allocation** anywhere; the bitmap sits at a statically computed physical address.
- Both the *search* (`simd_alloc_lut`) and the *bulk fill* (`set_used_lut`/`set_free_lut`) are dispatched once per call via `cpu::features::get()`, never re-checked per byte/word.
- **`[[gnu::target(...)]]` discipline:** every function using width-specific SIMD carries the *minimal* target set it actually needs; `always_inline` requires a callee's declared target set to be a subset of (or equal to) its caller's, so a wider target on a low-level helper forces every one of its callers to widen too — verified to actually matter via disassembly (an over-widened target let the auto-vectorizer emit real AVX2 instructions inside an SSE2-only-tier function).
- **`Set` as a non-type template parameter**, not a runtime `bool`: guarantees the fill value is a genuine compile-time constant inside the template body itself, independent of `-O0`/`-O3` — a named function parameter, even one that happens to receive a `constexpr` argument at the call site, is never itself a constant expression.
- The used/free dispatch tables are two separate instantiations of one `template<set_bits Set> struct simd_set_lut`, rather than two hand-duplicated structs — mirroring the same "one template, `Set`-parameterized" principle already used for the fill cores themselves.
- Verified at three independent levels beyond compile+run: **disassembly** (confirmed only legacy-encoded, non-VEX SSE2 instructions inside the SSE2-tier fill, no leaked AVX2), and **GCC call-graph info** (`-fcallgraph-info`, `.ci` files) confirming the `always_inline` templates leave no trace as separate symbols, the `noinline` fallbacks appear as small, real functions, and the used/free call graphs never cross-reference each other's `Set` instantiations.