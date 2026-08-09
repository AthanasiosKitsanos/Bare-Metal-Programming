# `kernel_heap.cpp` — Documentation

## File Purpose

Implements a **next-fit kernel heap allocator**, with **forward and backward coalescing** on free, via a doubly-linked list of free/used blocks. Provides the familiar `kmalloc` / `kfree` / `krealloc` trio — the equivalent of C's `malloc`/`free`/`realloc`, built from scratch (freestanding, no dependency on libc).

## Includes

- `memory/heap/kernel_heap.h`: declares the `block_header` structure and the public API.

## `block_header` structure (from the header, used here)

```cpp
struct alignas(8) block_header
{
    block_header* next;
    uint32_t flags;
    uint32_t size;
    block_header* prev;
    block_header* physical_prev;
};
```

Every block of memory (free or allocated) has such a header placed right before its data. Note the two **different** notions of "neighbor":
- `next` / `prev`: position within the **linked free-block list** — not necessarily adjacent in memory.
- `physical_prev`: the block that sits **physically right before** this one in memory (memory layout), regardless of whether it's in the free list. This is required for backward coalescing, since the free list alone isn't enough to find the "physical" neighbor.

## Global state (anonymous namespace)

```cpp
kernel::memory::block_header* free_list_head{nullptr};
kernel::memory::block_header* heap_end{nullptr};
kernel::memory::block_header* searching_block{nullptr};
```

- `free_list_head`: head of the free-block list.
- `heap_end`: the address right after the end of the entire heap — used to check whether a "physically next" block actually exists within the heap's bounds.
- `searching_block`: next-fit cursor — the position from which the **next** `kmalloc` search begins, similar logic to the PMM's `search_begin`.

## `heap_initialize(heap_start, heap_size)`

Initializes the entire heap as **one** single free block covering all available space (`heap_size - sizeof(block_header)` usable data bytes, since the header "eats" some space). Sets `physical_prev = nullptr` (nothing exists before it) and computes `heap_end` by adding `heap_size` to the starting address.

## `kmalloc(requested_size)` — `[[gnu::regparm(1)]]`

### Step 1: Next-fit search

```cpp
while(searching_block != nullptr)
{
    cached_size = searching_block->size;
    if((cached_size * (~(searching_block->flags) & 1)) >= requested_size) break;
    searching_block = searching_block->next;
}
```

This is a **branchless trick**: instead of `if(!is_used && size >= requested_size)`, the code multiplies the size by `(~flags) & 1`. If the block is used (`flags = 1`), `(~1) & 1 = 0`, so the product becomes `0` and the condition automatically fails without a separate `if(used)` check. If it's free (`flags = 0`), `(~0) & 1 = 1` and the real size gets compared. If no suitable block is found, it returns `nullptr` (the unified failure convention, same philosophy as the PMM).

### Step 2: Possible block split

```cpp
constexpr uint32_t split_limit{sizeof(block_header) + 8};
const uint32_t remaining{cached_size - requested_size};
if(remaining >= split_limit) { ... }
```

If the found block is significantly larger than what was requested (enough margin to fit a new `block_header` **and** at least 8 bytes of usable space — the `split_limit`), a second block (`remainder_block`) is created right after the allocated data. This new block is inserted into the free list in place of the original, and the `physical_prev` of its neighbor (`remainder_block->next`) is updated so the physical chain stays consistent.

If the margin *isn't* sufficient, the entire block is handed out (it's not worth creating a tiny free fragment — that would lead to external fragmentation with no practical benefit).

### Step 3: Mark as used and update the search cursor

```cpp
allocated->flags = 1;
const uintptr_t next_addr{reinterpret_cast<uintptr_t>(allocated->next)};
const bool is_null{next_addr == 0};
searching_block = reinterpret_cast<block_header*>(next_addr * !is_null + (reinterpret_cast<uintptr_t>(free_list_head) * is_null));
```

Another **branchless** pattern: instead of `if(allocated->next) searching_block = allocated->next; else searching_block = free_list_head;`, the code computes both possible values and picks the right one by multiplying with boolean masks (`is_null`/`!is_null`), avoiding branch mispredictions on the hot path.

Returns `allocated + 1` — i.e. the pointer right after the header, which is the actual data address seen by the caller (the classic "header before data" technique).

## `kfree(ptr)` — `[[gnu::regparm(1)]]`

1. If `ptr == nullptr`, returns immediately (safe no-op, just like standard `free`).
2. Recovers the header with `reinterpret_cast<block_header*>(ptr) - 1` (inverse of `allocated + 1` in `kmalloc`).
3. Clears the flag (`flags = 0`).
4. **Forward coalescing**: computes the physically next block (`ptr + size`). If it lies within the heap's bounds (`next < heap_end`) and is free, it's removed from the free list (fixing up the `prev`/`next` links, including special cases where it was the head of the list or the current `searching_block`) and the current block's size is **expanded** to absorb it, along with its own header's size (`sizeof(block_header) + next->size`).
5. Inserts the (possibly now larger) block at the **head** of the free list.
6. Sets `searching_block = allocated_memory`, so the next `kmalloc` starts its search right at the just-freed spot (next-fit heuristic — a block that was just freed is often a good candidate for the next allocation).

> Note: backward coalescing (using `physical_prev`) is referenced in the project's description of the next-fit algorithm with forward/backward coalescing; in this version of `kfree`, only the **forward** coalescing is explicitly implemented in the function body. The `physical_prev` field remains available in the structure and is correctly maintained on every `kmalloc`/split, ready to be used.

## `krealloc(ptr, new_size)` — `[[gnu::regparm(2)]]`

Follows the semantics of standard `realloc`:
- If `ptr == nullptr`, it's equivalent to `kmalloc(new_size)`.
- If `new_size == 0`, it's equivalent to `kfree(ptr)` and returns `nullptr`.
- If the existing block already has enough room (`size >= new_size`), it returns the **same** `ptr` with no copying at all (fast path — avoids unnecessary work).
- Otherwise, it allocates new space via `kmalloc`, copies byte by byte the smaller of the two sizes (`new_size` or the old `size`, whichever is smaller — computed branchlessly via multiplication with `is_lower`/`!is_lower`), frees the old block via `kfree`, and returns the new pointer.

## Design notes

- The next-fit scheme (as opposed to first-fit) keeps a moving search cursor, reducing the average number of comparisons when many consecutive allocations happen.
- The use of branchless arithmetic in the hot paths (`kmalloc`, part of `krealloc`) is consistent with the project's principle that hot-path kernel code should avoid branches to reduce branch-misprediction cost.
- The `sizeof(block_header) + 8` split limit prevents the creation of degenerate (practically useless) tiny free blocks.
- There is no alignment padding check beyond the `alignas(8)` on the `block_header` structure — this guarantees that every returned data pointer is at least 8-byte aligned, sufficient for most data types in a 32-bit environment.
