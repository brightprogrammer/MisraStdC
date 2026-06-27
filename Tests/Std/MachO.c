// Mach-O parser unit test. Builds a minimal 64-bit Mach-O image in
// memory with one __TEXT segment, one symbol, and a UUID. Verifies
// the parser decodes the header, segment + section, LC_SYMTAB, and
// LC_UUID, and that MachoResolveAddress returns the right symbol
// for an address inside the function body.

#include <Misra.h>
#include <Misra/Parsers/MachO.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Sys/Dir.h>

#include "../Util/TestRunner.h"

// Little-endian writers.
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

enum {
    HDR_SIZE     = 32,
    SEG64_HDR    = 72,
    SECT64_SIZE  = 80,
    SYMTAB_SIZE  = 24,
    UUID_SIZE    = 24,
    NLIST64_SIZE = 16,
    BLOB_SIZE    = 1024,
    SYM_OFF      = 0x100, // 256
    STR_OFF      = 0x110, // 272
    STR_SIZE     = 16,
};

// Mach-O command constants shared by the mutation-hardening builders.
enum {
    MH_MAGIC_64   = 0xFEEDFACFu,
    LC_SEGMENT_64 = 0x19,
    LC_SYMTAB     = 0x02,
    LC_UUID       = 0x1B,
};

// Fixed command layout used by the mutants1 scaffold builder:
//   [0x00] mach_header_64                                  (32)
//   [0x20] LC_SEGMENT_64 + one section_64                  (72 + 80 = 152)
//   [0xB8] LC_SYMTAB                                       (24)
//   [0xD0] LC_UUID                                         (24)
//   commands end at 0xE8 (232).
// nlist table and string table live further out, configurable.
enum {
    SEG_OFF      = HDR_SIZE,
    SYM_CMD_OFF  = SEG_OFF + SEG64_HDR + SECT64_SIZE, // 0xB8 = 184
    UUID_CMD_OFF = SYM_CMD_OFF + SYMTAB_SIZE,         // 0xD0 = 208
    CMDS_END     = UUID_CMD_OFF + UUID_SIZE,          // 0xE8 = 232
};

static u8       blob[BLOB_SIZE];
static const u8 kUuid[16] =
    {0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x11, 0x22, 0x33, 0x44, 0x55};

static void build_macho_blob(void) {
    MemSet(blob, 0, sizeof(blob));

    // --- Mach header (32 bytes) -------------------------------------------
    wr_u32(&blob[0], 0xFEEDFACFu); // magic = MH_MAGIC_64
    wr_u32(&blob[4], 0x01000007u); // cputype = x86_64
    wr_u32(&blob[8], 3);           // cpusubtype
    wr_u32(&blob[12], 0x2);        // filetype = MH_EXECUTE
    wr_u32(&blob[16], 3);          // ncmds
    wr_u32(&blob[20], SEG64_HDR + SECT64_SIZE + SYMTAB_SIZE + UUID_SIZE);
    wr_u32(&blob[24], 0);          // flags
    wr_u32(&blob[28], 0);          // reserved

    // --- LC_SEGMENT_64 (152 bytes: 72 hdr + 80 section) -------------------
    u32 seg_off = 32;
    u8 *seg     = &blob[seg_off];
    wr_u32(&seg[0], 0x19);            // cmd = LC_SEGMENT_64
    wr_u32(&seg[4], SEG64_HDR + SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);      // segname
    wr_u64(&seg[24], 0x100000000ull); // vmaddr
    wr_u64(&seg[32], 0x1000);         // vmsize
    wr_u64(&seg[40], 0);              // fileoff
    wr_u64(&seg[48], 0x1000);         // filesize
    wr_u32(&seg[56], 5);              // maxprot (r-x)
    wr_u32(&seg[60], 5);              // initprot
    wr_u32(&seg[64], 1);              // nsects
    wr_u32(&seg[68], 0);              // flags

    // Section header inside the segment command.
    u8        *sec          = &blob[seg_off + SEG64_HDR];
    const char sectname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sec[0], sectname, 16);   // sectname
    MemCopy(&sec[16], tname, 16);     // segname
    wr_u64(&sec[32], 0x100000000ull); // addr
    wr_u64(&sec[40], 0x100);          // size
    wr_u32(&sec[48], 0);              // offset
    wr_u32(&sec[52], 4);              // align
    wr_u32(&sec[64], 0x80000400u);    // flags = S_ATTR_PURE_INSTRUCTIONS | S_REGULAR

    // --- LC_SYMTAB (24 bytes) ---------------------------------------------
    u32 sym_cmd_off = seg_off + SEG64_HDR + SECT64_SIZE;
    u8 *symc        = &blob[sym_cmd_off];
    wr_u32(&symc[0], 0x2);       // cmd = LC_SYMTAB
    wr_u32(&symc[4], SYMTAB_SIZE);
    wr_u32(&symc[8], SYM_OFF);   // symoff
    wr_u32(&symc[12], 1);        // nsyms
    wr_u32(&symc[16], STR_OFF);  // stroff
    wr_u32(&symc[20], STR_SIZE); // strsize

    // --- LC_UUID (24 bytes) -----------------------------------------------
    u32 uuid_cmd_off = sym_cmd_off + SYMTAB_SIZE;
    u8 *uc           = &blob[uuid_cmd_off];
    wr_u32(&uc[0], 0x1B);
    wr_u32(&uc[4], UUID_SIZE);
    MemCopy(&uc[8], kUuid, 16);

    // --- Symbol table (nlist_64 * 1) at offset 256 ------------------------
    u8 *n = &blob[SYM_OFF];
    wr_u32(&n[0], 1);              // n_strx (skip leading NUL)
    n[4] = 0x0F;                   // n_type = N_SECT | N_EXT
    n[5] = 1;                      // n_sect (1-based)
    wr_u16(&n[6], 0);              // n_desc
    wr_u64(&n[8], 0x100000010ull); // n_value

    // --- String table at offset 272 ---------------------------------------
    u8 *strs        = &blob[STR_OFF];
    strs[0]         = '\0';
    const char nm[] = "my_function";
    MemCopy(&strs[1], nm, sizeof(nm));
}

// ---------------------------------------------------------------------------
// Mutation-hardening helpers (mutants1: symtab / uuid / find-section path).
// ---------------------------------------------------------------------------

// Knobs for the symtab + strtab portion.
typedef struct {
    u32 buf_size;
    u32 symoff;
    u32 nsyms;
    u32 stroff;
    u32 strsize;
} SymCfg;

// Write the common header / segment / section / uuid scaffolding into
// `b` (already zeroed, length >= CMDS_END) and the LC_SYMTAB command
// pointing at cfg. The caller fills the nlist table and string table.
static void build_scaffold(u8 *b, const SymCfg *cfg) {
    // mach_header_64
    wr_u32(&b[0], 0xFEEDFACFu);                                        // MH_MAGIC_64
    wr_u32(&b[4], 0x01000007u);                                        // cputype x86_64
    wr_u32(&b[8], 3);                                                  // cpusubtype
    wr_u32(&b[12], 0x2);                                               // filetype MH_EXECUTE
    wr_u32(&b[16], 3);                                                 // ncmds: segment, symtab, uuid
    wr_u32(&b[20], SEG64_HDR + SECT64_SIZE + SYMTAB_SIZE + UUID_SIZE); // sizeofcmds
    wr_u32(&b[24], 0);
    wr_u32(&b[28], 0);

    // LC_SEGMENT_64 + one __TEXT,__text section.
    u8 *seg = &b[SEG_OFF];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], SEG64_HDR + SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    wr_u64(&seg[24], 0x100000000ull); // vmaddr
    wr_u64(&seg[32], 0x1000);         // vmsize
    wr_u64(&seg[40], 0);              // fileoff
    wr_u64(&seg[48], 0x1000);         // filesize
    wr_u32(&seg[56], 5);
    wr_u32(&seg[60], 5);
    wr_u32(&seg[64], 1);              // nsects
    wr_u32(&seg[68], 0);

    u8        *sec          = &b[SEG_OFF + SEG64_HDR];
    const char sectname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sec[0], sectname, 16);   // sectname
    MemCopy(&sec[16], tname, 16);     // segname
    wr_u64(&sec[32], 0x100000000ull); // addr
    wr_u64(&sec[40], 0x100);          // size
    wr_u32(&sec[48], 0);              // offset
    wr_u32(&sec[52], 4);              // align
    wr_u32(&sec[64], 0x80000400u);    // flags

    // LC_SYMTAB
    u8 *symc = &b[SYM_CMD_OFF];
    wr_u32(&symc[0], LC_SYMTAB);
    wr_u32(&symc[4], SYMTAB_SIZE);
    wr_u32(&symc[8], cfg->symoff);
    wr_u32(&symc[12], cfg->nsyms);
    wr_u32(&symc[16], cfg->stroff);
    wr_u32(&symc[20], cfg->strsize);

    // LC_UUID
    u8 *uc = &b[UUID_CMD_OFF];
    wr_u32(&uc[0], LC_UUID);
    wr_u32(&uc[4], UUID_SIZE);
    MemCopy(&uc[8], kUuid, 16);
}

// Write a single nlist_64 at b[off].
static void wr_nlist(u8 *b, u32 off, u32 n_strx, u8 n_type, u8 n_sect, u64 n_value) {
    u8 *n = &b[off];
    wr_u32(&n[0], n_strx);
    n[4] = n_type;
    n[5] = n_sect;
    wr_u16(&n[6], 0);
    wr_u64(&n[8], n_value);
}

// Open helper. Returns true if opened; *out filled. Caller must
// MachoDeinit on success.
static bool open_blob(Macho *out, const u8 *bytes, u32 len, DefaultAllocator *alloc) {
    return MachoOpenFromMemoryCopy(out, bytes, len, ALLOCATOR_OF(alloc));
}

// ---------------------------------------------------------------------------
// Mutation-hardening helpers (mutants2: header / segment / resolve path).
// ---------------------------------------------------------------------------

// Write a minimal mach_header_64 into `p`. Caller fills ncmds /
// sizeofcmds appropriate to what follows.
static void put_header(u8 *p, u32 ncmds, u32 sizeofcmds) {
    wr_u32(&p[0], MH_MAGIC_64); // magic
    wr_u32(&p[4], 0x01000007u); // cputype = x86_64
    wr_u32(&p[8], 3);           // cpusubtype
    wr_u32(&p[12], 0x2);        // filetype = MH_EXECUTE
    wr_u32(&p[16], ncmds);      // ncmds
    wr_u32(&p[20], sizeofcmds); // sizeofcmds
    wr_u32(&p[24], 0);          // flags
    wr_u32(&p[28], 0);          // reserved
}

// Helper: build a one-segment image with `nsects` sections, each laid
// out so section i has addr = base_addr + i*0x1000 and size 0x100.
// Returns the total length written into `buf` (caller sizes buf big
// enough). The section names are "sectNN"/segment "__TEXT".
static u64 build_seg_with_sections(u8 *buf, u32 nsects) {
    u32 cmdsize = (u32)(SEG64_HDR + nsects * SECT64_SIZE);
    MemSet(buf, 0, HDR_SIZE + cmdsize);
    put_header(buf, 1, cmdsize);

    u8 *seg = &buf[HDR_SIZE];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], cmdsize);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    wr_u64(&seg[24], 0x100000000ull); // vmaddr
    wr_u64(&seg[32], 0x10000);        // vmsize
    wr_u32(&seg[64], nsects);         // nsects

    for (u32 i = 0; i < nsects; ++i) {
        u8        *sec    = &buf[HDR_SIZE + SEG64_HDR + i * SECT64_SIZE];
        const char sn[16] = {'_', '_', 's', 'e', 'c', 't', (char)('0' + (char)i), 0, 0, 0, 0, 0, 0, 0, 0, 0};
        MemCopy(&sec[0], sn, 16);                           // sectname
        MemCopy(&sec[16], tname, 16);                       // segname
        wr_u64(&sec[32], 0x100000000ull + (u64)i * 0x1000); // addr
        wr_u64(&sec[40], 0x100);                            // size
        wr_u32(&sec[48], 0);                                // offset
    }
    return HDR_SIZE + cmdsize;
}

// Build a one-__TEXT-segment image carrying `nsyms` nlist_64 symbols.
// Symbol i takes value `values[i]`, section_index 1, n_type N_SECT|N_EXT.
// The string table holds a single NUL + "f" so each symbol borrows a
// valid (non-empty) name. Layout: header, LC_SEGMENT_64 (1 section),
// LC_SYMTAB; symbols and strtab placed after the commands.
static u8 g_symblob[1024];

static u64 build_symbol_blob(const u64 *values, u32 nsyms) {
    MemSet(g_symblob, 0, sizeof(g_symblob));

    u32 seg_off    = HDR_SIZE;
    u32 seg_size   = SEG64_HDR + SECT64_SIZE;
    u32 sym_off    = seg_off + seg_size;
    u32 sizeofcmds = seg_size + SYMTAB_SIZE;

    // Symbol table / string table go after the load commands.
    u32 nlist_off = HDR_SIZE + sizeofcmds; // start of nlist_64 array
    u32 str_off   = nlist_off + nsyms * NLIST64_SIZE;
    u32 str_size  = 4;                     // "\0f\0" padded

    put_header(g_symblob, 2, sizeofcmds);  // 2 commands: SEGMENT_64 + SYMTAB

    // LC_SEGMENT_64 with one section spanning the symbol addresses.
    u8 *seg = &g_symblob[seg_off];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], seg_size);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    wr_u64(&seg[24], 0x100000000ull); // vmaddr
    wr_u64(&seg[32], 0x10000);        // vmsize
    wr_u32(&seg[64], 1);              // nsects

    u8        *sec       = &g_symblob[seg_off + SEG64_HDR];
    const char sname[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sec[0], sname, 16);
    MemCopy(&sec[16], tname, 16);
    wr_u64(&sec[32], 0x100000000ull); // addr
    wr_u64(&sec[40], 0x10000);        // size

    // LC_SYMTAB.
    u8 *symc = &g_symblob[sym_off];
    wr_u32(&symc[0], LC_SYMTAB);
    wr_u32(&symc[4], SYMTAB_SIZE);
    wr_u32(&symc[8], nlist_off); // symoff
    wr_u32(&symc[12], nsyms);    // nsyms
    wr_u32(&symc[16], str_off);  // stroff
    wr_u32(&symc[20], str_size); // strsize

    // nlist_64 array.
    for (u32 i = 0; i < nsyms; ++i) {
        u8 *n = &g_symblob[nlist_off + i * NLIST64_SIZE];
        wr_u32(&n[0], 1); // n_strx -> "f"
        n[4] = 0x0F;      // N_SECT | N_EXT
        n[5] = 1;         // n_sect (1-based)
        // n_desc (2 bytes @ offset 6) left zero by the MemSet above.
        wr_u64(&n[8], values[i]); // n_value
    }

    // String table: index 0 = NUL, index 1 = 'f', index 2 = NUL.
    u8 *strs = &g_symblob[str_off];
    strs[0]  = '\0';
    strs[1]  = 'f';
    strs[2]  = '\0';

    u64 total = (u64)str_off + str_size;
    return total;
}

bool test_macho_parses_synthetic_blob(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_macho_blob();

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, blob, sizeof(blob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = m.cputype == 0x01000007u && m.filetype == MACHO_FILE_TYPE_EXECUTE;
    ok = ok && m.has_uuid && MemCompare(m.uuid, kUuid, 16) == 0;
    ok = ok && VecLen(&m.segments) == 1;
    ok = ok && ZstrCompare(VecPtrAt(&m.segments, 0)->name, "__TEXT") == 0;
    ok = ok && VecLen(&m.sections) == 1;
    ok = ok && ZstrCompare(VecPtrAt(&m.sections, 0)->section, "__text") == 0;
    ok = ok && ZstrCompare(VecPtrAt(&m.sections, 0)->segment, "__TEXT") == 0;
    ok = ok && VecLen(&m.symbols) == 1;
    ok = ok && VecPtrAt(&m.symbols, 0)->name && ZstrCompare(VecPtrAt(&m.symbols, 0)->name, "my_function") == 0;
    ok = ok && VecPtrAt(&m.symbols, 0)->value == 0x100000010ull;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_macho_resolves_address(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_macho_blob();

    Macho m;
    if (!MachoOpenFromMemoryCopy(&m, blob, sizeof(blob), base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Address at the function start.
    const MachoSymbol *s  = MachoResolveAddress(&m, 0x100000010ull);
    bool               ok = s && s->name && ZstrCompare(s->name, "my_function") == 0;

    // Address just past the start, still inside the function body.
    s  = MachoResolveAddress(&m, 0x100000020ull);
    ok = ok && s && ZstrCompare(s->name, "my_function") == 0;

    // Address below the symbol value: no match.
    s  = MachoResolveAddress(&m, 0x100000000ull);
    ok = ok && s == NULL;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_macho_rejects_fat_binary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 fat[64];
    MemSet(fat, 0, sizeof(fat));
    wr_u32(&fat[0], 0xCAFEBABEu);

    Macho m;
    bool  ok = !MachoOpenFromMemoryCopy(&m, fat, sizeof(fat), base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// MachoFindSection finds the (segment, section) pair built into the
// synthetic blob and returns NULL for a pair that isn't present. The
// existing synthetic test never calls it (only the Darwin-only path
// does), so this pins the lookup contract on Linux too.
bool test_macho_find_section(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_macho_blob();

    Macho m;
    if (!MachoOpenFromMemoryCopy(&m, blob, sizeof(blob), base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    const MachoSection *hit = MachoFindSection(&m, "__TEXT", "__text");
    bool                ok  = hit != NULL && hit->addr == 0x100000000ull && hit->size == 0x100;
    // Right section name in the wrong segment -> no match.
    ok = ok && MachoFindSection(&m, "__DATA", "__text") == NULL;
    // Right segment, wrong section -> no match.
    ok = ok && MachoFindSection(&m, "__TEXT", "__nope") == NULL;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Helper: a malformed image must be rejected (returns false).
static bool macho_rejects(const u8 *bytes, u64 len) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Macho            m;
    bool             opened = MachoOpenFromMemoryCopy(&m, bytes, len, ALLOCATOR_OF(&alloc));
    if (opened)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return !opened;
}

// A buffer shorter than the 32-byte mach_header_64 is rejected, not
// read out of bounds.
bool test_macho_rejects_truncated_header(void) {
    build_macho_blob();
    return macho_rejects(blob, 16);
}

// 32-bit, byte-swapped, and arbitrary bad magics are all rejected (v1
// is 64-bit thin little-endian only).
bool test_macho_rejects_bad_magics(void) {
    u8 buf[64];

    MemSet(buf, 0, sizeof(buf));
    wr_u32(&buf[0], 0xFEEDFACEu); // MH_MAGIC_32
    bool ok = macho_rejects(buf, sizeof(buf));

    MemSet(buf, 0, sizeof(buf));
    wr_u32(&buf[0], 0xCFFAEDFEu); // MH_CIGAM_64 (byte-swapped 64-bit)
    ok = ok && macho_rejects(buf, sizeof(buf));

    MemSet(buf, 0, sizeof(buf));
    wr_u32(&buf[0], 0xDEADBEEFu); // not a Mach-O at all
    ok = ok && macho_rejects(buf, sizeof(buf));

    return ok;
}

// LC_SYMTAB whose symbol table runs past EOF is rejected.
bool test_macho_rejects_symtab_past_eof(void) {
    build_macho_blob();
    u8 bad[BLOB_SIZE];
    MemCopy(bad, blob, sizeof(bad));
    // The LC_SYMTAB command's symoff field lives at sym_cmd_off + 8.
    u32 sym_cmd_off = 32 + SEG64_HDR + SECT64_SIZE;
    wr_u32(&bad[sym_cmd_off + 8], (u32)sizeof(bad) + 0x1000); // symoff past EOF
    return macho_rejects(bad, sizeof(bad));
}

// Build a two-symbol __TEXT blob: first symbol at 0x...10 and second
// at 0x...40, so an address at/above the second is bounded out of the
// first symbol's span. Kept in a separate static so the single-symbol
// blob stays intact.
static u8 two_blob[BLOB_SIZE];

static void build_two_symbol_blob(void) {
    build_macho_blob();
    MemCopy(two_blob, blob, sizeof(two_blob));

    // Bump nsyms to 2 in the LC_SYMTAB command.
    u32 sym_cmd_off = 32 + SEG64_HDR + SECT64_SIZE;
    wr_u32(&two_blob[sym_cmd_off + 12], 2); // nsyms = 2

    // Second nlist_64 right after the first (first is at SYM_OFF).
    u8 *n2 = &two_blob[SYM_OFF + NLIST64_SIZE];
    wr_u32(&n2[0], 1);              // n_strx -> reuse "my_function" (name irrelevant here)
    n2[4] = 0x0F;                   // N_SECT | N_EXT
    n2[5] = 1;                      // n_sect
    wr_u16(&n2[6], 0);              // n_desc
    wr_u64(&n2[8], 0x100000040ull); // n_value (the "next" symbol)
}

// MachoResolveAddress bounds a match by the next symbol: an address at
// or above the next symbol's value does NOT resolve to the earlier
// symbol.
bool test_macho_resolve_bounded_by_next(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_two_symbol_blob();

    Macho m;
    if (!MachoOpenFromMemoryCopy(&m, two_blob, sizeof(two_blob), base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = VecLen(&m.symbols) == 2;

    // Inside the first symbol's span [0x10, 0x40): resolves to it.
    const MachoSymbol *s = MachoResolveAddress(&m, 0x100000010ull);
    ok                   = ok && s != NULL && s->value == 0x100000010ull;
    s                    = MachoResolveAddress(&m, 0x100000030ull);
    ok                   = ok && s != NULL && s->value == 0x100000010ull;

    // At the next symbol's value: resolves to the second symbol, not the
    // first.
    s  = MachoResolveAddress(&m, 0x100000040ull);
    ok = ok && s != NULL && s->value == 0x100000040ull;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

#if PLATFORM_DARWIN

// Forward-declare instead of `#include <mach-o/dyld.h>` to keep
// Misra's `bool = i8` invariant intact (system stdbool.h would
// otherwise `#define bool _Bool`).
extern int _NSGetExecutablePath(char *buf, unsigned int *bufsize);

// Open the currently running test binary as a Mach-O and verify its
// structure -- the Darwin counterpart of the Linux `Elf` test, which
// opens /proc/self/exe and round-trips through Parsers/Elf. Sanity
// checks: MH_MAGIC_64 (executable filetype), at least one segment,
// `__TEXT,__text` section present, LC_UUID present, LC_SYMTAB
// non-empty (debug builds aren't stripped).
bool test_macho_parses_running_binary(void) {
    char         path_buf[4096];
    unsigned int pathsize = sizeof(path_buf);
    if (_NSGetExecutablePath(path_buf, &pathsize) != 0)
        return false;
    // `_NSGetExecutablePath` wants `char *`; everything past this
    // point reads it through the project's `Zstr` (const char *).
    Zstr path = path_buf;

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Macho m;
    if (!MachoOpen(&m, path, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = m.filetype == MACHO_FILE_TYPE_EXECUTE;
    ok      = ok && m.has_uuid;
    ok      = ok && VecLen(&m.segments) > 0;
    ok      = ok && MachoFindSection(&m, "__TEXT", "__text") != NULL;
    ok      = ok && VecLen(&m.symbols) > 0;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Resolve a known function (this test function itself) by its runtime
// address. Validates that:
//   (a) the symbol table contains exported globals,
//   (b) `MachoResolveAddress` returns the correct entry after we
//       de-slide the runtime IP.
extern intptr_t _dyld_get_image_vmaddr_slide(uint32_t image_index);

bool test_macho_resolves_running_binary_symbol(void) {
    char         path_buf[4096];
    unsigned int pathsize = sizeof(path_buf);
    if (_NSGetExecutablePath(path_buf, &pathsize) != 0)
        return false;
    Zstr path = path_buf;

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Macho m;
    if (!MachoOpen(&m, path, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64 slide        = (u64)_dyld_get_image_vmaddr_slide(0);
    u64 runtime_addr = (u64)(uintptr_t)&test_macho_resolves_running_binary_symbol;
    u64 vaddr        = runtime_addr - slide;

    const MachoSymbol *sym = MachoResolveAddress(&m, vaddr);
    bool               ok  = sym != NULL && sym->name != NULL &&
              ZstrFindSubstring(sym->name, "test_macho_resolves_running_binary_symbol") != NULL;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

#endif // PLATFORM_DARWIN

// ===========================================================================
// Mutation-hardening tests (mutants1): symtab / uuid / find-section path.
// ===========================================================================

// A well-formed LC_SYMTAB with one NUL-terminated symbol must yield
// exactly one parsed symbol. If decode_symtab failed to record the
// symtab (have_symtab not set true), decode_symbols would early-return
// with zero symbols. (Pins 274 have_symtab = true; const-42 stays
// truthy so this is the structural anchor for the whole symtab path.)
bool test_mh1_symtab_one_symbol(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    SymCfg cfg = {.buf_size = BUF, .symoff = 0x100, .nsyms = 1, .stroff = 0x110, .strsize = 16};
    build_scaffold(b, &cfg);
    wr_nlist(b, cfg.symoff, /*n_strx*/ 1, /*n_type*/ 0x0F, /*n_sect*/ 1, /*n_value*/ 0x100000010ull);
    b[cfg.stroff + 0] = '\0';
    const char nm[]   = "fn_a";
    MemCopy(&b[cfg.stroff + 1], nm, sizeof(nm));

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.symbols) == 1;
    if (VecLen(&m.symbols) == 1)
        ok = ok && ZstrCompare(VecPtrAt(&m.symbols, 0)->name, "fn_a") == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// LC_UUID with a known 16-byte payload -> has_uuid true and the bytes
// decode exactly. (Pins 287 has_uuid = true; the byte-exact check also
// guards the MemCopy length/offset.)
bool test_mh1_uuid_exact_bytes(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    SymCfg cfg = {.buf_size = BUF, .symoff = 0x100, .nsyms = 0, .stroff = 0x110, .strsize = 0};
    build_scaffold(b, &cfg);

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && m.has_uuid && MemCompare(m.uuid, kUuid, 16) == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Two well-formed symbols with distinct names/values/types/sects parse
// to exactly two entries with the expected fields. Pins:
//   - the nlist loop bound (count == 2),
//   - n_strx offset arithmetic (each name correct),
//   - n_value 8-byte assembly (values exact),
//   - n_type / n_sect copy (405 section_index = n_sect, not 42).
bool test_mh1_two_symbols_fields(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 384
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    // strtab: \0 "alpha" \0 "beta" \0   (alpha at 1, beta at 7)
    u32    stroff  = 0x120;
    u32    strsize = 16;
    SymCfg cfg     = {.buf_size = BUF, .symoff = 0x100, .nsyms = 2, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    wr_nlist(b, cfg.symoff + 0 * NLIST64_SIZE, 1, 0x0F, 1, 0x100000010ull);
    wr_nlist(b, cfg.symoff + 1 * NLIST64_SIZE, 7, 0x24, 2, 0x100000040ull);
    b[stroff + 0] = '\0';
    MemCopy(&b[stroff + 1], "alpha", 6); // includes NUL at stroff+6
    MemCopy(&b[stroff + 7], "beta", 5);  // includes NUL at stroff+11

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.symbols) == 2;
    if (VecLen(&m.symbols) == 2) {
        const MachoSymbol *s0 = VecPtrAt(&m.symbols, 0);
        const MachoSymbol *s1 = VecPtrAt(&m.symbols, 1);
        ok = ok && ZstrCompare(s0->name, "alpha") == 0 && s0->value == 0x100000010ull && s0->type == 0x0F &&
             s0->section_index == 1;
        ok = ok && ZstrCompare(s1->name, "beta") == 0 && s1->value == 0x100000040ull && s1->type == 0x24 &&
             s1->section_index == 2;
    }
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// section_index must be n_sect, not a constant. A symbol with n_sect=5
// must parse to section_index == 5. (Kills 405 assign-const-42.)
bool test_mh1_symbol_section_index(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    SymCfg cfg = {.buf_size = BUF, .symoff = 0x100, .nsyms = 1, .stroff = 0x110, .strsize = 16};
    build_scaffold(b, &cfg);
    wr_nlist(b, cfg.symoff, 1, 0x0F, /*n_sect*/ 5, 0x100000010ull);
    b[cfg.stroff] = '\0';
    MemCopy(&b[cfg.stroff + 1], "z", 2);

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.symbols) == 1 && VecPtrAt(&m.symbols, 0)->section_index == 5;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// symoff + nsyms*16 == BufLength exactly: a `>` accepts (real), a `>=`
// rejects. Kills 361 gt_to_ge. The symtab must otherwise be valid so
// the real parser succeeds.
bool test_mh1_symtab_exact_fit_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    // Place strtab first, then the single nlist last so symoff+16 == BUF.
    // strtab at 0x100 (16 bytes), nlist at BUF-16.
    enum {
        BUF = 0x120
    }; // 288
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    u32    stroff  = 0x100;
    u32    strsize = 16;
    u32    symoff  = BUF - NLIST64_SIZE; // 0x110
    SymCfg cfg     = {.buf_size = BUF, .symoff = symoff, .nsyms = 1, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    wr_nlist(b, symoff, 1, 0x0F, 1, 0x100000010ull);
    b[stroff] = '\0';
    MemCopy(&b[stroff + 1], "fit", 4);

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc); // real: accepts
    ok       = ok && VecLen(&m.symbols) == 1 && ZstrCompare(VecPtrAt(&m.symbols, 0)->name, "fit") == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// stroff + strsize == BufLength exactly: `>` accepts (real), `>=`
// rejects. Kills 366 gt_to_ge.
bool test_mh1_strtab_exact_fit_accepted(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 0x120
    }; // 288
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    u32    symoff  = 0x100;         // nlist at 0x100..0x110
    u32    strsize = 16;
    u32    stroff  = BUF - strsize; // 0x110 -> str_end == BUF
    SymCfg cfg     = {.buf_size = BUF, .symoff = symoff, .nsyms = 1, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    wr_nlist(b, symoff, 1, 0x0F, 1, 0x100000010ull);
    b[stroff] = '\0';
    MemCopy(&b[stroff + 1], "edge", 5);

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.symbols) == 1 && ZstrCompare(VecPtrAt(&m.symbols, 0)->name, "edge") == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// tab_end = symoff + nsyms*16. With add->sub and nsyms*16 > symoff the
// subtraction underflows to a huge u64, so a valid symtab is wrongly
// rejected. Real accepts and parses nsyms symbols. Kills 360 add_to_sub.
// (nsyms*16 must exceed symoff to force the underflow.)
bool test_mh1_tab_end_addition(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    // symoff small-ish, nsyms large enough that nsyms*16 > symoff.
    // symoff=0xF0 (240); nsyms=17 -> 17*16=272 > 240.
    enum {
        NSY = 17
    };
    u32 symoff  = 0xF0;
    u32 tab_len = NSY * NLIST64_SIZE; // 272
    u32 stroff  = symoff + tab_len;   // strtab right after table
    u32 strsize = 16;
    u32 BUF     = stroff + strsize + 16;
    u8  b[0x300];                     // must cover BUF (=544 here); 0x200 overflowed
    MemSet(b, 0, sizeof(b));
    SymCfg cfg = {.buf_size = BUF, .symoff = symoff, .nsyms = NSY, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    for (u32 i = 0; i < NSY; ++i)
        wr_nlist(b, symoff + i * NLIST64_SIZE, 1, 0x0F, 1, 0x100000010ull + i);
    b[stroff] = '\0';
    MemCopy(&b[stroff + 1], "sym", 4);

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc); // real accepts
    ok       = ok && VecLen(&m.symbols) == NSY;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// str_end = stroff + strsize. The init-const-42 mutation drops the real
// computation; with a strtab that legitimately overruns the file the
// real parser rejects (str_end > BufLength) while the mutated str_end=42
// would accept. Build it so the real parser MUST reject, but keep a
// readable NUL-terminated name within the buffer so the only difference
// is the accept/reject decision. Kills 365 init_const and 366 boundary
// when str_end is real.
bool test_mh1_strtab_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    u32    symoff  = 0x100;
    u32    stroff  = 0x130; // within buffer
    u32    strsize = 0x100; // stroff+strsize = 0x230 = 560 > 320 -> overrun
    SymCfg cfg     = {.buf_size = BUF, .symoff = symoff, .nsyms = 1, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    wr_nlist(b, symoff, 1, 0x0F, 1, 0x100000010ull);
    // A name living within the buffer at stroff (so a buggy accept would
    // still find a NUL and parse one symbol -> distinguishable outcome).
    b[stroff] = '\0';
    MemCopy(&b[stroff + 1], "ovr", 4);

    Macho m;
    bool  opened = open_blob(&m, b, BUF, &alloc);
    if (opened)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return !opened; // real: rejected.
}

// symoff + nsyms*16 overruns the file -> rejected. Pins the tab_end
// bounds (361 / mul) on the reject side too.
bool test_mh1_symtab_overrun_rejected(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    u32    symoff  = 0x100;
    u32    strsize = 16;
    u32    stroff  = 0x110;
    SymCfg cfg     = {.buf_size = BUF, .symoff = symoff, .nsyms = 100, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg); // symoff + 100*16 = 0x100 + 0x640 >> 320 -> overrun

    Macho m;
    bool  opened = open_blob(&m, b, BUF, &alloc);
    if (opened)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return !opened;
}

// A symbol whose name region [n_strx, strsize) contains NO NUL must be
// SKIPPED (real). The mutations
//   - 392 `name_has_nul = false` -> 42 (truthy): would include it,
//   - 394 `== 0` -> `!= 0`      : first non-zero byte sets it true,
// both wrongly include the symbol. So count is 0 (real) vs 1 (mutant).
// The byte at offset `strsize` is non-zero so the 393 `<`->`<=`
// off-by-one does NOT accidentally find a NUL here.
bool test_mh1_name_without_nul_skipped(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    u32    symoff  = 0x100;
    u32    stroff  = 0x110;
    u32    strsize = 8;
    SymCfg cfg     = {.buf_size = BUF, .symoff = symoff, .nsyms = 1, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    wr_nlist(b, symoff, /*n_strx*/ 1, 0x0F, 1, 0x100000010ull);
    // strtab: byte0 anything, [1, strsize) all non-zero (no NUL), and
    // the byte at offset strsize also non-zero.
    for (u32 i = 0; i <= strsize; ++i)
        b[stroff + i] = 0x41 + (u8)i; // 'A','B',... all non-zero
    b[stroff] = 0x41;                 // ensure byte0 non-zero too

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.symbols) == 0; // real: symbol skipped
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 393 `p < strsize` -> `p <= strsize`: the name region [n_strx, strsize)
// has NO NUL, but the byte AT offset strsize is 0. Real (`<`) finds no
// NUL -> skips. Mutant (`<=`) reads str_base[strsize]==0 -> includes.
// So count 0 (real) vs 1 (mutant).
bool test_mh1_name_nul_scan_upper_bound(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    u32    symoff  = 0x100;
    u32    stroff  = 0x110;
    u32    strsize = 8;
    SymCfg cfg     = {.buf_size = BUF, .symoff = symoff, .nsyms = 1, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    wr_nlist(b, symoff, /*n_strx*/ 1, 0x0F, 1, 0x100000010ull);
    for (u32 i = 0; i < strsize; ++i)
        b[stroff + i] = 0x41 + (u8)i; // [0, strsize) all non-zero
    b[stroff + strsize] = 0x00;       // the off-by-one byte is NUL

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.symbols) == 0; // real: skipped (no NUL in range)
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// 393 `++p` -> `--p`: the NUL terminating the name sits AHEAD of n_strx
// (forward), while every byte from n_strx down to 0 is non-zero. Real
// (`++p`) scans forward, finds the NUL -> includes. Mutant (`--p`) scans
// backward to 0, finds no NUL, exits -> skips. Count 1 (real) vs 0.
bool test_mh1_name_nul_scan_forward(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    u32    symoff  = 0x100;
    u32    stroff  = 0x110;
    u32    strsize = 12;
    u32    n_strx  = 4;
    SymCfg cfg     = {.buf_size = BUF, .symoff = symoff, .nsyms = 1, .stroff = stroff, .strsize = strsize};
    build_scaffold(b, &cfg);
    wr_nlist(b, symoff, n_strx, 0x0F, 1, 0x100000010ull);
    // [0, n_strx] all non-zero (so a backward scan never finds a NUL),
    // a NUL ahead at n_strx+2.
    for (u32 i = 0; i <= n_strx; ++i)
        b[stroff + i] = 0x41 + (u8)i;
    b[stroff + n_strx + 1] = 0x5A; // 'Z'
    b[stroff + n_strx + 2] = 0x00; // forward NUL terminator

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.symbols) == 1; // real: forward scan finds NUL
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Found section returns correct addr/size/offset; absent name -> NULL.
// With one section in the vector, the `i < VecLen` loop must visit it
// (index 0) and stop. Pins the lookup result and the `++i` direction.
bool test_mh1_find_section_hit_and_miss(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF = 320
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));
    SymCfg cfg = {.buf_size = BUF, .symoff = 0x100, .nsyms = 0, .stroff = 0x110, .strsize = 0};
    build_scaffold(b, &cfg);

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    const MachoSection *hit = MachoFindSection(&m, "__TEXT", "__text");
    ok = ok && hit != NULL && hit->addr == 0x100000000ull && hit->size == 0x100 && hit->offset == 0;
    ok = ok && MachoFindSection(&m, "__DATA", "__text") == NULL;
    ok = ok && MachoFindSection(&m, "__TEXT", "__nope") == NULL;
    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Two sections in the same segment. The target lives at index 1, so a
// `++i`->`--i` mutation (which underflows after index 0 and exits) would
// fail to reach it -> returns NULL for a section that exists. Kills 487
// pre_inc_to_pre_dec. (487 lt_to_le's extra one-past read is an
// unobservable OOB and is left to the ledger.)
bool test_mh1_find_second_section(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    enum {
        BUF       = 512,
        SEG2_HDR  = SEG64_HDR,
        SECT_A    = SEG_OFF + SEG64_HDR,
        SECT_B    = SEG_OFF + SEG64_HDR + SECT64_SIZE,
        SEG2_SIZE = SEG64_HDR + 2 * SECT64_SIZE,
        SYM2_OFF  = SEG_OFF + SEG2_SIZE,
        UUID2_OFF = SYM2_OFF + SYMTAB_SIZE,
    };
    u8 b[BUF];
    MemSet(b, 0, sizeof(b));

    // mach_header_64
    wr_u32(&b[0], 0xFEEDFACFu);
    wr_u32(&b[4], 0x01000007u);
    wr_u32(&b[8], 3);
    wr_u32(&b[12], 0x2);
    wr_u32(&b[16], 3); // ncmds: segment, symtab, uuid
    wr_u32(&b[20], SEG2_SIZE + SYMTAB_SIZE + UUID_SIZE);

    // LC_SEGMENT_64 with two sections.
    u8 *seg = &b[SEG_OFF];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], SEG2_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    wr_u64(&seg[24], 0x100000000ull);
    wr_u64(&seg[32], 0x2000);
    wr_u64(&seg[40], 0);
    wr_u64(&seg[48], 0x2000);
    wr_u32(&seg[56], 5);
    wr_u32(&seg[60], 5);
    wr_u32(&seg[64], 2); // nsects = 2
    wr_u32(&seg[68], 0);

    // Section A (index 0): __TEXT,__text
    u8        *sa       = &b[SECT_A];
    const char sa_n[16] = {'_', '_', 't', 'e', 'x', 't', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sa[0], sa_n, 16);
    MemCopy(&sa[16], tname, 16);
    wr_u64(&sa[32], 0x100000000ull);
    wr_u64(&sa[40], 0x100);
    wr_u32(&sa[48], 0);

    // Section B (index 1): __TEXT,__cstring  <- the lookup target.
    u8        *sb       = &b[SECT_B];
    const char sb_n[16] = {'_', '_', 'c', 's', 't', 'r', 'i', 'n', 'g', 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&sb[0], sb_n, 16);
    MemCopy(&sb[16], tname, 16);
    wr_u64(&sb[32], 0x100000100ull);
    wr_u64(&sb[40], 0x80);
    wr_u32(&sb[48], 0x100);

    // LC_SYMTAB (empty) + LC_UUID so the rest of the parse is well-formed.
    u8 *symc = &b[SYM2_OFF];
    wr_u32(&symc[0], LC_SYMTAB);
    wr_u32(&symc[4], SYMTAB_SIZE);
    wr_u32(&symc[8], 0x200);
    wr_u32(&symc[12], 0);
    wr_u32(&symc[16], 0x210);
    wr_u32(&symc[20], 0);
    u8 *uc = &b[UUID2_OFF];
    wr_u32(&uc[0], LC_UUID);
    wr_u32(&uc[4], UUID_SIZE);
    MemCopy(&uc[8], kUuid, 16);

    Macho m;
    bool  ok = open_blob(&m, b, BUF, &alloc);
    ok       = ok && VecLen(&m.sections) == 2;
    // The target at index 1 must be found (real ++i reaches it).
    const MachoSection *hit = MachoFindSection(&m, "__TEXT", "__cstring");
    ok = ok && hit != NULL && hit->addr == 0x100000100ull && hit->size == 0x80 && hit->offset == 0x100;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// ===========================================================================
// Mutation-hardening tests (mutants2): header / segment / resolve / open.
// ===========================================================================

// A buffer of EXACTLY MH_HEADER_64_SIZE (32) bytes with a valid magic
// and zero load commands must open. Kills `BufLength < 32` -> `<= 32`
// (L120): the `<=` mutant rejects a 32-byte buffer.
bool test_mh2_header_min_exact_size(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[HDR_SIZE];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 0, 0); // ncmds = 0, sizeofcmds = 0

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, buf, sizeof(buf), base);
    ok       = ok && VecLen(&m.segments) == 0 && VecLen(&m.sections) == 0;
    ok       = ok && m.cputype == 0x01000007u && m.filetype == MACHO_FILE_TYPE_EXECUTE;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// LC_SEGMENT_64 with cmdsize == SEG64_CMD_SIZE_MIN (72) and nsects == 0
// must open: a segment header with no section table. Kills
// `cmdsize < 72` -> `<= 72` (L175): the `<=` mutant rejects the exact
// 72-byte minimal segment command.
bool test_mh2_segment_min_no_sections(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[HDR_SIZE + SEG64_HDR];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 1, SEG64_HDR);

    u8 *seg = &buf[HDR_SIZE];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], SEG64_HDR); // cmdsize == 72, exact minimum
    const char nm[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], nm, 16);
    wr_u64(&seg[24], 0x1000);   // vmaddr
    wr_u64(&seg[32], 0x1000);   // vmsize
    wr_u64(&seg[40], 0);        // fileoff
    wr_u64(&seg[48], 0x1000);   // filesize
    wr_u32(&seg[56], 5);        // maxprot
    wr_u32(&seg[60], 5);        // initprot
    wr_u32(&seg[64], 0);        // nsects == 0
    wr_u32(&seg[68], 0);        // flags

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, buf, sizeof(buf), base);
    ok       = ok && VecLen(&m.segments) == 1 && VecLen(&m.sections) == 0;
    ok       = ok && ZstrCompare(VecPtrAt(&m.segments, 0)->name, "__TEXT") == 0;
    ok       = ok && VecPtrAt(&m.segments, 0)->vmaddr == 0x1000;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A segment whose name fills all 16 bytes must be reported as exactly
// that 16-char string. Kills `seg.name[16] = '\0'` -> `= 42` (L186):
// without the NUL terminator at index 16 the decoded name reads past
// the 16 bytes and the compare fails.
bool test_mh2_segment_name_full16(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[HDR_SIZE + SEG64_HDR];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 1, SEG64_HDR);

    u8 *seg = &buf[HDR_SIZE];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], SEG64_HDR);
    // Exactly 16 non-NUL bytes -> the [16] terminator is the only thing
    // bounding the string.
    const char nm[16] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'};
    MemCopy(&seg[8], nm, 16);
    wr_u32(&seg[64], 0); // nsects == 0

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, buf, sizeof(buf), base);
    ok       = ok && VecLen(&m.segments) == 1;
    ok       = ok && ZstrCompare(VecPtrAt(&m.segments, 0)->name, "ABCDEFGHIJKLMNOP") == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A segment declaring nsects == 2 yields exactly two decoded sections.
// Kills the section loop `++i` -> `--i` (L212): the decrement makes the
// loop run a single iteration (i underflows after the first), so only
// one section would be decoded.
bool test_mh2_two_sections_count(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8  buf[HDR_SIZE + SEG64_HDR + 2 * SECT64_SIZE];
    u64 len = build_seg_with_sections(buf, 2);

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, buf, len, base);
    ok       = ok && VecLen(&m.segments) == 1 && VecLen(&m.sections) == 2;
    ok       = ok && VecPtrAt(&m.sections, 0)->addr == 0x100000000ull;
    ok       = ok && VecPtrAt(&m.sections, 1)->addr == 0x100001000ull;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A section whose sectname and segname each fill all 16 bytes must be
// reported as exactly those 16-char strings. Kills
// `sec.section[16] = '\0'` -> `= 42` (L223) and
// `sec.segment[16] = '\0'` -> `= 42` (L226): without the terminators
// the decoded names read past their 16 bytes and the compares fail.
bool test_mh2_section_names_full16(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[HDR_SIZE + SEG64_HDR + SECT64_SIZE];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 1, SEG64_HDR + SECT64_SIZE);

    u8 *seg = &buf[HDR_SIZE];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], SEG64_HDR + SECT64_SIZE);
    const char tname[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], tname, 16);
    wr_u32(&seg[64], 1); // nsects == 1

    u8 *sec = &buf[HDR_SIZE + SEG64_HDR];
    // 16 non-NUL bytes for sectname and 16 for segname; only the [16]
    // terminators bound them.
    const char sn[16] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'};
    const char gn[16] = {'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5'};
    MemCopy(&sec[0], sn, 16);
    MemCopy(&sec[16], gn, 16);
    wr_u64(&sec[32], 0x100000000ull); // addr
    wr_u64(&sec[40], 0x100);          // size

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, buf, sizeof(buf), base);
    ok       = ok && VecLen(&m.sections) == 1;
    ok       = ok && ZstrCompare(VecPtrAt(&m.sections, 0)->section, "abcdefghijklmnop") == 0;
    ok       = ok && ZstrCompare(VecPtrAt(&m.sections, 0)->segment, "QRSTUVWXYZ012345") == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A file whose length is EXACTLY MH_HEADER_64_SIZE + sizeofcmds must
// open. Kills `32 + sizeofcmds > BufLength` -> `>= BufLength` (L292):
// the `>=` mutant rejects the tightly-sized file.
bool test_mh2_loadcmds_tight_fit(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Header + one LC_SEGMENT_64 (72, nsects 0). Buffer length is
    // exactly 32 + 72 = 104.
    u8 buf[HDR_SIZE + SEG64_HDR];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 1, SEG64_HDR);

    u8 *seg = &buf[HDR_SIZE];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], SEG64_HDR);
    const char nm[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], nm, 16);
    wr_u32(&seg[64], 0); // nsects == 0

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, buf, sizeof(buf), base);
    ok       = ok && VecLen(&m.segments) == 1;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A load command with the minimum legal cmdsize of 8 (cmd + cmdsize
// only) at the end of the load-command region must be accepted and
// walked. Here the walker's remaining length is exactly 8 at that
// command. Kills both:
//   - `remaining < 8` -> `<= 8` (L305): the `<=` mutant rejects when
//     remaining == 8.
//   - `cmdsize < 8` -> `<= 8` (L316): the `<=` mutant rejects when
//     cmdsize == 8.
// The command's type is unrecognised, so it is skipped (no segment),
// but the file as a whole must still open.
bool test_mh2_minimal_cmdsize8(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 buf[HDR_SIZE + 8];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 1, 8); // one command, sizeofcmds == 8

    u8 *lc = &buf[HDR_SIZE];
    wr_u32(&lc[0], 0x99);  // unknown cmd type -> default (skip)
    wr_u32(&lc[4], 8);     // cmdsize == 8 (minimum); remaining == 8 here

    Macho m;
    bool  ok = MachoOpenFromMemoryCopy(&m, buf, sizeof(buf), base);
    ok       = ok && VecLen(&m.segments) == 0 && VecLen(&m.sections) == 0;
    if (ok)
        MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Two symbols, iterated in order [larger-value, smaller-value], both at
// or below the query address. The correct result is the symbol with the
// LARGEST value <= vaddr (the first one here). Kills
// `best_value = s->value` -> `best_value = 42` (L521): pinning
// best_value to 42 makes every later real-address symbol satisfy
// `s->value >= 42` and overwrite `best`, so the resolver would wrongly
// return the last-iterated (smaller-value) symbol.
bool test_mh2_resolve_picks_max_value(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // First-iterated symbol has the larger value.
    u64 values[2] = {0x100000040ull, 0x100000010ull};
    u64 len       = build_symbol_blob(values, 2);

    Macho m;
    if (!MachoOpenFromMemoryCopy(&m, g_symblob, len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = VecLen(&m.symbols) == 2;

    // Query above both; correct best is the 0x...40 symbol (max <= vaddr).
    const MachoSymbol *s = MachoResolveAddress(&m, 0x100000050ull);
    ok                   = ok && s != NULL && s->value == 0x100000040ull;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Two symbols with the SAME value but distinct names, iterated in
// order [first, second]. With `s->value >= best_value` the later equal
// symbol replaces the earlier, so the resolver returns the SECOND
// symbol. Kills `>=` -> `>` (L519): under `>` the equal second symbol
// would not replace the first, returning the wrong entry.
//
// Both symbols borrow the same one-char name "f", so we distinguish by
// section_index instead: give the two symbols different n_sect values
// (both non-zero) and assert the resolved entry is the second's.
bool test_mh2_resolve_equal_values_keeps_later(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u64 values[2] = {0x100000010ull, 0x100000010ull}; // equal values
    u64 len       = build_symbol_blob(values, 2);

    // Patch the two symbols' section_index to differ (1 vs 2). The
    // segment only declares one section, but section_index is only used
    // by the resolver as a NO_SECT (== 0) filter, so any non-zero value
    // is accepted; we use it as an identity tag.
    u32 nlist_off                               = HDR_SIZE + (SEG64_HDR + SECT64_SIZE) + SYMTAB_SIZE;
    g_symblob[nlist_off + 0 * NLIST64_SIZE + 5] = 1; // first  -> n_sect 1
    g_symblob[nlist_off + 1 * NLIST64_SIZE + 5] = 2; // second -> n_sect 2

    Macho m;
    if (!MachoOpenFromMemoryCopy(&m, g_symblob, len, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    bool ok = VecLen(&m.symbols) == 2;

    const MachoSymbol *s = MachoResolveAddress(&m, 0x100000020ull);
    // The later equal-valued symbol (n_sect 2) wins under `>=`.
    ok = ok && s != NULL && s->value == 0x100000010ull && s->section_index == 2;

    MachoDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// MachoOpen on a path that reads a VALID Mach-O image returns true.
// Kills `FileReadAndClose(...) < 0` -> `>= 0` (L466): the `>=` mutant
// treats a successful (>= 0) read as failure and returns false.
bool test_mh2_open_valid_file(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    // Mint a unique temp path, then write a minimal valid Mach-O to it.
    Str  path;
    File seed = FileOpenTemp(&path, base);
    if (!FileIsOpen(&seed)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    FileClose(&seed);

    u8 buf[HDR_SIZE + SEG64_HDR];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 1, SEG64_HDR);
    u8 *seg = &buf[HDR_SIZE];
    wr_u32(&seg[0], LC_SEGMENT_64);
    wr_u32(&seg[4], SEG64_HDR);
    const char nm[16] = {'_', '_', 'T', 'E', 'X', 'T', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    MemCopy(&seg[8], nm, 16);
    wr_u32(&seg[64], 0); // nsects == 0

    i64  wrote = FileWriteAndClose(&path, buf, sizeof(buf));
    bool ok    = (wrote == (i64)sizeof(buf));

    Macho m;
    bool  opened = MachoOpen(&m, &path, base);
    ok           = ok && opened && VecLen(&m.segments) == 1;
    if (opened)
        MachoDeinit(&m);

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// MachoOpen on a path that reads fine but holds NON-Mach-O bytes
// returns false. Kills the tail `return MachoOpenFromMemory(...)` being
// replaced by a truthy scalar constant (L471): the parse of garbage
// fails, so the real call returns false; the mutated `return 42` would
// wrongly report success.
bool test_mh2_open_invalid_file(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    Str  path;
    File seed = FileOpenTemp(&path, base);
    if (!FileIsOpen(&seed)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }
    FileClose(&seed);

    // Bad magic (not a Mach-O), but enough bytes to read successfully.
    u8 garbage[64];
    MemSet(garbage, 0, sizeof(garbage));
    wr_u32(&garbage[0], 0xDEADBEEFu);

    i64  wrote = FileWriteAndClose(&path, garbage, sizeof(garbage));
    bool ok    = (wrote == (i64)sizeof(garbage));

    Macho m;
    bool  opened = MachoOpen(&m, &path, base);
    if (opened)
        MachoDeinit(&m);
    ok = ok && !opened; // garbage must be rejected

    FileRemove(&path);
    StrDeinit(&path);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A parse that fails AFTER the buffer snapshot must free that buffer via
// MachoDeinit on the fail path -- no leak. We use a DebugAllocator and
// assert zero live allocations after a rejected open. Kills
// `MachoDeinit(out)` being removed from the fail path (L442): without it
// the snapshotted `data` Buf leaks and the live count stays non-zero.
//
// The image has a valid header but a load command whose cmdsize
// overruns the declared sizeofcmds bounds, so the walk fails after the
// buffer has been taken/allocated.
bool test_mh2_fail_path_frees_buffer(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *base = ALLOCATOR_OF(&dbg);

    // Header advertises sizeofcmds = 16 but the single command claims a
    // cmdsize that overruns the remaining region -> walk_load_commands
    // rejects after decode_header succeeds.
    u8 buf[HDR_SIZE + 16];
    MemSet(buf, 0, sizeof(buf));
    put_header(buf, 1, 16);
    u8 *lc = &buf[HDR_SIZE];
    wr_u32(&lc[0], LC_SEGMENT_64);
    wr_u32(&lc[4], 0x1000); // cmdsize far exceeds remaining -> reject

    Macho m;
    bool  opened = MachoOpenFromMemoryCopy(&m, buf, sizeof(buf), base);
    if (opened)
        MachoDeinit(&m);

    // The open must have failed AND left no live allocations behind.
    bool ok = !opened && DebugAllocatorLiveCount(&dbg) == 0;

    DebugAllocatorDeinit(&dbg);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting MachO tests\n\n");

    TestFunction tests[] = {
        test_macho_parses_synthetic_blob,
        test_macho_resolves_address,
        test_macho_rejects_fat_binary,
        test_macho_find_section,
        test_macho_rejects_truncated_header,
        test_macho_rejects_bad_magics,
        test_macho_rejects_symtab_past_eof,
        test_macho_resolve_bounded_by_next,
        test_mh1_symtab_one_symbol,
        test_mh1_uuid_exact_bytes,
        test_mh1_two_symbols_fields,
        test_mh1_symbol_section_index,
        test_mh1_symtab_exact_fit_accepted,
        test_mh1_strtab_exact_fit_accepted,
        test_mh1_tab_end_addition,
        test_mh1_strtab_overrun_rejected,
        test_mh1_symtab_overrun_rejected,
        test_mh1_name_without_nul_skipped,
        test_mh1_name_nul_scan_upper_bound,
        test_mh1_name_nul_scan_forward,
        test_mh1_find_section_hit_and_miss,
        test_mh1_find_second_section,
        test_mh2_header_min_exact_size,
        test_mh2_segment_min_no_sections,
        test_mh2_segment_name_full16,
        test_mh2_two_sections_count,
        test_mh2_section_names_full16,
        test_mh2_loadcmds_tight_fit,
        test_mh2_minimal_cmdsize8,
        test_mh2_resolve_picks_max_value,
        test_mh2_resolve_equal_values_keeps_later,
        test_mh2_open_valid_file,
        test_mh2_open_invalid_file,
        test_mh2_fail_path_frees_buffer,
#if PLATFORM_DARWIN
        test_macho_parses_running_binary,
        test_macho_resolves_running_binary_symbol,
#endif
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "MachO");
}
