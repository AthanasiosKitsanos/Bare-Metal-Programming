# `terminal_input.cpp` — Documentation

## File Purpose

Implements the `terminal::input` class: a command-line editor with a fixed-size buffer, an insertion cursor that can sit anywhere within the line, and support for navigation keys (arrows, Home, End) and control keys (Backspace, Enter, Tab, Escape). It consumes events from the keyboard driver (`driver::keyboard`) and simultaneously updates the screen via an internal `terminal::output`.

## Includes

- `terminal_input.h`: declares the class, the `input_buffer` (128 + 1 bytes), the `cursor`/`data_end` pointers.
- `io/output/terminal_output.h`: for the embedded output object (`m_output`).
- `internals/navigation_handlers.h`, `internals/control_input_handlers.h`: X-macros that generate the dispatch code for navigation/control keys.
- `keyboard/keyboard.h`: `keyboard_event`, `keyboard_key` types and event classifiers.

## Helper functions (anonymous namespace)

### `move_data_right(end, begin, step)` / `move_data_left(begin, end, step)`

Shift a range of characters within the buffer by `step` positions, right or left, byte by byte. Used when a character is inserted or deleted **in the middle** of the line (not just at the end): the buffer must "open up" or "close" space by shifting whatever comes after the cursor.

### `string_length(string)` — `[[gnu::regparm(1)]]`

Simple string length measurement up to `'\0'`, returning `uint8_t` (enough since the buffer's capacity is only 128 characters).

## Constructor

```cpp
input::input() noexcept: cursor{input_buffer}, data_end{input_buffer}, input_buffer{}, m_output{}, input_ready{false}
```

Initializes `cursor` and `data_end` to point at the start of an empty buffer.

## Buffer editing operations

### `add_character(c)` — `[[gnu::regparm(2)]]`

If the buffer is full (`buffer_full()`), returns `false`. If the cursor is **not** already at the end of the data (`*cursor != '\0'`), shifts the remaining data right by one position (`move_data_right`) to open up space. Writes the character, advances the cursor, and sets the new terminating `'\0'`.

### `add_string(string)` — `[[gnu::regparm(2)]]`

Same logic as `add_character` but for an entire string at once (used by Tab, which inserts 4 spaces together) — computes the length, checks capacity, shifts data once for the whole string (more efficient than calling `add_character` in a loop, which would repeatedly shift the same data).

### `delete_character()` — `[[gnu::regparm(1)]]`

If the cursor is already at the start (`buffer_begin()`), returns `false`. Otherwise steps the cursor back and shifts the following data left (`move_data_left`) to "close" the gap — implements backspace in the middle of the line.

### `reset_buffer()` — `[[gnu::regparm(1)]]`

Resets `cursor`/`data_end` to the start and `input_ready = false` — called between consecutive shell commands.

### `trim_end()` — `[[gnu::regparm(1)]]`

Strips trailing spaces (`' '`) from the end of the line before command execution, moving `data_end` back.

## Main input loop

### `start_data_receiving()` — `[[gnu::regparm(1)]]`

Waits/processes in a loop until `input_ready == true`:

1. As long as there's no pending keyboard event, the CPU "sleeps" with `hlt` (power saving — no busy-wait polling).
2. Takes an event (`poll_keyboard_event`).
3. If the event corresponds to a printable character (`try_translate_text_event`), it's inserted (`add_character`) and the screen is updated: the character is written, and if the cursor is **not** at the end of the text (i.e. it was inserted mid-line), the rest of the text after the cursor is reprinted and the hardware cursor is moved back to the correct position (`print_string_no_sync` + `move_cursor_left_n` + `call_cursor_sync`) — so the screen correctly shows the result even when typing in the middle of a line.
4. Otherwise, if it's a "control" event (Backspace, Tab, Enter, Escape), it calls `control_key_dispatch`.
5. Otherwise, if it's a "navigation" event (arrows, Home, End, Page Up/Down), it calls `navigation_key_dispatch`.

## Control key handlers

### `handle_escape()`

Moves the hardware cursor to the end of the line, visually deletes every character (`delete_last_char_no_sync` in a loop of length `count()`), syncs the cursor, and resets the buffer — implements "clear what I typed".

### `handle_backspace()`

Calls `delete_character()`; if successful, visually deletes the last character, and if the cursor wasn't at the end, reprints the remaining text with one extra trailing space (to visually "erase" the now-leftover character), then restores the cursor position.

### `handle_tab()`

Inserts 4 spaces (`add_string("    ")`) with the same "reprint the remainder if needed" logic as `add_character`.

### `handle_enter()`

If there's content (`count() > 0`), trims trailing spaces (`trim_end()`). Writes a newline and signals `input_ready = true`, ending the `start_data_receiving` loop.

### `control_key_dispatch(key)` — `[[gnu::regparm(2)]]`

A `switch` generated from the `CONTROL_INPUT_HANDLERS` X-macro — for every registered control key, it calls the corresponding `handle_<key>()`.

## Navigation handlers

### `handle_home()` / `handle_end()`

Move the hardware cursor to the start/end of the line (`move_cursor_left_n`/`move_cursor_right_n` on the buffer) and update the internal `cursor` pointer accordingly.

### `handle_arrow_left()` / `handle_arrow_right()`

Call `move_cursor_left()`/`move_cursor_right()` (inline methods that simply shift the `cursor` pointer by one position within the buffer, if possible) and, if the move succeeds, visually update the hardware cursor (`go_backwards()`/`go_forward()`).

### `handle_arrow_up()` / `handle_arrow_down()` / `handle_page_up()` / `handle_page_down()`

Currently **empty implementations** (`return;`) — placeholders for future functionality (e.g. command history), with no effect today.

### `navigation_key_dispatch(key)` — `[[gnu::regparm(2)]]`

The corresponding `switch`, generated from the `NAVIGATION_HANDLERS` X-macro.

## Design notes

- The buffer has a **fixed, static size** (`input_capacity = 128`) — no dynamic memory allocation is involved in processing input, appropriate for kernel code before a fully functional heap even exists.
- The use of X-macros (`CONTROL_INPUT_HANDLERS`, `NAVIGATION_HANDLERS`) to generate the `switch` blocks avoids code duplication (every new key is added in **one** place, the X-macro list, and the dispatch switch is generated automatically).
- The "first update the buffer, then update the screen" separation in every handler follows the pattern of "logical state first, visual representation second", keeping the two systems in sync without mixing their logic.
