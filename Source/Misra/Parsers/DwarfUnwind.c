/// file      : DwarfUnwind.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// DWARF `.eh_frame` CFI parser — the half of `Parsers/Dwarf` that
/// powers the frame-pointer-less stack unwinder.
///
/// `.eh_frame` is a stream of length-prefixed records, each either a
/// CIE (Common Information Entry) or an FDE (Frame Description Entry).
/// The encoding is *almost* identical to `.debug_frame` but uses
/// encoded pointers (DW_EH_PE_*) for the FDE address fields and a
/// different "CIE marker" sentinel.
///
/// References: System V Application Binary Interface, Linux Standard
/// Base ELF format, "DWARF Debugging Information Format" v4, §6.4.
///
/// What this file handles:
///   - 32-bit DWARF length form (rare to hit 64-bit on Linux).
///   - CIE versions 1, 3, 4.
///   - Augmentation strings starting with 'z' plus `L`, `P`, `R`, `S`.
///   - FDE pointer encodings DW_EH_PE_{absptr,udata2/4/8,sdata2/4/8,
///     pcrel,datarel} in their common combinations. DW_EH_PE_omit and
///     DW_EH_PE_aligned are recognised but the indirect and signed/
///     leb modifiers are minimal-case.
///
/// What this file does NOT yet handle (tracked in FUTURE-PLANS):
///   - DWARF expressions in pointer encodings (DW_EH_PE_indirect).
///   - 64-bit DWARF length form.
///   - `.eh_frame_hdr` binary-search index (we linear-scan FDEs;
///     adequate up to a few thousand FDEs).
///
/// The CFI bytecode interpreter that turns FDE instructions into a
/// per-IP unwind row is a separate piece (added in a later commit);
/// this file's job is only to give us an FDE by PC.

#include <Misra/Parsers/Dwarf.h>

#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

#include <stdint.h>

// ---------------------------------------------------------------------------
// Bounds-checked byte cursor + ULEB / SLEB readers (private)
// ---------------------------------------------------------------------------

typedef struct DwUCursor {
    const u8 *p;
    const u8 *end;
} DwUCursor;

static bool dwc_take_u8(DwUCursor *c, u8 *out) {
    if (c->p >= c->end)
        return false;
    *out = *c->p++;
    return true;
}

static bool dwc_take_u16_le(DwUCursor *c, u16 *out) {
    if (c->end - c->p < 2)
        return false;
    *out  = (u16)c->p[0] | ((u16)c->p[1] << 8);
    c->p += 2;
    return true;
}

static bool dwc_take_u32_le(DwUCursor *c, u32 *out) {
    if (c->end - c->p < 4)
        return false;
    *out  = (u32)c->p[0] | ((u32)c->p[1] << 8) | ((u32)c->p[2] << 16) | ((u32)c->p[3] << 24);
    c->p += 4;
    return true;
}

static bool dwc_take_u64_le(DwUCursor *c, u64 *out) {
    if (c->end - c->p < 8)
        return false;
    *out = 0;
    for (i32 i = 0; i < 8; ++i) {
        *out |= ((u64)c->p[i]) << (i * 8);
    }
    c->p += 8;
    return true;
}

static bool dwc_take_uleb128(DwUCursor *c, u64 *out) {
    u64 result = 0;
    u32 shift  = 0;
    while (c->p < c->end) {
        u8 b    = *c->p++;
        result |= ((u64)(b & 0x7f)) << shift;
        if ((b & 0x80) == 0) {
            *out = result;
            return true;
        }
        shift += 7;
        if (shift >= 64)
            return false;
    }
    return false;
}

static bool dwc_take_sleb128(DwUCursor *c, i64 *out) {
    u64 result = 0;
    u32 shift  = 0;
    u8  b      = 0;
    while (c->p < c->end) {
        b       = *c->p++;
        result |= ((u64)(b & 0x7f)) << shift;
        shift  += 7;
        if ((b & 0x80) == 0) {
            if (shift < 64 && (b & 0x40)) {
                result |= (~(u64)0) << shift;
            }
            *out = (i64)result;
            return true;
        }
        if (shift >= 64)
            return false;
    }
    return false;
}

static const char *dwc_take_cstr(DwUCursor *c) {
    const u8 *start = c->p;
    while (c->p < c->end && *c->p != 0)
        ++c->p;
    if (c->p >= c->end)
        return NULL;
    ++c->p; // skip NUL
    return (const char *)start;
}

// ---------------------------------------------------------------------------
// DW_EH_PE_* constants and encoded-pointer reader
// ---------------------------------------------------------------------------

enum {
    DW_EH_PE_OMIT = 0xff,

    // Low nibble: value encoding.
    DW_EH_PE_ABSPTR  = 0x00,
    DW_EH_PE_ULEB128 = 0x01,
    DW_EH_PE_UDATA2  = 0x02,
    DW_EH_PE_UDATA4  = 0x03,
    DW_EH_PE_UDATA8  = 0x04,
    DW_EH_PE_SIGNED  = 0x08,
    DW_EH_PE_SLEB128 = 0x09,
    DW_EH_PE_SDATA2  = 0x0a,
    DW_EH_PE_SDATA4  = 0x0b,
    DW_EH_PE_SDATA8  = 0x0c,

    // High nibble: applied-to base.
    DW_EH_PE_PCREL   = 0x10,
    DW_EH_PE_TEXTREL = 0x20,
    DW_EH_PE_DATAREL = 0x30,
    DW_EH_PE_FUNCREL = 0x40,
    DW_EH_PE_ALIGNED = 0x50,

    // Modifier: dereference once after applying the base.
    DW_EH_PE_INDIRECT = 0x80,
};

// Read an encoded pointer from `c` per `encoding`. `here_vaddr` is the
// file-relative virtual address of `c->p` (used by PCREL). On success
// `*out` receives the decoded absolute file-relative VA.
static bool decode_eh_ptr(DwUCursor *c, u8 encoding, u64 here_vaddr, u64 *out) {
    if (encoding == DW_EH_PE_OMIT) {
        return false;
    }

    u8 value_kind = encoding & 0x0f;
    u8 base_kind  = encoding & 0x70;

    // Track where we are at the start so PCREL can use it.
    u64 anchor = here_vaddr;

    i64  signed_val = 0;
    u64  raw_val    = 0;
    bool is_signed  = false;

    switch (value_kind) {
        case DW_EH_PE_ABSPTR : {
            // Treat as 8 bytes on our 64-bit target.
            if (!dwc_take_u64_le(c, &raw_val))
                return false;
            break;
        }
        case DW_EH_PE_ULEB128 : {
            if (!dwc_take_uleb128(c, &raw_val))
                return false;
            break;
        }
        case DW_EH_PE_UDATA2 : {
            u16 v = 0;
            if (!dwc_take_u16_le(c, &v))
                return false;
            raw_val = v;
            break;
        }
        case DW_EH_PE_UDATA4 : {
            u32 v = 0;
            if (!dwc_take_u32_le(c, &v))
                return false;
            raw_val = v;
            break;
        }
        case DW_EH_PE_UDATA8 : {
            if (!dwc_take_u64_le(c, &raw_val))
                return false;
            break;
        }
        case DW_EH_PE_SLEB128 : {
            if (!dwc_take_sleb128(c, &signed_val))
                return false;
            is_signed = true;
            break;
        }
        case DW_EH_PE_SDATA2 : {
            u16 v = 0;
            if (!dwc_take_u16_le(c, &v))
                return false;
            signed_val = (i64)(i16)v;
            is_signed  = true;
            break;
        }
        case DW_EH_PE_SDATA4 : {
            u32 v = 0;
            if (!dwc_take_u32_le(c, &v))
                return false;
            signed_val = (i64)(i32)v;
            is_signed  = true;
            break;
        }
        case DW_EH_PE_SDATA8 : {
            u64 v = 0;
            if (!dwc_take_u64_le(c, &v))
                return false;
            signed_val = (i64)v;
            is_signed  = true;
            break;
        }
        default :
            return false;
    }

    u64 result;
    if (is_signed) {
        // pc-relative signed offset is the common case for FDE pc_begin.
        if (base_kind == DW_EH_PE_PCREL) {
            result = anchor + (u64)signed_val;
        } else if (base_kind == 0) {
            result = (u64)signed_val;
        } else {
            // Other bases (textrel, datarel, funcrel, aligned) require
            // section-base context we don't carry. Fall back to absolute.
            result = (u64)signed_val;
        }
    } else {
        if (base_kind == DW_EH_PE_PCREL) {
            result = anchor + raw_val;
        } else {
            result = raw_val;
        }
    }

    if (encoding & DW_EH_PE_INDIRECT) {
        // We don't follow indirect pointers in a static-file parse —
        // they require the actual loaded image. Fail closed.
        return false;
    }

    *out = result;
    return true;
}

// Compute the file-relative VA of `byte_ptr` inside `.eh_frame`.
// `section_base_vaddr` = sh_addr of `.eh_frame`. `section_data_ptr` =
// pointer to the first byte of the section in our in-memory ELF copy.
static u64 eh_byte_vaddr(const u8 *section_data_ptr, u64 section_base_vaddr, const u8 *byte_ptr) {
    return section_base_vaddr + (u64)(byte_ptr - section_data_ptr);
}

// ---------------------------------------------------------------------------
// CIE / FDE record parse
// ---------------------------------------------------------------------------

static bool parse_cie(DwUCursor *body, u64 cie_offset, DwarfCie *out) {
    MemSet(out, 0, sizeof(*out));
    out->offset = cie_offset;

    u8 version = 0;
    if (!dwc_take_u8(body, &version))
        return false;
    if (version != 1 && version != 3 && version != 4) {
        return false;
    }
    out->version = version;

    const char *augmentation = dwc_take_cstr(body);
    if (!augmentation)
        return false;

    if (version >= 4) {
        u8 address_size = 0, segment_size = 0;
        if (!dwc_take_u8(body, &address_size))
            return false;
        if (!dwc_take_u8(body, &segment_size))
            return false;
        // We only handle non-segmented 64-bit (address_size==8).
        if (address_size != 8 || segment_size != 0)
            return false;
    }

    u64 caf = 0;
    i64 daf = 0;
    if (!dwc_take_uleb128(body, &caf))
        return false;
    if (!dwc_take_sleb128(body, &daf))
        return false;
    out->code_alignment_factor = (i64)caf;
    out->data_alignment_factor = daf;

    if (version >= 3) {
        u64 ra = 0;
        if (!dwc_take_uleb128(body, &ra))
            return false;
        out->return_address_register = (u8)ra;
    } else {
        u8 ra = 0;
        if (!dwc_take_u8(body, &ra))
            return false;
        out->return_address_register = ra;
    }

    out->fde_pointer_encoding = DW_EH_PE_ABSPTR; // default

    if (augmentation[0] == 'z') {
        out->has_augmentation = 1;
        u64 aug_len           = 0;
        if (!dwc_take_uleb128(body, &aug_len))
            return false;
        const u8 *aug_end = body->p + aug_len;
        if (aug_end > body->end)
            return false;

        for (const char *a = augmentation + 1; *a; ++a) {
            switch (*a) {
                case 'L' : {
                    u8 lsda_enc = 0;
                    if (!dwc_take_u8(body, &lsda_enc))
                        return false;
                    break;
                }
                case 'P' : {
                    u8 pers_enc = 0;
                    if (!dwc_take_u8(body, &pers_enc))
                        return false;
                    // Skip the personality routine pointer.
                    u64 dummy = 0;
                    (void)decode_eh_ptr(body, pers_enc, /*here_vaddr*/ 0, &dummy);
                    break;
                }
                case 'R' : {
                    u8 fde_enc = 0;
                    if (!dwc_take_u8(body, &fde_enc))
                        return false;
                    out->fde_pointer_encoding = fde_enc;
                    break;
                }
                case 'S' :
                    // Signal frame; just a flag.
                    break;
                default :
                    // Unknown aug char — bail to the declared aug-data end.
                    break;
            }
        }
        // Jump to the aug-data end regardless of what we consumed.
        body->p = aug_end;
    }

    out->initial_instructions      = body->p;
    out->initial_instructions_size = (u64)(body->end - body->p);
    return true;
}

static bool parse_fde(
    DwUCursor      *body,
    const u8       *body_start,
    u64             cie_offset,
    const DwarfCfi *cfi,
    const u8       *section_data,
    u64             section_addr,
    DwarfFde       *out
) {
    MemSet(out, 0, sizeof(*out));
    out->offset     = (u64)(body_start - section_data);
    out->cie_offset = cie_offset;

    const DwarfCie *cie = DwarfCfiFindCie(cfi, cie_offset);
    if (!cie)
        return false;

    // pc_begin (encoded)
    u64 pc_begin = 0;
    {
        u64 here = eh_byte_vaddr(section_data, section_addr, body->p);
        if (!decode_eh_ptr(body, cie->fde_pointer_encoding, here, &pc_begin))
            return false;
    }
    out->pc_begin = pc_begin;

    // pc_range: same encoding, but only the value-encoding part matters
    // (no base relativity, per the spec).
    u8  range_enc = cie->fde_pointer_encoding & 0x0f;
    u64 pc_range  = 0;
    {
        u64 here = eh_byte_vaddr(section_data, section_addr, body->p);
        if (!decode_eh_ptr(body, range_enc, here, &pc_range))
            return false;
    }
    out->pc_range = pc_range;

    if (cie->has_augmentation) {
        u64 aug_len = 0;
        if (!dwc_take_uleb128(body, &aug_len))
            return false;
        if (body->p + aug_len > body->end)
            return false;
        body->p += aug_len;
    }

    out->instructions      = body->p;
    out->instructions_size = (u64)(body->end - body->p);
    return true;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool dwarf_cfi_build_from_elf(DwarfCfi *out, const ElfFile *elf, Allocator *alloc) {
    if (!out || !elf || !alloc) {
        LOG_ERROR("DwarfCfiBuildFromElf: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->cies      = VecInitT(out->cies, alloc);
    out->fdes      = VecInitT(out->fdes, alloc);

    const ElfSection *eh = ElfFileFindSection(elf, ".eh_frame");
    if (!eh || eh->size == 0) {
        return true; // No CFI in this binary — still success, just empty.
    }
    out->eh_frame_addr = eh->addr;

    const u8 *section_data = elf->data + eh->offset;
    const u8 *end          = section_data + eh->size;

    DwUCursor section_cur = {.p = section_data, .end = end};
    while (section_cur.p < section_cur.end) {
        const u8 *rec_start = section_cur.p;
        u32       length32  = 0;
        if (!dwc_take_u32_le(&section_cur, &length32))
            break;
        if (length32 == 0) {
            // Terminator record.
            break;
        }
        if (length32 == 0xffffffff) {
            // 64-bit DWARF length form — not supported in v1.
            break;
        }
        const u8 *body_end = section_cur.p + length32;
        if (body_end > section_cur.end)
            break;

        u32 id = 0;
        if (!dwc_take_u32_le(&section_cur, &id))
            break;

        // In .eh_frame, id==0 means CIE; nonzero is the CIE_pointer for FDE
        // (back-offset from the start of *the id field*).
        if (id == 0) {
            u64       cie_offset = (u64)(rec_start - section_data);
            DwarfCie  cie;
            DwUCursor body = {.p = section_cur.p, .end = body_end};
            if (parse_cie(&body, cie_offset, &cie)) {
                if (!VecPushBackR(&out->cies, cie)) {
                    DwarfCfiDeinit(out);
                    return false;
                }
            }
        } else {
            // CIE pointer = (offset of the id field) - id, points at the
            // start of the CIE record (i.e. at the CIE's length field).
            u64       id_field_off = (u64)(section_cur.p - 4 - section_data);
            u64       cie_offset   = id_field_off - id;
            DwarfFde  fde;
            DwUCursor body = {.p = section_cur.p, .end = body_end};
            if (parse_fde(&body, rec_start, cie_offset, out, section_data, eh->addr, &fde)) {
                if (!VecPushBackR(&out->fdes, fde)) {
                    DwarfCfiDeinit(out);
                    return false;
                }
            }
        }

        section_cur.p = body_end;
    }

    return true;
}

const DwarfCie *DwarfCfiFindCie(const DwarfCfi *self, u64 cie_offset) {
    if (!self)
        return NULL;
    for (u64 i = 0; i < self->cies.length; ++i) {
        if (self->cies.data[i].offset == cie_offset) {
            return &self->cies.data[i];
        }
    }
    return NULL;
}

const DwarfFde *DwarfCfiFindFde(const DwarfCfi *self, u64 vaddr) {
    if (!self)
        return NULL;
    // Linear scan. `.eh_frame_hdr`-based binary search is in
    // FUTURE-PLANS; with a few thousand FDEs the linear scan is
    // still sub-microsecond.
    for (u64 i = 0; i < self->fdes.length; ++i) {
        const DwarfFde *f = &self->fdes.data[i];
        if (vaddr >= f->pc_begin && vaddr < f->pc_begin + f->pc_range) {
            return f;
        }
    }
    return NULL;
}

void DwarfCfiDeinit(DwarfCfi *self) {
    if (!self)
        return;
    VecDeinit(&self->cies);
    VecDeinit(&self->fdes);
    MemSet(self, 0, sizeof(*self));
}

// ---------------------------------------------------------------------------
// CFI bytecode interpreter
// ---------------------------------------------------------------------------
//
// Opcode classes (high 2 bits of the byte):
//   0b00 - extended opcode (low 6 bits = opcode number)
//   0b01 - DW_CFA_advance_loc        (operand: delta in low 6 bits)
//   0b10 - DW_CFA_offset             (register in low 6 bits, ULEB offset operand)
//   0b11 - DW_CFA_restore            (register in low 6 bits)

enum {
    DW_CFA_NOP                = 0x00,
    DW_CFA_SET_LOC            = 0x01,
    DW_CFA_ADVANCE_LOC1       = 0x02,
    DW_CFA_ADVANCE_LOC2       = 0x03,
    DW_CFA_ADVANCE_LOC4       = 0x04,
    DW_CFA_OFFSET_EXTENDED    = 0x05,
    DW_CFA_RESTORE_EXTENDED   = 0x06,
    DW_CFA_UNDEFINED          = 0x07,
    DW_CFA_SAME_VALUE         = 0x08,
    DW_CFA_REGISTER           = 0x09,
    DW_CFA_REMEMBER_STATE     = 0x0a,
    DW_CFA_RESTORE_STATE      = 0x0b,
    DW_CFA_DEF_CFA            = 0x0c,
    DW_CFA_DEF_CFA_REGISTER   = 0x0d,
    DW_CFA_DEF_CFA_OFFSET     = 0x0e,
    DW_CFA_DEF_CFA_EXPRESSION = 0x0f,
    DW_CFA_EXPRESSION         = 0x10,
    DW_CFA_OFFSET_EXTENDED_SF = 0x11,
    DW_CFA_DEF_CFA_SF         = 0x12,
    DW_CFA_DEF_CFA_OFFSET_SF  = 0x13,
    DW_CFA_VAL_OFFSET         = 0x14,
    DW_CFA_VAL_OFFSET_SF      = 0x15,
    DW_CFA_VAL_EXPRESSION     = 0x16,
};

#define CFI_STATE_STACK 4

typedef struct CfiVm {
    DwarfUnwindRow row;
    DwarfUnwindRow initial;  // snapshot after running the CIE's instructions
    DwarfUnwindRow saved_state_stack[CFI_STATE_STACK];
    u8             saved_state_top;
    u64            location; // current PC inside the FDE's range
    i64            code_align;
    i64            data_align;
} CfiVm;

static void cfi_vm_init(CfiVm *vm, const DwarfCie *cie, u64 fde_pc_begin, u8 ra_reg) {
    MemSet(vm, 0, sizeof(*vm));
    vm->row.return_address_register = ra_reg;
    vm->location                    = fde_pc_begin;
    vm->code_align                  = cie->code_alignment_factor ? cie->code_alignment_factor : 1;
    vm->data_align                  = cie->data_alignment_factor ? cie->data_alignment_factor : 1;
}

// One instruction. `stop_at` lets the caller bail mid-stream once an
// advance_loc has covered the requested target PC.
static bool cfi_vm_step(CfiVm *vm, DwUCursor *cur, u64 stop_at, bool *stop_now) {
    u8 op = 0;
    if (!dwc_take_u8(cur, &op))
        return false;

    u8 high = op & 0xc0;
    u8 low  = op & 0x3f;

    if (high == 0x40) {
        // DW_CFA_advance_loc
        u64 delta   = (u64)low * (u64)vm->code_align;
        u64 next_pc = vm->location + delta;
        if (vm->location <= stop_at && stop_at < next_pc) {
            *stop_now = true;
            return true;
        }
        vm->location = next_pc;
        return true;
    }
    if (high == 0x80) {
        // DW_CFA_offset (register in low bits, operand ULEB offset, scaled by data_align)
        u64 raw = 0;
        if (!dwc_take_uleb128(cur, &raw))
            return false;
        if (low < DWARF_UNWIND_MAX_REGS) {
            vm->row.regs[low].kind   = DWARF_REG_RULE_OFFSET;
            vm->row.regs[low].offset = (i64)raw * vm->data_align;
        }
        return true;
    }
    if (high == 0xc0) {
        // DW_CFA_restore
        if (low < DWARF_UNWIND_MAX_REGS) {
            vm->row.regs[low] = vm->initial.regs[low];
        }
        return true;
    }

    switch (op) {
        case DW_CFA_NOP :
            return true;

        case DW_CFA_SET_LOC : {
            u64 abs_pc = 0;
            if (!dwc_take_u64_le(cur, &abs_pc))
                return false;
            if (vm->location <= stop_at && stop_at < abs_pc) {
                *stop_now = true;
                return true;
            }
            vm->location = abs_pc;
            return true;
        }
        case DW_CFA_ADVANCE_LOC1 : {
            u8 d = 0;
            if (!dwc_take_u8(cur, &d))
                return false;
            u64 next = vm->location + (u64)d * (u64)vm->code_align;
            if (vm->location <= stop_at && stop_at < next) {
                *stop_now = true;
                return true;
            }
            vm->location = next;
            return true;
        }
        case DW_CFA_ADVANCE_LOC2 : {
            u16 d = 0;
            if (!dwc_take_u16_le(cur, &d))
                return false;
            u64 next = vm->location + (u64)d * (u64)vm->code_align;
            if (vm->location <= stop_at && stop_at < next) {
                *stop_now = true;
                return true;
            }
            vm->location = next;
            return true;
        }
        case DW_CFA_ADVANCE_LOC4 : {
            u32 d = 0;
            if (!dwc_take_u32_le(cur, &d))
                return false;
            u64 next = vm->location + (u64)d * (u64)vm->code_align;
            if (vm->location <= stop_at && stop_at < next) {
                *stop_now = true;
                return true;
            }
            vm->location = next;
            return true;
        }

        case DW_CFA_OFFSET_EXTENDED : {
            u64 reg = 0, off = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (!dwc_take_uleb128(cur, &off))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg].kind   = DWARF_REG_RULE_OFFSET;
                vm->row.regs[reg].offset = (i64)off * vm->data_align;
            }
            return true;
        }
        case DW_CFA_OFFSET_EXTENDED_SF : {
            u64 reg = 0;
            i64 off = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (!dwc_take_sleb128(cur, &off))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg].kind   = DWARF_REG_RULE_OFFSET;
                vm->row.regs[reg].offset = off * vm->data_align;
            }
            return true;
        }
        case DW_CFA_RESTORE_EXTENDED : {
            u64 reg = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg] = vm->initial.regs[reg];
            }
            return true;
        }
        case DW_CFA_UNDEFINED : {
            u64 reg = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg].kind = DWARF_REG_RULE_UNDEFINED;
            }
            return true;
        }
        case DW_CFA_SAME_VALUE : {
            u64 reg = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg].kind = DWARF_REG_RULE_SAME_VALUE;
            }
            return true;
        }
        case DW_CFA_REGISTER : {
            u64 reg = 0, src = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (!dwc_take_uleb128(cur, &src))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg].kind = DWARF_REG_RULE_REGISTER;
                vm->row.regs[reg].reg  = (u8)src;
            }
            return true;
        }
        case DW_CFA_REMEMBER_STATE : {
            if (vm->saved_state_top >= CFI_STATE_STACK)
                return false;
            vm->saved_state_stack[vm->saved_state_top++] = vm->row;
            return true;
        }
        case DW_CFA_RESTORE_STATE : {
            if (vm->saved_state_top == 0)
                return false;
            DwarfUnwindRow snapshot          = vm->saved_state_stack[--vm->saved_state_top];
            snapshot.return_address_register = vm->row.return_address_register;
            vm->row                          = snapshot;
            return true;
        }
        case DW_CFA_DEF_CFA : {
            u64 reg = 0, off = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (!dwc_take_uleb128(cur, &off))
                return false;
            vm->row.cfa.kind   = DWARF_CFA_RULE_REG_OFFSET;
            vm->row.cfa.reg    = (u8)reg;
            vm->row.cfa.offset = (i64)off;
            return true;
        }
        case DW_CFA_DEF_CFA_SF : {
            u64 reg = 0;
            i64 off = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (!dwc_take_sleb128(cur, &off))
                return false;
            vm->row.cfa.kind   = DWARF_CFA_RULE_REG_OFFSET;
            vm->row.cfa.reg    = (u8)reg;
            vm->row.cfa.offset = off * vm->data_align;
            return true;
        }
        case DW_CFA_DEF_CFA_REGISTER : {
            u64 reg = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (vm->row.cfa.kind != DWARF_CFA_RULE_REG_OFFSET) {
                vm->row.cfa.kind = DWARF_CFA_RULE_REG_OFFSET;
            }
            vm->row.cfa.reg = (u8)reg;
            return true;
        }
        case DW_CFA_DEF_CFA_OFFSET : {
            u64 off = 0;
            if (!dwc_take_uleb128(cur, &off))
                return false;
            if (vm->row.cfa.kind != DWARF_CFA_RULE_REG_OFFSET) {
                vm->row.cfa.kind = DWARF_CFA_RULE_REG_OFFSET;
            }
            vm->row.cfa.offset = (i64)off;
            return true;
        }
        case DW_CFA_DEF_CFA_OFFSET_SF : {
            i64 off = 0;
            if (!dwc_take_sleb128(cur, &off))
                return false;
            if (vm->row.cfa.kind != DWARF_CFA_RULE_REG_OFFSET) {
                vm->row.cfa.kind = DWARF_CFA_RULE_REG_OFFSET;
            }
            vm->row.cfa.offset = off * vm->data_align;
            return true;
        }

        case DW_CFA_DEF_CFA_EXPRESSION :
        case DW_CFA_EXPRESSION :
        case DW_CFA_VAL_EXPRESSION : {
            // Skip the embedded expression. v1 of the unwinder bails
            // when an active register's rule is EXPRESSION because we
            // don't evaluate DWARF expressions yet.
            if (op != DW_CFA_DEF_CFA_EXPRESSION) {
                u64 reg = 0;
                if (!dwc_take_uleb128(cur, &reg))
                    return false;
                if (reg < DWARF_UNWIND_MAX_REGS) {
                    vm->row.regs[reg].kind = DWARF_REG_RULE_EXPRESSION;
                }
            } else {
                vm->row.cfa.kind = DWARF_CFA_RULE_EXPRESSION;
            }
            u64 expr_len = 0;
            if (!dwc_take_uleb128(cur, &expr_len))
                return false;
            if (cur->p + expr_len > cur->end)
                return false;
            cur->p += expr_len;
            return true;
        }

        case DW_CFA_VAL_OFFSET : {
            u64 reg = 0, off = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (!dwc_take_uleb128(cur, &off))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg].kind   = DWARF_REG_RULE_VAL_OFFSET;
                vm->row.regs[reg].offset = (i64)off * vm->data_align;
            }
            return true;
        }
        case DW_CFA_VAL_OFFSET_SF : {
            u64 reg = 0;
            i64 off = 0;
            if (!dwc_take_uleb128(cur, &reg))
                return false;
            if (!dwc_take_sleb128(cur, &off))
                return false;
            if (reg < DWARF_UNWIND_MAX_REGS) {
                vm->row.regs[reg].kind   = DWARF_REG_RULE_VAL_OFFSET;
                vm->row.regs[reg].offset = off * vm->data_align;
            }
            return true;
        }

        default :
            return false; // Unknown opcode — bail rather than misinterpret.
    }
}

static bool cfi_vm_run(CfiVm *vm, const u8 *insns, u64 insns_size, u64 stop_at) {
    DwUCursor cur      = {.p = insns, .end = insns + insns_size};
    bool      stop_now = false;
    while (cur.p < cur.end) {
        if (!cfi_vm_step(vm, &cur, stop_at, &stop_now))
            return false;
        if (stop_now)
            break;
    }
    return true;
}

bool DwarfCfiBuildRow(const DwarfCfi *cfi, const DwarfFde *fde, u64 target_pc, DwarfUnwindRow *out) {
    if (!cfi || !fde || !out)
        return false;
    if (target_pc < fde->pc_begin || target_pc >= fde->pc_begin + fde->pc_range) {
        return false;
    }
    const DwarfCie *cie = DwarfCfiFindCie(cfi, fde->cie_offset);
    if (!cie)
        return false;

    CfiVm vm;
    cfi_vm_init(&vm, cie, fde->pc_begin, cie->return_address_register);

    if (!cfi_vm_run(&vm, cie->initial_instructions, cie->initial_instructions_size, (u64)-1)) {
        return false;
    }
    vm.initial = vm.row;

    if (!cfi_vm_run(&vm, fde->instructions, fde->instructions_size, target_pc)) {
        return false;
    }

    *out = vm.row;
    return true;
}
