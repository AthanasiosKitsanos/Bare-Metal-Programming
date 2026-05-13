# Bare Metal Programming

A small educational operating-system project written from the ground up for x86 bare-metal development.

This repository is not a hosted application, framework, or normal user-space program. It builds a raw bootable image, enters 32-bit protected mode, initializes early kernel systems, and runs directly under an emulator such as QEMU.

---

## Project Goal

The goal of this project is to build a minimal operating system step by step, while keeping the code readable, explicit, and close to the hardware.

The current focus is:

- understanding the boot process;
- building a clean 32-bit kernel foundation;
- handling CPU exceptions and hardware interrupts;
- driving basic hardware through port I/O;
- designing kernel subsystems with clear interfaces;
- improving correctness and performance as the project grows.

Long mode and 64-bit execution are intentionally left for a later stage.

---

## Current Status

The project currently has the following major components:

| Area | Status |
|---|---|
| Boot stage | Basic boot flow and stage-2 loading |
| 32-bit protected mode | Active development target |
| VGA text output | Working |
| Terminal abstraction | Working |
| Kernel logger | Working |
| Runtime assertions | Working |
| IDT setup | Working |
| CPU exception stubs | Working |
| Common interrupt entry | Working |
| PIC remapping / IRQ handling | Working |
| PIT timer | Working at a configurable frequency |
| PS/2 keyboard IRQ1 | Working |
| Keyboard key mapping | In progress / expanding |
| Keyboard modifier state | Working |
| Interrupt-safe keyboard event queue | Current development focus |

---

## Toolchain

This project is built with a freestanding cross-compiler toolchain.

Expected tools:

- `i686-elf-g++`
- `i686-elf-as`
- `i686-elf-ld`
- `i686-elf-objcopy`
- `i686-elf-ar`
- `make`
- `qemu-system-x86_64`

The code is currently compiled as freestanding C++17 with no exceptions and no RTTI.

```make
-std=gnu++17
-ffreestanding
-O3
-Wall
-Wextra
-fno-exceptions
-fno-rtti
```

---

## Build

To build the bootable image:

```bash
make
```

The final raw image is produced at:

```text
bin/os_image.bin
```

To run it in QEMU:

```bash
make run
```

To clean generated build files:

```bash
make clean
```

---

## Repository Layout

```text
.
├── boot/
│   ├── boot_stage_1.S
│   ├── boot_stage_2.S
│   └── pm_entry.S
│
├── exception_stubs/
│   └── common_interrupt_entry.S
│
├── include/
│   ├── drivers/
│   ├── kernel/
│   └── terminal/
│
├── kernel/
│   └── kernel.cpp
│
├── links/
│   └── code_32.ld
│
├── src/
│   ├── drivers/
│   ├── kernel/
│   └── terminal/
│
├── bin/
├── obj/
├── elf/
├── lib/
└── Makefile
```

Generated folders such as `bin/`, `obj/`, `elf/`, and `lib/` are build output locations.

---

## Kernel Initialization Flow

At a high level, the current kernel startup flow is:

1. Initialize the VGA terminal.
2. Create the kernel logger.
3. Connect the assertion system to the logger.
4. Initialize exception and interrupt handling.
5. Initialize the PIT timer.
6. Initialize the keyboard driver.
7. Enable interrupts with `sti`.
8. Enter an idle loop using `hlt`.

This keeps initialization explicit and easy to debug while the system is still small.

---

## Keyboard Driver

The keyboard driver currently handles PS/2 keyboard input from IRQ1.

Current functionality includes:

- reading raw scancodes from port `0x60`;
- checking keyboard controller status through port `0x64`;
- handling press and release events;
- tracking modifier state;
- tracking Caps Lock state;
- translating known keys into internal key codes;
- translating text input candidates into characters;
- storing keyboard events in a fixed-size ring queue;
- exposing polling APIs for higher kernel layers.

The current event structure stores:

- raw scancode;
- key code;
- mapped key;
- key state;
- extended-scancode flag;
- validity flag;
- modifier snapshot.

This is important because higher layers should receive a stable view of the keyboard state at the exact time the key event happened.

---

## Keyboard Event Queue

The keyboard event queue is a fixed-size interrupt-safe ring buffer.

Current design:

- 64 event slots;
- head pointer;
- tail pointer;
- event count;
- dropped-event counter;
- IRQ handler pushes events;
- polling code pops events.

The queue is intentionally small, static, and allocation-free. This is suitable for early kernel development where dynamic memory allocation does not exist yet.

The higher layer can query:

- whether pending events exist;
- how many events are pending;
- how many events were dropped;
- the next keyboard event, if available.

---

## Design Principles

This project follows a few strict rules:

### 1. No hidden runtime dependencies

The kernel is freestanding. It does not rely on an operating system, a C++ runtime, exceptions, RTTI, or dynamic allocation.

### 2. Prefer explicit hardware behavior

Hardware interactions should be visible in the code. Port I/O, interrupt setup, descriptor tables, and controller commands are kept close to the implementation.

### 3. Keep interrupt paths small

Interrupt handlers should do the minimum necessary work. Slow processing should move to polling or higher-level kernel code whenever possible.

### 4. Use compile-time checks where possible

Static checks are preferred for fixed hardware contracts, table sizes, descriptor sizes, and compile-time mappings.

### 5. Keep public APIs clean

Subsystems expose only the functions needed by the rest of the kernel. Internal helpers stay inside `.cpp` files or internal headers.

---

## Current Development Chapter

The current chapter is:

```text
Chapter 2.23 - Interrupt-safe keyboard event queue polling
```

Main focus:

- safe communication between IRQ1 and normal kernel code;
- protecting shared state with interrupt guards;
- exposing polling APIs;
- keeping the keyboard IRQ handler fast;
- preparing the input system for higher-level text handling.

---

## Roadmap

Near-term goals:

- improve keyboard event polling usage in `kernel_main`;
- print translated keyboard input through the terminal;
- distinguish text input from control input;
- improve keyboard mapping coverage;
- make input handling cleaner for future shell work.

Medium-term goals:

- introduce basic memory-management concepts;
- improve panic and fault reporting;
- add more CPU exception diagnostics;
- continue organizing kernel subsystems;
- prepare for a simple kernel command interface.

Long-term goals:

- physical memory manager;
- paging;
- heap allocator;
- scheduler;
- user mode;
- system calls;
- filesystem experiments;
- 64-bit long mode.

---

## Running Philosophy

This project is built as a learning operating system, but the code should still be treated seriously.

Every chapter should improve at least one of these:

- correctness;
- clarity;
- structure;
- performance;
- hardware understanding.

The goal is not to write a large amount of code quickly. The goal is to understand every byte that matters.
