# `keyboard.cpp` — Documentation

## File Purpose

Implements the PS/2 keyboard driver: device initialization, translation of raw scancodes (Scancode Set 1) into logical keys (`keyboard_key`), tracking of modifier state (Shift/Ctrl/Alt/CapsLock), and a ring-buffer queue of keyboard events that the interrupt handler fills and the rest of the kernel drains.

## Includes

- `pic/kernel_pic.h`: PIC (Programmable Interrupt Controller) functions.
- `keyboard.h`: public API and types.
- `internals/terminal_io_registers.h`: `inb`/`outb` for I/O ports.
- `internal/keyboard_key_list_n_map.h`: X-macros with the full scancode→key and key→character mapping tables.
- `internal/kernel_interrupt_frame.h`, `internal/kernel_interrupt_guard.h`: the `interrupt_frame` type and an RAII interrupt guard.
- `logger/kernel_logger.h`: reporting initialization errors.

## Port and protocol constants

```cpp
constexpr uint16_t data_port{0x60};
constexpr uint16_t status_port{0x64};
constexpr uint8_t output_buffer_full{0x01};
constexpr uint8_t input_buffer_full{0x02};
```

These are the classic PS/2 controller (i8042) ports: `0x60` for data, `0x64` for status/commands. The `output_buffer_full`/`input_buffer_full` bits indicate whether there's data available to read from the keyboard, or whether the controller is ready to accept a new command.

## Mapping tables built at compile time

Four structures, all built via a `constexpr` constructor that runs an X-macro:

- **`key_list` → `normal_key_map`**: scancode (0–127) → `keyboard_key` (without the extended prefix).
- **`normal_character_map_table` → `normal_characters_table`**: `keyboard_key` → character without Shift.
- **`shifted_character_map_table` → `shifted_characters_table`**: `keyboard_key` → character with Shift.
- **`extended_key_map_table` → `extended_key_table`**: scancode (with the extended prefix `0xE0`) → `keyboard_key`.

Because they're all `constexpr`, these tables are populated **at compile time** and end up embedded in the binary as ready-made data (in the data section) — no initialization is needed at boot (runtime), and lookups are always O(1) array accesses.

## Translation helper functions

### `map_scancode_set_1_key(key_code, extended)` — `[[gnu::regparm(2)]]`

Selects the correct table (`extended_key_table` or `normal_key_map`) depending on whether the `0xE0` byte preceded it.

### `get_normal_character(key)` — `[[gnu::regparm(1)]]` / `get_shifted_character(key)`

Simple lookups into the corresponding character tables.

## Controller communication protocol

### `wait_input_buffer_clear()` / `wait_output_buffer_full()`

Polling loops with an upper bound of `keyboard_timeout = 100000` attempts, calling `kernel::io_wait()` on every iteration (a small delay between consecutive I/O accesses, needed on old PS/2 hardware). Return `false` if the timeout expires — protection against a permanent hang if the controller never responds.

### `read_keyboard_ack()`

Waits for data and checks whether the response equals `keyboard_ack (0xFA)`.

### `send_keyboard_byte_and_wait_ack(byte)`

Sends a command byte to the controller and confirms it was accepted (ACK).

### `flush_keyboard_output_buffer()`

Drains any stale bytes left over from previous states before normal operation begins — prevents the first interrupt from processing a leftover byte from the BIOS.

## Event Queue

```cpp
struct alignas(64) keyboard_event_queue
{
    driver::keyboard::keyboard_event entries[64];
    uint8_t head; uint8_t tail; uint8_t count;
};
```

A ring buffer of 64 entries (a power of 2, so wraparound can be done via bitwise AND instead of modulo — see `next_keyboard_event`). The `alignas(64)` places the structure on a cache-line boundary, preventing false sharing and improving locality during accesses.

- **`next_keyboard_event(index)`** — `[[gnu::always_inline]]`: `(index + 1) & keyboard_event_queue_mask` — fast modulo wraparound with no real division, possible because the size is a power of 2 (checked via `static_assert`).
- **`commit_keyboard_event()`**: advances `tail` and increments `count` — called after a new event has been written into the queue.

## Modifier state tracking

### `update_modifier_state(key, state)` — `[[gnu::regparm(2)]]`

A `switch` that updates the `g_modifier_state` bitmask (type `modifier_state = uint8_t`) for every modifier key. Special attention goes to `caps_lock`: because the Caps Lock key is "toggle" (flip state) rather than "hold down", its handling explicitly distinguishes between **press state** (`caps_lock_down`, a sticky bit that prevents repeated toggling while the key remains held) and **active state** (`caps_lock_on`, the actual on/off): only at the **first** detected press moment (`!is_caps_down`) is `caps_lock_on` flipped.

## Public API

### `driver::initialize_keyboard()`

Clears the modifier state, flushes the output buffer, and sends the command to turn off all LEDs (`set_leds_command` + `all_leds_off`). If it fails, it logs a warning (`kernel::logger::warning`) but **does not** halt boot — failing to configure LEDs isn't critical.

### `try_translate_text_event(event, out_character)` — `[[gnu::regparm(2)]]`

First checks whether the event is a candidate for text input (`is_text_input_candidate_event`). For letters, it applies **XOR logic** between Shift and Caps Lock: `shift_pressed != caps_on` selects the shifted letter — this correctly implements the rule that Caps Lock inverts Shift **only** for letters (e.g. Shift+letter with Caps active yields a lowercase letter), while for non-letters (digits, symbols) it depends only on Shift.

### `handle_keyboard_interrupt(frame)` — `[[gnu::regparm(1)]]`

The actual IRQ1 handler, called by the interrupt dispatcher:
1. Checks that data is actually available (`output_buffer_full`) — otherwise returns immediately.
2. Reads the scancode byte.
3. If it's the extended prefix (`0xE0`), sets the `g_extended_pending` flag and returns (the real scancode arrives on the **next** interrupt).
4. If the queue is full, drops the event — prevents buffer overflow; losing one event is preferable to memory corruption.
5. Decodes: `key_code = scancode & 0x7F`, state (`pressed`/`released` from bit `0x80`), looks up the logical key, updates modifiers, and writes all fields of the `keyboard_event` directly at the queue's `tail` position before calling `commit_keyboard_event()`.

### `current_keyboard_modifier_state()`

Returns `g_modifier_state`, protected by `kernel::interrupt_guard` (RAII, disables interrupts for the duration of the read) — prevents an intermediate state if an interrupt fires while the variable is being read.

### `poll_keyboard_event(out_event)` — `[[gnu::regparm(1)]]`

If the queue is empty, returns `false`. Otherwise copies the event from the `head` position, advances `head`, and decrements `count`. Also protected by `interrupt_guard`, since the queue is shared between the interrupt context (writer) and normal code (reader) — a classic producer/consumer concurrency problem in an environment without threads but with interrupts.

### `has_pending_keyboard_event()`

Returns `count`, also protected.

## Design notes

- All static state (tables, queue) is confined to the anonymous namespace — no external translation unit can directly read or corrupt it.
- The `interrupt_guard` wrapped around every access to the shared queue is the kernel's core synchronization pattern, replacing locks/mutexes (pointless on a single-core, non-preemptive setup) with simple interrupt disabling.
- The choice of "drop the event when the queue is full" instead of waiting is the right choice inside an interrupt handler: the handler must return quickly and must never block.
