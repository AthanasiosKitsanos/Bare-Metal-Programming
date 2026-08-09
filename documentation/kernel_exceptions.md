# `kernel_exceptions.cpp` — Documentation

## File Purpose

The kernel's central interrupt "nervous system": installs every handler for CPU exceptions (e.g. Divide Error, Page Fault) and hardware IRQs (Timer, Keyboard) into the IDT, and implements the single **interrupt dispatcher** called by the assembly stub (`common_interrupt_entry.S`) for **every** interrupt, regardless of vector.

## Includes

- `idt/kernel_idt.h`: `set_interrupt_gate`, `load_idt`.
- `logger/kernel_logger.h`: reporting exceptions to the screen.
- `kernel_exceptions.h`: public API.
- `internal/kernel_interrupt_frame.h`: the `interrupt_frame` structure (CPU register state at the time of the interrupt).
- `pic/kernel_pic.h`: `pic_remap`, `send_eoi`, `mask_all_except_timer_and_keyboard`.
- `timer/kernel_timer.h`, `keyboard/keyboard.h`: the actual IRQ0/IRQ1 handlers.
- `internal/kernel_cpu_interrupts_list.h`, `internal/kernel_hardware_interrupts_list.h`: the `CPU_INTERRUPT_LIST`/`HARDWARE_INTERRUPT_LIST` X-macros — the full list of known exceptions/IRQs with vector, name, title, mnemonic.

## Constants

```cpp
constexpr uint16_t interrupt_vector_count{256};
constexpr uint16_t kernel_code_selector{0x08};
constexpr uint8_t interrupt_gate_attributes{0x8E};
constexpr uint8_t irq_base{32};
constexpr uint8_t irq_max{47};
```

x86 protected mode always has 256 possible interrupt vectors. `0x08` is the selector for the kernel code segment in the GDT. `0x8E` encodes: present, privilege (DPL) ring 0, 32-bit interrupt gate. After the PIC remap, hardware IRQs occupy vectors 32–47.

## Exception descriptor structures

### `exception_descriptor`

```cpp
struct exception_descriptor { uint8_t vector; exception_handler_ptr stub; const char* name; const char* mnemonic; };
```

Bundles all the information needed to install an IDT entry: the vector, the pointer to the assembly stub, the human-readable name, and the standard mnemonic (e.g. `"#DE"`, `"#PF"`).

### Generating declarations and descriptors via X-macros

```cpp
#define X(vector, name, title, mnemonic) extern "C" void isr_##vector() noexcept;
CPU_INTERRUPT_LIST
#undef X
```

This pattern repeats four times (twice for CPU exceptions, twice for hardware IRQs): once to declare the external assembly symbols (`isr_N`/`irq_N`, defined in `common_interrupt_entry.S`), and once to build the corresponding `constexpr exception_descriptor` values. This way, the **single** list of definitions in the `.h` file automatically generates both the declarations and the description data — no duplication, zero chance that an exception's vector doesn't match its stub.

## The dispatch table

```cpp
using interrupt_handler = void (*)(kernel::interrupt_frame*) noexcept [[gnu::regparm(1)]];
struct g_interrupt_handlers_table { interrupt_handler entries[interrupt_vector_count]; constexpr g_interrupt_handlers_table() ... };
constexpr g_interrupt_handlers_table g_interrupt_handlers{};
```

An array of 256 function pointers, populated at compile time:
1. First, **all** 256 entries are filled with `default_interrupt_handler` (the fallback for any unknown/unexpected vector).
2. Next, the vectors of known CPU exceptions are replaced with `handle_cpu_exception`.
3. Finally, the vectors of known hardware IRQs are replaced with their **actual** handler (`kernel::handle_timer_interrupt`, `driver::keyboard::handle_keyboard_interrupt`), via `name_space::handle_##name` dynamically generated from the X-macro.

This means every new IRQ added to the `HARDWARE_INTERRUPT_LIST` gets automatically "wired" to its correct handler, with no change needed to the dispatch logic.

## Handling CPU exceptions

### `handle_exception(name, mnemonic, frame)` — `[[gnu::regparm(1)]] [[noreturn]]`

Logs the full CPU register state (EIP, EFLAGS, error code, EAX/ECX/EDX/EBX/ESP/EBP/ESI/EDI, vector) at error level (`log.error()`), in hexadecimal, then calls `log.panic(...)`, which halts the CPU permanently. Used when a **recognized** CPU exception occurs — there's no safe way to continue, so the kernel panics in a controlled manner, giving the developer all the diagnostic data available.

### `handle_cpu_exception(frame)` — `[[gnu::regparm(1)]] [[noreturn]]`

A `switch` on `frame->vector`, generated from the `CPU_INTERRUPT_LIST` X-macro — every known vector calls `handle_exception` with the correct name/mnemonic. A `default` case covers the (theoretically impossible, but safely handled) case of an unrecognized vector, logging a warning and halting the CPU.

### `default_interrupt_handler(frame)` — `[[gnu::regparm(1)]]`

Handles **every** vector without an explicitly registered handler (neither a CPU exception nor a known IRQ): logs a warning with the vector and EIP. If the vector falls in the IRQ range (`irq_base`–`irq_max`), it returns normally (so the EOI still gets sent and the hardware doesn't "jam"). Otherwise, it halts the CPU — an unknown, non-IRQ interrupt is a serious error.

## `interrupt_dispatcher(frame)` — `extern "C"`

```cpp
extern "C" void interrupt_dispatcher(kernel::interrupt_frame* frame) noexcept
{
    uint32_t vector{frame->vector};
    g_interrupt_handlers.entries[vector](frame);
    if(vector >= irq_base && vector <= irq_max) kernel::send_eoi(static_cast<uint8_t>(vector - irq_base));
}
```

The **single, universal entry point** called by the assembly (`common_interrupt_entry.S`) for every kind of interrupt, regardless of its type. Dispatch happens through a simple array lookup (O(1), no `if` chain), and if the vector is a hardware IRQ, an End-Of-Interrupt (EOI) is sent to the PIC after processing — required so the PIC knows it can send the next IRQ of the same or lower priority.

## `kernel::initialize_exceptions()`

1. Remaps the PIC so IRQs move from the conflicting vectors 0–15 (which overlap with CPU exceptions) to 32–47.
2. Installs **all** CPU exception and IRQ gates into the IDT, via `install_exception` in an X-macro loop.
3. `kernel::mask_all_except_timer_and_keyboard()` — disables all other IRQs on the PIC, since the kernel doesn't yet have handlers for them.
4. `load_idt()` — loads the IDTR with the address of the IDT array via `lidt`.

## Design notes

- The philosophy of "one X-macro list, multiple generated views (declarations, descriptors, dispatch table, switch cases)" eliminates entire classes of synchronization bugs between IDT entries, handler tables, and human-readable names.
- Choosing a vector-indexed 256-entry array instead of a hash map or chain of checks is the natural choice here, since the vector's range is already known, small, and dense (0–255) — a perfect scenario for direct indexing.
