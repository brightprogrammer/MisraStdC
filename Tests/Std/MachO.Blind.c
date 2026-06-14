// Blind-spot mutation-hardening tests for Source/Misra/Parsers/MachO.c.
//
// Each test below targets a specific surviving mutant that the existing
// MachO.c / MachO.Mut.c suites do not detect. Tests pass on unmodified
// code and fail under the exact mutation they name.

#include <Misra.h>
#include <Misra/Parsers/MachO.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// Little-endian writers.
static void wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}

enum {
    HDR_SIZE    = 32,
    SEG64_HDR   = 72,
    SECT64_SIZE = 80,
    UUID_SIZE   = 24,

    MH_MAGIC_64   = 0xFEEDFACFu,
    LC_SEGMENT_64 = 0x19,
    LC_UUID       = 0x1B,
};

static const u8 kUuid[16] =
    {0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55};

// 287:24 cxx_assign_const -- `ctx->out->has_uuid = true;` -> `= 42`.
//
// `bool` is `i8` on this project (no _Bool normalization), so a mutated
// `has_uuid = 42` literally stores 42 in the public struct field. Every
// existing UUID test reads `has_uuid` only in a truthy context
// (`m.has_uuid && ...`), where 42 and 1 are indistinguishable. This test
// pins the field to its canonical boolean value: `has_uuid == true`
// (i.e. == 1) holds on real code and fails when the field is 42.
//
// The image is a minimal header + LC_UUID load command carrying kUuid.
bool test_mhb_has_uuid_is_canonical_true(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    enum {
        BUF = HDR_SIZE + UUID_SIZE
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));

    // mach_header_64: one load command (the LC_UUID), sizeofcmds = 24.
    wr_u32(&b[0], MH_MAGIC_64);
    wr_u32(&b[4], 0x01000007u); // cputype x86_64
    wr_u32(&b[8], 3);           // cpusubtype
    wr_u32(&b[12], 0x2);        // filetype MH_EXECUTE
    wr_u32(&b[16], 1);          // ncmds
    wr_u32(&b[20], UUID_SIZE);  // sizeofcmds
    wr_u32(&b[24], 0);
    wr_u32(&b[28], 0);

    // LC_UUID.
    u8 *uc = &b[HDR_SIZE];
    wr_u32(&uc[0], LC_UUID);
    wr_u32(&uc[4], UUID_SIZE);
    MemCopy(&uc[8], kUuid, 16);

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, b, BUF, base);
    // Canonical-true check: real code stores exactly `true` (== 1); the
    // mutant stores 42, so `== true` is false under the mutation.
    ok = ok && m.has_uuid == true && MemCompare(m.uuid, kUuid, 16) == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting MachO.Blind tests\n\n");

    TestFunction tests[] = {
        test_mhb_has_uuid_is_canonical_true,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "MachO.Blind");
}
