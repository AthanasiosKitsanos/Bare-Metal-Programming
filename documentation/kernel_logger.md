# `kernel_logger.cpp` — Documentation

## File Purpose

Implements the `kernel::logger` class: a thin wrapper around `terminal::output` that adds logging-level semantics (error/warning/panic), with colored prefixes so the message type visually stands out on screen.

## Includes

- `kernel_logger.h`: declares the `logger` class, the `m_terminal` member (presumably an embedded `terminal::output`), and the `vga_color`, `color_code` types.

## Private methods

### `set_prefix_text_and_color(error_type, foreground, background)`

```cpp
color_code temp{m_terminal.current_color_code()};
m_terminal.set_color(foreground, background);
m_terminal << error_type;
m_terminal.set_color_code(temp);
```

The pattern here is **"save, change, write, restore"**: it saves the current color, temporarily switches to a warning/error color to print the prefix (e.g. `"[ERROR]: "`), and immediately restores the **previous** color afterward (not necessarily the default color) — so the rest of the message that follows from the caller keeps whatever color it had before, without being "leaked" the prefix's color.

### `halt_forever()` const — `[[noreturn]]`

```cpp
while(true) asm volatile("cli; hlt");
```

Same pattern as the equivalent function in `kernel_assert.cpp` — halts the CPU permanently. Called only internally, from `panic`.

## Public methods

### `panic(panic_message)` — `[[noreturn]]`

```cpp
m_terminal.set_color(vga_color::white, vga_color::red);
m_terminal << "[PANIC]: " << panic_message << '\n';
halt_forever();
```

The most severe logging level: writes white text on a **red background** (maximum visual emphasis, since this represents a condition the kernel cannot recover from), then halts the CPU permanently — no return is possible, hence `[[noreturn]]`.

> Note: the header (`kernel_logger.h`) likely declares additional public methods such as `error()` and `warning()` (used extensively by other files, e.g. `kernel_exceptions.cpp`, `keyboard.cpp`), which are probably defined `inline` directly in the header (which is why they don't appear in the `.cpp`) — following the same logic as `panic`: they change color via `set_prefix_text_and_color`, print the appropriate prefix (`"[ERROR]: "`, `"[WARNING]: "`), and return a reference to the terminal stream so the caller can keep writing with `operator<<`.

## Design notes

- The `logger` class doesn't store its own text buffer — it writes directly to the shared, static `vga_text_buffer` via its internal `terminal::output`, just like every other output point in the kernel.
- The separation between `logger` (semantics/logging levels + color) and `terminal::output` (primitive character/number output) is a clean example of separation of concerns: `output` knows nothing about "errors" or "warnings", while `logger` knows nothing about how a character is actually written to the VGA screen.
