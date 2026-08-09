# `terminal_output.cpp` — Documentation

## File Purpose

Implements the `terminal::output` class: a minimal stream-style output type (modeled after `std::cout`, with overloaded `operator<<`) built on top of `vga_text_buffer`. Converts numeric values, characters, pointers, and booleans to text and writes them to the screen, with support for decimal (`dec`) or hexadecimal (`hex`) representation via stream manipulators.

## Includes

- `terminal_output.h`: declares the `output` class, the static `buffer` instance (`vga_text_buffer`), and all the methods/templates.

## Hex conversion table

```cpp
constexpr char table[] = {'0', ..., '9', 'A', ..., 'F'};
inline char hex_digit(uint8_t nibble) noexcept { return *(table + nibble); }
```

Simple table lookup (LUT) for converting a 4-bit value (a nibble, 0–15) into the corresponding hex character — avoids an `if(n < 10)` check on every digit.

## Private methods ("no_sync")

The "\_no\_sync" suffix means: write to the buffer **without** syncing the hardware cursor after every character. This lets methods that write many characters (e.g. an entire number) do the sync **once at the end**, instead of after every digit — an important optimization since updating the hardware cursor requires I/O port access (`outb`/`inb`), which is expensive relative to a plain memory write to the VGA buffer.

### `new_line()` — `[[gnu::regparm(1)]]`

Calls `buffer.move_to_next_line()`.

### `write_string_no_sync(text)` — `[[gnu::regparm(2)]]`

A loop that writes each character up to `'\0'`, via `put_char_no_sync`.

### `write_pointer_no_sync(value)` — `[[gnu::regparm(2)]]`

Writes the `"0x"` prefix and then the hex digits of an address (`uintptr_t`). Uses `__builtin_clz(value)` to compute how many leading zero nibbles should be skipped, so the output doesn't have pointless leading zeros (e.g. `0x1A` instead of `0x0000001A`).

### `write_signed_N_no_sync(value)` (N = 8, 16, 32, 64 bits)

Check the sign: if negative, write `'-'` and then the **positive** value of the magnitude (computed as `0 - value` in unsigned arithmetic, so it works correctly even at the type's minimum bound, e.g. `INT32_MIN`, where the classic `-value` would cause undefined behavior due to overflow). Then call the common `write_unsigned_no_sync` template.

### `write_unsigned_no_sync<T>(value)` (template, defined in the header)

Writes the digits **from last to first** into a local array (`digits[max_digits]`), extracting each digit with `value % 10` (computed as `value - (value/10)*10`, avoiding a second division) and reducing `value` with `/= 10`. At the end it writes the array in the correct order. `max_digits` is computed at compile time based on the size of type `T` (3 for `uint8_t`, 5 for `uint16_t`, 10 for `uint32_t`, 20 for `uint64_t`) — the minimum array size guaranteed to fit the type's maximum possible value.

### `write_hex_N_no_sync(value)` (N = 8, 16, 32, 64 bits)

Same logic as `write_pointer_no_sync`, but specialized per type size, with a special case for `value == 0` (simply writes `'0'`).

### `put_char_no_sync(c)` — inline (defined in the header)

Central dispatch point for characters: interprets `'\r'` as return-to-line-start (`line_start()`) and `'\n'` as a new line (`new_line()`); every other character goes straight to `buffer.put(c)`.

## Public API

### `initialize()` (static)

Clears the buffer, enables the hardware cursor (`vga_hardware_cursor::enable()`), and syncs its position.

### Overloaded `operator<<`

For every numeric type (`uint8_t`…`int64_t`), the operator checks the current base state (`state`, `integer_base::dec` or `integer_base::hex`) and calls the appropriate private `write_*_no_sync` method, then always calls `sync_cursor()` at the end — so every **public** `<<` call leaves the hardware cursor properly synced, while the internal `_no_sync` calls inside the method don't perform repeated syncs.

- `operator<<(char)`, `operator<<(const char*)`: direct text writes.
- `operator<<(bool)`: if `bool_alpha_enabled` is set, writes `"true"`/`"false"`; otherwise writes `'0'`/`'1'` (via `'0' + value`).
- `operator<<(const void*)`: calls `write_pointer_no_sync`.
- `operator<<(output_manipulator)`: applies a "manipulator" (e.g. `terminal::hex`) by calling it as a function on the stream itself — the classic `std::cout << std::hex` pattern.

### Free manipulator functions

- **`dec(out)`** — `[[gnu::regparm(1)]]`: sets `state = integer_base::dec`.
- **`hex(out)`** — `[[gnu::regparm(1)]]`: sets `state = integer_base::hex`.
- **`bool_alpha(out)`** / **`bool_no_alpha(out)`** — `[[gnu::regparm(1)]]`: enable/disable rendering `bool` as `"true"/"false"` instead of `'0'/'1'`.

Each returns a reference (`output&`) to the same object, enabling chaining, e.g. `out << terminal::hex << value << terminal::dec`.

## Design notes

- Splitting into `_no_sync` methods plus a single final `sync_cursor()` call is a clean example of **batching** an expensive operation (I/O port write) so it doesn't happen on every character but once per "logical" write of data.
- The `output` class doesn't hold its own buffer; the static member `buffer` (`inline static vga_text_buffer`) is **shared** across all instances of `output` — every instance (e.g. the one inside `terminal::input`, or the one inside `kernel::logger`) writes to the same, single VGA screen.
- Extracting digits via `value % 10` (computed as subtraction instead of a division/modulo operation) avoids a second expensive division instruction on the processor.
