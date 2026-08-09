# `kernel_e820.cpp` — Documentation

## File Purpose

Provides access to the **BIOS e820 memory map**, which has already been collected by the boot loader/boot code (in real mode, before the switch to protected mode) and stored at well-known, fixed addresses in low memory. This information is essential for the PMM to know which regions of physical memory are safe to use.

## Includes

- `kernel_e820.h`: declares the `e820_entry`, `e820_memory_map`, `e820_memory_type` types.

## Address constants (anonymous namespace)

```cpp
constexpr uintptr_t e820_count_address{0x500};
constexpr uintptr_t e820_entries_address{0x502};
```

These addresses are a **convention** between the boot loader/boot stage (in assembly, running in real mode and calling BIOS interrupt `INT 0x15, EAX=0xE820` to collect the map) and the kernel: the boot code writes the entry count at address `0x500` (as a `uint16_t`), and starts writing the actual entries (`e820_entry`) right after, at `0x502`. The area around `0x500` is typically chosen because it sits in a "safe" region of low memory, outside the BIOS Data Area/IVT that might otherwise still be actively in use.

## `get_e820_memory_map()`

```cpp
const uint16_t count{*reinterpret_cast<volatile const uint16_t*>(e820_count_address)};
const e820_entry* entries{reinterpret_cast<const e820_entry*>(e820_entries_address)};
return {entries, count};
```

Reads the entry count from the fixed address (with `volatile`, since this data was essentially written by a **different** stage of execution — the boot code — rather than by normal C++ program flow that the compiler could otherwise "rely on" for optimization purposes), and constructs a pointer to the start of the entries array. Returns both of these packaged into an `e820_memory_map` (`{entries, count}`), with no data copying at all — a simple "view" over data that already exists in low memory.

## Design notes

- The file performs **no validation** whatsoever of the contents — it fully trusts whatever the boot code wrote. This is acceptable here because the same person/project controls both ends of the convention (boot stage and kernel); a more defensive design could add a sanity check on `count` before it's used.
- The `e820_entry` structure is marked `[[gnu::packed]]` in the header (exactly 20 bytes in size, with no padding) — this is **mandatory** here, since the layout must match, byte for byte, the format produced by the BIOS `0xE820` interrupt, which knows nothing about C++ alignment rules.
- Returning by value (`return {entries, count};`, i.e. copying a small pointer+count structure) instead of a pointer to a global structure is a clean, simple interface — the caller gets an independent "snapshot" of the two values.
