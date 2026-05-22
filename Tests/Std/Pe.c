// PE/COFF parser unit test. Builds a synthetic PE32+ image in memory
// (just enough headers + one section + one CodeView debug record) and
// feeds it to PeOpenFromMemoryCopy. We assemble it byte-by-byte so
// the test runs on Linux without needing an actual Windows toolchain.
//
// The layout is documented inline below; offsets in comments are
// absolute within the blob and match the IMAGE_* structure field
// order from Microsoft's PE spec.

#include <Misra.h>
#include <Misra/Parsers/Pe.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Memory.h>

#include "../Util/TestRunner.h"

// Little-endian writers into a flat buffer.
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
    BLOB_SIZE         = 0x800,
    DOS_E_LFANEW_OFF  = 0x3C,
    NT_OFF            = 0x80,
    FILE_HDR_OFF      = NT_OFF + 4, // after "PE\0\0"
    OPT_HDR_OFF       = FILE_HDR_OFF + 20,
    OPT_HDR_SIZE_PEPP = 240,        // 24 fields + 16 data dirs
    SECTION_TBL_OFF   = OPT_HDR_OFF + OPT_HDR_SIZE_PEPP,
    DEBUG_RAW_OFF     = 0x400,
    DEBUG_DIR_RVA     = 0x1000,
    DEBUG_DIR_SIZE    = 28,
    CV_REC_RAW_OFF    = 0x420,
    CV_REC_RVA        = 0x1020,
    SECTION_VA        = 0x1000,
    SECTION_VSIZE     = 0x200,
    SECTION_RAW_SIZE  = 0x100,
};

static u8       blob[BLOB_SIZE];
static const u8 kGuid[16] =
    {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
static Zstr const kPdbPath = "C:\\build\\test.pdb";

static void build_pe_blob(void) {
    MemSet(blob, 0, sizeof(blob));

    // --- DOS header --------------------------------------------------------
    blob[0] = 'M';
    blob[1] = 'Z';
    wr_u32(&blob[DOS_E_LFANEW_OFF], NT_OFF);

    // --- NT signature ------------------------------------------------------
    blob[NT_OFF + 0] = 'P';
    blob[NT_OFF + 1] = 'E';
    blob[NT_OFF + 2] = 0;
    blob[NT_OFF + 3] = 0;

    // --- IMAGE_FILE_HEADER (20 bytes) --------------------------------------
    wr_u16(&blob[FILE_HDR_OFF + 0], 0x8664);             // Machine = x86-64
    wr_u16(&blob[FILE_HDR_OFF + 2], 1);                  // NumberOfSections
    wr_u32(&blob[FILE_HDR_OFF + 4], 0);                  // TimeDateStamp
    wr_u32(&blob[FILE_HDR_OFF + 8], 0);                  // PointerToSymbolTable
    wr_u32(&blob[FILE_HDR_OFF + 12], 0);                 // NumberOfSymbols
    wr_u16(&blob[FILE_HDR_OFF + 16], OPT_HDR_SIZE_PEPP); // SizeOfOptionalHeader
    wr_u16(&blob[FILE_HDR_OFF + 18], 0x2022);            // Characteristics

    // --- IMAGE_OPTIONAL_HEADER64 (240 bytes for 16 dirs) -------------------
    u8 *opt = &blob[OPT_HDR_OFF];
    wr_u16(&opt[0], 0x20B);   // Magic = PE32+
    opt[2] = 14;              // LinkerMajor
    opt[3] = 0;               // LinkerMinor
    wr_u32(&opt[4], 0x100);   // SizeOfCode
    wr_u32(&opt[8], 0);       // SizeOfInitData
    wr_u32(&opt[12], 0);      // SizeOfUninitData
    wr_u32(&opt[16], 0x1000); // AddressOfEntryPoint (RVA)
    wr_u32(&opt[20], 0x1000); // BaseOfCode
    // (no BaseOfData on PE32+)
    wr_u64(&opt[24], 0x140000000ull); // ImageBase
    wr_u32(&opt[32], 0x1000);         // SectionAlignment
    wr_u32(&opt[36], 0x200);          // FileAlignment
    wr_u16(&opt[40], 6);              // OSMajor
    wr_u16(&opt[42], 0);              // OSMinor
    wr_u16(&opt[44], 0);              // ImgMajor
    wr_u16(&opt[46], 0);              // ImgMinor
    wr_u16(&opt[48], 6);              // SubsysMajor
    wr_u16(&opt[50], 0);              // SubsysMinor
    wr_u32(&opt[52], 0);              // Win32Version
    wr_u32(&opt[56], 0x10000);        // SizeOfImage
    wr_u32(&opt[60], 0x400);          // SizeOfHeaders
    wr_u32(&opt[64], 0);              // CheckSum
    wr_u16(&opt[68], 3);              // Subsystem
    wr_u16(&opt[70], 0x8160);         // DllCharacteristics
    wr_u64(&opt[72], 0x100000);       // SizeOfStackReserve
    wr_u64(&opt[80], 0x1000);         // SizeOfStackCommit
    wr_u64(&opt[88], 0x100000);       // SizeOfHeapReserve
    wr_u64(&opt[96], 0x1000);         // SizeOfHeapCommit
    wr_u32(&opt[104], 0);             // LoaderFlags
    wr_u32(&opt[108], 16);            // NumberOfRvaAndSizes
    // 16 data dirs follow at offset 112: 16 * 8 = 128 bytes ends at 240.
    // DataDirectory[6] = DEBUG.
    wr_u32(&opt[112 + 6 * 8 + 0], DEBUG_DIR_RVA);
    wr_u32(&opt[112 + 6 * 8 + 4], DEBUG_DIR_SIZE);

    // --- Section header (40 bytes): a single ".debug" section --------------
    u8        *sec   = &blob[SECTION_TBL_OFF];
    const char nm[8] = {'.', 'd', 'e', 'b', 'u', 'g', 0, 0};
    MemCopy(sec, nm, 8);
    wr_u32(&sec[8], SECTION_VSIZE);
    wr_u32(&sec[12], SECTION_VA);
    wr_u32(&sec[16], SECTION_RAW_SIZE);
    wr_u32(&sec[20], DEBUG_RAW_OFF);
    wr_u32(&sec[24], 0);           // ptr_relocs
    wr_u32(&sec[28], 0);           // ptr_linenums
    wr_u16(&sec[32], 0);           // num_relocs
    wr_u16(&sec[34], 0);           // num_linenums
    wr_u32(&sec[36], 0x40000040u); // CNT_INITIALIZED_DATA | MEM_READ

    // --- Debug directory entry (28 bytes) ----------------------------------
    u8 *dbg = &blob[DEBUG_RAW_OFF];
    wr_u32(&dbg[0], 0);                // Characteristics
    wr_u32(&dbg[4], 0);                // TimeDateStamp
    wr_u16(&dbg[8], 0);                // MajorVersion
    wr_u16(&dbg[10], 0);               // MinorVersion
    wr_u32(&dbg[12], 2);               // Type = IMAGE_DEBUG_TYPE_CODEVIEW
    wr_u32(&dbg[16], 4 + 16 + 4 + 18); // SizeOfData (RSDS + GUID + age + "C:\build\test.pdb\0")
    wr_u32(&dbg[20], CV_REC_RVA);      // AddressOfRawData
    wr_u32(&dbg[24], CV_REC_RAW_OFF);  // PointerToRawData

    // --- CodeView (RSDS) record --------------------------------------------
    u8 *cv = &blob[CV_REC_RAW_OFF];
    cv[0]  = 'R';
    cv[1]  = 'S';
    cv[2]  = 'D';
    cv[3]  = 'S';
    MemCopy(&cv[4], kGuid, 16);
    wr_u32(&cv[20], 0x0000002a); // Age
    // PDB path, NUL-terminated.
    u64 path_len = ZstrLen(kPdbPath);
    MemCopy(&cv[24], kPdbPath, path_len + 1);
}

bool test_pe_parses_synthetic_blob(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_pe_blob();

    Pe   pe;
    bool ok = PeOpenFromMemoryCopy(&pe, blob, sizeof(blob), base);
    if (!ok) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    ok = pe.machine == PE_MACHINE_X86_64 && pe.is_pe32_plus && pe.image_base == 0x140000000ull;
    ok = ok && pe.sections.length == 1;
    ok = ok && ZstrCompare(pe.sections.data[0].name, ".debug") == 0;
    ok = ok && pe.sections.data[0].virtual_address == SECTION_VA;
    ok = ok && pe.codeview.present;
    ok = ok && pe.codeview.age == 0x2a;
    ok = ok && MemCompare(pe.codeview.guid, kGuid, 16) == 0;
    ok = ok && pe.codeview.pdb_path && ZstrCompare(pe.codeview.pdb_path, kPdbPath) == 0;

    PeDeinit(&pe);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_pe_rva_to_offset_round_trips(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    build_pe_blob();
    Pe pe;
    if (!PeOpenFromMemoryCopy(&pe, blob, sizeof(blob), base)) {
        DefaultAllocatorDeinit(&alloc);
        return false;
    }

    // The CodeView record sits at RVA 0x1020 -> file offset 0x420.
    u64  off = 0;
    bool ok  = PeRvaToOffset(&pe, CV_REC_RVA, &off) && off == CV_REC_RAW_OFF;

    // RVA outside any section should fail cleanly.
    u64 garbage = 0;
    ok          = ok && !PeRvaToOffset(&pe, 0xdead0000, &garbage);

    PeDeinit(&pe);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_pe_rejects_bad_magic(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();
    Allocator       *base  = ALLOCATOR_OF(&alloc);

    u8 garbage[256];
    MemSet(garbage, 0, sizeof(garbage));
    garbage[0] = 'X';
    garbage[1] = 'X';

    Pe   pe;
    bool ok = !PeOpenFromMemoryCopy(&pe, garbage, sizeof(garbage), base);

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Pe tests\n\n");

    TestFunction tests[] = {
        test_pe_parses_synthetic_blob,
        test_pe_rva_to_offset_round_trips,
        test_pe_rejects_bad_magic,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Pe");
}
