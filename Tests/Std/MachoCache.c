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
#include <Misra/Std/Allocator/Debug.h>
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

    Zstr    bin_path = "/tmp/misra_macho_main.bin";
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
    Zstr      name       = NULL;
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
    Zstr      name       = NULL;
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

// -----------------------------------------------------------------------------
// Mutation-hardening helpers (from MachoCache.Mutants1).
//
// Targets mull survivors in Source/Misra/Sys/MachoCache.c across
// cache_find_or_create, entry_open_dsym, entry_build_dwarf,
// MachoCacheDeinit, macho_cache_resolve_zstr and macho_cache_resolve_str.
// These mirror the dSYM-fixture idiom above but carry a DWARF blob builder
// and run under a DebugAllocator to catch leak/cleanup-removal mutants.
// -----------------------------------------------------------------------------

static void mc_wr_u16(u8 *p, u16 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
}
static void mc_wr_u32(u8 *p, u32 v) {
    p[0] = (u8)(v & 0xff);
    p[1] = (u8)(v >> 8);
    p[2] = (u8)(v >> 16);
    p[3] = (u8)(v >> 24);
}
static void mc_wr_u64(u8 *p, u64 v) {
    for (int i = 0; i < 8; ++i)
        p[i] = (u8)((v >> (i * 8)) & 0xff);
}

static bool mc_write_file(Zstr path, const u8 *data, u64 size) {
    File f = FileOpen(path, "wb");
    if (!FileIsOpen(&f))
        return false;
    bool ok = FileWrite(&f, data, size) == (i64)size;
    FileClose(&f);
    return ok;
}

// -----------------------------------------------------------------------------
// Mach-O blob builder (mirrors build_macho_image above): one __TEXT/__text
// section, LC_UUID, LC_SYMTAB with N symbols.
// -----------------------------------------------------------------------------

enum {
    MC_HDR_SIZE     = 32,
    MC_SEG64_HDR    = 72,
    MC_SECT64_SIZE  = 80,
    MC_SYMTAB_SIZE  = 24,
    MC_UUID_SIZE    = 24,
    MC_NLIST64_SIZE = 16,
    MC_BLOB_CAP     = 4096,
};

typedef struct McSymSpec {
    u64  vmaddr;
    Zstr name;
} McSymSpec;

static u64 mc_build_macho_image(u8 *out, const u8 uuid[16], const McSymSpec *syms, u32 nsyms) {
    MemSet(out, 0, MC_BLOB_CAP);

    u32 seg_off      = MC_HDR_SIZE;
    u32 sym_cmd_off  = seg_off + MC_SEG64_HDR + MC_SECT64_SIZE;
    u32 uuid_cmd_off = sym_cmd_off + MC_SYMTAB_SIZE;
    u32 cmds_end     = uuid_cmd_off + MC_UUID_SIZE;
    u32 sym_off      = cmds_end;
    u32 str_off      = sym_off + nsyms * MC_NLIST64_SIZE;
    u32 str_size     = 1;
    for (u32 i = 0; i < nsyms; ++i) {
        Zstr s = syms[i].name;
        u32  n = 0;
        while (s[n])
            ++n;
        str_size += n + 1;
    }
    u32 total = str_off + str_size;

    mc_wr_u32(&out[0], 0xFEEDFACFu);
    mc_wr_u32(&out[4], 0x01000007u);
    mc_wr_u32(&out[12], 0x2);
    mc_wr_u32(&out[16], 3);
    mc_wr_u32(&out[20], cmds_end - MC_HDR_SIZE);

    u8 *seg = &out[seg_off];
    mc_wr_u32(&seg[0], 0x19);
    mc_wr_u32(&seg[4], MC_SEG64_HDR + MC_SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    mc_wr_u64(&seg[24], 0x100000000ull);
    mc_wr_u64(&seg[32], 0x1000);
    mc_wr_u64(&seg[40], 0);
    mc_wr_u64(&seg[48], 0x1000);
    mc_wr_u32(&seg[56], 5);
    mc_wr_u32(&seg[60], 5);
    mc_wr_u32(&seg[64], 1);
    mc_wr_u32(&seg[68], 0);
    u8        *sec          = &out[seg_off + MC_SEG64_HDR];
    const char sectname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sec[0], sectname, 16);
    MemCopy(&sec[16], tname, 16);
    mc_wr_u64(&sec[32], 0x100000000ull);
    mc_wr_u64(&sec[40], 0x1000);
    mc_wr_u32(&sec[48], 0);

    u8 *symc = &out[sym_cmd_off];
    mc_wr_u32(&symc[0], 0x2);
    mc_wr_u32(&symc[4], MC_SYMTAB_SIZE);
    mc_wr_u32(&symc[8], sym_off);
    mc_wr_u32(&symc[12], nsyms);
    mc_wr_u32(&symc[16], str_off);
    mc_wr_u32(&symc[20], str_size);

    u8 *uc = &out[uuid_cmd_off];
    mc_wr_u32(&uc[0], 0x1B);
    mc_wr_u32(&uc[4], MC_UUID_SIZE);
    MemCopy(&uc[8], uuid, 16);

    u32 cur_strx = 1;
    for (u32 i = 0; i < nsyms; ++i) {
        u8 *n = &out[sym_off + i * MC_NLIST64_SIZE];
        mc_wr_u32(&n[0], cur_strx);
        n[4] = 0x0F;
        n[5] = 1;
        mc_wr_u16(&n[6], 0);
        mc_wr_u64(&n[8], syms[i].vmaddr);
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
// DWARF blobs (mirror Tests/Std/Dwarf.c). One-CU .debug_info describing a
// single DW_TAG_subprogram with name(string) + low_pc(addr) + high_pc(data8).
// -----------------------------------------------------------------------------

enum {
    MC_DW_TAG_subprogram = 0x2e,
    MC_DW_AT_name        = 0x03,
    MC_DW_AT_low_pc      = 0x11,
    MC_DW_AT_high_pc     = 0x12,
    MC_DW_FORM_addr      = 0x01,
    MC_DW_FORM_data8     = 0x07,
    MC_DW_FORM_string    = 0x08,
    MC_DW_FORM_strp      = 0x0e,
};

// Abbrev uses DW_FORM_strp for the name: the function name lives in
// .debug_str and is referenced by a 4-byte offset. This makes the
// .debug_str slice (str_b / str_n at lines 119/120) load-bearing.
static const u8 mc_kAbbrev[] = {
    0x01,
    MC_DW_TAG_subprogram,
    0x00,
    MC_DW_AT_name,
    MC_DW_FORM_strp,
    MC_DW_AT_low_pc,
    MC_DW_FORM_addr,
    MC_DW_AT_high_pc,
    MC_DW_FORM_data8,
    0x00,
    0x00,
    0x00,
};

// Build a one-CU .debug_info whose subprogram name is a strp offset into
// .debug_str. `name_strp` is that offset.
static u64 mc_build_debug_info(u8 *buf, u64 low, u64 high_off, u32 name_strp) {
    u8 *p     = buf + 4;
    u8 *start = p;
    mc_wr_u16(p, 4);         // version
    p += 2;
    mc_wr_u32(p, 0);         // abbrev_offset
    p    += 4;
    *p++  = 8;               // addr_size
    *p++  = 0x01;            // abbrev code 1
    mc_wr_u32(p, name_strp); // DW_FORM_strp: 4-byte .debug_str offset
    p += 4;
    mc_wr_u64(p, low);
    p += 8;
    mc_wr_u64(p, high_off);
    p            += 8;
    *p++          = 0x00;
    u32 body_len  = (u32)(p - start);
    mc_wr_u32(buf, body_len);
    return (u64)(p - buf);
}

// Build a dSYM Mach-O carrying a __DWARF segment with __debug_info,
// __debug_abbrev and __debug_str sections (no symtab entries -- so the
// resolver must fall through to the DWARF function table). The DWARF
// payloads live after the load commands; the section records point at
// their file offsets.
static u64 mc_build_dwarf_dsym(u8 *out, const u8 uuid[16], u64 low, u64 high_off, Zstr fn_name) {
    MemSet(out, 0, MC_BLOB_CAP);

    // ncmds: __TEXT segment (1 sect), LC_SYMTAB (0 syms), LC_UUID,
    // __DWARF segment (3 sects).
    u32 text_seg_off  = MC_HDR_SIZE;
    u32 sym_cmd_off   = text_seg_off + MC_SEG64_HDR + MC_SECT64_SIZE;
    u32 uuid_cmd_off  = sym_cmd_off + MC_SYMTAB_SIZE;
    u32 dwarf_seg_off = uuid_cmd_off + MC_UUID_SIZE;
    u32 dwarf_seg_sz  = MC_SEG64_HDR + 3 * MC_SECT64_SIZE;
    u32 cmds_end      = dwarf_seg_off + dwarf_seg_sz;

    // .debug_str: place the name at offset 50 (deliberately past 42).
    // The leading bytes are NUL padding. The section size is therefore
    // > 42, so the real str_n lets the name resolve while a mutated
    // str_n == 42 would reject the strp offset (50 < 42 is false).
    enum {
        MC_NAME_STRP = 50
    };
    u8 strbuf[256];
    MemSet(strbuf, 0, sizeof(strbuf));
    u64 name_len = ZstrLen(fn_name);
    MemCopy(&strbuf[MC_NAME_STRP], fn_name, name_len);
    strbuf[MC_NAME_STRP + name_len] = '\0';
    u64 str_n                       = MC_NAME_STRP + name_len + 1;

    // DWARF payload placement (after all commands).
    u8  info[256];
    u64 info_n   = mc_build_debug_info(info, low, high_off, MC_NAME_STRP);
    u64 abbrev_n = sizeof(mc_kAbbrev);

    u32 info_off   = cmds_end;
    u32 abbrev_off = info_off + (u32)info_n;
    u32 str_off    = abbrev_off + (u32)abbrev_n;
    u32 total      = str_off + (u32)str_n;

    // Mach header.
    mc_wr_u32(&out[0], 0xFEEDFACFu);
    mc_wr_u32(&out[4], 0x01000007u);
    mc_wr_u32(&out[12], 0xA); // MH_DSYM
    mc_wr_u32(&out[16], 4);   // ncmds
    mc_wr_u32(&out[20], cmds_end - MC_HDR_SIZE);

    // __TEXT segment + __text section.
    u8 *seg = &out[text_seg_off];
    mc_wr_u32(&seg[0], 0x19);
    mc_wr_u32(&seg[4], MC_SEG64_HDR + MC_SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    mc_wr_u64(&seg[24], 0x100000000ull);
    mc_wr_u64(&seg[32], 0x1000);
    mc_wr_u32(&seg[64], 1); // nsects
    u8        *tsec         = &out[text_seg_off + MC_SEG64_HDR];
    const char sectname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&tsec[0], sectname, 16);
    MemCopy(&tsec[16], tname, 16);
    mc_wr_u64(&tsec[32], 0x100000000ull);
    mc_wr_u64(&tsec[40], 0x1000);

    // LC_SYMTAB with zero symbols (cmd present, no entries).
    u8 *symc = &out[sym_cmd_off];
    mc_wr_u32(&symc[0], 0x2);
    mc_wr_u32(&symc[4], MC_SYMTAB_SIZE);
    mc_wr_u32(&symc[8], total);
    mc_wr_u32(&symc[12], 0);
    mc_wr_u32(&symc[16], total);
    mc_wr_u32(&symc[20], 1);

    // LC_UUID.
    u8 *uc = &out[uuid_cmd_off];
    mc_wr_u32(&uc[0], 0x1B);
    mc_wr_u32(&uc[4], MC_UUID_SIZE);
    MemCopy(&uc[8], uuid, 16);

    // __DWARF segment with 3 sections.
    u8 *dseg = &out[dwarf_seg_off];
    mc_wr_u32(&dseg[0], 0x19);
    mc_wr_u32(&dseg[4], dwarf_seg_sz);
    const char dname[16] = {'_', '_', 'D', 'W', 'A', 'R', 'F', 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&dseg[8], dname, 16);
    mc_wr_u64(&dseg[24], 0); // vmaddr
    mc_wr_u64(&dseg[32], 0); // vmsize
    mc_wr_u64(&dseg[40], info_off);
    mc_wr_u64(&dseg[48], (u64)total - info_off);
    mc_wr_u32(&dseg[64], 3); // nsects

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
        u8 *s = &out[dwarf_seg_off + MC_SEG64_HDR + i * MC_SECT64_SIZE];
        MemSet(&s[0], 0, 16);
        u32 j = 0;
        while (dsects[i].name[j]) {
            s[j] = (u8)dsects[i].name[j];
            ++j;
        }
        MemCopy(&s[16], dname, 16);
        mc_wr_u64(&s[32], 0);              // addr
        mc_wr_u64(&s[40], dsects[i].size); // size
        mc_wr_u32(&s[48], dsects[i].off);  // file offset
    }

    // DWARF payloads.
    MemCopy(&out[info_off], info, info_n);
    MemCopy(&out[abbrev_off], mc_kAbbrev, abbrev_n);
    MemCopy(&out[str_off], strbuf, str_n);

    return total;
}

// -----------------------------------------------------------------------------
// fixtures
// -----------------------------------------------------------------------------

static const u8 mc_kUuid[16] =
    {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

static u8 mc_bin_buf[MC_BLOB_CAP];
static u8 mc_dsym_buf[MC_BLOB_CAP];

// -----------------------------------------------------------------------------
// Mutation-hardening tests
// -----------------------------------------------------------------------------

// cache_find_or_create + resolve_zstr happy path with a main-symtab hit,
// driven under a DebugAllocator. After MachoCacheDeinit the live count
// must return to the pre-cache baseline -- this catches removal of the
// per-entry StrDeinit (54/141) and MachoDeinit (140) cleanups.
bool test_mc_main_symtab_no_leak(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Zstr      bin_path = "/tmp/misra_mc_main.bin";
    McSymSpec sym      = {.vmaddr = 0x100000100ull, .name = "real_main_proc"};
    u64       bin_size = mc_build_macho_image(mc_bin_buf, mc_kUuid, &sym, 1);
    bool      ok       = mc_write_file(bin_path, mc_bin_buf, bin_size);

    size baseline = DebugAllocatorLiveCount(&dbg);

    MachoCache cache      = MachoCacheInit(base);
    Zstr       name       = NULL;
    u32        offset     = 0;
    const u64  slide      = 0x100;
    const u64  runtime_ip = 0x100000100ull + 0x10 + slide;
    bool       resolved   = MachoCacheResolve(&cache, bin_path, slide, runtime_ip, &name, &offset);
    ok                    = ok && resolved && name && ZstrCompare(name, "real_main_proc") == 0 && offset == 0x10;

    // Cache now holds allocations; live count must have grown.
    ok = ok && DebugAllocatorLiveCount(&dbg) > baseline;

    MachoCacheDeinit(&cache);
    ok = ok && DebugAllocatorLiveCount(&dbg) == baseline;

    FileRemove(bin_path);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Same-module resolve twice => cache HIT: the 2nd resolve must NOT grow
// the entries vector or re-open the Mach-O. Distinct module => a new
// entry. Catches cache_find_or_create's loop index (40 ++->--) and key
// compare (42 ZstrCompare->42).
bool test_mc_cache_hit_and_distinct(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Zstr      bin_a = "/tmp/misra_mc_hit_a.bin";
    Zstr      bin_b = "/tmp/misra_mc_hit_b.bin";
    McSymSpec sym   = {.vmaddr = 0x100000100ull, .name = "fn_alpha"};
    u64       sz_a  = mc_build_macho_image(mc_bin_buf, mc_kUuid, &sym, 1);
    bool      ok    = mc_write_file(bin_a, mc_bin_buf, sz_a);
    u64       sz_b  = mc_build_macho_image(mc_dsym_buf, mc_kUuid, &sym, 1);
    ok              = ok && mc_write_file(bin_b, mc_dsym_buf, sz_b);

    size baseline = DebugAllocatorLiveCount(&dbg);

    MachoCache cache = MachoCacheInit(base);

    Zstr name = NULL;
    u32  off  = 0;

    // Resolve module A then module B: two distinct entries (0 and 1).
    ok             = ok && MachoCacheResolve(&cache, bin_a, 0, 0x100000110ull, &name, &off);
    ok             = ok && VecLen(&cache.entries) == 1;
    ok             = ok && MachoCacheResolve(&cache, bin_b, 0, 0x100000110ull, &name, &off);
    ok             = ok && VecLen(&cache.entries) == 2;
    size after_two = DebugAllocatorLiveCount(&dbg);

    // Re-resolve module B: its entry lives at index 1, so the find loop
    // must advance the index past 0 to hit it. A broken loop index
    // (40 ++->--) underflows after index 0 and never reaches entry 1,
    // forcing a spurious 3rd entry. A forced-equal key compare (42)
    // mis-matches entry 0 first. Either way the cache HIT breaks: the
    // entries vector grows and the live count rises.
    ok = ok && MachoCacheResolve(&cache, bin_b, 0, 0x100000110ull, &name, &off);
    ok = ok && VecLen(&cache.entries) == 2;
    ok = ok && DebugAllocatorLiveCount(&dbg) == after_two;

    // Re-resolve module A (index 0) too: still a hit, no growth.
    ok = ok && MachoCacheResolve(&cache, bin_a, 0, 0x100000110ull, &name, &off);
    ok = ok && VecLen(&cache.entries) == 2;
    ok = ok && DebugAllocatorLiveCount(&dbg) == after_two;

    // Two entries are live; MachoCacheDeinit must free BOTH and return to
    // baseline. A broken Deinit loop index (133 ++->--) underflows after
    // entry 0 and leaks entry 1.
    MachoCacheDeinit(&cache);
    ok = ok && DebugAllocatorLiveCount(&dbg) == baseline;

    FileRemove(bin_a);
    FileRemove(bin_b);
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// dSYM fall-through with no leak: main is stripped, dSYM carries the
// symbol. Drives entry_open_dsym's success path (StrDeinit at 92,
// MachoDeinit at 138) plus the main MachoDeinit (140). Asserts the live
// count returns to baseline after deinit.
bool test_mc_dsym_symtab_no_leak(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Zstr bin_path  = "/tmp/misra_mc_dsym";
    Zstr dsym_path = "/tmp/misra_mc_dsym.dSYM/Contents/Resources/DWARF/misra_mc_dsym";
    DirCreateAll("/tmp/misra_mc_dsym.dSYM/Contents/Resources/DWARF");

    u64       bin_size  = mc_build_macho_image(mc_bin_buf, mc_kUuid, NULL, 0);
    McSymSpec dsym_sym  = {.vmaddr = 0x100000200ull, .name = "dsym_only_fn"};
    u64       dsym_size = mc_build_macho_image(mc_dsym_buf, mc_kUuid, &dsym_sym, 1);
    bool      ok        = mc_write_file(bin_path, mc_bin_buf, bin_size);
    ok                  = ok && mc_write_file(dsym_path, mc_dsym_buf, dsym_size);

    size baseline = DebugAllocatorLiveCount(&dbg);

    MachoCache cache    = MachoCacheInit(base);
    Zstr       name     = NULL;
    u32        offset   = 0;
    bool       resolved = MachoCacheResolve(&cache, bin_path, 0, 0x100000208ull, &name, &offset);
    ok                  = ok && resolved && name && ZstrCompare(name, "dsym_only_fn") == 0 && offset == 0x8;
    ok                  = ok && DebugAllocatorLiveCount(&dbg) > baseline;

    MachoCacheDeinit(&cache);
    ok = ok && DebugAllocatorLiveCount(&dbg) == baseline;

    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_mc_dsym.dSYM");
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// UUID-mismatch rejection drives entry_open_dsym's error branch:
// MachoOpen succeeds but the UUID check fails, so the dSYM Macho is
// closed via MachoDeinit (96). Under DebugAllocator the failed-open
// path must not leak: the post-deinit live count returns to baseline.
bool test_mc_dsym_uuid_mismatch_no_leak(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Zstr bin_path  = "/tmp/misra_mc_uuidmiss";
    Zstr dsym_path = "/tmp/misra_mc_uuidmiss.dSYM/Contents/Resources/DWARF/misra_mc_uuidmiss";
    DirCreateAll("/tmp/misra_mc_uuidmiss.dSYM/Contents/Resources/DWARF");

    u8 bad_uuid[16];
    MemCopy(bad_uuid, mc_kUuid, 16);
    bad_uuid[0] ^= 0xff;

    u64       bin_size  = mc_build_macho_image(mc_bin_buf, mc_kUuid, NULL, 0);
    McSymSpec sym       = {.vmaddr = 0x100000200ull, .name = "stale_fn"};
    u64       dsym_size = mc_build_macho_image(mc_dsym_buf, bad_uuid, &sym, 1);
    bool      ok        = mc_write_file(bin_path, mc_bin_buf, bin_size);
    ok                  = ok && mc_write_file(dsym_path, mc_dsym_buf, dsym_size);

    size baseline = DebugAllocatorLiveCount(&dbg);

    MachoCache cache = MachoCacheInit(base);
    Zstr       name  = NULL;
    // Resolve must fail: stripped main + UUID-mismatched dSYM.
    ok = ok && !MachoCacheResolve(&cache, bin_path, 0, 0x100000208ull, &name, NULL);

    MachoCacheDeinit(&cache);
    ok = ok && DebugAllocatorLiveCount(&dbg) == baseline;

    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_mc_uuidmiss.dSYM");
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// DWARF fall-through: main + dSYM both have empty symtabs; the function
// name lives only in the dSYM's __DWARF .debug_info. Resolving a known
// address must return the DWARF function name and the correct offset.
// Catches entry_build_dwarf's section-size inits (116/120), the fns_ok
// assign (122), the build gate (190) and the offset arithmetic (195).
// Also drives DwarfFunctionsDeinit (136) on deinit; verified leak-free.
bool test_mc_dwarf_resolves_and_no_leak(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    Zstr bin_path  = "/tmp/misra_mc_dwarf";
    Zstr dsym_path = "/tmp/misra_mc_dwarf.dSYM/Contents/Resources/DWARF/misra_mc_dwarf";
    DirCreateAll("/tmp/misra_mc_dwarf.dSYM/Contents/Resources/DWARF");

    // Name length is chosen so the resulting .debug_info size is NOT 42
    // bytes -- otherwise the info_n->42 value-substitution would be a
    // no-op against this exact fixture.
    Zstr      fn_name   = "dwarf_widget_function_with_long_name";
    const u64 low       = 0x100004000ull;
    const u64 high_off  = 0x80;
    u64       bin_size  = mc_build_macho_image(mc_bin_buf, mc_kUuid, NULL, 0);
    u64       dsym_size = mc_build_dwarf_dsym(mc_dsym_buf, mc_kUuid, low, high_off, fn_name);
    bool      ok        = mc_write_file(bin_path, mc_bin_buf, bin_size);
    ok                  = ok && mc_write_file(dsym_path, mc_dsym_buf, dsym_size);

    size baseline = DebugAllocatorLiveCount(&dbg);

    MachoCache cache  = MachoCacheInit(base);
    Zstr       name   = NULL;
    u32        offset = 0;
    // file-relative VA low+0x20 -> 0x20 into the function.
    bool resolved = MachoCacheResolve(&cache, bin_path, 0, low + 0x20, &name, &offset);
    ok            = ok && resolved && name && ZstrCompare(name, fn_name) == 0 && offset == 0x20;
    ok            = ok && DebugAllocatorLiveCount(&dbg) > baseline;

    MachoCacheDeinit(&cache);
    ok = ok && DebugAllocatorLiveCount(&dbg) == baseline;

    FileRemove(bin_path);
    FileRemove(dsym_path);
    DirRemoveAll("/tmp/misra_mc_dwarf.dSYM");
    DebugAllocatorDeinit(&dbg);
    return ok;
}

// Boundary on `runtime_ip < slide` (166 lt_to_le). The fixture places a
// symbol at vmaddr 0, so when runtime_ip == slide the file-relative VA is
// 0 and must STILL resolve (real `<` lets it through). The mutant `<=`
// would reject the equal case and fail to resolve. Also covers the
// strictly-below case which must reject either way.
bool test_mc_slide_boundary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Zstr bin_path = "/tmp/misra_mc_slide.bin";
    // Symbol at vmaddr 0: resolves a file-relative VA of 0.
    McSymSpec sym      = {.vmaddr = 0x0ull, .name = "fn_at_zero"};
    u64       bin_size = mc_build_macho_image(mc_bin_buf, mc_kUuid, &sym, 1);
    bool      ok       = mc_write_file(bin_path, mc_bin_buf, bin_size);

    MachoCache cache = MachoCacheInit(base);
    Zstr       name  = NULL;
    u32        off   = 0;

    // runtime_ip strictly below slide -> reject (both `<` and `<=`).
    ok = ok && !MachoCacheResolve(&cache, bin_path, 0x200, 0x100, &name, &off);

    // runtime_ip == slide: file-relative VA 0 must resolve. The mutant
    // `runtime_ip <= slide` would short-circuit to false here.
    name         = NULL;
    off          = 0;
    const u64 eq = 0x9000;
    ok           = ok && MachoCacheResolve(&cache, bin_path, eq, eq, &name, &off);
    ok           = ok && name && ZstrCompare(name, "fn_at_zero") == 0 && off == 0;

    MachoCacheDeinit(&cache);
    FileRemove(bin_path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// macho_cache_resolve_str delegates to _zstr (215). The Str overload must
// resolve the same module + address as the Zstr overload.
bool test_mc_resolve_str_delegates(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Zstr      bin_path = "/tmp/misra_mc_str.bin";
    McSymSpec sym      = {.vmaddr = 0x100000100ull, .name = "str_fn"};
    u64       bin_size = mc_build_macho_image(mc_bin_buf, mc_kUuid, &sym, 1);
    bool      ok       = mc_write_file(bin_path, mc_bin_buf, bin_size);

    MachoCache cache = MachoCacheInit(base);

    Str mod = StrInit(base);
    StrPushBackMany(&mod, bin_path);

    Zstr name = NULL;
    u32  off  = 0;
    // Str overload -> macho_cache_resolve_str -> macho_cache_resolve_zstr.
    ok = ok && MachoCacheResolve(&cache, &mod, 0, 0x100000110ull, &name, &off);
    ok = ok && name && ZstrCompare(name, "str_fn") == 0 && off == 0x10;

    // Negative delegation: an address far outside any symbol must come
    // back false. If resolve_str's return is forced truthy (215), this
    // would wrongly report success.
    name = NULL;
    off  = 0;
    ok   = ok && !MachoCacheResolve(&cache, &mod, 0, 0xdeadbeefull, &name, &off);

    StrDeinit(&mod);
    MachoCacheDeinit(&cache);
    FileRemove(bin_path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting MachoCache tests\n\n");

    TestFunction tests[] = {
        test_macho_cache_resolves_via_main_symtab,
        test_macho_cache_falls_through_to_dsym,
        test_macho_cache_rejects_uuid_mismatch,
        test_mc_main_symtab_no_leak,
        test_mc_cache_hit_and_distinct,
        test_mc_dsym_symtab_no_leak,
        test_mc_dsym_uuid_mismatch_no_leak,
        test_mc_dwarf_resolves_and_no_leak,
        test_mc_slide_boundary,
        test_mc_resolve_str_delegates,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "MachoCache");
}
