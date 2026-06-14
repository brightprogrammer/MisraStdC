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

// 24:11 cxx_le_to_lt -- sqrt_f64 `if (x <= 0.0)` -> `x < 0.0`. Reached via
// BitVecCorrelation: a constant vector has zero variance so the sqrt argument
// is 0.0. Real sqrt_f64(0)=0 -> denominator==0 -> Correlation returns 0.0; the
// mutant runs Newton on 0.0 yielding a tiny non-zero denominator and returns
// numerator/denominator instead.
bool test_blind_sqrt_zero_guard_correlation(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 6; i++) {
        BitVecPush(&a, true); // all ones -> zero variance
        BitVecPush(&b, i % 2 == 0);
    }

    double corr   = BitVecCorrelation(&a, &b);
    bool   result = (corr == 0.0);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 31:9 cxx_assign_const -- sqrt_f64 `u.d = x` -> `u.d = 42`. The Newton seed
// comes from x's bit pattern; forcing it to 42's bits gives a wrong sqrt.
// Cosine of two identical single-bit vectors is 1/(sqrt(1)*sqrt(1)) = 1.0.
bool test_blind_sqrt_uses_x_cosine(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&a, true);
    BitVecPush(&b, true);

    double cos    = BitVecCosineSimilarity(&a, &b);
    bool   result = (cos > 0.999999 && cos < 1.000001);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 101:43 cxx_gt_to_le / cxx_gt_to_ge -- BitVecClear `bitvec->byte_size > 0`
// guard for the MemSet. The le-mutant skips zeroing a populated vec; we clear,
// re-grow, and require the bits read back as zero. (ge is always-true here but
// the same assertion holds; the joint kill is the zeroed re-grown bits.)
bool test_blind_clear_zeroes_data(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 16; i++) {
        BitVecPush(&bv, true);
    }
    BitVecClear(&bv);
    bool result = (BitVecLen(&bv) == 0);

    BitVecResize(&bv, 16);
    for (int i = 0; i < 16; i++) {
        result = result && (BitVecGet(&bv, i) == false);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 114:18 cxx_gt_to_ge -- BitVecResize `new_size > bitvec->length` guards the
// zero-fill of newly exposed range. Shrink then grow so stale 1-bits exist;
// grown bits must read zero.
bool test_blind_resize_grow_zeroes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, true);
    }
    BitVecResize(&bv, 3);
    BitVecResize(&bv, 8);

    bool result = true;
    for (int i = 3; i < 8; i++) {
        result = result && (BitVecGet(&bv, i) == false);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 118:23 cxx_gt_to_ge -- BitVecResize `new_bytes > old_bytes` guards the
// MemSet of newly added whole bytes. Grow across byte boundaries from a vec
// with set high bits; the new range must read zero.
bool test_blind_resize_crosses_byte_boundary_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, true);
    }
    BitVecResize(&bv, 4);
    BitVecResize(&bv, 24);

    bool result = true;
    for (int i = 4; i < 24; i++) {
        result = result && (BitVecGet(&bv, i) == false);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 150:23 cxx_gt_to_ge / cxx_lt_to_le -- BitVecReserve
// `new_byte_size > bitvec->byte_size` guards zeroing freshly grown bytes. The
// le-mutant skips zeroing on a real grow. Fill one byte, reserve far more,
// resize into the new region: every new bit must read zero.
bool test_blind_reserve_zeroes_grown_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, true);
    }
    BitVecReserve(&bv, 256);
    BitVecResize(&bv, 256);

    bool result = true;
    for (int i = 8; i < 256; i++) {
        result = result && (BitVecGet(&bv, i) == false);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 167:23 cxx_assign_const -- BitVecShrinkToFit length==0 branch
// `bv->capacity = 0` -> `= 42`. After shrinking an emptied vec, capacity must
// be 0.
bool test_blind_shrink_empty_capacity_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 10; i++) {
        BitVecPush(&bv, true);
    }
    BitVecClear(&bv);
    BitVecShrinkToFit(&bv);

    bool result = (BitVecCapacity(&bv) == 0) && (BitVecLen(&bv) == 0);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 172:22 cxx_le_to_lt -- BitVecShrinkToFit `if (bv->capacity <= bv->length)`
// early-return. le->lt (`capacity < length`, never true) makes the mutant
// proceed instead of returning when capacity == length. A no-op shrink on a
// capacity==length vec must keep capacity and bits intact.
bool test_blind_shrink_capacity_equals_length_noop(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = bitvec_init_with_capacity(8, ALLOCATOR_OF(&alloc));

    BitVecResize(&bv, 8);
    for (int i = 0; i < 8; i++) {
        BitVecSet(&bv, i, i % 2 == 0);
    }
    u64 cap_before = BitVecCapacity(&bv);
    BitVecShrinkToFit(&bv);

    bool result = (BitVecCapacity(&bv) == cap_before) && (BitVecLen(&bv) == 8);
    for (int i = 0; i < 8; i++) {
        result = result && (BitVecGet(&bv, i) == (i % 2 == 0));
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 177:23 cxx_eq_to_ne -- BitVecShrinkToFit `if (new_byte_size == bv->byte_size)`
// fast path. Build cap 16 (2 bytes) / length 9 (2 bytes) so byte_size matches;
// the fast path must set capacity = length without losing data.
bool test_blind_shrink_same_byte_fast_path(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = bitvec_init_with_capacity(16, ALLOCATOR_OF(&alloc));

    BitVecResize(&bv, 9);
    for (int i = 0; i < 9; i++) {
        BitVecSet(&bv, i, i % 3 == 0);
    }
    BitVecShrinkToFit(&bv);

    bool result = (BitVecCapacity(&bv) == 9) && (BitVecLen(&bv) == 9);
    for (int i = 0; i < 9; i++) {
        result = result && (BitVecGet(&bv, i) == (i % 3 == 0));
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 233:10 cxx_replace_scalar_call -- BitVecTryClone
// `!BitVecReserve(out,..) || !BitVecResize(out,..)`. A scalar-replaced truthy
// return breaks the sizing logic; the clone of a populated vec must match the
// source exactly in length and bits.
bool test_blind_clone_reserve_resize_succeeds(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           src   = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 20; i++) {
        BitVecPush(&src, (i * 7) % 5 < 2);
    }

    BitVec clone = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   ok    = BitVecTryClone(&clone, &src);

    bool result = ok && (BitVecLen(&clone) == 20);
    for (int i = 0; i < 20; i++) {
        result = result && (BitVecGet(&clone, i) == BitVecGet(&src, i));
    }

    BitVecDeinit(&clone);
    BitVecDeinit(&src);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 295:24 cxx_ge_to_gt, 296:13 cxx_init_const, 296:73 cxx_mul_to_div --
// BitVecPush growth: from capacity 0, pushing 100 bits one at a time must keep
// every bit and reach length 100. A div-mutated growth under-reserves.
bool test_blind_push_growth_from_empty(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    bool result = true;
    for (int i = 0; i < 100; i++) {
        result = result && BitVecPush(&bv, (i % 3) == 0);
    }
    result = result && (BitVecLen(&bv) == 100);
    for (int i = 0; i < 100; i++) {
        result = result && (BitVecGet(&bv, i) == ((i % 3) == 0));
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 296:45 cxx_eq_to_ne -- BitVecPush `capacity == 0 ? 8 : capacity*2`. With ne,
// an empty vec takes the `capacity*2 == 0` branch, Reserve(0) no-ops, and the
// first push misbehaves. A single push onto a fresh vec must yield length 1.
bool test_blind_push_first_from_zero_capacity(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    bool result = BitVecPush(&bv, true);
    result      = result && (BitVecLen(&bv) == 1);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecCapacity(&bv) >= 1);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 425:13 cxx_ge_to_gt -- BitVecRemove `idx >= bv->length` bounds check. ge->gt
// lets idx == length pass; removing at index == length must abort.
bool test_blind_remove_at_length_aborts(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecPush(&bv, true);
    BitVecPush(&bv, false);
    BitVecRemove(&bv, 2); // idx == length -> LOG_FATAL
    return false;
}

// 452:15 -- see report (EQUIVALENT). This pins RemoveRange exactness for the
// neighbouring arithmetic.
bool test_blind_remove_range_middle_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    bool pat[8] = {true, false, true, true, false, false, true, false};
    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, pat[i]);
    }
    BitVecRemoveRange(&bv, 2, 3); // remove indices 2,3,4

    bool exp[5] = {true, false, false, true, false};
    bool result = (BitVecLen(&bv) == 5);
    for (int i = 0; i < 5; i++) {
        result = result && (BitVecGet(&bv, i) == exp[i]);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 465:5 / 477:5 / 489:5 / 527:5 / 532:5 / 549:5 / 566:5 / 583:5
// cxx_remove_void_call -- ValidateBitVec on the first/result operand of
// RemoveFirst, RemoveLast, RemoveAll, And, Or, Xor, Not. A NULL operand must
// abort; removing the validate call lets it through.
bool test_blind_remove_first_validates(void) {
    BitVecRemoveFirst(NULL, true);
    return false;
}
bool test_blind_remove_last_validates(void) {
    BitVecRemoveLast(NULL, true);
    return false;
}
bool test_blind_remove_all_validates(void) {
    BitVecRemoveAll(NULL, true);
    return false;
}
bool test_blind_and_validates_result(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecAnd(NULL, &a, &b);
    return false;
}
bool test_blind_or_validates_result(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecOr(NULL, &a, &b);
    return false;
}
bool test_blind_xor_validates_result(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecXor(NULL, &a, &b);
    return false;
}
bool test_blind_not_validates_result(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecNot(NULL, &bv);
    return false;
}

// 632:19 cxx_init_const -- bitvec_hash FNV seed -> 42. An empty vec's hash is
// fully determined by the real seed and the 8-byte length tail-mix; compute
// the expected value from the real seed and require an exact match.
bool test_blind_hash_seed_value(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    u64 seed     = 1469598103934665603ULL;
    u64 prime    = 1099511628211ULL;
    u64 expected = seed;
    for (int i = 0; i < 8; i++) {
        expected ^= 0u;
        expected *= prime;
    }

    u64  h      = bitvec_hash(&bv, 0);
    bool result = (h == expected);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 644:45 / 644:23 / 645:14 / 645:34 -- bitvec_hash length tail-mix (loop
// direction `i++`, bound `i < sizeof`, xor->or, shift amount `i*8`). We pin
// the EXACT hash of an 8-bit all-zero vec, computed by replaying the real
// algorithm. Any of those mutations changes the absolute value:
//   - `i--` runs the tail loop only once instead of 8 times
//   - `<=` adds a 9th iteration (i=8) with an out-of-range shift
//   - `|=` instead of `^=` ORs the byte in
//   - `i/8` collapses every shift amount to 0
bool test_blind_hash_tail_mix_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, false); // all-zero -> byte_count == 1, byte value 0
    }

    u64 seed      = 1469598103934665603ULL;
    u64 prime     = 1099511628211ULL;
    u64 bit_count = 8;
    u64 expected  = seed;
    // byte loop: 1 zero byte
    expected ^= 0u;
    expected *= prime;
    // length tail-mix: 8 bytes of bit_count, LSB first
    for (u64 i = 0; i < sizeof(bit_count); i++) {
        expected ^= (bit_count >> (i * 8u)) & 0xFFu;
        expected *= prime;
    }

    u64  h      = bitvec_hash(&bv, 0);
    bool result = (h == expected);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
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

// 726:5 / 727:5 cxx_remove_void_call -- BitVecWeightCompare validates bv1/bv2.
bool test_blind_weight_compare_validates_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecWeightCompare(NULL, &b);
    return false;
}
bool test_blind_weight_compare_validates_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecWeightCompare(&a, NULL);
    return false;
}

// 879:10 cxx_replace_scalar_call -- bitvec_try_from_str_impl
// `if (!BitVecReserve(out, str_len))`. The decoded vector must equal the input
// bit string exactly.
bool test_blind_from_str_decode_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Zstr   s  = "1100101011110000101011001100111100001010";
    BitVec bv = BitVecFromStr(s, &alloc);

    bool result = (BitVecLen(&bv) == 40);
    for (int i = 0; i < 40; i++) {
        result = result && (BitVecGet(&bv, i) == (s[i] == '1'));
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 916:12 cxx_replace_scalar_call -- bitvec_try_from_str_str returns the impl's
// result; a scalar-replace skips the impl, leaving `*out` uninitialised.
// Decoding a Str must produce the exact bits.
bool test_blind_from_str_str_decode(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str    s  = StrInitFromZstr("10110", &alloc);
    BitVec bv = BitVecFromStr(&s, &alloc);

    bool result = (BitVecLen(&bv) == 5);
    result      = result && (BitVecGet(&bv, 0) == true);
    result      = result && (BitVecGet(&bv, 1) == false);
    result      = result && (BitVecGet(&bv, 2) == true);
    result      = result && (BitVecGet(&bv, 3) == true);
    result      = result && (BitVecGet(&bv, 4) == false);

    BitVecDeinit(&bv);
    StrDeinit(&s);
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

// 952:38 -- see report (EQUIVALENT). Pins BitVecToBytes exact byte content.
bool test_blind_to_bytes_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    int ones[] = {0, 2, 4, 5, 7, 9, 10};
    for (int i = 0; i < 12; i++) {
        BitVecPush(&bv, false);
    }
    for (unsigned k = 0; k < sizeof(ones) / sizeof(ones[0]); k++) {
        BitVecSet(&bv, ones[k], true);
    }

    u8  buf[8] = {0};
    u64 n      = BitVecToBytes(&bv, buf, 8);

    bool result = (n == 2);
    result      = result && (buf[0] == (u8)((1 << 0) | (1 << 2) | (1 << 4) | (1 << 5) | (1 << 7)));
    result      = result && (buf[1] == (u8)((1 << 1) | (1 << 2)));

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 982:10 cxx_replace_scalar_call -- bitvec_try_from_bytes Reserve||Resize. The
// decoded vector must match the byte input exactly.
bool test_blind_from_bytes_decode_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    u8     bytes[2] = {0xB5, 0x06};
    BitVec bv       = BitVecFromBytes(bytes, 12, &alloc);

    bool result = (BitVecLen(&bv) == 12);
    for (int i = 0; i < 12; i++) {
        int  byte = i / 8;
        int  off  = i % 8;
        bool exp  = ((bytes[byte] >> off) & 1u) != 0;
        result    = result && (BitVecGet(&bv, i) == exp);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1015:31 -- see report (EQUIVALENT). Pins BitVecToInteger exact value.
bool test_blind_to_integer_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 8; i++) {
        BitVecPush(&bv, false);
    }
    BitVecSet(&bv, 0, true);
    BitVecSet(&bv, 1, true);
    BitVecSet(&bv, 3, true);
    BitVecSet(&bv, 7, true);

    bool result = (BitVecToInteger(&bv) == 139);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1040:10 cxx_replace_scalar_call -- bitvec_try_from_integer Reserve||Resize.
// Pins from_integer exact content.
bool test_blind_from_integer_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv     = BitVecFromInteger((u64)0xABC, 12, &alloc);
    bool   result = (BitVecLen(&bv) == 12);
    for (int i = 0; i < 12; i++) {
        bool exp = ((0xABCu >> i) & 1u) != 0;
        result   = result && (BitVecGet(&bv, i) == exp);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1036:14 cxx_gt_to_ge -- `if (bits > 64) bits = 64`. bits > 64 must clamp to
// 64. (ge differs only at bits==64, harmless; this pins the clamp via bits=100.)
bool test_blind_from_integer_clamps_to_64(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    BitVec bv     = BitVecFromInteger((u64)0xFFFFFFFFFFFFFFFFULL, 100, &alloc);
    bool   result = (BitVecLen(&bv) == 64);
    result        = result && (BitVecCountOnes(&bv) == 64);

    BitVecDeinit(&bv);
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

// Pins BitVecRotateLeft exact content (neighbour of the rotate-left Deinit).
bool test_blind_rotate_left_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    bool pat[5] = {true, false, false, true, true};
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, pat[i]);
    }
    BitVecRotateLeft(&bv, 2);

    bool result = true;
    for (int i = 0; i < 5; i++) {
        result = result && (BitVecGet(&bv, i) == pat[(i + 2) % 5]);
    }

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1195:36 -- see report (EQUIVALENT). Pins BitVecFindLast / BitVecFind exact.
bool test_blind_find_last_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    bool pat[6] = {false, true, false, true, false, false};
    for (int i = 0; i < 6; i++) {
        BitVecPush(&bv, pat[i]);
    }
    bool result = (BitVecFindLast(&bv, true) == 3) && (BitVecFindLast(&bv, false) == 5);
    result      = result && (BitVecFind(&bv, true) == 1);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1217:5 cxx_remove_void_call -- BitVecAny validates bv.
bool test_blind_any_validates(void) {
    BitVecAny(NULL, true);
    return false;
}

// 1238:29 -- see report (EQUIVALENT). Pins BitVecLongestRun exact.
bool test_blind_longest_run_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    bool pat[] = {true, true, false, true, true, true, false, false};
    for (unsigned i = 0; i < sizeof(pat) / sizeof(pat[0]); i++) {
        BitVecPush(&bv, pat[i]);
    }
    bool result = (BitVecLongestRun(&bv, true) == 3) && (BitVecLongestRun(&bv, false) == 2);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1257:37 -- see report (EQUIVALENT on result). Pins BitVecFindPattern,
// including a pattern that matches only at the final valid offset.
bool test_blind_find_pattern_at_end(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           pat   = BitVecInit(ALLOCATOR_OF(&alloc));

    bool t[6] = {false, false, false, true, false, true};
    for (int i = 0; i < 6; i++) {
        BitVecPush(&bv, t[i]);
    }
    BitVecPush(&pat, false);
    BitVecPush(&pat, true);
    bool result = (BitVecFindPattern(&bv, &pat) == 2);

    // pattern "01" occurs only at the last valid offset (idx 3) of text 1 1 1 0 1
    BitVec text2 = BitVecInit(ALLOCATOR_OF(&alloc));
    bool   t2[5] = {true, true, true, false, true};
    for (int i = 0; i < 5; i++) {
        BitVecPush(&text2, t2[i]);
    }
    result = result && (BitVecFindPattern(&text2, &pat) == 3);

    BitVecDeinit(&text2);
    BitVecDeinit(&pat);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1274:50 -- see report (EQUIVALENT). Pins BitVecFindLastPattern exact.
bool test_blind_find_last_pattern_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           pat   = BitVecInit(ALLOCATOR_OF(&alloc));

    bool t[5] = {false, true, false, true, false};
    for (int i = 0; i < 5; i++) {
        BitVecPush(&bv, t[i]);
    }
    BitVecPush(&pat, false);
    BitVecPush(&pat, true);
    bool result = (BitVecFindLastPattern(&bv, &pat) == 2);

    BitVecDeinit(&pat);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1298:37 -- see report (EQUIVALENT on output). Pins find_all_pattern_raw.
bool test_blind_find_all_pattern_positions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           pat   = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 6; i++) {
        BitVecPush(&bv, i % 2 == 1);
    }
    BitVecPush(&pat, false);
    BitVecPush(&pat, true);

    size results[8] = {0};
    u64  n          = bitvec_find_all_pattern_raw(&bv, &pat, results, 8);

    bool result = (n == 3) && (results[0] == 0) && (results[1] == 2) && (results[2] == 4);

    BitVecDeinit(&pat);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1319:37 -- see report (EQUIVALENT on output). Pins find_all_pattern_vec.
bool test_blind_find_all_pattern_vec_positions(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           pat   = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 6; i++) {
        BitVecPush(&bv, i % 2 == 1);
    }
    BitVecPush(&pat, false);
    BitVecPush(&pat, true);

    BitVecMatchIndices out = VecInitT(out, ALLOCATOR_OF(&alloc));
    bool               ok  = bitvec_find_all_pattern_vec(&bv, &pat, &out);

    bool result = ok && (VecLen(&out) == 3);
    result      = result && (*VecPtrAt(&out, 0) == 0) && (*VecPtrAt(&out, 1) == 2) && (*VecPtrAt(&out, 2) == 4);

    VecDeinit(&out);
    BitVecDeinit(&pat);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1438:23 -- see report (EQUIVALENT). Pins BitVecJaccardSimilarity exact.
bool test_blind_jaccard_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    bool av[4]  = {true, true, false, false};
    bool bvv[4] = {true, false, true, false};
    for (int i = 0; i < 4; i++) {
        BitVecPush(&a, av[i]);
        BitVecPush(&b, bvv[i]);
    }
    double j      = BitVecJaccardSimilarity(&a, &b);
    bool   result = (j > (1.0 / 3.0) - 1e-9) && (j < (1.0 / 3.0) + 1e-9);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1451:5 / 1452:5 cxx_remove_void_call -- BitVecCosineSimilarity validates
// bv1/bv2.
bool test_blind_cosine_validates_bv1(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecCosineSimilarity(NULL, &b);
    return false;
}
bool test_blind_cosine_validates_bv2(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecCosineSimilarity(&a, NULL);
    return false;
}

// 1534:36 / 1535:39 -- see report (EQUIVALENT). Pins BitVecEditDistance exact.
bool test_blind_edit_distance_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    bool av[5]  = {true, false, true, true, false};
    bool bvv[4] = {true, true, false, false};
    for (int i = 0; i < 5; i++) {
        BitVecPush(&a, av[i]);
    }
    for (int i = 0; i < 4; i++) {
        BitVecPush(&b, bvv[i]);
    }
    u64  d      = BitVecEditDistance(&a, &b);
    bool result = (d == 2);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1553:10 / 1554:10 -- see report (EQUIVALENT). Pins WithError success path.
bool test_blind_edit_distance_with_error_success(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 5; i++) {
        BitVecPush(&a, i % 2 == 0);
        BitVecPush(&b, i % 2 == 0);
    }
    bool err = true;
    u64  d   = BitVecEditDistanceWithError(&a, &b, &err);

    bool result = (d == 0) && (err == false);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1573:23 -- see report (EQUIVALENT). Pins BitVecCorrelation exact (+1 and -1).
bool test_blind_correlation_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 8; i++) {
        bool v = (i % 2 == 0);
        BitVecPush(&a, v);
        BitVecPush(&b, v);
    }
    double c      = BitVecCorrelation(&a, &b);
    bool   result = (c > 1.0 - 1e-9 && c < 1.0 + 1e-9);

    BitVec c1 = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec c2 = BitVecInit(ALLOCATOR_OF(&alloc));
    for (int i = 0; i < 8; i++) {
        BitVecPush(&c1, i % 2 == 0);
        BitVecPush(&c2, i % 2 == 1);
    }
    double c3 = BitVecCorrelation(&c1, &c2);
    result    = result && (c3 < -1.0 + 1e-9);

    BitVecDeinit(&c1);
    BitVecDeinit(&c2);
    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1635:9 / 1639:13 / 1638:33 / 1648:20 -- BitVecBestAlignment: best_offset init,
// per-offset score init, offset loop bound, overlap counter. A unique non-zero
// best offset pins them.
bool test_blind_best_alignment_offset(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    bool av[7]  = {false, false, true, true, false, true, false};
    bool bvv[4] = {true, true, false, true};
    for (int i = 0; i < 7; i++) {
        BitVecPush(&a, av[i]);
    }
    for (int i = 0; i < 4; i++) {
        BitVecPush(&b, bvv[i]);
    }
    u64  off    = BitVecBestAlignment(&a, &b);
    bool result = (off == 2);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Pins the best offset being the LAST overlapping one (offset loop bound /
// overlap counter).
bool test_blind_best_alignment_last_overlap(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           a     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           b     = BitVecInit(ALLOCATOR_OF(&alloc));

    bool av[5] = {false, false, false, true, true};
    for (int i = 0; i < 5; i++) {
        BitVecPush(&a, av[i]);
    }
    BitVecPush(&b, true);
    BitVecPush(&b, true); // matches a[3..4] at offset 3
    u64  off    = BitVecBestAlignment(&a, &b);
    bool result = (off == 3);

    BitVecDeinit(&a);
    BitVecDeinit(&b);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1712:37 -- see report (EQUIVALENT on count). Pins BitVecCountPattern.
bool test_blind_count_pattern_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           pat   = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 4; i++) {
        BitVecPush(&bv, true);
    }
    BitVecPush(&pat, true);
    BitVecPush(&pat, true);
    bool result = (BitVecCountPattern(&bv, &pat) == 3);

    BitVecDeinit(&pat);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1725:71 cxx_ge_to_gt -- BitVecRFindPattern `start >= bv->length` guard: a
// start == length must return SIZE_MAX. 1729:33 -- see report (EQUIVALENT).
// Also pins normal RFind positions.
bool test_blind_rfind_pattern_exact(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           pat   = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 6; i++) {
        BitVecPush(&bv, i % 2 == 1);
    }
    BitVecPush(&pat, false);
    BitVecPush(&pat, true);

    bool result = (BitVecRFindPattern(&bv, &pat, 5) == 4);
    result      = result && (BitVecRFindPattern(&bv, &pat, 3) == 2);
    result      = result && (BitVecRFindPattern(&bv, &pat, 6) == SIZE_MAX); // start==length

    BitVecDeinit(&pat);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1743:5 cxx_remove_void_call -- BitVecReplace validates bv.
bool test_blind_replace_validates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           o     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           n     = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVecReplace(NULL, &o, &n);
    return false;
}

// 1772:14 cxx_init_const -- BitVecReplaceAll `bool found = false` -> 42. With a
// truthy `found`, a no-match input would not break and would call
// RemoveRange(SIZE_MAX). Real code returns 0 replacements and leaves the vec
// intact. 1775:54 / 1778:31 -- see report.
bool test_blind_replace_all_no_match_zero(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           oldp  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           newp  = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 6; i++) {
        BitVecPush(&bv, false);
    }
    BitVecPush(&oldp, true);
    BitVecPush(&oldp, true);
    BitVecPush(&newp, false);

    u64  reps   = BitVecReplaceAll(&bv, &oldp, &newp);
    bool result = (reps == 0) && (BitVecLen(&bv) == 6);
    for (int i = 0; i < 6; i++) {
        result = result && (BitVecGet(&bv, i) == false);
    }

    BitVecDeinit(&newp);
    BitVecDeinit(&oldp);
    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// 1770:23 -- see report (EQUIVALENT). Pins ReplaceAll exact content.
bool test_blind_replace_all_content(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           oldp  = BitVecInit(ALLOCATOR_OF(&alloc));
    BitVec           newp  = BitVecInit(ALLOCATOR_OF(&alloc));

    for (int i = 0; i < 6; i++) {
        BitVecPush(&bv, i % 2 == 0); // 1 0 1 0 1 0
    }
    BitVecPush(&oldp, true);
    BitVecPush(&oldp, false);        // "10"
    BitVecPush(&newp, true);         // "1"

    u64  reps   = BitVecReplaceAll(&bv, &oldp, &newp);
    bool result = (reps == 3) && (BitVecLen(&bv) == 3);
    for (int i = 0; i < 3; i++) {
        result = result && (BitVecGet(&bv, i) == true);
    }

    BitVecDeinit(&newp);
    BitVecDeinit(&oldp);
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

// 1926:22 -- see report (EQUIVALENT). Pins that a healthy vec validates cleanly
// across repeated structural re-checks after a grow.
bool test_blind_structural_accepts_valid(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    BitVec           bv    = BitVecInit(ALLOCATOR_OF(&alloc));

    BitVecReserve(&bv, 100);
    BitVecResize(&bv, 50);
    bool result = true;
    for (int i = 0; i < 50; i++) {
        BitVecSet(&bv, i, i % 2 == 0);
    }
    result = result && (BitVecLen(&bv) == 50) && (BitVecCapacity(&bv) >= 100);

    BitVecDeinit(&bv);
    DefaultAllocatorDeinit(&alloc);
    return result;
}

// Rotate clones a temp internally and Deinits it on the success path. Routing a
// rotate through a DebugAllocator pins that the temp is freed (live count back
// to baseline) -- reinforces the rotate-temp lifecycle.
bool test_blind_rotate_no_leak(void) {
    DebugAllocator dbg  = blind_debug_alloc();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    BitVec bv = BitVecInit(adbg);
    for (int i = 0; i < 20; i++) {
        BitVecPush(&bv, i % 3 == 0);
    }
    u64 baseline = DebugAllocatorLiveCount(&dbg);

    BitVecRotateLeft(&bv, 5);
    BitVecRotateRight(&bv, 3);

    bool result = (DebugAllocatorLiveCount(&dbg) == baseline);

    BitVecDeinit(&bv);
    result = result && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return result;
}

int main(void) {
    WriteFmt("[INFO] Starting BitVec.Blind tests\n\n");

    TestFunction tests[] = {
        test_blind_sqrt_zero_guard_correlation,
        test_blind_sqrt_uses_x_cosine,
        test_blind_clear_zeroes_data,
        test_blind_resize_grow_zeroes,
        test_blind_resize_crosses_byte_boundary_zero,
        test_blind_reserve_zeroes_grown_bytes,
        test_blind_shrink_empty_capacity_zero,
        test_blind_shrink_capacity_equals_length_noop,
        test_blind_shrink_same_byte_fast_path,
        test_blind_clone_reserve_resize_succeeds,
        test_blind_push_growth_from_empty,
        test_blind_push_first_from_zero_capacity,
        test_blind_remove_range_middle_exact,
        test_blind_hash_seed_value,
        test_blind_hash_tail_mix_exact,
        test_blind_hash_multibyte_length_shift,
        test_blind_from_str_decode_exact,
        test_blind_from_str_str_decode,
        test_blind_from_str_str_wrapper_content,
        test_blind_to_bytes_exact,
        test_blind_from_bytes_decode_exact,
        test_blind_to_integer_exact,
        test_blind_from_integer_exact,
        test_blind_from_integer_clamps_to_64,
        test_blind_rotate_right_exact,
        test_blind_rotate_left_exact,
        test_blind_find_last_exact,
        test_blind_longest_run_exact,
        test_blind_find_pattern_at_end,
        test_blind_find_last_pattern_exact,
        test_blind_find_all_pattern_positions,
        test_blind_find_all_pattern_vec_positions,
        test_blind_jaccard_exact,
        test_blind_edit_distance_exact,
        test_blind_edit_distance_with_error_success,
        test_blind_correlation_exact,
        test_blind_best_alignment_offset,
        test_blind_best_alignment_last_overlap,
        test_blind_count_pattern_exact,
        test_blind_rfind_pattern_exact,
        test_blind_replace_all_no_match_zero,
        test_blind_replace_all_content,
        test_blind_structural_accepts_valid,
        test_blind_rotate_no_leak,
    };
    TestFunction deadend_tests[] = {
        test_blind_remove_at_length_aborts,
        test_blind_remove_first_validates,
        test_blind_remove_last_validates,
        test_blind_remove_all_validates,
        test_blind_and_validates_result,
        test_blind_or_validates_result,
        test_blind_xor_validates_result,
        test_blind_not_validates_result,
        test_blind_weight_compare_validates_bv1,
        test_blind_weight_compare_validates_bv2,
        test_blind_any_validates,
        test_blind_cosine_validates_bv1,
        test_blind_cosine_validates_bv2,
        test_blind_replace_validates,
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
