// MachoCache end-to-end test. Builds a synthetic Mach-O binary plus
// a paired dSYM bundle (with a matching LC_UUID), writes both to
// /tmp, and verifies the cache:
//
//   1. Resolves an IP through the main file's LC_SYMTAB when present.
//   2. Falls through to the dSYM's LC_SYMTAB when the main symtab is
//      empty.
//
// The DWARF-only fallback path (no symtab in either file, names live
// only in .debug_info) is covered by Tests/Std/Dwarf.c via
// DwarfFunctionsBuildFromSlices.

#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/File.h>
#include <Misra/Sys.h>
#include <Misra/Sys/MachoCache.h>

#include "../Util/TestRunner.h"

// -----------------------------------------------------------------------------
// Common helpers
// -----------------------------------------------------------------------------

static void wr_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}
static void wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void wr_u64(u8 *p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = (u8)((v >> (i * 8)) & 0xff);
}

static bool write_file(Zstr path, const u8 *data, u64 size) {
    File f = FileOpen(path, "wb");
    if (!FileIsOpen(&f))
        return false;
    bool ok = FileWrite(&f, data, size) == (i64)size;
    FileClose(&f);
    return ok;
}

// -----------------------------------------------------------------------------
// Mach-O blob builder: configurable name + UUID + symbol presence
// -----------------------------------------------------------------------------

enum {
    HDR_SIZE     = 32,
    SEG64_HDR    = 72,
    SECT64_SIZE  = 80,
    SYMTAB_SIZE  = 24,
    UUID_SIZE    = 24,
    NLIST64_SIZE = 16,
    BLOB_CAP     = 2048,
};

// Build a Mach-O image with:
//   - one __TEXT segment + __text section (vmaddr 0x100000000),
//   - LC_UUID (always)
//   - LC_SYMTAB with N symbols (N may be 0; SYMTAB cmd is still present
//     so the parser exercises the cmd even with no entries).
//
// `syms` is an array of (vmaddr, name) pairs; `nsyms` is its length.
// Returns the total bytes written into `out`.
typedef struct SymSpec {
    u64  vmaddr;
    Zstr name;
} SymSpec;

static u64 build_macho_image(u8 *out, const u8 uuid[16], const SymSpec *syms, u32 nsyms) {
    MemSet(out, 0, BLOB_CAP);

    // Compute placement.
    u32 seg_off      = HDR_SIZE;
    u32 sym_cmd_off  = seg_off + SEG64_HDR + SECT64_SIZE;
    u32 uuid_cmd_off = sym_cmd_off + SYMTAB_SIZE;
    u32 cmds_end     = uuid_cmd_off + UUID_SIZE;
    u32 sym_off      = cmds_end;
    u32 str_off      = sym_off + nsyms * NLIST64_SIZE;
    // Build a tiny string table: leading NUL, then NUL-terminated names.
    u32 str_size = 1;
    for (u32 i = 0; i < nsyms; ++i) {
        Zstr s = syms[i].name;
        u32  n = 0;
        while (s[n])
            ++n;
        str_size += n + 1;
    }
    u32 total = str_off + str_size;

    // Mach header.
    wr_u32(&out[0], 0xFEEDFACFu);
    wr_u32(&out[4], 0x01000007u); // x86_64
    wr_u32(&out[12], 0x2);        // MH_EXECUTE
    wr_u32(&out[16], 3);          // ncmds
    wr_u32(&out[20], cmds_end - HDR_SIZE);

    // LC_SEGMENT_64 (__TEXT, __text)
    u8 *seg = &out[seg_off];
    wr_u32(&seg[0], 0x19);
    wr_u32(&seg[4], SEG64_HDR + SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    wr_u64(&seg[24], 0x100000000ull);
    wr_u64(&seg[32], 0x1000);
    wr_u64(&seg[40], 0);
    wr_u64(&seg[48], 0x1000);
    wr_u32(&seg[56], 5);
    wr_u32(&seg[60], 5);
    wr_u32(&seg[64], 1);
    wr_u32(&seg[68], 0);
    u8        *sec          = &out[seg_off + SEG64_HDR];
    const char sectname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sec[0], sectname, 16);
    MemCopy(&sec[16], tname, 16);
    wr_u64(&sec[32], 0x100000000ull);
    wr_u64(&sec[40], 0x1000);
    wr_u32(&sec[48], 0);

    // LC_SYMTAB
    u8 *symc = &out[sym_cmd_off];
    wr_u32(&symc[0], 0x2);
    wr_u32(&symc[4], SYMTAB_SIZE);
    wr_u32(&symc[8], sym_off);
    wr_u32(&symc[12], nsyms);
    wr_u32(&symc[16], str_off);
    wr_u32(&symc[20], str_size);

    // LC_UUID
    u8 *uc = &out[uuid_cmd_off];
    wr_u32(&uc[0], 0x1B);
    wr_u32(&uc[4], UUID_SIZE);
    MemCopy(&uc[8], uuid, 16);

    // Symbols + strings.
    u32 cur_strx = 1;
    for (u32 i = 0; i < nsyms; ++i) {
        u8 *n = &out[sym_off + i * NLIST64_SIZE];
        wr_u32(&n[0], cur_strx);
        n[4] = 0x0F; // N_SECT | N_EXT
        n[5] = 1;
        wr_u16(&n[6], 0);
        wr_u64(&n[8], syms[i].vmaddr);

        Zstr s    = syms[i].name;
        u32  nlen = 0;
        while (s[nlen])
            ++nlen;
        MemCopy(&out[str_off + cur_strx], s, nlen);
        out[str_off + cur_strx + nlen]  = '\0';
        cur_strx                       += nlen + 1;
    }
    out[str_off] = '\0';

    return total;
}

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

static const u8 kUuid[16] =
    {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

static u8 bin_buf[BLOB_CAP];
static u8 dsym_buf[BLOB_CAP];

bool test_macho_cache_resolves_via_main_symtab(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Zstr bin_path = "/tmp/misra_macho_main.bin";
    SymSpec sym      = {.vmaddr = 0x100000100ull, .name = "real_main_proc"};
    u64     bin_size = build_macho_image(bin_buf, kUuid, &sym, 1);
    if (!write_file(bin_path, bin_buf, bin_size)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    MachoCache cache = MachoCacheInit(base);

    // slide chosen so runtime_ip - slide = 0x100000110 (10 bytes past
    // function start)
    const u64 slide      = 0x100;
    const u64 runtime_ip = 0x100000100ull + 0x10 + slide;
    Zstr name       = NULL;
    u32       offset     = 0;
    bool      ok         = MachoCacheResolve(&cache, bin_path, slide, runtime_ip, &name, &offset);
    ok                   = ok && name && ZstrCompare(name, "real_main_proc") == 0 && offset == 0x10;

    MachoCacheDeinit(&cache);
    DefaultAllocatorDeinit(&alloc);
    FileRemove(bin_path);
    return ok;
}

bool test_macho_cache_falls_through_to_dsym(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Zstr bin_path  = "/tmp/misra_macho_stripped";
    Zstr dsym_dir  = "/tmp/misra_macho_stripped.dSYM/Contents/Resources/DWARF";
    Zstr dsym_path = "/tmp/misra_macho_stripped.dSYM/Contents/Resources/DWARF/misra_macho_stripped";

    // Make the dSYM bundle directory.
    DirCreateAll("/tmp/misra_macho_stripped.dSYM/Contents/Resources/DWARF");
    (void)dsym_dir;

    // Main binary: no symbols at all (stripped).
    u64 bin_size = build_macho_image(bin_buf, kUuid, NULL, 0);
    // dSYM: same UUID, one symbol.
    SymSpec dsym_sym  = {.vmaddr = 0x100000200ull, .name = "dsym_only_fn"};
    u64     dsym_size = build_macho_image(dsym_buf, kUuid, &dsym_sym, 1);
    if (!write_file(bin_path, bin_buf, bin_size) || !write_file(dsym_path, dsym_buf, dsym_size)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    MachoCache cache = MachoCacheInit(base);

    const u64 slide      = 0;
    const u64 runtime_ip = 0x100000208ull; // 8 bytes into dsym_only_fn
    Zstr name       = NULL;
    u32       offset     = 0;
    bool      ok         = MachoCacheResolve(&cache, bin_path, slide, runtime_ip, &name, &offset);
    ok                   = ok && name && ZstrCompare(name, "dsym_only_fn") == 0 && offset == 0x8;

    MachoCacheDeinit(&cache);
    DefaultAllocatorDeinit(&alloc);

    // Cleanup
    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_macho_stripped.dSYM");
    return ok;
}

bool test_macho_cache_rejects_uuid_mismatch(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Zstr bin_path  = "/tmp/misra_macho_uuidmiss";
    Zstr dsym_path = "/tmp/misra_macho_uuidmiss.dSYM/Contents/Resources/DWARF/misra_macho_uuidmiss";

    DirCreateAll("/tmp/misra_macho_uuidmiss.dSYM/Contents/Resources/DWARF");

    u8 bad_uuid[16];
    MemCopy(bad_uuid, kUuid, 16);
    bad_uuid[0] ^= 0xff;

    u64     bin_size  = build_macho_image(bin_buf, kUuid, NULL, 0);
    SymSpec sym       = {.vmaddr = 0x100000200ull, .name = "stale_fn"};
    u64     dsym_size = build_macho_image(dsym_buf, bad_uuid, &sym, 1);
    write_file(bin_path, bin_buf, bin_size);
    write_file(dsym_path, dsym_buf, dsym_size);

    MachoCache cache = MachoCacheInit(base);

    Zstr name = NULL;
    bool ok   = !MachoCacheResolve(&cache, bin_path, 0, 0x100000208ull, &name, NULL);

    MachoCacheDeinit(&cache);
    DefaultAllocatorDeinit(&alloc);

    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_macho_uuidmiss.dSYM");
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting MachoCache tests\n\n");

    TestFunction tests[] = {
        test_macho_cache_resolves_via_main_symtab,
        test_macho_cache_falls_through_to_dsym,
        test_macho_cache_rejects_uuid_mismatch,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "MachoCache");
}
