/// file      : std/memory.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Raw memory operations: compare / copy / move / set / sort.

#include <Misra/Std/Log.h>
#include <Misra/Std/Memory.h>

i32 MemCompare(const void *p1, const void *p2, size n) {
    if (n == 0) {
        return 0;
    }

    if (!p1 || !p2) {
        LOG_FATAL("Invalid arguments");
    }

    const u8 *s1 = (const u8 *)p1;
    const u8 *s2 = (const u8 *)p2;
    while (n--) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        s1++;
        s2++;
    }
    return 0;
}

void *MemCopy(void *dst, const void *src, size n) {
    if (n == 0) {
        return dst;
    }

    if (!dst || !src) {
        LOG_FATAL("Invalid arguments");
    }

    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *MemMove(void *dst, const void *src, size n) {
    if (n == 0) {
        return dst;
    }

    if (!dst || !src) {
        LOG_FATAL("Invalid arguments");
    }

    u8       *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;
    if (d < s) {
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    }
    return dst;
}

void *MemSet(void *dst, i32 val, size n) {
    if (n == 0) {
        return dst;
    }

    if (!dst) {
        LOG_FATAL("Invalid arguments");
    }

    u8 *d = (u8 *)dst;
    while (n--) {
        *d++ = (u8)val;
    }
    return dst;
}

// ---------------------------------------------------------------------------
// MemSort: in-tree generic-flat-array sort. Quicksort with
// median-of-three pivot, insertion-sort fallback for small partitions
// (<= 16 items), tail-iteration on the larger half to bound stack at
// O(log n). Worst-case O(n^2) on adversarial inputs is accepted -- no
// caller in the project drives sort with attacker-controlled data.
// ---------------------------------------------------------------------------

static inline void memsort_swap(u8 *a, u8 *b, size n) {
    if (a == b)
        return;
    while (n--) {
        u8 t = *a;
        *a++ = *b;
        *b++ = t;
    }
}

static void memsort_insertion(u8 *base, size n, size sz, GenericCompare cmp) {
    for (size i = 1; i < n; ++i) {
        size j = i;
        while (j > 0 && cmp(base + (j - 1) * sz, base + j * sz) > 0) {
            memsort_swap(base + (j - 1) * sz, base + j * sz, sz);
            --j;
        }
    }
}

void MemSort(void *base_, size n_items, size item_size, GenericCompare cmp) {
    if (!base_ || !cmp || item_size == 0 || n_items < 2) {
        return;
    }
    u8  *base = (u8 *)base_;
    size n    = n_items;

    while (n > 16) {
        // Median-of-three pivot: sort the (lo, mid, hi) trio in place so
        // mid ends up as the median, then stash it just before hi as the
        // partition pivot.
        u8 *lo  = base;
        u8 *mid = base + (n / 2) * item_size;
        u8 *hi  = base + (n - 1) * item_size;
        if (cmp(lo, mid) > 0)
            memsort_swap(lo, mid, item_size);
        if (cmp(lo, hi) > 0)
            memsort_swap(lo, hi, item_size);
        if (cmp(mid, hi) > 0)
            memsort_swap(mid, hi, item_size);
        // Pivot now lives at `mid`; move it to position n-2 so we can
        // partition over the inner range [1 .. n-2].
        u8 *pivot = base + (n - 2) * item_size;
        memsort_swap(mid, pivot, item_size);

        size i = 0;
        size j = n - 2;
        for (;;) {
            do {
                ++i;
            } while (cmp(base + i * item_size, pivot) < 0);
            do {
                --j;
            } while (j > 0 && cmp(base + j * item_size, pivot) > 0);
            if (i >= j)
                break;
            memsort_swap(base + i * item_size, base + j * item_size, item_size);
        }
        // Drop pivot into its final slot.
        memsort_swap(base + i * item_size, pivot, item_size);

        // Recurse on the smaller partition; iterate on the larger to
        // keep stack depth bounded at O(log n).
        size left_n  = i;
        size right_n = n - i - 1;
        u8  *right   = base + (i + 1) * item_size;
        if (left_n < right_n) {
            MemSort(base, left_n, item_size, cmp);
            base = right;
            n    = right_n;
        } else {
            MemSort(right, right_n, item_size, cmp);
            n = left_n;
        }
    }

    memsort_insertion(base, n, item_size, cmp);
}
