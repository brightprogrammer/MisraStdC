/// file      : DwarfInfo.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// `.debug_info` -> function-name index. Walks the DIE tree of every
/// compilation unit, filters for `DW_TAG_subprogram`, and harvests
/// (`DW_AT_name`, `DW_AT_low_pc`, `DW_AT_high_pc`) triples into a
/// sorted-by-low_pc table.
///
/// This is the fallback path used when `.symtab` is stripped but
/// debug info remains (own-binary debug build, sidecar `.debug` file,
/// objcopy --only-keep-debug output). When `.symtab` is present, the
/// resolver still prefers it -- those entries are first-class symbols
/// with sizes and visibility; `.debug_info` only knows function bodies.
///
/// v1 scope: DWARF version 4, 32-bit length form, contiguous functions
/// only (`DW_AT_low_pc` + `DW_AT_high_pc`; `DW_AT_ranges` is skipped).
/// Discontiguous functions just don't end up in the table; the resolver
/// still falls back to `module+offset`.

#include <Misra/Parsers/Dwarf.h>
#include <Misra/Std.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// ---------------------------------------------------------------------------
// DWARF constants (subset we use)
// ---------------------------------------------------------------------------

enum {
    DW_TAG_compile_unit = 0x11,
    DW_TAG_subprogram   = 0x2e,
};

enum {
    DW_AT_name              = 0x03,
    DW_AT_low_pc            = 0x11,
    DW_AT_high_pc           = 0x12,
    DW_AT_specification     = 0x47,
    DW_AT_linkage_name      = 0x6e,
    DW_AT_MIPS_linkage_name = 0x2007,
};

enum {
    DW_FORM_addr         = 0x01,
    DW_FORM_block2       = 0x03,
    DW_FORM_block4       = 0x04,
    DW_FORM_data2        = 0x05,
    DW_FORM_data4        = 0x06,
    DW_FORM_data8        = 0x07,
    DW_FORM_string       = 0x08,
    DW_FORM_block        = 0x09,
    DW_FORM_block1       = 0x0a,
    DW_FORM_data1        = 0x0b,
    DW_FORM_flag         = 0x0c,
    DW_FORM_sdata        = 0x0d,
    DW_FORM_strp         = 0x0e,
    DW_FORM_udata        = 0x0f,
    DW_FORM_ref_addr     = 0x10,
    DW_FORM_ref1         = 0x11,
    DW_FORM_ref2         = 0x12,
    DW_FORM_ref4         = 0x13,
    DW_FORM_ref8         = 0x14,
    DW_FORM_ref_udata    = 0x15,
    DW_FORM_indirect     = 0x16,
    DW_FORM_sec_offset   = 0x17,
    DW_FORM_exprloc      = 0x18,
    DW_FORM_flag_present = 0x19,
    DW_FORM_ref_sig8     = 0x20,
};

// ---------------------------------------------------------------------------
// Byte cursor (local copy; the one in Dwarf.c is file-static)
// ---------------------------------------------------------------------------

typedef struct ByteCursor {
    const u8 *p;
    const u8 *end;
} ByteCursor;

static bool bc_take_u8(ByteCursor *c, u8 *out) {
    if (c->p >= c->end)
        return false;
    *out = *c->p++;
    return true;
}
static bool bc_take_u16_le(ByteCursor *c, u16 *out) {
    if (c->end - c->p < 2)
        return false;
    *out  = (u16)c->p[0] | (u16)c->p[1] << 8;
    c->p += 2;
    return true;
}
static bool bc_take_u32_le(ByteCursor *c, u32 *out) {
    if (c->end - c->p < 4)
        return false;
    *out  = (u32)c->p[0] | (u32)c->p[1] << 8 | (u32)c->p[2] << 16 | (u32)c->p[3] << 24;
    c->p += 4;
    return true;
}
static bool bc_take_u64_le(ByteCursor *c, u64 *out) {
    if (c->end - c->p < 8)
        return false;
    *out = 0;
    for (int i = 0; i < 8; ++i)
        *out |= (u64)c->p[i] << (i * 8);
    c->p += 8;
    return true;
}
static bool bc_take_uleb128(ByteCursor *c, u64 *out) {
    u64 result = 0;
    int shift  = 0;
    while (c->p < c->end) {
        u8 b    = *c->p++;
        result |= (u64)(b & 0x7f) << shift;
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
static bool bc_take_sleb128(ByteCursor *c, i64 *out) {
    i64 result = 0;
    int shift  = 0;
    u8  b      = 0;
    while (c->p < c->end) {
        b       = *c->p++;
        result |= (i64)(b & 0x7f) << shift;
        shift  += 7;
        if ((b & 0x80) == 0)
            break;
        if (shift >= 64)
            return false;
    }
    if (shift < 64 && (b & 0x40))
        result |= -((i64)1 << shift);
    *out = result;
    return true;
}
static const char *bc_take_cstr(ByteCursor *c) {
    const char *s = (const char *)c->p;
    while (c->p < c->end && *c->p)
        ++c->p;
    if (c->p >= c->end)
        return NULL;
    ++c->p; // consume NUL
    return s;
}
static bool bc_skip(ByteCursor *c, u64 n) {
    if ((u64)(c->end - c->p) < n)
        return false;
    c->p += n;
    return true;
}

// ---------------------------------------------------------------------------
// Abbreviation table (per-CU)
// ---------------------------------------------------------------------------

typedef struct AbbrevAttr {
    u32 name; // DW_AT_*
    u32 form; // DW_FORM_*
    // DW_FORM_implicit_const carries a signed constant baked into the
    // abbrev itself, but that's a DWARF 5 form so we ignore it.
} AbbrevAttr;

typedef Vec(AbbrevAttr) AbbrevAttrVec;

typedef struct AbbrevEntry {
    u64           code;
    u32           tag;
    bool          has_children;
    AbbrevAttrVec attrs;
} AbbrevEntry;

typedef Vec(AbbrevEntry) AbbrevTable;

static void abbrev_table_deinit(AbbrevTable *t) {
    for (size i = 0; i < t->length; ++i) {
        VecDeinit(&t->data[i].attrs);
    }
    VecDeinit(t);
}

// Parse the abbrev table starting at `start`. The table is terminated
// by an entry with code 0. Returns false on malformed input.
static bool parse_abbrev_table(ByteCursor cur, AbbrevTable *out, Allocator *alloc) {
    *out = VecInitT(*out, alloc);
    while (cur.p < cur.end) {
        u64 code;
        if (!bc_take_uleb128(&cur, &code)) {
            LOG_ERROR("DWARF info: malformed abbrev table (code)");
            return false;
        }
        if (code == 0)
            return true;

        u64 tag;
        if (!bc_take_uleb128(&cur, &tag))
            return false;
        u8 has_children;
        if (!bc_take_u8(&cur, &has_children))
            return false;

        AbbrevEntry e;
        e.code         = code;
        e.tag          = (u32)tag;
        e.has_children = has_children != 0;
        e.attrs        = VecInitT(e.attrs, alloc);

        for (;;) {
            u64 name, form;
            if (!bc_take_uleb128(&cur, &name)) {
                VecDeinit(&e.attrs);
                return false;
            }
            if (!bc_take_uleb128(&cur, &form)) {
                VecDeinit(&e.attrs);
                return false;
            }
            if (name == 0 && form == 0)
                break;
            AbbrevAttr a = {.name = (u32)name, .form = (u32)form};
            if (!VecPushBackR(&e.attrs, a)) {
                VecDeinit(&e.attrs);
                return false;
            }
        }
        if (!VecPushBackR(out, e)) {
            VecDeinit(&e.attrs);
            return false;
        }
    }
    // Hitting end-of-cursor without a terminator is suspicious but not
    // strictly fatal: callers will fail to find later codes and that's
    // a separate error.
    return true;
}

static const AbbrevEntry *abbrev_table_find(const AbbrevTable *t, u64 code) {
    for (size i = 0; i < t->length; ++i) {
        if (t->data[i].code == code)
            return &t->data[i];
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Attribute value
// ---------------------------------------------------------------------------

typedef enum {
    ATTR_VAL_NONE = 0,
    ATTR_VAL_U64,
    ATTR_VAL_I64,
    ATTR_VAL_CSTR,       // pointer to bytes inside .debug_info (or .debug_str)
    ATTR_VAL_STR_OFFSET, // pending lookup into .debug_str
    ATTR_VAL_FLAG_PRESENT,
} AttrValKind;

typedef struct AttrVal {
    AttrValKind kind;
    union {
        u64         u;
        i64         i;
        const char *s;
        u64         off;
    };
} AttrVal;

// Read an attribute value of `form` from `cur` given the CU's
// `addr_size`. The result is stored in `out`. Indirect/exprloc/block
// forms have their data skipped but yield ATTR_VAL_NONE.
//
// Returns false only on truncated / malformed bytes; unknown forms
// fail-closed so the caller can stop cleanly.
static bool read_form(ByteCursor *cur, u32 form, u8 addr_size, AttrVal *out) {
    *out = (AttrVal) {.kind = ATTR_VAL_NONE};
    switch (form) {
        case DW_FORM_addr : {
            u64 v;
            if (addr_size == 4) {
                u32 v32;
                if (!bc_take_u32_le(cur, &v32))
                    return false;
                v = v32;
            } else if (addr_size == 8) {
                if (!bc_take_u64_le(cur, &v))
                    return false;
            } else {
                LOG_ERROR("DWARF info: unsupported addr_size {}", (u32)addr_size);
                return false;
            }
            *out = (AttrVal) {.kind = ATTR_VAL_U64, .u = v};
            return true;
        }
        case DW_FORM_data1 :
        case DW_FORM_flag :
        case DW_FORM_ref1 : {
            u8 v;
            if (!bc_take_u8(cur, &v))
                return false;
            *out = (AttrVal) {.kind = ATTR_VAL_U64, .u = v};
            return true;
        }
        case DW_FORM_data2 :
        case DW_FORM_ref2 : {
            u16 v;
            if (!bc_take_u16_le(cur, &v))
                return false;
            *out = (AttrVal) {.kind = ATTR_VAL_U64, .u = v};
            return true;
        }
        case DW_FORM_data4 :
        case DW_FORM_ref4 :
        case DW_FORM_strp :
        case DW_FORM_sec_offset :
        case DW_FORM_ref_addr : {
            // ref_addr is 4 bytes in 32-bit DWARF, 8 in 64-bit; v1
            // doesn't support 64-bit DWARF so it's always 4 here.
            u32 v;
            if (!bc_take_u32_le(cur, &v))
                return false;
            if (form == DW_FORM_strp) {
                *out = (AttrVal) {.kind = ATTR_VAL_STR_OFFSET, .off = v};
            } else {
                *out = (AttrVal) {.kind = ATTR_VAL_U64, .u = v};
            }
            return true;
        }
        case DW_FORM_data8 :
        case DW_FORM_ref8 :
        case DW_FORM_ref_sig8 : {
            u64 v;
            if (!bc_take_u64_le(cur, &v))
                return false;
            *out = (AttrVal) {.kind = ATTR_VAL_U64, .u = v};
            return true;
        }
        case DW_FORM_udata :
        case DW_FORM_ref_udata : {
            u64 v;
            if (!bc_take_uleb128(cur, &v))
                return false;
            *out = (AttrVal) {.kind = ATTR_VAL_U64, .u = v};
            return true;
        }
        case DW_FORM_sdata : {
            i64 v;
            if (!bc_take_sleb128(cur, &v))
                return false;
            *out = (AttrVal) {.kind = ATTR_VAL_I64, .i = v};
            return true;
        }
        case DW_FORM_string : {
            const char *s = bc_take_cstr(cur);
            if (!s)
                return false;
            *out = (AttrVal) {.kind = ATTR_VAL_CSTR, .s = s};
            return true;
        }
        case DW_FORM_flag_present :
            *out = (AttrVal) {.kind = ATTR_VAL_FLAG_PRESENT, .u = 1};
            return true;
        case DW_FORM_block1 : {
            u8 n;
            if (!bc_take_u8(cur, &n))
                return false;
            return bc_skip(cur, n);
        }
        case DW_FORM_block2 : {
            u16 n;
            if (!bc_take_u16_le(cur, &n))
                return false;
            return bc_skip(cur, n);
        }
        case DW_FORM_block4 : {
            u32 n;
            if (!bc_take_u32_le(cur, &n))
                return false;
            return bc_skip(cur, n);
        }
        case DW_FORM_block :
        case DW_FORM_exprloc : {
            u64 n;
            if (!bc_take_uleb128(cur, &n))
                return false;
            return bc_skip(cur, n);
        }
        case DW_FORM_indirect : {
            u64 actual_form;
            if (!bc_take_uleb128(cur, &actual_form))
                return false;
            return read_form(cur, (u32)actual_form, addr_size, out);
        }
        default :
            LOG_ERROR("DWARF info: unsupported FORM {x}", form);
            return false;
    }
}

// ---------------------------------------------------------------------------
// CU walk
// ---------------------------------------------------------------------------

// Pending entry: we want to push into `out->entries`, but the name
// pointer must come from `string_pool` which may relocate during
// growth. We store the offset and patch in a final pass.
typedef struct PendingFn {
    u64 low_pc;
    u64 high_pc;
    u64 name_offset_in_pool;
} PendingFn;

typedef Vec(PendingFn) PendingFns;

static bool walk_cu_dies(
    ByteCursor         cu_cur, // positioned past CU header
    const AbbrevTable *abbrevs,
    u8                 addr_size,
    const u8          *debug_str,
    u64                debug_str_size,
    Str               *pool,
    PendingFns        *pending
) {
    int depth = 0;
    for (;;) {
        if (cu_cur.p >= cu_cur.end)
            return true;

        u64 abbrev_code;
        if (!bc_take_uleb128(&cu_cur, &abbrev_code)) {
            LOG_ERROR("DWARF info: truncated DIE");
            return false;
        }
        if (abbrev_code == 0) {
            // End-of-sibling-list marker; pop a level of nesting.
            if (depth == 0)
                return true; // end of CU
            --depth;
            continue;
        }

        const AbbrevEntry *e = abbrev_table_find(abbrevs, abbrev_code);
        if (!e) {
            LOG_ERROR("DWARF info: unknown abbrev code {}", (u32)abbrev_code);
            return false;
        }

        // For subprograms, collect interesting attrs; for everything
        // else, just advance the cursor past their data.
        bool        is_subprogram = (e->tag == DW_TAG_subprogram);
        bool        have_name = false, have_low = false, have_high = false;
        u64         low_pc = 0, high_pc = 0;
        bool        high_pc_is_offset = false;
        const char *name              = NULL;
        u64         name_str_off      = 0;
        bool        name_from_strp    = false;

        for (size ai = 0; ai < e->attrs.length; ++ai) {
            const AbbrevAttr *a = &e->attrs.data[ai];
            AttrVal           v;
            if (!read_form(&cu_cur, a->form, addr_size, &v))
                return false;

            if (!is_subprogram)
                continue;

            switch (a->name) {
                case DW_AT_name :
                case DW_AT_linkage_name :
                case DW_AT_MIPS_linkage_name : {
                    // Prefer DW_AT_name when it's present and unambiguous;
                    // otherwise accept linkage names as a fallback. We
                    // overwrite only if we don't already have a non-linkage
                    // name.
                    if (have_name && a->name != DW_AT_name)
                        break;
                    if (v.kind == ATTR_VAL_CSTR) {
                        name           = v.s;
                        name_from_strp = false;
                        have_name      = true;
                    } else if (v.kind == ATTR_VAL_STR_OFFSET) {
                        if (debug_str && v.off < debug_str_size) {
                            name_str_off   = v.off;
                            name_from_strp = true;
                            have_name      = true;
                        }
                    }
                    break;
                }
                case DW_AT_low_pc :
                    if (v.kind == ATTR_VAL_U64) {
                        low_pc   = v.u;
                        have_low = true;
                    }
                    break;
                case DW_AT_high_pc :
                    if (v.kind == ATTR_VAL_U64) {
                        // DWARF 4 rule: addr form -> absolute; constant
                        // form -> offset from low_pc. The form determines
                        // which.
                        high_pc           = v.u;
                        high_pc_is_offset = (a->form != DW_FORM_addr);
                        have_high         = true;
                    } else if (v.kind == ATTR_VAL_I64) {
                        high_pc           = (u64)v.i;
                        high_pc_is_offset = true;
                        have_high         = true;
                    }
                    break;
                default :
                    break;
            }
        }

        if (is_subprogram && have_name && have_low && have_high) {
            u64 hi = high_pc_is_offset ? low_pc + high_pc : high_pc;
            if (hi > low_pc) {
                // Resolve name into the pool now (or later if it came
                // from .debug_str — same pool either way).
                const char *src;
                if (name_from_strp) {
                    src = (const char *)(debug_str + name_str_off);
                } else {
                    src = name;
                }
                // Validate the string is NUL-terminated within bounds.
                // For strp-source, the segment is bounded by debug_str_size.
                u64 src_max = name_from_strp ? (debug_str_size - name_str_off) : 0x10000;
                u64 nlen    = 0;
                while (nlen < src_max && src[nlen] != '\0')
                    ++nlen;
                if (nlen > 0 && nlen < src_max) {
                    u64 offset = pool->length;
                    for (u64 i = 0; i < nlen; ++i) {
                        if (!StrPushBack(pool, src[i]))
                            return false;
                    }
                    if (!StrPushBack(pool, '\0'))
                        return false;
                    PendingFn pf = {.low_pc = low_pc, .high_pc = hi, .name_offset_in_pool = offset};
                    if (!VecPushBackR(pending, pf))
                        return false;
                }
            }
        }

        if (e->has_children)
            ++depth;
    }
}

static int cmp_dwarf_function(const void *a, const void *b) {
    const DwarfFunction *fa = a;
    const DwarfFunction *fb = b;
    if (fa->low_pc < fb->low_pc)
        return -1;
    if (fa->low_pc > fb->low_pc)
        return 1;
    return 0;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool DwarfFunctionsBuildFromSlices(
    DwarfFunctions *out,
    const u8       *info_bytes,
    u64             info_size,
    const u8       *abbrev_bytes,
    u64             abbrev_size,
    const u8       *str_bytes,
    u64             str_size,
    Allocator      *alloc
) {
    if (!out || !alloc) {
        LOG_ERROR("DwarfFunctionsBuildFromSlices: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator   = alloc;
    out->entries     = VecInitT(out->entries, alloc);
    out->string_pool = StrInit(alloc);

    if (!info_bytes || info_size == 0)
        return true; // no debug info -> empty success

    if (!abbrev_bytes || abbrev_size == 0) {
        LOG_ERROR("DWARF info: .debug_info present but .debug_abbrev missing");
        return false;
    }

    ByteCursor info_cur = {
        .p   = info_bytes,
        .end = info_bytes + info_size,
    };

    PendingFns pending = VecInitT(pending, alloc);

    bool ok = true;
    while (info_cur.p < info_cur.end) {
        const u8 *unit_start = info_cur.p;

        u32 unit_length;
        if (!bc_take_u32_le(&info_cur, &unit_length)) {
            ok = false;
            break;
        }
        if (unit_length == 0xffffffff) {
            LOG_ERROR("DWARF info: 64-bit length form not supported");
            ok = false;
            break;
        }
        const u8 *unit_end = unit_start + 4 + unit_length;
        if (unit_end > info_cur.end) {
            LOG_ERROR("DWARF info: CU overruns section");
            ok = false;
            break;
        }

        u16 version;
        u32 abbrev_offset;
        u8  addr_size;
        if (!bc_take_u16_le(&info_cur, &version) || !bc_take_u32_le(&info_cur, &abbrev_offset) ||
            !bc_take_u8(&info_cur, &addr_size)) {
            ok = false;
            break;
        }

        if (version != 4) {
            // Skip CUs in versions we don't yet parse; we still pick
            // up the rest of the binary's CUs in case some are v4.
            info_cur.p = unit_end;
            continue;
        }

        if (abbrev_offset >= abbrev_size) {
            LOG_ERROR("DWARF info: abbrev_offset past .debug_abbrev end");
            ok = false;
            break;
        }

        AbbrevTable abbrevs;
        ByteCursor  abbrev_cur = {
             .p   = abbrev_bytes + abbrev_offset,
             .end = abbrev_bytes + abbrev_size,
        };
        if (!parse_abbrev_table(abbrev_cur, &abbrevs, alloc)) {
            ok = false;
            break;
        }

        ByteCursor die_cur = {.p = info_cur.p, .end = unit_end};
        if (!walk_cu_dies(die_cur, &abbrevs, addr_size, str_bytes, str_size, &out->string_pool, &pending)) {
            abbrev_table_deinit(&abbrevs);
            ok = false;
            break;
        }
        abbrev_table_deinit(&abbrevs);

        info_cur.p = unit_end;
    }

    if (ok) {
        // Resolve name_offset_in_pool into char* now that string_pool
        // has stopped growing.
        for (size i = 0; i < pending.length; ++i) {
            const PendingFn *pf = &pending.data[i];
            DwarfFunction    f  = {
                    .low_pc  = pf->low_pc,
                    .high_pc = pf->high_pc,
                    .name    = out->string_pool.data + pf->name_offset_in_pool,
            };
            if (!VecPushBackR(&out->entries, f)) {
                ok = false;
                break;
            }
        }
        // Sort by low_pc to enable binary-search lookup.
        if (ok && out->entries.length > 1) {
            VecSort(&out->entries, cmp_dwarf_function);
        }
    } else {
        // Pool may already have content; that's fine — `out` is deinit'd
        // by the caller on failure paths or repurposed on success. The
        // caller calling Deinit on a failed-build is supported.
    }

    VecDeinit(&pending);

    if (!ok) {
        DwarfFunctionsDeinit(out);
        MemSet(out, 0, sizeof(*out));
    }

    return ok;
}

bool dwarf_functions_build_from_elf(DwarfFunctions *out, const ElfFile *elf, Allocator *alloc) {
    if (!out || !elf || !alloc) {
        LOG_ERROR("DwarfFunctionsBuildFromElf: NULL argument");
        return false;
    }
    const ElfSection *info_sec   = ElfFileFindSection(elf, ".debug_info");
    const ElfSection *abbrev_sec = ElfFileFindSection(elf, ".debug_abbrev");
    const ElfSection *str_sec    = ElfFileFindSection(elf, ".debug_str");

    const u8 *info_b   = info_sec ? elf->data + info_sec->offset : NULL;
    u64       info_n   = info_sec ? info_sec->size : 0;
    const u8 *abbrev_b = abbrev_sec ? elf->data + abbrev_sec->offset : NULL;
    u64       abbrev_n = abbrev_sec ? abbrev_sec->size : 0;
    const u8 *str_b    = str_sec ? elf->data + str_sec->offset : NULL;
    u64       str_n    = str_sec ? str_sec->size : 0;
    return DwarfFunctionsBuildFromSlices(out, info_b, info_n, abbrev_b, abbrev_n, str_b, str_n, alloc);
}

const DwarfFunction *DwarfFunctionsResolve(const DwarfFunctions *self, u64 vaddr) {
    if (!self || self->entries.length == 0)
        return NULL;
    // Binary-search for the largest low_pc <= vaddr.
    size lo = 0, hi = self->entries.length;
    while (lo < hi) {
        size mid = lo + (hi - lo) / 2;
        if (self->entries.data[mid].low_pc <= vaddr)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return NULL;
    const DwarfFunction *e = &self->entries.data[lo - 1];
    if (vaddr >= e->low_pc && vaddr < e->high_pc)
        return e;
    return NULL;
}

void DwarfFunctionsDeinit(DwarfFunctions *self) {
    if (!self)
        return;
    if (self->entries.allocator)
        VecDeinit(&self->entries);
    if (self->string_pool.allocator)
        StrDeinit(&self->string_pool);
    MemSet(self, 0, sizeof(*self));
}
