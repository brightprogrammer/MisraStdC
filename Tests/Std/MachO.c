// Mach-O parser unit test. Builds a minimal 64-bit Mach-O image in
// memory with one __TEXT segment, one symbol, and a UUID. Verifies
// the parser decodes the header, segment + section, LC_SYMTAB, and
// LC_UUID, and that MachoFileResolveAddress returns the right symbol
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

    MachoFile m;
    bool      ok = MachoFileOpenFromMemoryCopy(&m, blob, sizeof(blob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = m.cputype == 0x01000007u && m.filetype == MACHO_FILE_TYPE_EXECUTE;
    ok = ok && m.has_uuid && MemCompare(m.uuid, kUuid, 16) == 0;
    ok = ok && m.segments.length == 1;
    ok = ok && ZstrCompare(m.segments.data[0].name, "__TEXT") == 0;
    ok = ok && m.sections.length == 1;
    ok = ok && ZstrCompare(m.sections.data[0].section, "__text") == 0;
    ok = ok && ZstrCompare(m.sections.data[0].segment, "__TEXT") == 0;
    ok = ok && m.symbols.length == 1;
    ok = ok && m.symbols.data[0].name && ZstrCompare(m.symbols.data[0].name, "my_function") == 0;
    ok = ok && m.symbols.data[0].value == 0x100000010ull;

    MachoFileDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_macho_resolves_address(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_macho_blob();

    MachoFile m;
    if (!MachoFileOpenFromMemoryCopy(&m, blob, sizeof(blob), base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // Address at the function start.
    const MachoSymbol *s  = MachoFileResolveAddress(&m, 0x100000010ull);
    bool               ok = s && s->name && ZstrCompare(s->name, "my_function") == 0;

    // Address just past the start, still inside the function body.
    s  = MachoFileResolveAddress(&m, 0x100000020ull);
    ok = ok && s && ZstrCompare(s->name, "my_function") == 0;

    // Address below the symbol value: no match.
    s  = MachoFileResolveAddress(&m, 0x100000000ull);
    ok = ok && s == NULL;

    MachoFileDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_macho_rejects_fat_binary(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 fat[64];
    MemSet(fat, 0, sizeof(fat));
    wr_u32(&fat[0], 0xCAFEBABEu);

    MachoFile m;
    bool      ok = !MachoFileOpenFromMemoryCopy(&m, fat, sizeof(fat), base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

#if defined(__APPLE__)

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
    char         path[4096];
    unsigned int pathsize = sizeof(path);
    if (_NSGetExecutablePath(path, &pathsize) != 0)
        return false;

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    MachoFile m;
    if (!MachoFileOpen(&m, path, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    bool ok = m.filetype == MACHO_FILE_TYPE_EXECUTE;
    ok      = ok && m.has_uuid;
    ok      = ok && m.segments.length > 0;
    ok      = ok && MachoFileFindSection(&m, "__TEXT", "__text") != NULL;
    ok      = ok && m.symbols.length > 0;

    MachoFileDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Resolve a known function (this test function itself) by its runtime
// address. Validates that:
//   (a) the symbol table contains exported globals,
//   (b) `MachoFileResolveAddress` returns the correct entry after we
//       de-slide the runtime IP.
extern intptr_t _dyld_get_image_vmaddr_slide(uint32_t image_index);

bool test_macho_resolves_running_binary_symbol(void) {
    char         path[4096];
    unsigned int pathsize = sizeof(path);
    if (_NSGetExecutablePath(path, &pathsize) != 0)
        return false;

    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    MachoFile m;
    if (!MachoFileOpen(&m, path, base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    u64 slide        = (u64)_dyld_get_image_vmaddr_slide(0);
    u64 runtime_addr = (u64)(uintptr_t)&test_macho_resolves_running_binary_symbol;
    u64 vaddr        = runtime_addr - slide;

    const MachoSymbol *sym = MachoFileResolveAddress(&m, vaddr);
    bool               ok  = sym != NULL && sym->name != NULL &&
              ZstrFindSubstring(sym->name, "test_macho_resolves_running_binary_symbol") != NULL;

    MachoFileDeinit(&m);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

#endif // __APPLE__

int main(void) {
    WriteFmt("[INFO] Starting MachO tests\n\n");

    TestFunction tests[] = {
        test_macho_parses_synthetic_blob,
        test_macho_resolves_address,
        test_macho_rejects_fat_binary,
#if defined(__APPLE__)
        test_macho_parses_running_binary,
        test_macho_resolves_running_binary_symbol,
#endif
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "MachO");
}
