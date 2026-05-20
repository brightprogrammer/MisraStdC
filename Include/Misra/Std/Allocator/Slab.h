/// file      : std/allocator/slab.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Fixed-size slot allocator. Each "slab" is exactly one OS page of user
/// slots; the slab page contains *only* user data, no header, no inline
/// metadata. Per-slab free/in-use bitmaps live in a single packed buffer
/// owned by the SlabAllocator, separate from the slab pages themselves.
///
/// Slot size is fixed at init and MUST be a power of two in [16, PAGE_SIZE].
/// The 16-byte minimum matches `MAX_ALIGN` so every returned pointer is
/// safe for any payload type. The power-of-two constraint lets the
/// allocator translate offsets to slot indices and back via shifts, and
/// keeps the in-page bitmap free of tail-bit guards.
///
/// Free is O(log N slabs): binary-search the sorted `slabs[]` array for
/// the page that `ptr & ~(PAGE_SIZE-1)` lands in, compute the slot index
/// by shift, clear the bit in the corresponding bitmap word. No
/// chunk-list walk, no per-slab header, no inline bitmap.
///
/// Alloc is O(N slabs) worst case (linear scan of bitmaps until a free
/// bit is found), O(1) when the first slab has space. Growing the
/// allocator means asking PageAllocator for one more OS page and
/// inserting it sorted into `slabs[]`.

#ifndef MISRA_STD_ALLOCATOR_SLAB_H
#define MISRA_STD_ALLOCATOR_SLAB_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Allocator/Page.h>

///
/// Per-type magic for `SlabAllocator`. Stamped into
/// `Allocator.base.__magic` by `SlabAllocatorInit*`. The slab
/// implementation functions validate this exact value so other
/// allocator instances reinterpreted as a `SlabAllocator *` are
/// rejected at runtime as type-confusion.
///
#define SLAB_ALLOCATOR_MAGIC MAKE_NEW_MAGIC_VALUE("slaballc")

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct SlabAllocator {
        Allocator base;

        // Per-slab page bases. Sorted ascending by address so free can
        // bsearch by `ptr & ~(PAGE_SIZE-1)`. Each entry is one OS page
        // of pure user slots -- no header, no bitmap.
        void **slabs;
        u32    slabs_len;
        u32    slabs_cap;

        // Per-slab bitmaps packed into one contiguous buffer.
        // bitmap-for-slabs[i] = bitmaps + i*bitmap_words_per_slab.
        // Sized at first slab grow (when page size is known); grown in
        // lockstep with slabs[] capacity.
        u64 *bitmaps;

        // User-configured slot size. Must be a power of two in
        // [16, PAGE_SIZE]. Validated at first grow / on validate-full.
        size slot_size;

        // ctz(slot_size). Cached so the hot path computes
        // `slot_in_slab = (ptr - slab_base) >> slot_size_shift`
        // in one instruction instead of an integer division.
        u8 slot_size_shift;

        // u64 words per slab's bitmap. Computed at first grow once we
        // know the OS page size. The hot free path needs this for
        // `bitmap_w = bitmaps + slab_idx * bitmap_words_per_slab`.
        // For slot_size >= 64 and PAGE_SIZE = 4096 this is 1, so the
        // multiply degenerates to a no-op.
        u8 bitmap_words_per_slab;

        PageAllocator page;
    } SlabAllocator;

    void *slab_allocator_allocate(Allocator *self, size bytes, i8 zeroed);
    i8    slab_allocator_resize(Allocator *self, void *ptr, size new_size);
    void *slab_allocator_remap(Allocator *self, void *ptr, size new_size);
    size  slab_allocator_deallocate(Allocator *self, void *ptr);

    ///
    /// Release every slab page and the bitmaps buffer owned by `self`,
    /// then zero the struct so any post-deinit dispatch trips
    /// `ValidateAllocator` on the cleared `__magic`.
    ///
    /// self[in,out] : SlabAllocator instance, or NULL.
    ///
    /// SUCCESS: Function returns. Every slot previously handed out by
    ///          this slab is invalid; the struct is fully zeroed and
    ///          cannot be used until re-initialised.
    /// FAILURE: No action when `self` is NULL.
    ///
    /// TAGS: Allocator, Slab, Cleanup
    ///
    void SlabAllocatorDeinit(SlabAllocator *self);

#ifdef __cplusplus
}
#endif

///
/// Round a requested slot size up to the nearest power of two in
/// [16, 4096]. Sub-16-byte requests round up to 16 (the
/// MAX_ALIGN-derived minimum). Requests above 4096 fall through to
/// 4096; the validator catches them at first grow when it compares
/// against the actual OS page size.
///
/// This is the policy that lets callers write `SlabAllocatorInit(
/// sizeof(MyType))` for arbitrary type sizes -- e.g. a 28-byte
/// struct goes into 32-byte slots, like the way glibc rounds a
/// 2-byte malloc up to its 24-byte minimum chunk.
///
#define MISRA_SLAB_ROUNDUP_POW2(s)                                                                                     \
    ((s) <= 16u     ? 16u                                                                                              \
     : (s) <= 32u   ? 32u                                                                                              \
     : (s) <= 64u   ? 64u                                                                                              \
     : (s) <= 128u  ? 128u                                                                                             \
     : (s) <= 256u  ? 256u                                                                                             \
     : (s) <= 512u  ? 512u                                                                                             \
     : (s) <= 1024u ? 1024u                                                                                            \
     : (s) <= 2048u ? 2048u                                                                                            \
                    : 4096u)

///
/// Compile-time `ctz` for the supported slot sizes. Folded to a
/// constant when the input is a literal (the common case --
/// `SlabAllocatorInit(16)`). For runtime values, evaluates as a small
/// conditional cascade once at init. Always paired with
/// `MISRA_SLAB_ROUNDUP_POW2` so the input is guaranteed to land on a
/// supported power of two.
///
#define MISRA_SLAB_SHIFT_FROM_SIZE(s)                                                                                  \
    ((s) == 16u     ? 4u                                                                                               \
     : (s) == 32u   ? 5u                                                                                               \
     : (s) == 64u   ? 6u                                                                                               \
     : (s) == 128u  ? 7u                                                                                               \
     : (s) == 256u  ? 8u                                                                                               \
     : (s) == 512u  ? 9u                                                                                               \
     : (s) == 1024u ? 10u                                                                                              \
     : (s) == 2048u ? 11u                                                                                              \
     : (s) == 4096u ? 12u                                                                                              \
                    : 0u)

///
/// Initialize a `SlabAllocator` with the given slot size. Slot size
/// must be a power of two in [16, PAGE_SIZE]; the validator aborts
/// on first allocation otherwise. Default alignment = 16 (`MAX_ALIGN`)
/// since the minimum slot size is also 16 and every slot is naturally
/// MAX_ALIGN-aligned within its OS page.
///
#define SlabAllocatorInit(slot_size_bytes)                                                                             \
    ((SlabAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = slab_allocator_allocate,                                                                   \
                   .resize      = slab_allocator_resize,                                                                     \
                   .remap       = slab_allocator_remap,                                                                      \
                   .deallocate  = slab_allocator_deallocate,                                                                 \
                   .alignment   = 16,                                                                                        \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = SLAB_ALLOCATOR_MAGIC},                                                                         \
        .slabs                 = NULL,                                                                                 \
        .slabs_len             = 0,                                                                                    \
        .slabs_cap             = 0,                                                                                    \
        .bitmaps               = NULL,                                                                                 \
        .slot_size             = MISRA_SLAB_ROUNDUP_POW2(slot_size_bytes),                                             \
        .slot_size_shift       = (u8)MISRA_SLAB_SHIFT_FROM_SIZE(MISRA_SLAB_ROUNDUP_POW2(slot_size_bytes)),             \
        .bitmap_words_per_slab = 0,                                                                                    \
        .page                  = PageAllocatorInit()                                                                   \
    })

///
/// Initialize a `SlabAllocator` with a custom alignment floor.
/// `alignment_value` must not exceed `slot_size_bytes`; alignments
/// stronger than `MAX_ALIGN` are accepted but the underlying slab
/// page guarantees only `MAX_ALIGN` so over-strong alignment is
/// the caller's responsibility to honour at use.
///
#define SlabAllocatorInitAligned(slot_size_bytes, alignment_value)                                                     \
    ((SlabAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate    = slab_allocator_allocate,                                                                   \
                   .resize      = slab_allocator_resize,                                                                     \
                   .remap       = slab_allocator_remap,                                                                      \
                   .deallocate  = slab_allocator_deallocate,                                                                 \
                   .alignment   = (alignment_value) ? (alignment_value) : 16,                                                \
                   .effort      = ALLOCATOR_EFFORT_ONCE,                                                                     \
                   .retry_limit = 0,                                                                                         \
                   .__magic     = SLAB_ALLOCATOR_MAGIC},                                                                         \
        .slabs                 = NULL,                                                                                 \
        .slabs_len             = 0,                                                                                    \
        .slabs_cap             = 0,                                                                                    \
        .bitmaps               = NULL,                                                                                 \
        .slot_size             = MISRA_SLAB_ROUNDUP_POW2(slot_size_bytes),                                             \
        .slot_size_shift       = (u8)MISRA_SLAB_SHIFT_FROM_SIZE(MISRA_SLAB_ROUNDUP_POW2(slot_size_bytes)),             \
        .bitmap_words_per_slab = 0,                                                                                    \
        .page                  = PageAllocatorInit()                                                                   \
    })

#endif // MISRA_STD_ALLOCATOR_SLAB_H
