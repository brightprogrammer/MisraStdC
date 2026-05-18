/// file      : Dwarf.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `.debug_line` parser (DWARF 4). Walks each CU's Line Number Program
/// and produces the (address, file, line) matrix as a flat
/// `Vec(DwarfLineEntry)`.

#include <Misra/Parsers/ByteIter.h>
#include <Misra/Parsers/Dwarf.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// Each `Vec(u64)` use is its own anonymous struct type, so a typedef
// is required to pass these around between functions.
typedef Vec(u64) U64Vec;

// ---------------------------------------------------------------------------
// DWARF opcode + standard-length constants
// ---------------------------------------------------------------------------

enum {
    DW_LNS_COPY               = 0x01,
    DW_LNS_ADVANCE_PC         = 0x02,
    DW_LNS_ADVANCE_LINE       = 0x03,
    DW_LNS_SET_FILE           = 0x04,
    DW_LNS_SET_COLUMN         = 0x05,
    DW_LNS_NEGATE_STMT        = 0x06,
    DW_LNS_SET_BASIC_BLOCK    = 0x07,
    DW_LNS_CONST_ADD_PC       = 0x08,
    DW_LNS_FIXED_ADVANCE_PC   = 0x09,
    DW_LNS_SET_PROLOGUE_END   = 0x0a,
    DW_LNS_SET_EPILOGUE_BEGIN = 0x0b,
    DW_LNS_SET_ISA            = 0x0c,
};

enum {
    DW_LNE_END_SEQUENCE      = 0x01,
    DW_LNE_SET_ADDRESS       = 0x02,
    DW_LNE_DEFINE_FILE       = 0x03,
    DW_LNE_SET_DISCRIMINATOR = 0x04,
};

// ---------------------------------------------------------------------------
// String pool helpers
// ---------------------------------------------------------------------------

// Append `s` (NUL-terminated) including its terminator into `pool` and
// return the offset at which it was inserted.
static bool pool_append(Str *pool, const char *s, u64 *out_offset) {
    u64 start = pool->length;
    while (*s) {
        if (!StrPushBack(pool, *s))
            return false;
        ++s;
    }
    if (!StrPushBack(pool, '\0'))
        return false;
    *out_offset = start;
    return true;
}

// ---------------------------------------------------------------------------
// Line Number Program header (DWARF 4, 32-bit length form)
// ---------------------------------------------------------------------------

typedef struct LineProgHeader {
    u32       version;
    u32       header_length;
    u8        min_instr_len;
    u8        max_ops_per_instr;
    bool      default_is_stmt;
    i8        line_base;
    u8        line_range;
    u8        opcode_base;
    const u8 *standard_opcode_lengths;  // points into the section bytes
    u64       std_opcode_lengths_count; // = opcode_base - 1
    const u8 *strings_start;            // first byte of include_directories
} LineProgHeader;

// Decode the DWARF 4 line-program header at `cur`. On return the
// cursor sits at the start of the include_directories table; the
// directory / file tables are walked separately by collect_cu_strings.
static bool decode_line_program_header(ByteIter *cur, LineProgHeader *out) {
    MemSet(out, 0, sizeof(*out));

    u32 unit_length = 0;
    if (!bi_take_u32_le(cur, &unit_length))
        return false;
    if (unit_length == 0xffffffff) {
        LOG_ERROR("DWARF: 64-bit DWARF length form not supported in v1");
        return false;
    }

    u16 version = 0;
    if (!bi_take_u16_le(cur, &version))
        return false;
    if (version != 3 && version != 4) {
        // DWARF 5+ has a different header / table layout. Caller can
        // skip this CU and continue.
        out->version = version;
        return false;
    }
    out->version = version;

    u32 header_length = 0;
    if (!bi_take_u32_le(cur, &header_length))
        return false;
    out->header_length = header_length;

    if (!bi_take_u8(cur, &out->min_instr_len))
        return false;

    if (version >= 4) {
        if (!bi_take_u8(cur, &out->max_ops_per_instr))
            return false;
    } else {
        out->max_ops_per_instr = 1;
    }

    u8 def_is_stmt = 0;
    if (!bi_take_u8(cur, &def_is_stmt))
        return false;
    out->default_is_stmt = def_is_stmt != 0;

    u8 raw_lb = 0;
    if (!bi_take_u8(cur, &raw_lb))
        return false;
    out->line_base = (i8)raw_lb;

    if (!bi_take_u8(cur, &out->line_range))
        return false;
    // line_range == 0 is a malformed CU header: every special opcode
    // and DW_LNS_CONST_ADD_PC computes `adjusted / line_range`, which
    // would divide by zero. The DWARF spec forbids line_range=0 in
    // practice (it must be a positive integer); refuse the unit.
    if (out->line_range == 0) {
        LOG_ERROR("DWARF: line program header has line_range == 0");
        return false;
    }
    if (!bi_take_u8(cur, &out->opcode_base))
        return false;

    out->std_opcode_lengths_count = out->opcode_base ? (u64)(out->opcode_base - 1) : 0;
    if (bi_remaining(cur) < out->std_opcode_lengths_count)
        return false;
    out->standard_opcode_lengths  = cur->data + cur->pos;
    cur->pos                     += out->std_opcode_lengths_count;
    out->strings_start            = cur->data + cur->pos;
    return true;
}

// Walk past include_directories + file_names, leaving cur at the
// start of the line number program body.
static bool skip_line_program_tables(ByteIter *cur) {
    while (cur->pos < cur->length && cur->data[cur->pos] != 0) {
        if (!bi_take_cstr(cur))
            return false;
    }
    if (cur->pos < cur->length)
        ++cur->pos; // empty terminator

    while (cur->pos < cur->length && cur->data[cur->pos] != 0) {
        if (!bi_take_cstr(cur))
            return false;
        u64 dir_idx = 0, mtime = 0, length_ = 0;
        if (!bi_take_uleb128(cur, &dir_idx))
            return false;
        if (!bi_take_uleb128(cur, &mtime))
            return false;
        if (!bi_take_uleb128(cur, &length_))
            return false;
    }
    if (cur->pos < cur->length)
        ++cur->pos; // empty terminator
    return true;
}

// ---------------------------------------------------------------------------
// CU-scoped tables: we need a second pass that actually stashes file /
// directory strings into the shared `string_pool`. We do this by
// re-decoding the header in `collect_cu_strings` since it's a one-shot
// linear walk anyway, then run the program with the resolved tables.
// ---------------------------------------------------------------------------

typedef struct CuStrings {
    U64Vec dir_offsets;  // global pool offsets for include_directories
    U64Vec file_dir_idx; // dir index per file (parallel with file_offsets)
    U64Vec file_offsets; // global pool offsets for file_names
} CuStrings;

static void cu_strings_init(CuStrings *cs, Allocator *alloc) {
    cs->dir_offsets  = VecInitT(cs->dir_offsets, alloc);
    cs->file_dir_idx = VecInitT(cs->file_dir_idx, alloc);
    cs->file_offsets = VecInitT(cs->file_offsets, alloc);
}

static void cu_strings_deinit(CuStrings *cs) {
    VecDeinit(&cs->dir_offsets);
    VecDeinit(&cs->file_dir_idx);
    VecDeinit(&cs->file_offsets);
}

// Walk the header again, this time copying the directory / file
// strings into the shared pool. `header_after_opcode_lengths_p` points
// to the first byte after the standard_opcode_lengths array; we
// continue from there.
static bool collect_cu_strings(ByteIter cur, Str *pool, CuStrings *cs) {
    // include_directories
    while (cur.pos < cur.length && cur.data[cur.pos] != 0) {
        const char *dir = bi_take_cstr(&cur);
        if (!dir)
            return false;
        u64 off = 0;
        if (!pool_append(pool, dir, &off))
            return false;
        if (!VecPushBackR(&cs->dir_offsets, off))
            return false;
    }
    if (cur.pos < cur.length)
        ++cur.pos; // empty terminator

    // file_names
    while (cur.pos < cur.length && cur.data[cur.pos] != 0) {
        const char *name = bi_take_cstr(&cur);
        if (!name)
            return false;
        u64 dir_idx = 0, mtime = 0, length_ = 0;
        if (!bi_take_uleb128(&cur, &dir_idx))
            return false;
        if (!bi_take_uleb128(&cur, &mtime))
            return false;
        if (!bi_take_uleb128(&cur, &length_))
            return false;
        u64 off = 0;
        if (!pool_append(pool, name, &off))
            return false;
        if (!VecPushBackR(&cs->file_offsets, off))
            return false;
        if (!VecPushBackR(&cs->file_dir_idx, dir_idx))
            return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Line number program interpreter
// ---------------------------------------------------------------------------

typedef struct LnpState {
    u64  address;
    u32  op_index;
    u64  file; // 1-based
    u32  line;
    u32  column;
    bool is_stmt;
    bool basic_block;
    bool end_sequence;
    bool prologue_end;
    bool epilogue_begin;
    u32  isa;
    u32  discriminator;
} LnpState;

static void lnp_reset(LnpState *st, bool default_is_stmt) {
    MemSet(st, 0, sizeof(*st));
    st->file    = 1;
    st->line    = 1;
    st->is_stmt = default_is_stmt;
}

static bool lnp_emit(
    DwarfLines      *out,
    Str             *pool,
    const CuStrings *cs,
    const LnpState  *st,
    U64Vec          *pending_file_offsets,
    U64Vec          *pending_dir_offsets
) {
    DwarfLineEntry e;
    MemSet(&e, 0, sizeof(e));
    e.address      = st->address;
    e.line         = st->line;
    e.column       = st->column;
    e.is_stmt      = st->is_stmt;
    e.end_sequence = st->end_sequence;

    // Resolve file / dir offsets at the end (when string_pool no
    // longer grows). For now, stash the offsets in parallel arrays.
    u64 file_off = 0;
    u64 dir_off  = 0;
    if (st->file >= 1 && st->file - 1 < cs->file_offsets.length) {
        file_off    = cs->file_offsets.data[st->file - 1];
        u64 dir_idx = cs->file_dir_idx.data[st->file - 1];
        if (dir_idx >= 1 && dir_idx - 1 < cs->dir_offsets.length) {
            dir_off = cs->dir_offsets.data[dir_idx - 1];
        }
    }
    if (!VecPushBackR(pending_file_offsets, file_off))
        return false;
    if (!VecPushBackR(pending_dir_offsets, dir_off))
        return false;

    if (!VecPushBackR(&out->entries, e))
        return false;
    (void)pool;
    return true;
}

static bool run_line_program(
    ByteIter              cur,
    const u8             *prog_end,
    const LineProgHeader *hdr,
    const CuStrings      *cs,
    DwarfLines           *out,
    U64Vec               *pending_file_offsets,
    U64Vec               *pending_dir_offsets
) {
    LnpState st;
    lnp_reset(&st, hdr->default_is_stmt);

    while (cur.data + cur.pos < prog_end) {
        u8 op = 0;
        if (!bi_take_u8(&cur, &op))
            return false;

        if (op == 0) {
            // Extended opcode: <length:uleb> <sub_op:u8> <operands>
            u64 length = 0;
            if (!bi_take_uleb128(&cur, &length))
                return false;
            if (length == 0 || (u64)(prog_end - (cur.data + cur.pos)) < length)
                return false;
            const u8 *body_end = cur.data + cur.pos + length;
            u8        sub_op   = 0;
            if (!bi_take_u8(&cur, &sub_op))
                return false;
            switch (sub_op) {
                case DW_LNE_END_SEQUENCE :
                    st.end_sequence = true;
                    if (!lnp_emit(out, &out->string_pool, cs, &st, pending_file_offsets, pending_dir_offsets))
                        return false;
                    lnp_reset(&st, hdr->default_is_stmt);
                    break;
                case DW_LNE_SET_ADDRESS :
                    // operand size = remaining body bytes; on x86-64 always 8.
                    if (body_end - (cur.data + cur.pos) == 8) {
                        if (!bi_take_u64_le(&cur, &st.address))
                            return false;
                    } else if (body_end - (cur.data + cur.pos) == 4) {
                        u32 a32 = 0;
                        if (!bi_take_u32_le(&cur, &a32))
                            return false;
                        st.address = a32;
                    } else {
                        return false;
                    }
                    st.op_index = 0;
                    break;
                case DW_LNE_DEFINE_FILE :
                    // Skip; runtime-defined files are rare.
                    break;
                case DW_LNE_SET_DISCRIMINATOR : {
                    u64 disc = 0;
                    if (!bi_take_uleb128(&cur, &disc))
                        return false;
                    st.discriminator = (u32)disc;
                    break;
                }
                default :
                    break; // ignore unknown
            }
            cur.pos = (size)(body_end - cur.data);
        } else if (op < hdr->opcode_base) {
            // Standard opcode
            switch (op) {
                case DW_LNS_COPY :
                    if (!lnp_emit(out, &out->string_pool, cs, &st, pending_file_offsets, pending_dir_offsets))
                        return false;
                    st.basic_block    = false;
                    st.prologue_end   = false;
                    st.epilogue_begin = false;
                    st.discriminator  = 0;
                    break;
                case DW_LNS_ADVANCE_PC : {
                    u64 adv = 0;
                    if (!bi_take_uleb128(&cur, &adv))
                        return false;
                    st.address += (u64)hdr->min_instr_len * adv;
                    break;
                }
                case DW_LNS_ADVANCE_LINE : {
                    i64 adv = 0;
                    if (!bi_take_sleb128(&cur, &adv))
                        return false;
                    st.line = (u32)((i64)st.line + adv);
                    break;
                }
                case DW_LNS_SET_FILE : {
                    u64 f = 0;
                    if (!bi_take_uleb128(&cur, &f))
                        return false;
                    st.file = f;
                    break;
                }
                case DW_LNS_SET_COLUMN : {
                    u64 c = 0;
                    if (!bi_take_uleb128(&cur, &c))
                        return false;
                    st.column = (u32)c;
                    break;
                }
                case DW_LNS_NEGATE_STMT :
                    st.is_stmt = !st.is_stmt;
                    break;
                case DW_LNS_SET_BASIC_BLOCK :
                    st.basic_block = true;
                    break;
                case DW_LNS_CONST_ADD_PC : {
                    // Add the address advance of special opcode 255.
                    u32 adjusted  = 255u - hdr->opcode_base;
                    u32 op_adv    = adjusted / hdr->line_range;
                    st.address   += (u64)hdr->min_instr_len * op_adv;
                    break;
                }
                case DW_LNS_FIXED_ADVANCE_PC : {
                    u16 adv = 0;
                    if (!bi_take_u16_le(&cur, &adv))
                        return false;
                    st.address  += adv;
                    st.op_index  = 0;
                    break;
                }
                case DW_LNS_SET_PROLOGUE_END :
                    st.prologue_end = true;
                    break;
                case DW_LNS_SET_EPILOGUE_BEGIN :
                    st.epilogue_begin = true;
                    break;
                case DW_LNS_SET_ISA : {
                    u64 isa = 0;
                    if (!bi_take_uleb128(&cur, &isa))
                        return false;
                    st.isa = (u32)isa;
                    break;
                }
                default : {
                    // Unknown standard opcode — skip its declared
                    // number of ULEB128 operands.
                    if (op - 1 < hdr->std_opcode_lengths_count) {
                        u8 nops = hdr->standard_opcode_lengths[op - 1];
                        for (u8 i = 0; i < nops; ++i) {
                            u64 dummy = 0;
                            if (!bi_take_uleb128(&cur, &dummy))
                                return false;
                        }
                    }
                    break;
                }
            }
        } else {
            // Special opcode
            u32 adjusted  = (u32)op - hdr->opcode_base;
            u32 op_adv    = adjusted / hdr->line_range;
            i32 line_adv  = hdr->line_base + (i32)(adjusted % hdr->line_range);
            st.address   += (u64)hdr->min_instr_len * op_adv;
            st.line       = (u32)((i32)st.line + line_adv);
            if (!lnp_emit(out, &out->string_pool, cs, &st, pending_file_offsets, pending_dir_offsets))
                return false;
            st.basic_block    = false;
            st.prologue_end   = false;
            st.epilogue_begin = false;
            st.discriminator  = 0;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool dwarf_lines_build_from_elf(DwarfLines *out, const ElfFile *elf, Allocator *alloc) {
    if (!out || !elf || !alloc) {
        LOG_ERROR("DwarfLinesBuildFromElf: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator   = alloc;
    out->entries     = VecInitT(out->entries, alloc);
    out->string_pool = StrInit(alloc);

    const ElfSection *line_section = ElfFileFindSection(elf, ".debug_line");
    if (!line_section || line_section->size == 0) {
        // No debug info — empty result is still success.
        return true;
    }

    // Parallel arrays of file / dir offsets, one per emitted row. We
    // resolve them to pointers at the end after string_pool is final.
    U64Vec pending_file_offsets = VecInitT(pending_file_offsets, alloc);
    U64Vec pending_dir_offsets  = VecInitT(pending_dir_offsets, alloc);

    ByteIter section_cur = BYTE_ITER_FROM_MEMORY(elf->data + line_section->offset, line_section->size);

    bool ok = true;
    while (bi_remaining(&section_cur) > 0) {
        const u8 *unit_start  = section_cur.data + section_cur.pos;
        u32       unit_length = 0;
        ByteIter  peek        = section_cur;
        if (!bi_take_u32_le(&peek, &unit_length)) {
            ok = false;
            break;
        }
        if (unit_length == 0xffffffff) {
            LOG_ERROR("DWARF: 64-bit length form not supported (skipping rest of section)");
            ok = false;
            break;
        }
        if (4u + (u64)unit_length > bi_remaining(&section_cur)) {
            ok = false;
            break;
        }
        const u8 *unit_end     = unit_start + 4 + unit_length;
        size      unit_end_pos = section_cur.pos + 4 + unit_length;

        // Decode header (fields only), then walk the directory / file
        // tables once to populate the shared string pool, then run
        // the program body.
        ByteIter       hdr_cur = section_cur;
        LineProgHeader hdr;
        if (!decode_line_program_header(&hdr_cur, &hdr)) {
            // Unsupported version (5+) or malformed: skip this CU and
            // keep parsing the rest. The unit_length field we already
            // consumed gives us the size of this whole unit.
            section_cur.pos = unit_end_pos;
            continue;
        }

        CuStrings cs;
        cu_strings_init(&cs, alloc);

        // String/program iters cover the bytes from `hdr.strings_start`
        // up to the end of this CU.
        size     strings_start_pos = (size)(hdr.strings_start - section_cur.data);
        ByteIter str_cur           = BYTE_ITER_FROM_MEMORY(section_cur.data, unit_end_pos);
        str_cur.pos                = strings_start_pos;
        if (!collect_cu_strings(str_cur, &out->string_pool, &cs)) {
            cu_strings_deinit(&cs);
            ok = false;
            break;
        }

        // Skip past the tables to find the program body start.
        ByteIter prog_anchor = BYTE_ITER_FROM_MEMORY(section_cur.data, unit_end_pos);
        prog_anchor.pos      = strings_start_pos;
        if (!skip_line_program_tables(&prog_anchor)) {
            cu_strings_deinit(&cs);
            ok = false;
            break;
        }

        ByteIter prog_cur = prog_anchor;
        if (!run_line_program(prog_cur, unit_end, &hdr, &cs, out, &pending_file_offsets, &pending_dir_offsets)) {
            cu_strings_deinit(&cs);
            ok = false;
            break;
        }

        cu_strings_deinit(&cs);
        section_cur.pos = unit_end_pos;
    }

    // Resolve offsets -> pointers now that string_pool won't grow.
    if (ok) {
        for (u64 i = 0; i < out->entries.length; ++i) {
            u64 fo                    = pending_file_offsets.data[i];
            u64 dofs                  = pending_dir_offsets.data[i];
            out->entries.data[i].file = fo ? (const char *)(out->string_pool.data + fo) : NULL;
            out->entries.data[i].dir  = dofs ? (const char *)(out->string_pool.data + dofs) : NULL;
        }
    }

    VecDeinit(&pending_file_offsets);
    VecDeinit(&pending_dir_offsets);

    if (!ok) {
        DwarfLinesDeinit(out);
        return false;
    }
    return true;
}

void DwarfLinesDeinit(DwarfLines *self) {
    if (!self)
        return;
    VecDeinit(&self->entries);
    StrDeinit(&self->string_pool);
    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

const DwarfLineEntry *DwarfLinesResolve(const DwarfLines *self, u64 vaddr) {
    if (!self || self->entries.length == 0)
        return NULL;

    // Linear scan finding the greatest address <= vaddr inside an
    // open sequence. The entries form a series of sequences delimited
    // by end_sequence rows; vaddr must not exceed the sequence's
    // closing row.
    const DwarfLineEntry *best     = NULL;
    const DwarfLineEntry *seq_open = NULL;
    for (u64 i = 0; i < self->entries.length; ++i) {
        const DwarfLineEntry *e = &self->entries.data[i];
        if (e->end_sequence) {
            // Sequence ends at this row's address (exclusive upper).
            if (seq_open && vaddr >= seq_open->address && vaddr < e->address) {
                // Within this sequence — best already points at the
                // greatest row with address <= vaddr.
                return best;
            }
            seq_open = NULL;
            best     = NULL;
            continue;
        }
        if (!seq_open) {
            seq_open = e;
            best     = e->address <= vaddr ? e : NULL;
            continue;
        }
        if (e->address <= vaddr) {
            best = e;
        }
    }
    return NULL;
}
