# `kernel_timer.cpp` — Documentation

## File Purpose

Maintains the system's **tick counter** (updated by the PIT/IRQ0 interrupt handler), and provides helper functions for tick↔time conversions and waiting (sleep), both in ticks and in milliseconds.

## Includes

- `<stdint.h>`, `kernel_timer.h`: public API.
- `logger/kernel_logger.h`: (available, though not used directly in this file outside of transitive dependency).
- `internal/kernel_interrupt_frame.h`: the `interrupt_frame` type.
- `internal/kernel_interrupt_guard.h`: an RAII interrupt guard.

## Global state (anonymous namespace)

```cpp
constexpr uint32_t milliseconds_per_second{1000};
volatile uint32_t g_timer_ticks{0};
uint32_t g_timer_frequency{0};
```

`g_timer_ticks` is marked **`volatile`**: it's updated inside interrupt context (`handle_timer_interrupt`) and read from normal code (e.g. `sleep_ticks`) — `volatile` prevents the compiler from "pinning" the value in a register inside a wait loop, which would otherwise create an infinite loop since the compiler wouldn't "see" that the value changes asynchronously from an interrupt.

## `handle_timer_interrupt(frame)` — `[[gnu::regparm(1)]]`

The actual IRQ0 handler. Completely ignores the contents of `frame` (`static_cast<void>(frame)`) — the timer needs no information from the CPU's state at the moment of the interrupt, it just increments the counter:

```cpp
++g_timer_ticks;
```

The minimum possible amount of work inside an interrupt handler — exactly appropriate for a handler called with high frequency (100 times per second in this system).

## Getters/Setters

- **`set_timer_frequency(frequency)`**: stores the frequency configured on the PIT (called from `main.cpp` right after `initialize_pit`), so other functions can convert ticks into real time.
- **`timer_ticks()`**: returns the current counter.
- **`timer_frequency()`**: returns the stored frequency.
- **`uptime_seconds()`**: `ticks / frequency`, protected against division by zero (`if(frequency == 0) return 0;`) — important if called before the frequency has been configured.

## `sleep_ticks(ticks)`

```cpp
constexpr uint32_t interrupt_flag{1u << 9};
const uint32_t start{g_timer_ticks};
bool was_enabled{static_cast<bool>(kernel::read_eflags() & interrupt_flag)};
while((g_timer_ticks - start) < ticks) asm volatile("sti; hlt");
if(!was_enabled) asm volatile("cli");
```

Implements **passive (busy-free) waiting**: instead of busy-polling, which would burn CPU cycles for no reason, the loop executes `sti; hlt` — enabling interrupts and **halting** the CPU until the next interrupt (any interrupt, not just the timer). Every time the CPU "wakes up", the condition is checked again. The subtraction `g_timer_ticks - start` works correctly even across a `uint32_t` counter **overflow** (unsigned arithmetic wraparound), since subtracting two unsigned values always gives the correct "distance" regardless of whether the counter has wrapped around.

Before the loop, it checks (via `read_eflags()` and the `IF` bit, bit 9) whether interrupts were already enabled; if they **weren't**, they are restored to disabled (`cli`) after the wait — the function leaves the interrupt state **exactly as it found it**, an important principle of "non-intrusive" helper function design.

## `sleep_ms(ms)`

Converts milliseconds into ticks and calls `sleep_ticks`, with careful **overflow** handling:

1. If `frequency == 0` or `ms == 0`, returns immediately (nothing to wait for).
2. Splits `ms` into whole seconds (`whole_seconds`) and the remainder (`remaining_milliseconds`).
3. Checks **before** multiplying whether `whole_seconds * frequency` would overflow (`whole_seconds > UINT32_MAX / frequency`) — if so, it clamps the wait to the maximum possible value (`sleep_ticks(UINT32_MAX)`) instead of letting the operation silently overflow and produce an incorrectly shorter wait.
4. Computes ticks for both parts (whole seconds + remainder, rounding the remainder up: `(ms * freq + 999) / 1000`), and again checks for overflow in their sum before calling the final `sleep_ticks`.

## Design notes

- The `sti; hlt` pattern is fundamental in kernels: it lets a kernel with no multi-threading "sleep" efficiently, leaving the CPU in a low-power state until there's actual work to do.
- The explicit handling of potential `uint32_t` overflow in every arithmetic operation (`sleep_ms`) reflects the general principle of "empirical verification, not assumption": rather than assuming values will always fit, every possible overflow scenario is checked explicitly before it can happen.
