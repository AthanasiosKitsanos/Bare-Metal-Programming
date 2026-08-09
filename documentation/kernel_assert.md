# `kernel_assert.cpp` — Documentation

## File Purpose

Implements an assertion mechanism for the kernel — the equivalent of the C library's `assert()`, but adapted to write the error message to the screen via `kernel::logger`, instead of calling `abort()`.

## Includes

- `kernel_assert.h`: public API (`assert_failed`, `assert_failed_msg`, `set_assert_logger`), likely also a `KERNEL_ASSERT` macro that calls these functions.
- `logger/kernel_logger.h`: the `logger` class, with its `error()`/`panic()` methods.

## Global state (anonymous namespace)

```cpp
kernel::logger* g_assert_logger{nullptr};
```

A **pointer** (not an instance) to a logger, injected from outside via `set_assert_logger`. This choice (dependency injection via pointer) allows the assertion system to work even before any specific, permanent logger is ready — the pointer simply remains `nullptr` until it's configured.

## `halt_forever()` — `[[noreturn]]`

```cpp
while(true) asm volatile("cli; hlt");
```

Disables interrupts and halts the CPU, permanently. Used as a **last line of defense**: if a failed assertion occurs **before** any logger has been configured (`g_assert_logger == nullptr`), there's no safe way to print an error message — better to halt the CPU immediately than to continue in an unstable state or attempt to use a null pointer.

## `build_assert_message(expression, file, line)`

```cpp
if(!g_assert_logger) halt_forever();
return g_assert_logger->error() << "Assertion failed: " << expression << "\nFile: " << file << "\nLine: " << line << '\n';
```

A shared helper function, called from both public entry points, so the format of the base message (expression, file, line) is defined in **one** place. Returns a reference to the logger's own output stream, letting the caller append additional information (e.g. a custom message) before the final `panic` is called.

## Public API (namespace `kernel`)

### `set_assert_logger(log)`

A simple setter — registers the logger pointer that will be used by all future failed assertions.

### `assert_failed(expression, file, line)` — `[[noreturn]]`

Called when a plain assertion fails (e.g. `KERNEL_ASSERT(x > 0)`). Builds the base message via `build_assert_message` and then calls `g_assert_logger->panic(panic_message)`, which prints on a red background and halts the CPU permanently.

### `assert_failed_msg(expression, message, file, line)` — `[[noreturn]]`

A variant that also accepts a custom explanatory message, appended **after** the base message, before the final `panic`. Useful when an assertion needs more context than the textual representation of the expression alone can provide.

## Design notes

- Having **two layers of protection against crashing with no information** (first the `!g_assert_logger` check in `build_assert_message`, then the final `panic` of the logger itself) ensures the kernel never "silently falls over" — it either provides a full diagnostic message, or (in the worst case, before a logger exists) halts in a predictable, controlled way.
- Keeping `assert` as a separate file/subsystem (instead of embedding it directly into `kernel_logger.cpp`) lets **any** other kernel subsystem use assertions without needing to know the logger's internal details — just the assertion macro/function.
