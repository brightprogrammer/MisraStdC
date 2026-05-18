/// file      : Pdb.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PDB reader: MSF container, PDB Info stream (GUID + age), DBI +
/// SymRecord + SectionHdr streams (sorted RVA -> name table).

#include <Misra/Parsers/ByteIter.h>
#include <Misra/Parsers/Pdb.h>
#include <Misra/Std.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static const char kMsfMagic7[32] = {'M', 'i', 'c',  'r',  'o',    's', 'o', 'f',  't',  ' ', 'C',
                                    '/', 'C', '+',  '+',  ' ',    'M', 'S', 'F',  ' ',  '7', '.',
                                    '0', '0', '\r', '\n', '\x1A', 'D', 'S', '\0', '\0', '\0'};

enum {
    SUPERBLOCK_SIZE = 56,
    NIL_STREAM      = 0xFFFFFFFFu,
};

// ---------------------------------------------------------------------------
// On-disk record layouts (LE)
// ---------------------------------------------------------------------------

#define FMT_PDB_SUPERBLOCK_LE                                                                                          \
    "{<4r}" /* block_size           */                                                                                 \
    "{<4r}" /* free_block_map_block */                                                                                 \
    "{<4r}" /* num_blocks           */                                                                                 \
    "{<4r}" /* num_directory_bytes  */                                                                                 \
    "{<4r}" /* unknown              */                                                                                 \
    "{<4r}" /* block_map_addr       */

// PDB Info stream header: 3 u32s (version, signature, age). GUID +
// stream-name table follow but the parser only needs the prefix.
#define FMT_PDB_INFO_LE                                                                                                \
    "{<4r}" /* version   */                                                                                            \
    "{<4r}" /* signature */                                                                                            \
    "{<4r}" /* age       */

// DBI header (first 64 bytes). u32 = read; u16 = read; mixed.
// S_PUB32 record body prefix (10 bytes). Variable-length NUL-terminated name follows.
#define FMT_S_PUB32_PREFIX_LE                                                                                          \
    "{<4r}" /* flags   */                                                                                              \
    "{<4r}" /* offset  */                                                                                              \
    "{<2r}" /* segment */

#define FMT_PDB_DBI_HEADER_LE                                                                                          \
    "{<4r}" /* version_signature    */                                                                                 \
    "{<4r}" /* version_header       */                                                                                 \
    "{<4r}" /* age                  */                                                                                 \
    "{<2r}" /* global_stream_index  */                                                                                 \
    "{<2r}" /* build_number         */                                                                                 \
    "{<2r}" /* public_stream_index  */                                                                                 \
    "{<2r}" /* pdb_dll_version      */                                                                                 \
    "{<2r}" /* sym_record_stream    */                                                                                 \
    "{<2r}" /* pdb_dll_rbld         */                                                                                 \
    "{<4r}" /* mod_info_size        */                                                                                 \
    "{<4r}" /* section_contrib_size */                                                                                 \
    "{<4r}" /* section_map_size     */                                                                                 \
    "{<4r}" /* source_info_size     */                                                                                 \
    "{<4r}" /* type_server_map_size */                                                                                 \
    "{<4r}" /* mfc_type_server_idx  */                                                                                 \
    "{<4r}" /* opt_dbg_header_size  */                                                                                 \
    "{<4r}" /* ec_substream_size    */                                                                                 \
    "{<2r}" /* flags                */                                                                                 \
    "{<2r}" /* machine              */                                                                                 \
    "{<4r}" /* padding              */

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static u32 read_u32_le(const u8 *p) {
    return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24;
}

static u32 div_ceil_u32(u32 a, u32 b) {
    return (a + b - 1u) / b;
}

// Return a pointer to the first byte of block `block_id` inside the
// PDB file. NULL on out-of-range.
static const u8 *block_ptr(const PdbFile *self, u32 block_id) {
    u64 off = (u64)block_id * self->block_size;
    if (off + self->block_size > self->data_size)
        return NULL;
    return self->data + off;
}

// Read `n` bytes from stream `idx` starting at byte offset `offset`
// into `dest`. Walks the stream's block chain.
static bool stream_read(const PdbFile *self, u32 idx, u64 offset, u8 *dest, u64 n) {
    if (idx >= self->num_streams)
        return false;
    u32 size = self->stream_sizes[idx];
    if (size == NIL_STREAM)
        return false;
    if (offset + n > size)
        return false;

    const u32 *blocks      = self->stream_blocks[idx];
    u32        block_count = self->stream_block_counts[idx];

    u64 remaining = n;
    while (remaining > 0) {
        u32 page  = (u32)(offset / self->block_size);
        u32 inoff = (u32)(offset % self->block_size);
        if (page >= block_count)
            return false;
        u32       block_id = blocks[page];
        const u8 *src      = block_ptr(self, block_id);
        if (!src)
            return false;
        u64 chunk = self->block_size - inoff;
        if (chunk > remaining)
            chunk = remaining;
        MemCopy(dest, src + inoff, chunk);
        dest      += chunk;
        offset    += chunk;
        remaining -= chunk;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Superblock + directory
// ---------------------------------------------------------------------------

static bool parse_superblock(PdbFile *self, u32 *out_num_dir_bytes, u32 *out_block_map_addr) {
    if (self->data_size < SUPERBLOCK_SIZE) {
        LOG_ERROR("PDB: file too small for MSF superblock");
        return false;
    }
    if (MemCompare(self->data, kMsfMagic7, sizeof(kMsfMagic7)) != 0) {
        LOG_ERROR("PDB: bad MSF magic (not 7.00)");
        return false;
    }
    ByteIter sb = ByteIterFromMemory(self->data + 32, self->data_size - 32);
    u32      free_blk, num_blocks, unknown;
    if (!ByteIterReadFmt(
            &sb,
            FMT_PDB_SUPERBLOCK_LE,
            self->block_size,
            free_blk,
            num_blocks,
            *out_num_dir_bytes,
            unknown,
            *out_block_map_addr
        )) {
        LOG_ERROR("PDB: superblock truncated");
        return false;
    }
    (void)free_blk;
    (void)unknown;

    if (self->block_size != 512 && self->block_size != 1024 && self->block_size != 2048 && self->block_size != 4096) {
        LOG_ERROR("PDB: unsupported MSF block size {}", self->block_size);
        return false;
    }
    if ((u64)num_blocks * self->block_size != self->data_size) {
        // Some PDBs come with trailing padding; the spec says num_blocks
        // * block_size should equal file size. We warn but don't fail.
        // No-op in v1 (kept for future "strict" mode).
    }
    return true;
}

// Reconstruct the stream directory into a contiguous buffer.
static bool reconstruct_directory(PdbFile *self, u32 num_dir_bytes, u32 block_map_addr) {
    // The block_map_addr page holds an array of u32 block indices,
    // one per `block_size` chunk of the directory. The number of
    // those indices is `ceil(num_dir_bytes / block_size)`.
    u32 num_dir_blocks = div_ceil_u32(num_dir_bytes, self->block_size);
    if ((u64)num_dir_blocks * sizeof(u32) > self->block_size) {
        LOG_ERROR("PDB: directory block-map exceeds one page");
        return false;
    }
    const u8 *map_page = block_ptr(self, block_map_addr);
    if (!map_page) {
        LOG_ERROR("PDB: block_map_addr out of range");
        return false;
    }
    self->dir_stream_blocks       = (const u32 *)map_page;
    self->dir_stream_blocks_count = num_dir_blocks;

    self->stream_dir_size = num_dir_bytes;
    self->stream_dir      = AllocatorAlloc(self->allocator, num_dir_bytes, /*zeroed=*/0);
    if (!self->stream_dir)
        return false;

    for (u32 i = 0; i < num_dir_blocks; ++i) {
        u32       block_id = read_u32_le((const u8 *)&self->dir_stream_blocks[i]);
        const u8 *src      = block_ptr(self, block_id);
        if (!src) {
            LOG_ERROR("PDB: directory block id {} out of range", block_id);
            return false;
        }
        // Promote to u64; i * block_size can wrap u32.
        u64 done = (u64)i * (u64)self->block_size;
        u64 want = self->block_size;
        if (done + want > num_dir_bytes) {
            if (done >= num_dir_bytes) {
                LOG_ERROR("PDB: directory copy offset past stream-dir size");
                return false;
            }
            want = num_dir_bytes - done;
        }
        MemCopy(self->stream_dir + done, src, want);
    }
    return true;
}

// Parse the reconstructed directory bytes into per-stream metadata.
static bool parse_directory(PdbFile *self) {
    if (self->stream_dir_size < 4) {
        LOG_ERROR("PDB: directory truncated (no stream count)");
        return false;
    }
    ByteIter dir_iter = ByteIterFromMemory(self->stream_dir, self->stream_dir_size);
    if (!bi_take_u32_le(&dir_iter, &self->num_streams))
        return false;
    if (self->num_streams == 0)
        return true;

    u64 expected = 4 + (u64)self->num_streams * 4;
    if (expected > self->stream_dir_size) {
        LOG_ERROR("PDB: directory truncated in sizes table");
        return false;
    }

    self->stream_sizes        = AllocatorAlloc(self->allocator, self->num_streams * sizeof(u32), 0);
    self->stream_blocks       = AllocatorAlloc(self->allocator, self->num_streams * sizeof(const u32 *), 0);
    self->stream_block_counts = AllocatorAlloc(self->allocator, self->num_streams * sizeof(u32), 0);
    if (!self->stream_sizes || !self->stream_blocks || !self->stream_block_counts)
        return false;

    u64 total_block_words = 0;
    for (u32 i = 0; i < self->num_streams; ++i) {
        if (!bi_take_u32_le(&dir_iter, &self->stream_sizes[i]))
            return false;
        u32 sz = self->stream_sizes[i];
        if (sz != NIL_STREAM) {
            total_block_words += div_ceil_u32(sz, self->block_size);
        }
    }
    if (expected + total_block_words * 4 > self->stream_dir_size) {
        LOG_ERROR("PDB: directory truncated in block-id table");
        return false;
    }

    // Block indices follow. Each stream's block list is a contiguous
    // run inside the directory; we record a pointer to it.
    const u8 *block_cursor = self->stream_dir + expected;
    for (u32 i = 0; i < self->num_streams; ++i) {
        u32 sz = self->stream_sizes[i];
        if (sz == NIL_STREAM) {
            self->stream_blocks[i]       = NULL;
            self->stream_block_counts[i] = 0;
            continue;
        }
        u32 cnt                       = div_ceil_u32(sz, self->block_size);
        self->stream_blocks[i]        = (const u32 *)block_cursor;
        self->stream_block_counts[i]  = cnt;
        block_cursor                 += cnt * sizeof(u32);
    }
    return true;
}

// ---------------------------------------------------------------------------
// PDB Info stream (#1)
// ---------------------------------------------------------------------------

static bool parse_pdb_info(PdbFile *self) {
    if (self->num_streams <= 1)
        return true; // no info stream
    if (self->stream_sizes[1] == NIL_STREAM)
        return true;
    if (self->stream_sizes[1] < 28) {
        LOG_ERROR("PDB: info stream too small");
        return false;
    }
    u8 buf[28];
    if (!stream_read(self, 1, 0, buf, sizeof(buf)))
        return false;
    ByteIter bi = ByteIterFromMemory(buf, sizeof(buf));
    if (!ByteIterReadFmt(&bi, FMT_PDB_INFO_LE, self->info.version, self->info.signature, self->info.age)) {
        LOG_ERROR("PDB: info stream prefix truncated");
        return false;
    }
    MemCopy(self->info.guid, buf + 12, 16);
    return true;
}

// ---------------------------------------------------------------------------
// DBI stream (#3) -- enough of it to find SymRecord + SectionHdr streams
// ---------------------------------------------------------------------------

enum {
    DBI_STREAM_INDEX          = 3,
    DBI_HEADER_SIZE           = 64,
    OPT_DBG_SECTION_HDR_INDEX = 5, // offset in u16 words into OptionalDbgHeader
    CV_SYMTYPE_PUB32          = 0x110E,
    CV_PUBSYM_FLAG_FUNCTION   = 0x2,
};

typedef struct DbiSubstreamInfo {
    u16  symrec_stream;
    u16  section_hdr_stream;
    bool ok;
} DbiSubstreamInfo;

static DbiSubstreamInfo parse_dbi_header(const PdbFile *self) {
    DbiSubstreamInfo r = {0};
    if (DBI_STREAM_INDEX >= self->num_streams)
        return r;
    u32 dbi_size = self->stream_sizes[DBI_STREAM_INDEX];
    if (dbi_size == NIL_STREAM || dbi_size < DBI_HEADER_SIZE)
        return r;

    u8 hdr[DBI_HEADER_SIZE];
    if (!stream_read(self, DBI_STREAM_INDEX, 0, hdr, DBI_HEADER_SIZE))
        return r;

    ByteIter bi = ByteIterFromMemory(hdr, DBI_HEADER_SIZE);
    u32      version_sig, version_hdr, age, mod_size, seccontrib, secmap, srcinfo, tsm, mfc_tsm_idx, optdbg_size,
        ec_size, padding;
    u16 global_idx, build_num, public_idx, pdb_dll_ver, pdb_dll_rbld, flags, machine;
    if (!ByteIterReadFmt(
            &bi,
            FMT_PDB_DBI_HEADER_LE,
            version_sig,
            version_hdr,
            age,
            global_idx,
            build_num,
            public_idx,
            pdb_dll_ver,
            r.symrec_stream,
            pdb_dll_rbld,
            mod_size,
            seccontrib,
            secmap,
            srcinfo,
            tsm,
            mfc_tsm_idx,
            optdbg_size,
            ec_size,
            flags,
            machine,
            padding
        )) {
        return r;
    }
    (void)version_sig;
    (void)version_hdr;
    (void)age;
    (void)global_idx;
    (void)build_num;
    (void)public_idx;
    (void)pdb_dll_ver;
    (void)pdb_dll_rbld;
    (void)mfc_tsm_idx;
    (void)flags;
    (void)machine;
    (void)padding;

    u64 optdbg_off = (u64)DBI_HEADER_SIZE + mod_size + seccontrib + secmap + srcinfo + tsm + ec_size;
    if (optdbg_off + optdbg_size > dbi_size)
        return r;

    // OptionalDbgHeader is an array of u16 stream indices; index 5 is
    // SectionHdr. Earlier PDBs may have a shorter array (we check the
    // bounds before reading).
    u64 sec_hdr_off = optdbg_off + (u64)OPT_DBG_SECTION_HDR_INDEX * 2;
    if (sec_hdr_off + 2 > optdbg_off + optdbg_size)
        return r;

    u8 sh[2];
    if (!stream_read(self, DBI_STREAM_INDEX, sec_hdr_off, sh, 2))
        return r;
    r.section_hdr_stream = (u16)sh[0] | (u16)sh[1] << 8;

    r.ok = true;
    return r;
}

// ---------------------------------------------------------------------------
// Publics walker (S_PUB32 records inside the SymRecord stream)
// ---------------------------------------------------------------------------

typedef struct SectionRva {
    u32 virtual_address;
    u32 virtual_size;
} SectionRva;

// Read all IMAGE_SECTION_HEADERs out of the SectionHdr stream and
// return a small allocator-backed array of (RVA, VSize) pairs. Caller
// frees via AllocatorFree.
static SectionRva *load_section_table(const PdbFile *self, u16 section_hdr_stream, u32 *out_count) {
    *out_count = 0;
    if (section_hdr_stream >= self->num_streams)
        return NULL;
    if (self->stream_sizes[section_hdr_stream] == NIL_STREAM)
        return NULL;
    u32 sz = self->stream_sizes[section_hdr_stream];
    if (sz % 40 != 0) {
        // Not a clean array of IMAGE_SECTION_HEADER -- bail.
        LOG_ERROR("PDB: section-hdr stream size {} not multiple of 40", sz);
        return NULL;
    }
    u32         n   = sz / 40;
    SectionRva *out = AllocatorAlloc(self->allocator, n * sizeof(SectionRva), 0);
    if (!out)
        return NULL;

    u8 *buf = AllocatorAlloc(self->allocator, sz, 0);
    if (!buf) {
        AllocatorFree(self->allocator, out);
        return NULL;
    }
    bool read_ok = stream_read(self, section_hdr_stream, 0, buf, sz);
    if (!read_ok) {
        AllocatorFree(self->allocator, buf);
        AllocatorFree(self->allocator, out);
        return NULL;
    }
    for (u32 i = 0; i < n; ++i) {
        // IMAGE_SECTION_HEADER: name[8] + VirtualSize(4) + VirtualAddress(4) + ...
        ByteIter rec = ByteIterFromMemory(buf + i * 40 + 8, 40 - 8);
        (void)bi_take_u32_le(&rec, &out[i].virtual_size);
        (void)bi_take_u32_le(&rec, &out[i].virtual_address);
    }
    AllocatorFree(self->allocator, buf);
    *out_count = n;
    return out;
}

// Append `s` (NUL-terminated, plus its terminator) to the name pool
// and return the offset at which it was written. The pool is owned by
// the caller (a Str); we keep names there because pool resizing
// invalidates earlier pointers -- we resolve to pointers in a second
// pass after the walk completes.
static bool pool_append_cstr(Str *pool, const char *s, u64 *out_offset) {
    *out_offset = pool->length;
    for (; *s; ++s) {
        if (!StrPushBack(pool, *s))
            return false;
    }
    return StrPushBack(pool, '\0');
}

// Walk the SymRecord stream, picking out S_PUB32 entries and pushing
// `(rva, name_offset)` tuples into `pending`. Name strings go into
// `pool`. RVAs are computed from (Segment, Offset) using `sections`.
typedef struct PendingPub {
    u32 rva;
    u64 name_offset_in_pool;
} PendingPub;

typedef Vec(PendingPub) PendingPubs;

static int cmp_pending(const void *a, const void *b) {
    const PendingPub *pa = a;
    const PendingPub *pb = b;
    if (pa->rva < pb->rva)
        return -1;
    if (pa->rva > pb->rva)
        return 1;
    return 0;
}

static bool walk_publics(
    const PdbFile    *self,
    u16               symrec_stream,
    const SectionRva *sections,
    u32               num_sections,
    Str              *pool,
    PendingPubs      *pending
) {
    if (symrec_stream >= self->num_streams)
        return false;
    if (self->stream_sizes[symrec_stream] == NIL_STREAM)
        return true;
    u32 sz = self->stream_sizes[symrec_stream];
    if (sz == 0)
        return true;

    // Stream into a flat buffer; the record stream is typically large
    // but not unbounded.
    u8 *buf = AllocatorAlloc(self->allocator, sz, 0);
    if (!buf)
        return false;
    if (!stream_read(self, symrec_stream, 0, buf, sz)) {
        AllocatorFree(self->allocator, buf);
        return false;
    }

    u32 cur = 0;
    while (cur + 4 <= sz) {
        u16 rec_len  = (u16)buf[cur] | (u16)buf[cur + 1] << 8;
        u16 rec_kind = (u16)buf[cur + 2] | (u16)buf[cur + 3] << 8;
        if (rec_len < 2)
            break; // malformed
        u32 next = cur + 2 + rec_len;
        if (next > sz)
            break;

        if (rec_kind == CV_SYMTYPE_PUB32 && rec_len >= 2 + 4 + 4 + 2 + 1) {
            // Record body starts at cur + 4 (past len + kind). 10-byte
            // prefix (Flags/Offset/Segment) then NUL-terminated Name.
            ByteIter body = ByteIterFromMemory(buf + cur + 4, rec_len - 2);
            u32      flags, offset;
            u16      segment;
            if (!ByteIterReadFmt(&body, FMT_S_PUB32_PREFIX_LE, flags, offset, segment)) {
                cur = next;
                continue;
            }
            const char *name = (const char *)(body.data + body.pos);

            (void)flags; // permissive: we don't filter by FUNCTION bit;
                         // many real-world PDBs leave it unset.
            if (segment >= 1 && segment <= num_sections) {
                u32 rva = sections[segment - 1].virtual_address + offset;
                // Validate name is NUL-terminated within the record.
                bool ok_name = false;
                u32  end     = next;
                for (u32 p = cur + 14; p < end; ++p) {
                    if (buf[p] == 0) {
                        ok_name = true;
                        break;
                    }
                }
                if (ok_name && name[0]) {
                    PendingPub pp;
                    pp.rva = rva;
                    if (!pool_append_cstr(pool, name, &pp.name_offset_in_pool)) {
                        AllocatorFree(self->allocator, buf);
                        return false;
                    }
                    if (!VecPushBackR(pending, pp)) {
                        AllocatorFree(self->allocator, buf);
                        return false;
                    }
                }
            }
        }
        cur = next;
    }

    AllocatorFree(self->allocator, buf);
    return true;
}

// Top-level: pull DBI -> SectionHdr table + SymRecord stream -> publics.
static bool parse_pdb_functions(PdbFile *self) {
    DbiSubstreamInfo dbi = parse_dbi_header(self);
    if (!dbi.ok)
        return true; // No DBI / not enough -- just leave functions empty.

    u32         num_sections = 0;
    SectionRva *sections     = load_section_table(self, dbi.section_hdr_stream, &num_sections);
    if (!sections || num_sections == 0) {
        if (sections)
            AllocatorFree(self->allocator, sections);
        return true; // can't compute RVAs without section table
    }

    // Per-function names need an offset-into-pool indirection because
    // the pool may grow during the walk.
    Str         name_pool = StrInit(self->allocator);
    PendingPubs pending   = VecInitT(pending, self->allocator);
    bool        ok        = walk_publics(self, dbi.symrec_stream, sections, num_sections, &name_pool, &pending);
    AllocatorFree(self->allocator, sections);

    if (!ok) {
        VecDeinit(&pending);
        StrDeinit(&name_pool);
        return false;
    }

    if (pending.length == 0) {
        VecDeinit(&pending);
        StrDeinit(&name_pool);
        return true;
    }

    // Sort by RVA.
    VecSort(&pending, cmp_pending);

    // Re-anchor PendingPub.name_offset_in_pool to pointers into the
    // (now-stable) pool buffer, push into self->functions, and fill
    // sizes by next-rva diff. Steal the pool's buffer at the end so
    // names stay alive for the lifetime of the PdbFile.
    for (size i = 0; i < pending.length; ++i) {
        PdbFunction f = {
            .rva  = pending.data[i].rva,
            .size = 0,
            .name = name_pool.data + pending.data[i].name_offset_in_pool,
        };
        if (i + 1 < pending.length) {
            f.size = pending.data[i + 1].rva - f.rva;
        }
        if (!VecPushBackR(&self->functions, f)) {
            ok = false;
            break;
        }
    }
    VecDeinit(&pending);

    if (!ok) {
        StrDeinit(&name_pool);
        return false;
    }

    // Transfer ownership of the name-pool buffer to the PdbFile so the
    // function->name pointers stay valid. We stash it in a dedicated
    // field for cleanup; see PdbFileDeinit.
    self->name_pool      = name_pool.data;
    self->name_pool_size = name_pool.capacity;
    self->name_pool_used = name_pool.length;
    // Suppress the Str's own free of `data` so we don't double-free.
    name_pool.data     = NULL;
    name_pool.length   = 0;
    name_pool.capacity = 0;
    StrDeinit(&name_pool);

    return true;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool pdb_file_open_from_memory(PdbFile *out, u8 *data, size data_size, Allocator *alloc) {
    if (!out || !data || !alloc) {
        LOG_ERROR("PdbFileOpenFromMemory: NULL argument");
        return false;
    }
    MemSet(out, 0, sizeof(*out));
    out->allocator = alloc;
    out->data      = data;
    out->data_size = data_size;
    out->owns_data = false;
    out->functions = VecInitT(out->functions, alloc);

    u32 num_dir_bytes  = 0;
    u32 block_map_addr = 0;
    if (!parse_superblock(out, &num_dir_bytes, &block_map_addr))
        goto fail;
    if (!reconstruct_directory(out, num_dir_bytes, block_map_addr))
        goto fail;
    if (!parse_directory(out))
        goto fail;
    if (!parse_pdb_info(out))
        goto fail;
    if (!parse_pdb_functions(out))
        goto fail;

    return true;

fail:
    PdbFileDeinit(out);
    return false;
}

bool pdb_file_open(PdbFile *out, const char *path, Allocator *alloc) {
    if (!out || !path || !alloc) {
        LOG_ERROR("PdbFileOpen: NULL argument");
        return false;
    }
    char *buf      = NULL;
    u64   bytes    = 0;
    u64   capacity = 0;
    if (!ReadCompleteFile(path, &buf, &bytes, &capacity, alloc)) {
        LOG_ERROR("PdbFileOpen: failed to read {}", path);
        return false;
    }
    if (!PdbFileOpenFromMemory(out, (u8 *)buf, (size)bytes, alloc)) {
        AllocatorFree(alloc, buf);
        return false;
    }
    out->owns_data = true;
    out->data      = (u8 *)buf;
    out->data_size = (size)bytes;
    (void)capacity;
    return true;
}

void PdbFileDeinit(PdbFile *self) {
    if (!self)
        return;
    if (self->owns_data && self->data && self->allocator) {
        AllocatorFree(self->allocator, self->data);
    }
    if (self->name_pool && self->allocator) {
        AllocatorFree(self->allocator, self->name_pool);
    }
    if (self->stream_dir && self->allocator) {
        AllocatorFree(self->allocator, self->stream_dir);
    }
    if (self->stream_sizes && self->allocator) {
        AllocatorFree(self->allocator, self->stream_sizes);
    }
    if (self->stream_blocks && self->allocator) {
        AllocatorFree(self->allocator, self->stream_blocks);
    }
    if (self->stream_block_counts && self->allocator) {
        AllocatorFree(self->allocator, self->stream_block_counts);
    }
    VecDeinit(&self->functions);
    MemSet(self, 0, sizeof(*self));
}

const PdbFunction *PdbFileResolveRva(const PdbFile *self, u32 rva) {
    if (!self || self->functions.length == 0)
        return NULL;
    // Binary search for the largest rva <= input.
    size lo = 0, hi = self->functions.length;
    while (lo < hi) {
        size mid = lo + (hi - lo) / 2;
        if (self->functions.data[mid].rva <= rva)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return NULL;
    const PdbFunction *f = &self->functions.data[lo - 1];
    // size == 0 means "until next entry"; we already accept that case.
    if (f->size > 0 && rva >= f->rva + f->size)
        return NULL;
    return f;
}
