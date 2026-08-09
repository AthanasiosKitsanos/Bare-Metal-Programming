# `main.cpp` — Documentation

## File Purpose

This is the kernel's entry point. `kernel_main` is called from the boot code (assembly) once the CPU has already switched to protected mode and a temporary stack has been set up. The file's job is to initialize every kernel subsystem in the correct order and then hand control over to the shell.

## Includes

- `main.h`: contains the declarations needed to compile `kernel_main` (transitively pulls in PIT, e820, PMM, exceptions, keyboard, shell, etc.).

## External symbols (extern "C")

```cpp
extern "C" uint32_t _kernel_start;
extern "C" uint32_t _kernel_end;
```

These are **not** variables; they are symbols defined by the linker script. Their **address** (not their value) gives the physical bounds of the kernel image in memory — i.e. where the loaded kernel starts and ends. This address range is used by `pmm_initialize` so the Physical Memory Manager knows which frames are already occupied by the kernel itself and must not be handed out to anyone.

## `kernel_main()`

```cpp
extern "C" [[noreturn]] void kernel_main()
```

- It is `extern "C"` so its name is not mangled by the C++ compiler — this lets the assembly boot code do `call kernel_main` without knowing anything about C++ name mangling.
- It is marked `[[noreturn]]`: the compiler knows the function never returns normally, which allows better optimizations (e.g. it doesn't need to keep a return mechanism around).

### Initialization order and why it matters

1. **`kernel::initialize_pit(timer_frequency_hz)`** — Configures the Programmable Interval Timer (PIT) at 100 Hz, before interrupts are enabled. This way, when `sti` is executed later, the PIT is already in a known state and won't produce "confused" ticks.
2. **`kernel::set_timer_frequency(timer_frequency_hz)`** — Records the frequency in `kernel_timer`'s internal state, so functions like `uptime_seconds()` and `sleep_ms()` can convert ticks into real time.
3. **`terminal::output::initialize()`** — Clears the VGA text buffer and enables/syncs the hardware cursor. Must run before any log message, otherwise output would be in an undefined state.
4. **`kernel::memory::get_e820_memory_map()`** — Reads the memory map left by the BIOS/bootloader at the well-known address `0x500`. This map says which physical memory regions are usable and which are reserved (ACPI, etc.).
5. **`kernel::memory::pmm_initialize(&map, kernel_start, kernel_end)`** — Builds the Physical Memory Manager's bitmap on top of the e820 map, additionally excluding the `[kernel_start, kernel_end]` range so the kernel can't accidentally "swallow" its own memory.
6. **`kernel::initialize_exceptions()`** — Fills the IDT (Interrupt Descriptor Table) with handlers for CPU exceptions and hardware IRQs, remaps the PIC, and loads the IDT with `lidt`. Must run **before** interrupts are enabled (`sti`), otherwise a premature interrupt would lead to undefined behavior (calling into an uninitialized IDT entry).
7. **`driver::initialize_keyboard()`** — Sends commands to the PS/2 keyboard controller (clearing LEDs) before interrupts from it start arriving.
8. **Construction of `app::shell shell{}`** — Constructed *before* `sti`, so the object is fully ready by the time the first keystroke arrives.
9. **`asm volatile("sti")`** — Enables maskable interrupts. From here on, the timer and keyboard start producing IRQs.
10. **`shell.run()`** — Enters the shell's main loop, which reads commands and executes them indefinitely.
11. **Final `for(;;) asm volatile("hlt");` loop** — Safety net: if `shell.run()` were ever to return (it normally shouldn't), the kernel simply halts the CPU in an `hlt` loop instead of falling through into unknown code past the end of the function.

## Design notes

- The order of calls here **is** the documentation of the boot sequence's dependency graph: each step assumes the previous one. Any reordering (e.g. enabling interrupts before the IDT is ready) would immediately lead to a crash or a triple fault.
- `kernel_main` is deliberately kept "thin" (a thin orchestrator): it does no work by itself, it simply calls each subsystem's initializer in the right order. This aids readability and debugging, since the entry point acts as a "table of contents" for boot.
