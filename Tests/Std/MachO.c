// Mach-O parser unit test. Builds a minimal 64-bit Mach-O image in
// memory with one __TEXT segment, one symbol, and a UUID. Verifies
// the parser decodes the header, segment + section, LC_SYMTAB, and
// LC_UUID, and that MachoResolveAddress returns the right symbol
// for an address inside the function body.

#include <Misra.h>
#include <Misra/Parsers/MachO.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Memory.h>

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
#if PLATFORM_DARWIN
        test_macho_parses_running_binary,
        test_macho_resolves_running_binary_symbol,
#endif
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "MachO");
}
