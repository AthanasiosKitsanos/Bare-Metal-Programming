# `kernel_pic.cpp` — Documentation

## File Purpose

Driver for the **8259 PIC (Programmable Interrupt Controller)** pair — master and slave. Implements the classic "remap" procedure (shifting IRQ vectors so they don't collide with CPU exceptions), sending End-Of-Interrupt (EOI), and selectively masking IRQs.

## Includes

- `kernel_pic.h`: declarations of `inb`/`outb` (via the `terminal::` namespace) and the public API.
- `<stdint.h>`.

## Ports and protocol constants (anonymous namespace)

```cpp
constexpr uint16_t master_command{0x20}; constexpr uint16_t master_data{0x21};
constexpr uint16_t slave_command{0xA0};  constexpr uint16_t slave_data{0xA1};
constexpr uint8_t enable{0x11};
constexpr uint8_t master_bit{0x04}; constexpr uint8_t slave_bit{0x02};
constexpr uint8_t x86_mode{0x01};
constexpr uint8_t eoi_command{0x20};
```

These are the classic i8259 ports and protocol values: `0x20`/`0x21` for the master PIC, `0xA0`/`0xA1` for the slave. `0x11` is the ICW1 (Initialization Control Word 1) command requesting ICW4. `0x04`/`0x02` (ICW3) tell the master that the slave is connected on IRQ line 2, and tell the slave its own "cascade identity". `0x01` (ICW4) enables 8086/88 mode.

## `pic_remap(offset_1, offset_2)`

The classic four-pair ICW (Initialization Control Words) sequence, with `io_wait()` after every write (necessary because the old PIC might not process consecutive writes fast enough on modern hardware):

1. **Reads and temporarily saves the current masks** for both controllers, so they can be restored at the end without losing any pre-existing configuration.
2. **ICW1** (`enable = 0x11`) on both master and slave: begins the initialization sequence.
3. **ICW2** (`offset_1`/`offset_2`): sets the **new base vector** — i.e. which IDT vector IRQ0 of the master and IRQ0 of the slave will map to, respectively. This is the actual "remap": by default the IRQs start at vector 8, colliding with CPU exceptions (0–31); the remap shifts them, typically to 32 (master) and 40 (slave).
4. **ICW3**: declares the cascade relationship (master↔slave) via `master_bit`/`slave_bit`.
5. **ICW4** (`x86_mode`): sets 8086 mode.
6. **Restores the original masks** saved in step 1.

## `send_eoi(vector)` — `[[gnu::regparm(1)]]`

```cpp
if(vector > 7) terminal::outb(slave_command, eoi_command);
terminal::outb(master_command, eoi_command);
```

Sends the End-Of-Interrupt command. Here `vector` is the **local** IRQ number (0–15, not the IDT vector — the `vector - irq_base` conversion happens before the call, in `kernel_exceptions.cpp`). If the IRQ came from the slave (>7), EOI must be sent to **both** controllers — first to the slave, then to the master — since the master knows nothing about the slave's internal state, only that it received a cascade interrupt.

## `mask_all_except_timer_and_keyboard()`

```cpp
terminal::outb(master_data, 0xFC);
terminal::outb(slave_data, 0xFF);
```

Writes the interrupt masks directly: `0xFC` = `0b11111100` leaves only bits 0 and 1 active (IRQ0 = timer, IRQ1 = keyboard), masking off every other IRQ on the master. The slave is masked entirely (`0xFF`), since the kernel doesn't yet have any handler for devices behind it.

## Design notes

- This file is purely hardware configuration-time code, not hot path — there are no `regparm`-style optimizations aside from `send_eoi`, which **is called on every interrupt** (so it lives on the hot path and rightfully carries `[[gnu::regparm(1)]]`).
- Saving/restoring the original masks in `pic_remap` is a defensive best practice: even if the BIOS had already masked some IRQs for its own reasons, the remap doesn't accidentally unmask them.
