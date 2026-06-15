#include <Misra.h>
#include <Misra/Std/Container/BitVec.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Log.h>
#include <Misra/Types.h>

#include "../Util/TestRunner.h"

// =============================================================================
// Blind mutation-hardening tests for Source/Misra/Std/Container/BitVec.c.
// Each test pins down an exact observable result that a specific surviving
// mutant would change. Comments tie each test to its mutant(s); EQUIVALENT
// mutants are documented in the campaign report, not tested here.
// =============================================================================

static DebugAllocator blind_debug_alloc(void) {
    DebugAllocatorConfig cfg = {.capture_traces = false, .detect_overflow = false, .track_freed_history = false};
    return DebugAllocatorInitWith(cfg);
}

bool test_blind_remove_all_validates(void) {
    BitVecRemoveAll(NULL, true);
    return false;
}
// 645:28 cxx_rshift_to_lshift -- `bit_count >> (i*8)` -> `<<`. For lengths that
// fit in one byte the high bytes are zero either way, so it only diverges when
// bit_count spans multiple bytes. Use a length of 256 (0x100): real reads
// byte0=0x00, byte1=0x01, ...; the left-shift mutant reads `(256 << (i*8)) &
// 0xFF == 0` for every i, dropping the length signal entirely. So a length-256
// all-zero vec and a length-512 all-zero vec must hash DIFFERENTLY (real); the
// `<<` mutant collapses both to the same value.
bool test_blind_hash_multibyte_length_shift(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec a = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec b = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 256; i++) {
        BitVecPush(&a, false);
    }
    for (int i = 0; i < 512; i++) {
        BitVecPush(&b, false);
    }
    // Byte content differs (32 vs 64 zero bytes), so this alone is not a pure
    // length test. Compute the real expected hashes by replay and require the
    // implementation to match them exactly -- the `<<` mutant cannot, since it
    // zeroes every length byte beyond the lowest.
    u64 ha = bitvec_hash(&a, 0);
    u64 hb = bitvec_hash(&b, 0);

    u64 seed  = 1469598103934665603ULL;
    u64 prime = 1099511628211ULL;
    u64 exp_a = seed, exp_b = seed;
    for (int i = 0; i < 32; i++) { // 256 bits -> 32 bytes
        exp_a ^= 0u;
        exp_a *= prime;
    }
    for (u64 i = 0, bc = 256; i < sizeof(bc); i++) {
        exp_a ^= (bc >> (i * 8u)) & 0xFFu;
        exp_a *= prime;
    }
    for (int i = 0; i < 64; i++) { // 512 bits -> 64 bytes
        exp_b ^= 0u;
        exp_b *= prime;
    }
    for (u64 i = 0, bc = 512; i < sizeof(bc); i++) {
        exp_b ^= (bc >> (i * 8u)) & 0xFFu;
        exp_b *= prime;
    }

    bool result = (ha == exp_a) && (hb == exp_b);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 932:10 cxx_replace_scalar_call -- bitvec_from_str_str wrapper must return the
// decoded vector via the success branch.
bool test_blind_from_str_str_wrapper_content(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str    s      = StrInitFromZstr("111000111", &alloc);
    BitVec bv     = bitvec_from_str_str(&s, ALLOCATOR_OF(&alloc));
    bool   result = (BitVecLen(&bv) == 9) && (BitVecCountOnes(&bv) == 6);
    for (int i = 0; i < 9; i++) {
        result = result && (BitVecGet(&bv, i) == ((i / 3) % 2 == 0));
    }

    BitVecDeinit(&bv);
    StrDeinit(&s);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1152:27 cxx_add_to_sub -- BitVecRotateRight `src_idx = (i + bv->length -
// positions) % bv->length` -> `(i - bv->length - positions)`. Pins the exact
// rotated content.
bool test_blind_rotate_right_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    bool pat[5] = {true, false, false, true, true};
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, pat[i]);
    }
    BitVecRotateRight(&bv, 2);

    bool result = true;
    for (int i = 0; i < 5; i++) {
        result = result && (BitVecGet(&bv, i) == pat[(i + 5 - 2) % 5]);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1871:5 cxx_remove_void_call -- bitvec_regex_match_str validates bv.
bool test_blind_regex_match_str_validates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Str              p     = StrInitFromZstr("1", &alloc);
    bitvec_regex_match_str(NULL, &p);
    return false;
}

int main(void) {
    WriteFmt("[INFO] Starting BitVec.Blind tests\n\n");

    TestFunction tests[] = {
        test_blind_hash_multibyte_length_shift,
        test_blind_from_str_str_wrapper_content,
        test_blind_rotate_right_exact,
    };
    TestFunction deadend_tests[] = {
        test_blind_remove_all_validates,
        test_blind_regex_match_str_validates,
    };

    return run_test_suite(
        tests,
        (int)(sizeof(tests) / sizeof(tests[0])),
        deadend_tests,
        (int)(sizeof(deadend_tests) / sizeof(deadend_tests[0])),
        "BitVec.Blind"
    );
}
