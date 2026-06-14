// MachoCache.Blind: targeted mutation-hardening for the four blind
// survivors in Source/Misra/Sys/MachoCache.c:
//
//   99:18:cxx_assign_const  e->dsym_open = true;
//  107:18:cxx_assign_const  e->fns_built = true;
//  118:15:cxx_init_const    u64 abbrev_n = abbrev_sec ? abbrev_sec->size : 0;
//  122:15:cxx_assign_const  e->fns_ok = DwarfFunctionsBuildFromSlices(...);
//
// All four live on the DWARF fall-through path: main + dSYM symtabs are
// empty and the function name lives only in the dSYM's __DWARF
// .debug_info. Resolving a known address there exercises dsym_open
// (99), fns_built (107), the abbrev slice size (118) and the fns_ok
// build result (122). The fixtures mirror the dSYM/DWARF blob idiom of
// the behavioural suite but are self-contained.

#include <Misra.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/File.h>
#include <Misra/Sys.h>
#include <Misra/Sys/MachoCache.h>

#include "../Util/TestRunner.h"

// -----------------------------------------------------------------------------
// little-endian writers
// -----------------------------------------------------------------------------

static void bl_wr_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}
static void bl_wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void bl_wr_u64(u8 *p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = (u8)((v >> (i * 8)) & 0xff);
}

static bool bl_write_file(Zstr path, const u8 *data, u64 size) {
    File f = FileOpen(path, "wb");
    if (!FileIsOpen(&f))
        return false;
    bool ok = FileWrite(&f, data, size) == (i64)size;
    FileClose(&f);
    return ok;
}

enum {
    BL_HDR_SIZE     = 32,
    BL_SEG64_HDR    = 72,
    BL_SECT64_SIZE  = 80,
    BL_SYMTAB_SIZE  = 24,
    BL_UUID_SIZE    = 24,
    BL_NLIST64_SIZE = 16,
    BL_BLOB_CAP     = 4096,
};

// -----------------------------------------------------------------------------
// Stripped main Mach-O (no symbols): forces the resolver past the main
// symtab onto the dSYM path.
// -----------------------------------------------------------------------------

static u64 bl_build_stripped_main(u8 *out, const u8 uuid[16]) {
    MemSet(out, 0, BL_BLOB_CAP);

    u32 seg_off      = BL_HDR_SIZE;
    u32 sym_cmd_off  = seg_off + BL_SEG64_HDR + BL_SECT64_SIZE;
    u32 uuid_cmd_off = sym_cmd_off + BL_SYMTAB_SIZE;
    u32 cmds_end     = uuid_cmd_off + BL_UUID_SIZE;
    u32 total        = cmds_end + 1;

    bl_wr_u32(&out[0], 0xFEEDFACFu);
    bl_wr_u32(&out[4], 0x01000007u);
    bl_wr_u32(&out[12], 0x2);
    bl_wr_u32(&out[16], 3);
    bl_wr_u32(&out[20], cmds_end - BL_HDR_SIZE);

    u8 *seg = &out[seg_off];
    bl_wr_u32(&seg[0], 0x19);
    bl_wr_u32(&seg[4], BL_SEG64_HDR + BL_SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    bl_wr_u64(&seg[24], 0x100000000ull);
    bl_wr_u64(&seg[32], 0x1000);
    bl_wr_u32(&seg[64], 1);
    u8        *sec          = &out[seg_off + BL_SEG64_HDR];
    const char sectname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sec[0], sectname, 16);
    MemCopy(&sec[16], tname, 16);
    bl_wr_u64(&sec[32], 0x100000000ull);
    bl_wr_u64(&sec[40], 0x1000);

    u8 *symc = &out[sym_cmd_off];
    bl_wr_u32(&symc[0], 0x2);
    bl_wr_u32(&symc[4], BL_SYMTAB_SIZE);
    bl_wr_u32(&symc[8], total);
    bl_wr_u32(&symc[12], 0);
    bl_wr_u32(&symc[16], total);
    bl_wr_u32(&symc[20], 1);

    u8 *uc = &out[uuid_cmd_off];
    bl_wr_u32(&uc[0], 0x1B);
    bl_wr_u32(&uc[4], BL_UUID_SIZE);
    MemCopy(&uc[8], uuid, 16);

    return total;
}

// -----------------------------------------------------------------------------
// DWARF .debug_info / .debug_abbrev for one DW_TAG_subprogram with
// name(strp) + low_pc(addr) + high_pc(data8).
// -----------------------------------------------------------------------------

enum {
    BL_DW_TAG_subprogram = 0x2e,
    BL_DW_AT_name        = 0x03,
    BL_DW_AT_low_pc      = 0x11,
    BL_DW_AT_high_pc     = 0x12,
    BL_DW_FORM_addr      = 0x01,
    BL_DW_FORM_data8     = 0x07,
    BL_DW_FORM_strp      = 0x0e,
};

static const u8 bl_kAbbrev[] = {
    0x01,
    BL_DW_TAG_subprogram,
    0x00,
    BL_DW_AT_name,
    BL_DW_FORM_strp,
    BL_DW_AT_low_pc,
    BL_DW_FORM_addr,
    BL_DW_AT_high_pc,
    BL_DW_FORM_data8,
    0x00,
    0x00,
    0x00,
};

static u64 bl_build_debug_info(u8 *buf, u64 low, u64 high_off, u32 name_strp) {
    u8 *p     = buf + 4;
    u8 *start = p;
    bl_wr_u16(p, 4);
    p += 2;
    bl_wr_u32(p, 0);
    p    += 4;
    *p++  = 8;
    *p++  = 0x01;
    bl_wr_u32(p, name_strp);
    p += 4;
    bl_wr_u64(p, low);
    p += 8;
    bl_wr_u64(p, high_off);
    p            += 8;
    *p++          = 0x00;
    u32 body_len  = (u32)(p - start);
    bl_wr_u32(buf, body_len);
    return (u64)(p - buf);
}

// Build a dSYM Mach-O carrying a __DWARF segment with __debug_info,
// __debug_abbrev and __debug_str. The abbrev table length is exactly
// sizeof(bl_kAbbrev) == 12, which is NOT 42, and it is immediately
// followed in the file by the __debug_str payload. Forcing abbrev_n to
// 42 (mutant 118) makes the parser read 30 bytes of __debug_str as
// abbrev entries, corrupting the abbrev table.
static u64 bl_build_dwarf_dsym(u8 *out, const u8 uuid[16], u64 low, u64 high_off, Zstr fn_name) {
    MemSet(out, 0, BL_BLOB_CAP);

    u32 text_seg_off  = BL_HDR_SIZE;
    u32 sym_cmd_off   = text_seg_off + BL_SEG64_HDR + BL_SECT64_SIZE;
    u32 uuid_cmd_off  = sym_cmd_off + BL_SYMTAB_SIZE;
    u32 dwarf_seg_off = uuid_cmd_off + BL_UUID_SIZE;
    u32 dwarf_seg_sz  = BL_SEG64_HDR + 3 * BL_SECT64_SIZE;
    u32 cmds_end      = dwarf_seg_off + dwarf_seg_sz;

    enum {
        BL_NAME_STRP = 50
    };
    u8 strbuf[256];
    MemSet(strbuf, 0, sizeof(strbuf));
    u64 name_len = ZstrLen(fn_name);
    MemCopy(&strbuf[BL_NAME_STRP], fn_name, name_len);
    strbuf[BL_NAME_STRP + name_len] = '\0';
    u64 str_n                       = BL_NAME_STRP + name_len + 1;

    u8  info[256];
    u64 info_n   = bl_build_debug_info(info, low, high_off, BL_NAME_STRP);
    u64 abbrev_n = sizeof(bl_kAbbrev);

    u32 info_off   = cmds_end;
    u32 abbrev_off = info_off + (u32)info_n;
    u32 str_off    = abbrev_off + (u32)abbrev_n;
    u32 total      = str_off + (u32)str_n;

    bl_wr_u32(&out[0], 0xFEEDFACFu);
    bl_wr_u32(&out[4], 0x01000007u);
    bl_wr_u32(&out[12], 0xA);
    bl_wr_u32(&out[16], 4);
    bl_wr_u32(&out[20], cmds_end - BL_HDR_SIZE);

    u8 *seg = &out[text_seg_off];
    bl_wr_u32(&seg[0], 0x19);
    bl_wr_u32(&seg[4], BL_SEG64_HDR + BL_SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    bl_wr_u64(&seg[24], 0x100000000ull);
    bl_wr_u64(&seg[32], 0x1000);
    bl_wr_u32(&seg[64], 1);
    u8        *tsec         = &out[text_seg_off + BL_SEG64_HDR];
    const char sectname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&tsec[0], sectname, 16);
    MemCopy(&tsec[16], tname, 16);
    bl_wr_u64(&tsec[32], 0x100000000ull);
    bl_wr_u64(&tsec[40], 0x1000);

    u8 *symc = &out[sym_cmd_off];
    bl_wr_u32(&symc[0], 0x2);
    bl_wr_u32(&symc[4], BL_SYMTAB_SIZE);
    bl_wr_u32(&symc[8], total);
    bl_wr_u32(&symc[12], 0);
    bl_wr_u32(&symc[16], total);
    bl_wr_u32(&symc[20], 1);

    u8 *uc = &out[uuid_cmd_off];
    bl_wr_u32(&uc[0], 0x1B);
    bl_wr_u32(&uc[4], BL_UUID_SIZE);
    MemCopy(&uc[8], uuid, 16);

    u8 *dseg = &out[dwarf_seg_off];
    bl_wr_u32(&dseg[0], 0x19);
    bl_wr_u32(&dseg[4], dwarf_seg_sz);
    const char dname[16] = {'_', '_', 'D', 'W', 'A', 'R', 'F', 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&dseg[8], dname, 16);
    bl_wr_u64(&dseg[24], 0);
    bl_wr_u64(&dseg[32], 0);
    bl_wr_u64(&dseg[40], info_off);
    bl_wr_u64(&dseg[48], (u64)total - info_off);
    bl_wr_u32(&dseg[64], 3);

    struct {
        const char *name;
        u32         off;
        u64         size;
    } dsects[3] = {
        {  "__debug_info",   info_off,   info_n},
        {"__debug_abbrev", abbrev_off, abbrev_n},
        {   "__debug_str",    str_off,    str_n},
    };
    for (u32 i = 0; i < 3; ++i) {
        u8 *s = &out[dwarf_seg_off + BL_SEG64_HDR + i * BL_SECT64_SIZE];
        MemSet(&s[0], 0, 16);
        u32 j = 0;
        while (dsects[i].name[j]) {
            s[j] = (u8)dsects[i].name[j];
            ++j;
        }
        MemCopy(&s[16], dname, 16);
        bl_wr_u64(&s[32], 0);
        bl_wr_u64(&s[40], dsects[i].size);
        bl_wr_u32(&s[48], dsects[i].off);
    }

    MemCopy(&out[info_off], info, info_n);
    MemCopy(&out[abbrev_off], bl_kAbbrev, abbrev_n);
    MemCopy(&out[str_off], strbuf, str_n);

    return total;
}

// -----------------------------------------------------------------------------
// fixtures
// -----------------------------------------------------------------------------

static const u8 bl_kUuid[16] =
    {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

static u8 bl_bin_buf[BL_BLOB_CAP];
static u8 bl_dsym_buf[BL_BLOB_CAP];

// -----------------------------------------------------------------------------
// Tests
// -----------------------------------------------------------------------------

// Primary DWARF fall-through: stripped main + dSYM with empty symtab; the
// name is reachable only through .debug_info. Drives dsym_open (99),
// fns_built (107), abbrev_n (118) and fns_ok (122) on the success path and
// asserts the resolved name + offset. A broken abbrev slice (118 -> 42)
// or a build that wrongly reports success (122) would corrupt or skip the
// DWARF function table, so the name/offset assertion fails.
bool test_bl_dwarf_resolves(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Zstr bin_path  = "/tmp/misra_bl_dwarf";
    Zstr dsym_path = "/tmp/misra_bl_dwarf.dSYM/Contents/Resources/DWARF/misra_bl_dwarf";
    DirCreateAll("/tmp/misra_bl_dwarf.dSYM/Contents/Resources/DWARF");

    Zstr      fn_name   = "dwarf_widget_function_with_long_name";
    const u64 low       = 0x100004000ull;
    const u64 high_off  = 0x80;
    u64       bin_size  = bl_build_stripped_main(bl_bin_buf, bl_kUuid);
    u64       dsym_size = bl_build_dwarf_dsym(bl_dsym_buf, bl_kUuid, low, high_off, fn_name);
    bool      ok        = bl_write_file(bin_path, bl_bin_buf, bin_size);
    ok                  = ok && bl_write_file(dsym_path, bl_dsym_buf, dsym_size);

    MachoCache cache    = MachoCacheInit(base);
    Zstr       name     = NULL;
    u32        offset   = 0;
    bool       resolved = MachoCacheResolve(&cache, bin_path, 0, low + 0x20, &name, &offset);
    ok                  = ok && resolved && name && ZstrCompare(name, fn_name) == 0 && offset == 0x20;

    MachoCacheDeinit(&cache);
    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_bl_dwarf.dSYM");
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// DWARF build memoisation: resolving twice on the same module must yield
// the same name both times. The first resolve sets fns_built (107) and
// fns_ok (122); the second is gated by the fns_built early-return
// (105/106 returning fns_ok). dsym_open (99) likewise gates the dSYM
// reuse. A success-path that fails to mark these built/open would re-run
// or skip the build and break the second resolve.
bool test_bl_dwarf_resolves_twice(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Zstr bin_path  = "/tmp/misra_bl_dwarf2";
    Zstr dsym_path = "/tmp/misra_bl_dwarf2.dSYM/Contents/Resources/DWARF/misra_bl_dwarf2";
    DirCreateAll("/tmp/misra_bl_dwarf2.dSYM/Contents/Resources/DWARF");

    Zstr      fn_name   = "second_pass_dwarf_routine";
    const u64 low       = 0x100008000ull;
    const u64 high_off  = 0x40;
    u64       bin_size  = bl_build_stripped_main(bl_bin_buf, bl_kUuid);
    u64       dsym_size = bl_build_dwarf_dsym(bl_dsym_buf, bl_kUuid, low, high_off, fn_name);
    bool      ok        = bl_write_file(bin_path, bl_bin_buf, bin_size);
    ok                  = ok && bl_write_file(dsym_path, bl_dsym_buf, dsym_size);

    MachoCache cache = MachoCacheInit(base);

    Zstr name1 = NULL;
    u32  off1  = 0;
    ok         = ok && MachoCacheResolve(&cache, bin_path, 0, low + 0x10, &name1, &off1);
    ok         = ok && name1 && ZstrCompare(name1, fn_name) == 0 && off1 == 0x10;

    // Second resolve hits the cached entry: dsym already open, dwarf already
    // built. Same answer expected.
    Zstr name2 = NULL;
    u32  off2  = 0;
    ok         = ok && MachoCacheResolve(&cache, bin_path, 0, low + 0x30, &name2, &off2);
    ok         = ok && name2 && ZstrCompare(name2, fn_name) == 0 && off2 == 0x30;
    ok         = ok && VecLen(&cache.entries) == 1;

    MachoCacheDeinit(&cache);
    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_bl_dwarf2.dSYM");
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// DWARF fall-through under a DebugAllocator with a live-count contract.
// Drives the same dsym_open (99), fns_built (107), abbrev_n (118) and
// fns_ok (122) success path, then asserts the post-deinit live count
// returns to the pre-cache baseline. fns_ok (122) gates the
// DwarfFunctionsDeinit at MachoCacheDeinit (135 `fns_built && fns_ok`):
// forcing fns_ok = 42 (truthy) when the build succeeds keeps the same
// path here, but pairing the resolve assertion with the no-leak baseline
// pins the build's success/failure result to an observable.
bool test_bl_dwarf_resolves_no_leak(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Zstr bin_path  = "/tmp/misra_bl_dwarf3";
    Zstr dsym_path = "/tmp/misra_bl_dwarf3.dSYM/Contents/Resources/DWARF/misra_bl_dwarf3";
    DirCreateAll("/tmp/misra_bl_dwarf3.dSYM/Contents/Resources/DWARF");

    Zstr      fn_name   = "dwarf_leakcheck_routine_name";
    const u64 low       = 0x10000c000ull;
    const u64 high_off  = 0x60;
    u64       bin_size  = bl_build_stripped_main(bl_bin_buf, bl_kUuid);
    u64       dsym_size = bl_build_dwarf_dsym(bl_dsym_buf, bl_kUuid, low, high_off, fn_name);
    bool      ok        = bl_write_file(bin_path, bl_bin_buf, bin_size);
    ok                  = ok && bl_write_file(dsym_path, bl_dsym_buf, dsym_size);

    size baseline = DebugAllocatorLiveCount(&dbg);

    MachoCache cache    = MachoCacheInit(base);
    Zstr       name     = NULL;
    u32        offset   = 0;
    bool       resolved = MachoCacheResolve(&cache, bin_path, 0, low + 0x14, &name, &offset);
    ok                  = ok && resolved && name && ZstrCompare(name, fn_name) == 0 && offset == 0x14;
    ok                  = ok && DebugAllocatorLiveCount(&dbg) > baseline;

    MachoCacheDeinit(&cache);
    ok = ok && DebugAllocatorLiveCount(&dbg) == baseline;

    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_bl_dwarf3.dSYM");
    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting MachoCache.Blind tests\n\n");

    TestFunction tests[] = {
        test_bl_dwarf_resolves,
        test_bl_dwarf_resolves_twice,
        test_bl_dwarf_resolves_no_leak,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "MachoCache.Blind");
}
