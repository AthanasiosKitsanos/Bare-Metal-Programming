# `kernel_pit.cpp` — Documentation

## File Purpose

Initializes the **PIT (Programmable Interval Timer, 8253/8254 chip)** on channel 0, configuring it to produce IRQ0 at a desired frequency (Hz), within a supported range.

## Includes

- `kernel_pit.h`: public API.
- `internals/terminal_io_registers.h`: `inb`/`outb`.
- `kernel/logger/kernel_logger.h`: reporting a critical error if initialization fails.

## Constants

```cpp
constexpr uint32_t input_frequency{1193182};
constexpr uint32_t min_divisor{1}; constexpr uint32_t max_divisor{0xFFFF};
constexpr uint32_t min_supported_frequency{20}; constexpr uint32_t max_supported_frequency{100};
constexpr uint16_t channel_0_data_port{0x40}; constexpr uint16_t command_port{0x43};
constexpr uint8_t channel_0_lobyte_hibyte_mode_3_binary{0x36};
```

- `input_frequency = 1193182`: the **fixed** internal clock frequency of the PIT chip (approximately 1.193182 MHz), determined by the hardware itself — it never changes.
- `min_divisor`/`max_divisor`: the PIT accepts a 16-bit divisor (1–65535). A divisor of 0 is interpreted by the hardware as 65536, which is why it's explicitly excluded here for simplicity/clarity.
- `min_supported_frequency`/`max_supported_frequency`: an artificial limit of **this** kernel (20–100 Hz) — not a hardware limit, but a deliberate design choice so the timer neither runs too fast (wasting CPU cycles on interrupts) nor too slow (losing timing accuracy).
- `0x36`: the PIT command byte — channel 0, access mode lobyte/hibyte (both), mode 3 (square wave generator), binary (not BCD) counting.

## `initialize_pit(frequency)`

1. **Bounds check**: if the requested frequency is outside `[20, 100]`, calls `log.panic(...)` — there's no point continuing boot with an incorrect timing setup, since every other subsystem (sleep, uptime) depends on it.
2. **Divisor computation with rounding**:

   ```cpp
   const uint32_t divisor_32{(input_frequency + frequency / 2) / frequency};
   ```

   Instead of a plain integer division `input_frequency / frequency` (which would always round down, truncation), `frequency / 2` is added before dividing — the classic trick for **rounding an integer division to the nearest integer** rather than truncating downward, minimizing the error between the requested and actually-produced frequency.
3. **Second bounds check** on the computed divisor (`[1, 0xFFFF]`) — a safeguard in case some future change to the frequency bounds left an inconsistent combination of values.
4. **Sends the command and divisor**: the command byte (`0x36`), then the low byte and high byte of the divisor, in the order required by the PIT's lobyte/hibyte protocol.

## Design notes

- The code here runs **once**, at boot, before interrupts are enabled — there's no need for `regparm` or other hot-path micro-optimizations.
- Checking frequency bounds before every computation follows the "fail fast and clearly" principle: a controlled `panic` with a clear message is far better than a PIT running at an undefined or nonsensical frequency.
