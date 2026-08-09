# `stopwatch.cpp` — Documentation

## File Purpose

Implements a **RAII stopwatch** diagnostic tool: an object that, simply by existing (scope-based lifetime), automatically measures how much time has passed between its construction and its destruction, and prints the result to the screen when it goes out of scope.

## Includes

- `stopwatch.h`: declares the `stopwatch` class (member `start`).
- `kernel/timer/kernel_timer.h`: `timer_ticks()`, `timer_frequency()`.
- `io/output/terminal_output.h`: for the final print.

## Constructor

```cpp
stopwatch::stopwatch() noexcept: start{kernel::timer_ticks()} {}
```

Simply records the current tick count at the moment of construction — does nothing else.

## Destructor

```cpp
stopwatch::~stopwatch() noexcept
{
    const uint32_t time_diff{kernel::timer_ticks() - start};
    const uint32_t frequency{kernel::timer_frequency()};
    if(frequency != 0) { ... }
}
```

All the real work happens here, following the **RAII (Resource Acquisition Is Initialization)** design pattern: because the destructor is called **automatically** by the compiler whenever the `stopwatch` object goes out of scope (whether normally or due to an early return/exception in that scope), the tool's user doesn't need to remember to call some `stop()` or `report()` manually — simply declaring a local `stopwatch` at the start of a code region is enough to time it.

The conversion from ticks to milliseconds is done with careful **overflow** handling for `uint32_t` arithmetic, following the same philosophy used in `kernel_timer.cpp::sleep_ms`:

```cpp
if(whole_seconds > UINT32_MAX / ms_per_sec) elapsed_ms = UINT32_MAX;
else { ... overflow check on the final addition too ... }
```

The value is "clamped" to `UINT32_MAX` instead of allowing silent overflow, which would produce a misleadingly small result in a diagnostic tool — critical, since the whole point of the tool is measurement reliability.

Finally, it constructs a local `terminal::output console{}` and prints `"Time elapsed: <N>ms\n"`.

## Design notes

- The pattern "create a stopwatch at the start of a code block `{ ... }`, use it as an invisible (named or unnamed) local object, and let it print the elapsed time on its own when the block closes" is especially useful in kernel code, where no external profiling tools are available — the program itself reports its own performance.
- If the timer's frequency hasn't been configured yet (`frequency == 0`), the destructor simply prints nothing — avoiding a division by zero without dropping the system into an error state over something as minor as a failed time measurement.
