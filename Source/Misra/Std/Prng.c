/// file      : Source/Misra/Std/Prng.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// xorshift64* PRNG with periodic kernel-CSPRNG reseed.
///
/// **The function-local statics in `prng_internal` are the one and only
/// program-lifetime statics permitted in MisraStdC.** Every other module
/// is init-by-value with no globals or TLS-as-globals. PRNG is the
/// explicit exception, by design, because a state-pointer-per-call API
/// would force every random-using site to thread an extra argument
/// through call stacks where it is not relevant. The state cannot leak
/// out -- it has function-internal linkage with no accessor.

#include <Misra/Config.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Prng.h>
#include <Misra/Sys.h>

#include "../_Syscall.h"

#if PLATFORM_WINDOWS
#    include <bcrypt.h>
#    include <windows.h>
#endif

// Pull `sizeof(*out)` bytes of kernel entropy into `*out`. Aborts on
// failure -- a system without entropy is broken and silently
// proceeding with weak randomness is worse than crashing.
static void prng_seed_from_kernel(u64 *out) {
    u8 *p   = (u8 *)out;
    u64 rem = sizeof(*out);
#if PLATFORM_WINDOWS
    if (BCryptGenRandom(NULL, p, (unsigned long)rem, 2) != 0) { // BCRYPT_USE_SYSTEM_PREFERRED_RNG = 2
        LOG_FATAL("Prng: BCryptGenRandom failed");
    }
#elif FEATURE_DIRECT_SYSCALL
#    if PLATFORM_DARWIN
    // getentropy: max 256 bytes per call. We only ask for 8.
    while (rem > 0) {
        long chunk = rem > 256 ? 256 : (long)rem;
        long ret   = misra_sys2(MISRA_SYS_getentropy, (long)(u64)p, chunk);
        if (ret < 0) {
            LOG_FATAL("Prng: getentropy failed");
        }
        p   += chunk;
        rem -= (u64)chunk;
    }
#    else
    // Linux getrandom: short reads possible on signal. Loop until full.
    while (rem > 0) {
        long ret = misra_sys3(MISRA_SYS_getrandom, (long)(u64)p, (long)rem, 0);
        if (ret < 0) {
            if (ErrnoOf(ret) == EINTR) {
                continue;
            }
            LOG_FATAL("Prng: getrandom failed");
        }
        p   += (u64)ret;
        rem -= (u64)ret;
    }
#    endif
#else
    // Non-direct-syscall fallback: weakly-linked libc getentropy if
    // available, else /dev/urandom. Avoid the libc <stdlib.h> include
    // by forward-declaring.
    extern int getentropy(void *buf, unsigned long n) __attribute__((weak));
    if (getentropy && getentropy(p, (unsigned long)rem) == 0) {
        return;
    }
    LOG_FATAL("Prng: no entropy source available");
#endif
}

// xorshift64* step (Vigna). Cycle length 2^64 - 1; the multiplier
// scrambles the low-bit linearity of plain xorshift.
static inline u64 xorshift64_star(u64 *state) {
    u64 x   = *state;
    x      ^= x >> 12;
    x      ^= x << 25;
    x      ^= x >> 27;
    *state  = x;
    return x * 0x2545F4914F6CDD1DULL;
}

// The PRNG core. Function-local static state, no accessor.
static u64 prng_internal(void) {
    // ===========================================================
    // The ONE program-lifetime static in MisraStdC. See Prng.h.
    // ===========================================================
    static u64 state      = 0;
    static u64 call_count = 0;

    if (state == 0) {
        prng_seed_from_kernel(&state);
        // Extremely unlikely (1 in 2^64), but xorshift breaks on
        // all-zero state -- nudge it to a known non-zero constant.
        if (state == 0) {
            state = 0x9E3779B97F4A7C15ULL;
        }
    }

    ++call_count;
    // Periodic reseed: XOR the current state with a fresh entropy
    // draw. We keep the existing state's contribution so repeated
    // sequences from the same process can never collide even if the
    // kernel CSPRNG were re-pulled identically (which it won't be).
    if ((call_count % PRNG_RESEED_INTERVAL) == 0) {
        u64 mix;
        prng_seed_from_kernel(&mix);
        state ^= mix;
        if (state == 0) {
            state = 0x9E3779B97F4A7C15ULL;
        }
    }

    return xorshift64_star(&state);
}

u64 Prng64(void) {
    return prng_internal();
}
u32 Prng32(void) {
    return (u32)prng_internal();
}
u16 Prng16(void) {
    return (u16)prng_internal();
}
u8 Prng8(void) {
    return (u8)prng_internal();
}
