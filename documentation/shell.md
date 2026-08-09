# `shell.cpp` — Documentation

## File Purpose

Implements `app::shell`: a minimal interactive command-line shell running on top of `terminal::input`/`terminal::output`. It receives a command, recognizes it via binary search over an alphabetically sorted list, and calls the matching execution function through a function-pointer table.

## Includes

- `shell.h`: declares the `shell` class and its members (`m_input`, `m_output`, `m_is_running`) as well as the public command methods.
- `io/output/terminal_output.h`: for text output.
- `internal/shell_commands_list.h`: the `COMMAND_LIST` and `COMMAND_FUNCTIONS` X-macros — the **single source of truth** for which commands exist.
- `cpu/features.h`: for the `flag` command.
- `timer/kernel_timer.h`: for the `ticks` command.

## Comparison helper

### `str_compare(comparer, other)` — anonymous namespace

Implements a `strcmp`-style string comparison: returns the difference (`int8_t`) of the first differing characters, or `0` if equal. Used by the command binary search, which needs a sign (positive/negative/zero), not just boolean equality.

## Command tables built at compile time

### `command_list` → `g_command_list`

```cpp
constexpr uint8_t command_list_size{6};
struct command_list { const char* entries[command_list_size]; constexpr command_list(): entries{} { ... COMMAND_LIST ... } };
constexpr command_list g_command_list{};
```

An array of the **names** (strings) of the 6 commands, populated from the `COMMAND_LIST` X-macro — their order **must** be alphabetical, since a binary search runs over this array.

### Execution wrapper functions

```cpp
inline void execute_clear(app::shell* shell) noexcept { shell->clear(); }
```

Every public command method of the `shell` class (e.g. `clear()`, `exit()`) is "wrapped" in a free function `execute_<name>` with a uniform signature `void(app::shell*) noexcept`, so it can be placed into a function-pointer array — C++ member function pointers don't share the same simple pointer signature as free functions, so this "flattening" simplifies the dispatch table. All marked `[[gnu::always_inline]]` — there's no real call overhead, they're fully inlined.

### `command_functions` → `g_command_functions`

```cpp
struct command_functions { command_list_functions entries[command_list_size]; constexpr command_functions(): entries{} { ... COMMAND_FUNCTIONS ... } };
constexpr command_functions g_command_functions{};
```

An array of function pointers (`execute_*`), populated in **positional correspondence** with `g_command_list` — position `i` in `g_command_functions` must correspond to the same command as position `i` in `g_command_list`. This correspondence is maintained by the two X-macros (`COMMAND_LIST`, `COMMAND_FUNCTIONS`), which share the same `index` values.

## Public methods of the `shell` class (namespace `app`)

### Constructor

```cpp
shell::shell() noexcept: m_input{}, m_output{}, m_is_running{true}
```

Initializes the internal input/output objects and sets the running flag to active.

### `command_exists()` const

Implements **binary search** over `g_command_list.entries`, using the user's input string (`m_input.read_buffer()`) as the search key, via `str_compare`. Returns the **index** of the command if a match is found, or `-1` otherwise. O(log n) complexity instead of a linear O(n) comparison — significant even for a small number of commands, as good practice that scales as more commands get added.

### `execute_command()`

Calls `command_exists()`; if a valid index is found, it directly calls `g_command_functions.entries[index](this)` — **no** chain of `if`/`else if` or `switch` is needed, dispatch happens via a single array lookup and an indirect function call. If no command is found, it prints `"Command not found\n"`.

### `run()`

The shell's main loop: while `m_is_running`, it prints the prompt (`"my_OS:> "`), starts receiving input (`m_input.start()`, blocks until Enter is pressed), executes the command, and resets the input buffer (`m_input.reset()`).

### `clear()`

Resets the input buffer and clears the screen (`m_output.clear()`) — implements the `clear` command.

### `exit()`

Sets `m_is_running = 0`, ending the `run()` loop on the next iteration, and prints a termination message.

### `flag()`

Prints the CPU's current SIMD support level (`cpu::features::get()`) — a useful diagnostic to confirm which implementation (fallback/SSE2/AVX2) the VGA text buffer is using.

### `ticks()`

Prints the current timer tick count (`kernel::timer_ticks()`).

### `interrupt_stack()` / `kernel_stack()`

Compute and print the size (in bytes) of the interrupt stack and the kernel stack respectively, by subtracting the addresses of the linker-defined symbols `_interrupt_stack_top`/`_bottom` and `_kernel_stack_top`/`_bottom` — diagnostic tools for verifying that the stacks have the expected size (related to `diagnostic_tools/stack_calculator.cpp`, which statically computes the theoretical required size ahead of time, before execution).

## Design notes

- The "name array + parallel function array + binary search" architecture is a fully **data-driven** command dispatch scheme: adding a new command only requires one line in the `COMMAND_LIST`/`COMMAND_FUNCTIONS` X-macro (keeping alphabetical order) and a new method/wrapper — no change to the dispatch logic itself.
- All dispatch data (`g_command_list`, `g_command_functions`) is `constexpr`, so it lives in the binary's read-only section (`.rodata`), with no initialization cost at boot and no risk of accidental runtime modification.
