#include <Misra.h>

typedef struct {
    u8 class;
    u8 encoding;
    u8 version;
    u8 os_abi;
} ElfMeta;

typedef struct {
    ElfMeta meta;
    u16     type;
    u16     machine;
    u32     version;
    u32     entry;
    u32     program_header_table_offset;
    u32     section_header_table_offset;
    u32     flags;
    u16     elf_header_size;
    u16     program_header_entry_size;
    u16     program_header_count;
    u16     section_header_entry_size;
    u16     section_header_count;
    u16     string_table_index;
} ElfHeader32;

int main(int argc, char** argv) {
    if (argc < 2) {
        LOG_FATAL("USAGE: {} {}", FMT(argv[0]), FMT(argv[1]));
    }

    FILE* f = fopen(argv[1], "rb");
    if (!f) {
        LOG_ERROR("Failed to open file for reading.");
        return 1;
    }

    ElfHeader32 eh32    = {0};
    u64         padding = 0;
    u32 magic = 0;
    FReadFmt(
        f,
        "{4r}{1r}{1r}{1r}{1r}{8r}",
        FMT(magic),
        FMT(eh32.meta.class),
        FMT(eh32.meta.encoding),
        FMT(eh32.meta.version),
        FMT(eh32.meta.os_abi),
        FMT(padding)
    );

    u64 pos = ftell(f);
    WriteFmtLn("magic : {c}, file_pos = {}", FMT(magic), FMT(pos));

    // discard padding
    (void)(padding);

    FReadFmt(
        f,
        "{<2r}{<2r}{<4r}{<4r}{<4r}{<4r}{<4r}{<2r}{<2r}{<2r}{<2r}{<2r}{<2r}",
        FMT(eh32.type),
        FMT(eh32.machine),
        FMT(eh32.version),
        FMT(eh32.entry),
        FMT(eh32.program_header_table_offset),
        FMT(eh32.section_header_table_offset),
        FMT(eh32.flags),
        FMT(eh32.elf_header_size),
        FMT(eh32.program_header_entry_size),
        FMT(eh32.program_header_count),
        FMT(eh32.section_header_entry_size),
        FMT(eh32.section_header_count),
        FMT(eh32.string_table_index)
    );

    WriteFmtLn(
        "ElfHeader32 {{\n"
        "  meta: {{class: {}, encoding: {}, version: {}, os_abi: {}}}\n"
        "  type: {x}\n"
        "  machine: {x}\n"
        "  version: {}\n"
        "  entry: {x}\n"
        "  program_header_table_offset: {x}\n"
        "  section_header_table_offset: {x}\n"
        "  flags: {b}\n"
        "  elf_header_size: {x}\n"
        "  program_header_entry_size: {x}\n"
        "  program_header_count: {}\n"
        "  section_header_entry_size: {x}\n"
        "  section_header_count: {}\n"
        "  string_table_index: {}\n"
        "}}",

        FMT(eh32.meta.class),
        FMT(eh32.meta.encoding),
        FMT(eh32.meta.version),
        FMT(eh32.meta.os_abi),

        FMT(eh32.type),
        FMT(eh32.machine),
        FMT(eh32.version),
        FMT(eh32.entry),
        FMT(eh32.program_header_table_offset),
        FMT(eh32.section_header_table_offset),
        FMT(eh32.flags),
        FMT(eh32.elf_header_size),
        FMT(eh32.program_header_entry_size),
        FMT(eh32.program_header_count),
        FMT(eh32.section_header_entry_size),
        FMT(eh32.section_header_count),
        FMT(eh32.string_table_index)
    );

    return 0;
}
