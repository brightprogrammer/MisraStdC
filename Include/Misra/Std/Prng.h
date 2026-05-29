/// file      : std/prng.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Pseudo-random number generator. Process-lifetime internal state,
/// reseeded from the kernel CSPRNG periodically.
///
/// **Design note (utmost importance).** MisraStdC is init-by-value
/// throughout -- no globals, no TLS-as-globals, no hidden defaults.
/// A small, deliberate set of process-lifetime singletons exists for
/// one-time platform init or signal-context state that genuinely has
/// no caller to thread an allocator through (the Abort callback slot
/// in `Sys.c`, the Windows-only `dbghelp` init flag and Winsock
/// once-init state under `Sys/`). PRNG is one of these: its state is
/// wholly encapsulated as function-local statics in `Prng.c` and not
/// accessible by any other TU or identifier. Each such singleton is
/// documented where it lives; outside this small set, the rule is
/// still init-by-value.
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
/// same so each call still consumes one position in the PRNG
/// sequence.
///
/// SUCCESS : Returns the truncated low bits of the next sample (32 /
///           16 / 8 bits respectively). The shared PRNG state advances
///           by one position per call.
/// FAILURE : Aborts via `LOG_FATAL` on first PRNG use if the OS
///           entropy source is unavailable (same fatal-on-bad-OS
///           contract as `Prng64`; no safe fallback exists).
///
/// TAGS: Prng, Random
///
u32 Prng32(void);
u16 Prng16(void);
u8  Prng8(void);

#endif // MISRA_STD_PRNG_H
