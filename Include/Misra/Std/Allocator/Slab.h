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
/// allocator means asking the kernel for one more OS page and
/// inserting it sorted into `slabs[]`. SlabAllocator talks to the kernel
/// directly; there is no embedded PageAllocator.

#ifndef MISRA_STD_ALLOCATOR_SLAB_H
#define MISRA_STD_ALLOCATOR_SLAB_H

#include <Misra/Std/Allocator.h>

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
    } SlabAllocator;

    ///
    /// Allocate one fixed-size slot. `bytes` must fit within the slab's
    /// configured `slot_size` (caught at the validator on first use).
    /// Scans the bitmap of each live slab for the first free bit; if
    /// every slab is full, asks the kernel for one more OS page and
    /// inserts it sorted into `slabs[]`.
    ///
    /// SUCCESS: Returns a writable, `slot_size`-aligned pointer to a
    ///          fresh slot. Zeroed when `zeroed` is non-zero.
    /// FAILURE: Returns NULL when no free slot exists and the grow
    ///          path (kernel page-mapping call or descriptor /
    ///          bitmap-table grow) fails.
    ///
    /// TAGS: Allocator, Slab, Memory, Allocation
    ///
    void *slab_allocator_allocate(SlabAllocator *self, size bytes, i8 zeroed);

    ///
    /// In-place resize. Every slot has the same fixed size, so a
    /// resize succeeds iff `new_size` still fits in `slot_size`.
    ///
    /// SUCCESS: Returns 1 when `new_size <= slot_size`. The pointer
    ///          stays valid; no slab state changes.
    /// FAILURE: Returns 0 when `new_size > slot_size`. The slot is
    ///          unchanged; the caller can move to a different slab.
    ///
    /// TAGS: Allocator, Slab, Memory, InPlace
    ///
    i8 slab_allocator_resize(SlabAllocator *self, void *ptr, size new_size);

    ///
    /// Resize a slab allocation with relocation allowed. Because every
    /// slab has one fixed slot size, any request that does not fit in
    /// `slot_size` cannot be served by the same allocator and the call
    /// fails; callers that need a different size class must route to a
    /// different `SlabAllocator`.
    ///
    /// SUCCESS: Returns `ptr` unchanged when `new_size <= slot_size`.
    ///          When `ptr` is NULL this behaves like
    ///          `slab_allocator_allocate(self, new_size, true)` --
    ///          fresh allocations from a remap-NULL are zeroed. When
    ///          `new_size == 0` the allocation is freed and NULL is
    ///          returned.
    /// FAILURE: Returns NULL when `new_size > slot_size`. The old
    ///          allocation is left untouched.
    ///
    /// TAGS: Allocator, Slab, Memory, Reallocation
    ///
    void *slab_allocator_remap(SlabAllocator *self, void *ptr, size new_size);

    ///
    /// Free a slot. Binary-searches `slabs[]` for the page that
    /// contains `ptr`, computes the in-slab slot index by shift, and
    /// clears the bitmap bit. No chunk-list walk, no per-slab header.
    ///
    /// SUCCESS: Returns `slot_size` (what stats accounting sees).
    /// FAILURE: Aborts via `LOG_FATAL` when `ptr` is foreign to this
    ///          slab, mis-aligned within its page, or its bitmap bit
    ///          is already clear (double-free).
    ///
    /// TAGS: Allocator, Slab, Memory, Deallocation
    ///
    size slab_allocator_deallocate(SlabAllocator *self, void *ptr);

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
/// struct goes into 32-byte slots: the next power of two at or
/// above the requested size is the effective slot size.
///
/// s[in] : Requested slot size in bytes.
///
/// SUCCESS: Returns the smallest supported power-of-two slot size
///          `>= s`, clamped to [16, 4096].
/// FAILURE: No runtime failure mode. For `s > 4096` returns 4096;
///          first-allocation validation aborts when the rounded
///          slot does not fit in the OS page size.
///
/// TAGS: Slab, Macro, PowerOfTwo, Utility
#define SLAB_ROUNDUP_POW2(s)                                                                                           \
    ((s) <= 16u   ? 16u :                                                                                              \
     (s) <= 32u   ? 32u :                                                                                              \
     (s) <= 64u   ? 64u :                                                                                              \
     (s) <= 128u  ? 128u :                                                                                             \
     (s) <= 256u  ? 256u :                                                                                             \
     (s) <= 512u  ? 512u :                                                                                             \
     (s) <= 1024u ? 1024u :                                                                                            \
     (s) <= 2048u ? 2048u :                                                                                            \
                    4096u)

///
/// Compile-time `ctz` for the supported slot sizes. Folded to a
/// constant when the input is a literal (the common case --
/// `SlabAllocatorInit(16)`). For runtime values, evaluates as a small
/// conditional cascade once at init. Always paired with
/// `SLAB_ROUNDUP_POW2` so the input is guaranteed to land on a
/// supported power of two.
///
/// s[in] : Power-of-two slot size in bytes (caller-guaranteed via
///         `SLAB_ROUNDUP_POW2`).
///
/// SUCCESS: Returns `log2(s)` for supported sizes (16..4096), as a
///          `u32` literal in [4, 12].
/// FAILURE: Returns 0 for any unsupported input; the slab validator
///          rejects a `slot_size_shift == 0` slab at first allocation.
///
/// TAGS: Slab, Macro, BitScan, Utility
#define SLAB_SHIFT_FROM_SIZE(s)                                                                                        \
    ((s) == 16u   ? 4u :                                                                                               \
     (s) == 32u   ? 5u :                                                                                               \
     (s) == 64u   ? 6u :                                                                                               \
     (s) == 128u  ? 7u :                                                                                               \
     (s) == 256u  ? 8u :                                                                                               \
     (s) == 512u  ? 9u :                                                                                               \
     (s) == 1024u ? 10u :                                                                                              \
     (s) == 2048u ? 11u :                                                                                              \
     (s) == 4096u ? 12u :                                                                                              \
                    0u)

///
/// Initialize a `SlabAllocator` with the given slot size. Slot size
/// must be a power of two in [16, PAGE_SIZE]; the validator aborts
/// on first allocation otherwise. Default alignment = 16 (`MAX_ALIGN`)
/// since the minimum slot size is also 16 and every slot is naturally
/// MAX_ALIGN-aligned within its OS page.
///
/// `slot_size_bytes` MUST be side-effect-free -- the macro expands it
/// through `SLAB_ROUNDUP_POW2` and `SLAB_SHIFT_FROM_SIZE`, each of
/// which evaluates its argument many times in a ternary cascade.
/// Pass a literal or a const local; do NOT pass `expr++` or a
/// function call.
///
/// SUCCESS: Returns a fully-initialised `SlabAllocator` value. No
///          OS calls happen at init; the first allocation triggers
///          the first kernel page map.
/// FAILURE: Cannot fail at macro-expansion time. A bad `slot_size_bytes`
///          is caught on first allocation by the validator.
///
/// TAGS: Allocator, Slab, Init
///
#define SlabAllocatorInit(slot_size_bytes)                                                                             \
    ((SlabAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate        = (AllocatorAllocateFn)slab_allocator_allocate,                                          \
                   .resize          = (AllocatorResizeFn)slab_allocator_resize,                                              \
                   .remap           = (AllocatorRemapFn)slab_allocator_remap,                                                \
                   .deallocate      = (AllocatorDeallocateFn)slab_allocator_deallocate,                                      \
                   .alignment       = 16,                                                                                    \
                   .effort          = ALLOCATOR_EFFORT_ONCE,                                                                 \
                   .retry_limit     = 0,                                                                                     \
                   .__magic         = SLAB_ALLOCATOR_MAGIC,                                                                  \
                   .footprint_bytes = 0},                                                                                    \
        .slabs                 = NULL,                                                                                 \
        .slabs_len             = 0,                                                                                    \
        .slabs_cap             = 0,                                                                                    \
        .bitmaps               = NULL,                                                                                 \
        .slot_size             = SLAB_ROUNDUP_POW2(slot_size_bytes),                                                   \
        .slot_size_shift       = (u8)SLAB_SHIFT_FROM_SIZE(SLAB_ROUNDUP_POW2(slot_size_bytes)),                         \
        .bitmap_words_per_slab = 0,                                                                                    \
    })

///
/// Initialize a `SlabAllocator` with a custom alignment floor.
/// `alignment_value` must not exceed `slot_size_bytes`; alignments
/// stronger than `MAX_ALIGN` are accepted but the underlying slab
/// page guarantees only `MAX_ALIGN` so over-strong alignment is
/// the caller's responsibility to honour at use.
///
/// `slot_size_bytes` and `alignment_value` MUST both be side-effect-free.
/// Both expand through ternary cascades that evaluate the argument
/// many times. Pass literals or const locals.
///
/// SUCCESS: Returns a fully-initialised `SlabAllocator` value with
///          the requested slot size and alignment floor recorded in
///          the base.
/// FAILURE: Cannot fail at macro-expansion time. Invalid slot size /
///          alignment combinations are caught on first allocation
///          by the validator.
///
/// TAGS: Allocator, Slab, Init, Alignment
///
#define SlabAllocatorInitAligned(slot_size_bytes, alignment_value)                                                     \
    ((SlabAllocator) {                                                                                                 \
        .base =                                                                                                        \
            {.allocate        = (AllocatorAllocateFn)slab_allocator_allocate,                                          \
                   .resize          = (AllocatorResizeFn)slab_allocator_resize,                                              \
                   .remap           = (AllocatorRemapFn)slab_allocator_remap,                                                \
                   .deallocate      = (AllocatorDeallocateFn)slab_allocator_deallocate,                                      \
                   .alignment       = (alignment_value) ? (alignment_value) : 16,                                            \
                   .effort          = ALLOCATOR_EFFORT_ONCE,                                                                 \
                   .retry_limit     = 0,                                                                                     \
                   .__magic         = SLAB_ALLOCATOR_MAGIC,                                                                  \
                   .footprint_bytes = 0},                                                                                    \
        .slabs                 = NULL,                                                                                 \
        .slabs_len             = 0,                                                                                    \
        .slabs_cap             = 0,                                                                                    \
        .bitmaps               = NULL,                                                                                 \
        .slot_size             = SLAB_ROUNDUP_POW2(slot_size_bytes),                                                   \
        .slot_size_shift       = (u8)SLAB_SHIFT_FROM_SIZE(SLAB_ROUNDUP_POW2(slot_size_bytes)),                         \
        .bitmap_words_per_slab = 0,                                                                                    \
    })

#endif // MISRA_STD_ALLOCATOR_SLAB_H
