# my_OS — Bare Metal x86 Kernel (32-bit Protected Mode)

A from-scratch, educational operating system written in C++17 and x86 AT&T Assembly.
This is not a hosted application. It builds a raw bootable image, enters 32-bit protected
mode, and runs directly under QEMU with no underlying OS support.

---

## Project Philosophy

Every line of code in this project is written to be understood, not just to work.
The focus is on correctness, clarity, and hardware-level understanding.
Performance and low memory usage are considered at every step.

Long mode (64-bit) is intentionally deferred to a later stage.

---

## Toolchain

| Tool              | Purpose                          |
|-------------------|----------------------------------|
| `i686-elf-g++`    | Cross-compiler (C++17 freestanding) |
| `i686-elf-as`     | AT&T syntax assembler            |
| `i686-elf-ld`     | Linker                           |
| `i686-elf-objcopy`| Raw binary conversion            |
| `i686-elf-ar`     | Static library archiver          |
| `make`            | Build system                     |
| `qemu-system-x86_64` | Emulator for testing          |

**Compiler flags:**
-std=gnu++17 -ffreestanding -O3 -Wall -Wextra -fno-exceptions -fno-rtti

---

## Build

```bash
make        # Build the OS image
make run    # Launch in QEMU
make clean  # Remove all generated files
```

Output image: `bin/os_image.bin`

---

## Repository Layout
## Repository Layout

- **assembly/**
  - **boot/**
    - `boot_stage_1.S` — 16-bit Stage 1 bootloader (512 bytes, MBR)
    - `boot_stage_2.S` — 16-bit Stage 2 — E820, GDT, protected mode switch
    - `pm_entry.S` — 32-bit entry point — BSS zero, stack, kernel_main call
  - **exception_stubs/**
    - `common_interrupt_entry.S` — ISR/IRQ per-vector stubs + common entry routine
  - **internal/**
    - `stage_2_sectors.inc` — boot sector count constant

- **kernel/**
  - **assert/** — `KERNEL_ASSERT` / `KERNEL_ASSERT_MSG` macros
  - **exceptions/** — CPU exception + IRQ dispatcher registration
  - **idt/** — IDT entry table, `set_interrupt_gate`, `load_idt`
  - **internal/**
    - `kernel_cpu_interrupts_list.h` — X-macro: #DE, #UD, #GP, #PF
    - `kernel_hardware_interrupts_list.h` — X-macro: IRQ0 (timer), IRQ1 (keyboard)
    - `kernel_interrupt_frame.h` — packed struct, 52 bytes, static_assert
    - `kernel_interrupt_guard.h` — RAII interrupt enable/disable guard
  - **logger/** — info / warning / error / debug / panic
  - **memory/**
    - **e820/** — E820 physical memory map reader
    - **pmm/** — Physical Memory Manager, bitmap allocator *(In Progress)*
  - **pic/** — 8259 PIC remap, send_eoi, IRQ masking
  - **pit/** — PIT channel 0, configurable frequency (20–100 Hz)
  - **timer/** — tick counter, uptime, sleep_ticks, sleep_ms

- **drivers/**
  - **keyboard/** — PS/2 keyboard IRQ1 driver (full implementation)
  - **internal/** — scancode→key mappings (normal, shifted, extended)

- **utilities/**
  - **internals/** — `inb` / `outb` port I/O inline helpers
  - **vga/**
    - **vga_text_buffer/** — 80×25 VGA text buffer (put, scroll, color)
    - **vga_hardware_cursor/** — CRT hardware cursor (port 0x3D4 / 0x3D5)
  - **io/**
    - **output/** — `terminal::output` (`<<`, hex, dec, bool_alpha)
    - **input/** — `terminal::input` *(Skeleton — not yet implemented)*

- **apps/**
  - **shell/** — kernel shell application *(Skeleton — not yet implemented)*
    - **internal/** — X-macro command table (currently only `clear`)

- **links/**
  - `code_16.ld` — linker script for 16-bit stages (0x7C00)
  - `code_32.ld` — linker script for 32-bit kernel (0x7E00)

- **mk_files/** — modular Makefile includes (decl + rules per subsystem)

- `main.cpp` — `kernel_main`, top-level kernel initialization
- `Makefile` — root build entry point

## System Status

| Subsystem                         | Status              |
|-----------------------------------|---------------------|
| Stage 1 Bootloader                | ✅ Complete         |
| Stage 2 Bootloader (E820 + GDT)   | ✅ Complete         |
| 32-bit Protected Mode Entry       | ✅ Complete         |
| BSS zeroing                       | ✅ Complete         |
| VGA Text Buffer (80×25)           | ✅ Complete         |
| VGA Hardware Cursor               | ✅ Complete         |
| Terminal Output (`terminal::output`) | ✅ Complete      |
| Kernel Logger                     | ✅ Complete         |
| Kernel Assert System              | ✅ Complete         |
| Port I/O helpers (`inb`/`outb`)   | ✅ Complete         |
| IDT Setup                         | ✅ Complete         |
| 8259 PIC Remap + EOI + Masking    | ✅ Complete         |
| Common Interrupt Entry (Assembly) | ✅ Complete         |
| ISR/IRQ per-vector stubs          | ✅ Complete         |
| CPU Exception Dispatcher (#DE, #UD, #GP, #PF) | ✅ Complete |
| Hardware Interrupt Dispatcher (IRQ0, IRQ1) | ✅ Complete |
| Interrupt Frame (52-byte struct)  | ✅ Complete         |
| Interrupt Guard (RAII)            | ✅ Complete         |
| PIT Timer (configurable Hz)       | ✅ Complete         |
| Kernel Timer (ticks, uptime, sleep) | ✅ Complete      |
| PS/2 Keyboard Driver              | ✅ Complete         |
| Keyboard Key Mapping (normal + shifted + extended) | ✅ Complete |
| Keyboard Modifier State Tracking  | ✅ Complete         |
| Keyboard Event Queue (ring buffer, 64 slots) | ✅ Complete |
| E820 Memory Map Reader            | ✅ Complete         |
| Physical Memory Manager (PMM)     | 🔄 In Progress     |
| Terminal Input                    | 🔧 Skeleton Only   |
| Kernel Shell                      | 🔧 Skeleton Only   |

---

## Kernel Initialization Flow (`kernel_main`)

Construct terminal::output  (VGA buffer + hardware cursor)
Construct kernel::logger    (wraps terminal output)
console.initialize()        (clear screen, enable cursor)
set_exception_logger()
initialize_exceptions()     (IDT, PIC remap, install stubs, load IDT)
set_timer_logger()
initialize_pit(100 Hz)      (PIT channel 0 @ ~100 ticks/sec)
set_timer_frequency(100)
initialize_keyboard()       (flush buffer, disable LEDs)
sti                        (enable hardware interrupts)
idle loop (hlt inside for(;;))


---

## Interrupt Architecture

### CPU Exceptions (vectors 0–31)
Handled via `kernel_cpu_interrupts_list.h` X-macro.
Currently registered: `#DE` (0), `#UD` (6), `#GP` (13), `#PF` (14).
All produce a full register dump and kernel panic.

### Hardware Interrupts (vectors 32–47, remapped from IRQ 0–15)
Handled via `kernel_hardware_interrupts_list.h` X-macro.
Currently registered:
- **IRQ0 (vector 32):** PIT timer → `kernel::handle_timer_interrupt`
- **IRQ1 (vector 33):** PS/2 keyboard → `driver::handle_keyboard_interrupt`

All other vectors fall through to `default_interrupt_handler`.

### Common Entry Path
[per-vector stub]  
└─ push error_code (0 if none)  
└─ push vector number  
└─ jmp common_interrupt_entry  
└─ pusha (save all GPRs)  
└─ push esp → call interrupt_dispatcher(frame*)  
└─ addl $4, esp  
└─ popa  
└─ addl $8, esp  (discard error_code + vector)  
└─ iret

---

## Keyboard Driver Design

- **IRQ1 handler:** reads port `0x60`, decodes scancode set 1, updates modifier state, pushes to ring queue.
- **Ring queue:** 64 slots, power-of-two masked, interrupt-safe (no allocations).
- **Polling API:**
  - `poll_keyboard_event()`
  - `has_pending_keyboard_event()`
  - `pending_keyboard_event_count()`
  - `dropped_keyboard_event_count()`
- **Translation API:** `try_translate_text_event()` → maps event to `char`.
- **Extended scancodes:** tracked via `g_extended_pending` flag (prefix `0xE0`).

---

## Memory Management (In Progress)

### E820
The bootloader calls INT 0x15 / EAX=0xE820 in real mode and stores entries at address `0x502` (count at `0x500`). The kernel reads this via `get_e820_memory_map()`.

### Physical Memory Manager (PMM)
A bitmap-based frame allocator is being implemented.

- Enum class `pmm_result: uint8_t`  
{  
    - success = 0x00,  
    - failed = 0x01,  
    - lb_deny = 0x02,  
    - hb_deny = 0x03  
}
- Frame size: **4096 bytes**
- Bitmap: 1 bit per frame (set = used, clear = free)
- Public API (defined, implementation pending):
  - `pmm_initialize(map, kernel_start, kernel_end)`
  - `pmm_allocate_frame(uintptr_t*)` → `pmm_result`
  - `pmm_free_frame(uintptr_t)` → `pmm_result`
  - `pmm_total_frames()`, `pmm_used_frames()`, `pmm_free_frames()`

---

## Current Chapter
Chapter 3 — Physical Memory Manager (PMM)
Bitmap-based frame allocator over E820 usable regions.

---

## Roadmap

### Near-Term
- [✅] Complete PMM (`pmm_initialize`, `pmm_allocate_frame`, `pmm_free_frame`)
- [ ] Integrate PMM into `kernel_main`
- [ ] Log memory map and PMM statistics at boot

### Medium-Term
- [ ] Virtual memory / paging (identity map kernel, page directory/tables)
- [ ] Kernel heap allocator (`kmalloc` / `kfree`)
- [ ] Improve terminal input and begin shell implementation
- [ ] Expand CPU exception coverage

### Long-Term
- [ ] Scheduler
- [ ] User mode
- [ ] System calls
- [ ] Filesystem
- [ ] 64-bit Long Mode

---

## Design Principles

1. **Freestanding** — no libc, no C++ runtime, no exceptions, no RTTI, no dynamic allocation (yet).
2. **Explicit hardware** — every port access, every IDT entry, every PIC command is visible in code.
3. **Lean interrupt paths** — IRQ handlers do the minimum; all heavy work is deferred to polling.
4. **Compile-time correctness** — `static_assert` on every packed struct, every table size.
5. **Clean subsystem boundaries** — internal helpers stay in `.cpp` or `internal/` headers.
6. **X-macro driven tables** — CPU exceptions and hardware interrupts are registered from
   a single source of truth, eliminating duplication.