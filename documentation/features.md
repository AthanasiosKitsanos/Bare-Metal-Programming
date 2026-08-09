# `features.cpp` — Documentation

## File Purpose

Defines the single, global variable holding the result of CPU feature detection — specifically, which SIMD level the current CPU supports (none / SSE2 / AVX2), as determined at boot via CPUID.

## Includes

- `features.h`: declares the `simd_flags` type and likely the detection function (`detect()`/`initialize()`) that populates this variable, as well as `get()`, which reads it.

## File contents

```cpp
namespace cpu::features
{
    extern "C" simd_flags mm_flag{0};
}
```

The entire `.cpp` file consists of **one** definition: the `mm_flag` variable, initialized to `0` (no special capability detected yet — a safe fallback default before the actual detection runs).

- **`extern "C"`**: avoids C++ name mangling for this symbol. This matters in practice if feature detection (which usually involves inline assembly or specialized boot-time code) is partly written in assembly, or needs to refer to this symbol under a predictable, unmangled name.
- The `simd_flags` type serves as the **index** used directly in the dispatch tables (`g_dispatch`, `g_dispatch_cpy`) of `terminal_vga_text_buffer.cpp`, with the convention `0 = fallback, 1 = SSE2, 2 = AVX2`.

## Design notes

- Isolating this variable in its own, minimal file (instead of, say, being static within some other module) turns it into a **clear, globally accessible single source of truth** for the CPU's SIMD capabilities — any other kernel subsystem (not just the VGA buffer) can consult it without needing to re-run CPUID.
- The initial value of `0` ensures that even if some code reads this variable **before** actual detection has run (a theoretical initialization-order bug scenario), the result will be the safest possible choice (scalar fallback), never SIMD instructions on hardware that might not support them.
