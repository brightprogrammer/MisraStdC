/// file      : misra/std/prng.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Pseudo-random number generator. Process-lifetime internal state,
/// reseeded from the kernel CSPRNG periodically.
///
/// **Design note (utmost importance).** PRNG is the **one and only**
/// place in MisraStdC where a function-local `static` variable lives
/// for the lifetime of the program. The state is wholly encapsulated
/// inside `prng_internal` (in `Prng.c`) and is not accessible by any
/// other TU or by any other identifier. Every other part of the
/// library is required to be init-by-value, no globals, no TLS-as-
/// globals -- PRNG is the deliberate, documented exception.
///
/// First call seeds from the OS entropy source (Linux `getrandom`,
/// Darwin `getentropy`, Windows `BCryptGenRandom`). After that each
/// call advances an xorshift64* state. Every
/// `PRNG_RESEED_INTERVAL` calls (see `Misra/Config.h`) the
/// state is XOR-mixed with a fresh kernel-entropy draw; smaller
/// values give stronger sequences at the cost of more frequent
/// kernel calls.

#ifndef MISRA_STD_PRNG_H
#define MISRA_STD_PRNG_H

#include <Misra/Types.h>

///
/// Next pseudo-random `u64`. Thread-safety: NOT thread-safe; callers
/// in multi-threaded contexts need an external mutex over the
/// sequence of calls if they want a fixed serialization order.
///
/// SUCCESS : Returns the next sample.
/// FAILURE : Aborts via LOG_FATAL on first call if the OS entropy
///           source is unavailable (treated as a fatal system
///           condition; no safe fallback exists).
///
/// TAGS: Prng, Random
///
u64 Prng64(void);

///
/// Lower 32 / 16 / 8 bits of the next sample. Faster at the call
/// site than masking `Prng64` by hand; the underlying state is the
/// same so they consume the same PRNG sequence.
///
/// TAGS: Prng, Random
///
u32 Prng32(void);
u16 Prng16(void);
u8  Prng8(void);

#endif // MISRA_STD_PRNG_H
