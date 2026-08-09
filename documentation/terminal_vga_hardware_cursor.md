# `terminal_vga_hardware_cursor.cpp` — Documentation

## File Purpose

Driver for the control registers of the VGA card's **hardware text-mode cursor** and its **display start register**, via the classic CRTC (CRT Controller) ports. It's distinct from `vga_text_buffer` (which manages the text *data* in memory) — this file manages exclusively the *position* of the blinking cursor the user sees on screen, as well as which line is displayed first (scrolling via the CRTC start address).

## Includes

- `terminal_vga_hardware_cursor.h`: declares the `vga_hardware_cursor` class/namespace.
- `internals/terminal_io_registers.h`: `outb`/`inb` for I/O ports.

## CRTC register index constants

```cpp
constexpr uint8_t cursor_register_low{0x0F}; constexpr uint8_t cursor_register_high{0x0E};
constexpr uint8_t display_register_low{0x0D}; constexpr uint8_t display_register_high{0x0C};
```

The VGA's CRTC controller is accessed via **indexed access**: you first write the index of the register you want to the command port, then read/write its value on the data port. `0x0E`/`0x0F` are the cursor position index (high/low byte), and `0x0C`/`0x0D` are the "display start address" index — the position in VGA memory from which the visible screen begins (used to scroll the circular buffer).

## `write_register(index, value)` — `[[gnu::regparm(2)]]`

```cpp
outb(command_port, index);
outb(data_port, value);
```

The fundamental indexed I/O operation: selects the register and writes its value.

## `read_register(index)` — `[[gnu::regparm(1)]]`

Symmetric: selects the register and reads its current value.

## `enable(start, end)`

```cpp
uint8_t cursor_start{static_cast<uint8_t>((read_register(0x0A) & 0xC0) | (start & 0x1F))};
write_register(0x0A, cursor_start);
uint8_t cursor_end{static_cast<uint8_t>((read_register(0x0B) & 0xE0) | (end & 0x1F))};
write_register(0x0B, cursor_end);
```

Enables the hardware cursor and sets its **shape** (height, via start/end "scan lines" — e.g. a thin line at the bottom of the cell versus a full filled block). Uses **read-modify-write** instead of a direct write: registers `0x0A`/`0x0B` also contain other, unrelated bits (e.g. a cursor-disable bit in `0x0A`) that **must not** be disturbed — the `0xC0`/`0xE0` masks preserve those other bits untouched, while replacing only the scan-line-range bits.

## `set_position(position)` — `[[gnu::regparm(1)]]`

Writes the cursor's position (a linear index within the 80×25 buffer, `row * 80 + column`) to the two position registers, split into low and high bytes — called on every `sync_cursor()` from `terminal::output`.

## `set_display_start(position)` — `[[gnu::regparm(1)]]`

Writes the "display start" to the two display start registers — this is the command that makes the screen show a different portion of the (larger, circular) VGA buffer, implementing visual scrolling without needing to move any data in memory; called by `vga_text_buffer` whenever `base_row` changes.

## Design notes

- The explicit distinction between "cursor position" and "display start" reflects a fundamental property of VGA hardware: the cursor (where it blinks) and the scroll (what's visible) are **two independent** hardware mechanisms, and combining them correctly is the software's responsibility (here, `vga_text_buffer`, which calls both at the appropriate points).
- The use of read-modify-write in `enable()` (but **not** in `set_position`/`set_display_start`, where the whole byte is written) shows careful understanding of which registers share bits with other functions and which are exclusively dedicated to a single value.
