/// file      : Pdb.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// PDB reader. Two layers in one file:
///
///   - MSF (Multi-Stream File) container: validates the superblock,
///     follows the directory-block map to reconstruct the stream
///     directory, then exposes a `read N bytes from stream S at offset
///     O` primitive on top.
///
///   - PDB-on-MSF: reads stream #1 (PDB Info) for GUID/age validation.
///     The Publics-stream walker that fills `functions` lives in
///     `PdbFunctions.c` (built in a later commit so this file stays
///     focused on the container layer).
///
/// Spec references:
///   - https://github.com/microsoft/microsoft-pdb (Microsoft's own
///     half-documented dump of the on-disk format)
///   - LLVM's `DebugInfo/PDB/Native/*` headers (the cleanest existing
///     open-source reader; we mirror the same field names where
///     practical)

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
    self->block_size   = read_u32_le(self->data + 32);
    u32 free_blk       = read_u32_le(self->data + 36);
    u32 num_blocks     = read_u32_le(self->data + 40);
    *out_num_dir_bytes = read_u32_le(self->data + 44);
    /* u32 unknown        */ read_u32_le(self->data + 48);
    *out_block_map_addr = read_u32_le(self->data + 52);
    (void)free_blk;

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
        // Some PDBs have a directory big enough that the index array
        // itself spills multiple pages -- the spec calls these the
        // "block-map pages" and `block_map_addr` becomes a single
        // page index pointing at a *list* of more block indices.
        // For v1 we cap at one block-map page, which covers PDBs
        // under ~4 MB of directory bytes (i.e. enormous PDBs).
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
        u32 want = self->block_size;
        u32 done = i * self->block_size;
        if (done + want > num_dir_bytes)
            want = num_dir_bytes - done;
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
    self->num_streams = read_u32_le(self->stream_dir);
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
        self->stream_sizes[i] = read_u32_le(self->stream_dir + 4 + i * 4);
        u32 sz                = self->stream_sizes[i];
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
    self->info.version   = read_u32_le(buf + 0);
    self->info.signature = read_u32_le(buf + 4);
    self->info.age       = read_u32_le(buf + 8);
    MemCopy(self->info.guid, buf + 12, 16);
    return true;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool PdbFileOpenFromMemory(PdbFile *out, u8 *data, size data_size, Allocator *alloc) {
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

    return true;

fail:
    PdbFileDeinit(out);
    return false;
}

bool PdbFileOpen(PdbFile *out, const char *path, Allocator *alloc) {
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
        AllocatorFree(alloc, buf, capacity);
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
        AllocatorFree(self->allocator, self->data, self->data_size);
    }
    if (self->stream_dir && self->allocator) {
        AllocatorFree(self->allocator, self->stream_dir, self->stream_dir_size);
    }
    if (self->stream_sizes && self->allocator) {
        AllocatorFree(self->allocator, self->stream_sizes, self->num_streams * sizeof(u32));
    }
    if (self->stream_blocks && self->allocator) {
        AllocatorFree(self->allocator, self->stream_blocks, self->num_streams * sizeof(const u32 *));
    }
    if (self->stream_block_counts && self->allocator) {
        AllocatorFree(self->allocator, self->stream_block_counts, self->num_streams * sizeof(u32));
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
