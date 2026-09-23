#include "features.h"

extern "C" uint8_t _mm_flag{0};

// namespace cpu::features
// {
//     uint8_t get() noexcept { return _mm_flag; }
// }

// namespace cpu
// {
//     class mm_flag
//     {
//         static uint8_t flag;

//         public:
//         [[gnu::always_inline]]
//         static inline uint8_t get() noexcept { return flag; }
//     };
// }