# `kernel_idt.cpp` — Documentation

## File Purpose

Implements the two fundamental hardware-level operations for managing the x86 **Interrupt Descriptor Table (IDT)**: writing a single entry (gate) into the table, and loading the table into the CPU via the `lidt` instruction.

## Includes

- `kernel_idt.h`: declares the `idt_entry`/`idtr_descriptor` structures and the public API.
- `<stddef.h>`: for `size_t`.

## The IDT array (anonymous namespace)

```cpp
constexpr size_t total_entries{256};
kernel::idt_entry idt_entry_table[total_entries];
```

The actual IDT array lives as a static array confined to this translation unit — no other file has direct access to it, only through the two public functions. x86 protected mode supports exactly 256 possible interrupt vectors, hence the fixed size.

## `set_interrupt_gate(vector, handler_address, selector, type_attributes)`

Fills **one** IDT entry at position `vector`. The `idt_entry` structure (x86 interrupt gate descriptor) splits the handler's address into two 16-bit halves (`offset_low`, `offset_high`) due to the historical segmented design of x86:

```cpp
entry->offset_low = static_cast<uint16_t>(handler_address & 0xFFFFu);
entry->selector = selector;
entry->zero = 0;
entry->type_attributes = type_attributes;
entry->offset_high = static_cast<uint16_t>((handler_address >> 16) & 0xFFFFu);
```

- **`offset_low`/`offset_high`**: the low and high 16 bits of the handler's 32-bit address.
- **`selector`**: the GDT segment selector to be used when entering the handler (typically the kernel code segment, `0x08`).
- **`zero`**: a reserved byte, must be zero per the architecture.
- **`type_attributes`**: encodes the gate type (interrupt/trap), the DPL (privilege level), and the present bit.

This function is called once per exception/IRQ during initialization (from `kernel_exceptions.cpp`), not on a hot path, so it carries no special optimization annotations like `regparm`.

## `load_idt()`

```cpp
const idtr_descriptor descriptor
{
    static_cast<uint16_t>(sizeof(idt_entry_table) - 1),
    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(idt_entry_table))
};
asm volatile("lidt %0" : : "m"(descriptor) : "memory");
```

Builds an **IDTR descriptor** (limit + base address) and loads it into the CPU via the special `lidt` assembly instruction. The `limit` is the table's size **minus one** (x86 architecture convention: `limit` is the last valid byte offset, not the total size). The `"memory"` clobber in the inline assembly tells the compiler this instruction may affect memory in ways that can't be statically predicted, preventing dangerous instruction reordering around it.

## Design notes

- Splitting this into two small, clear functions (one for "write an entry", one for "activate the table") follows the single-responsibility principle: `kernel_exceptions.cpp` calls `set_interrupt_gate` many times in a loop, and `load_idt` only **once**, at the end, after every entry has already been defined.
- The `idt_entry_table` array is **not** explicitly initialized before the `set_interrupt_gate` calls — every element remains in an undefined state until explicitly written. This is safe because `kernel_exceptions.cpp` covers **all** 256 vectors before calling `load_idt()` (every known exception/IRQ, plus `default_interrupt_handler` for everything else, via its own `g_interrupt_handlers_table` — though note this is a separate, higher-level dispatch table; the raw hardware IDT array itself needs *some* stub at every position before it's loaded, so an unexpected interrupt doesn't end up reading an invalid IDT entry).
