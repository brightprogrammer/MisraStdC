#include <Misra.h>
#include <Misra/Std/Allocator/Page.h>

#include "../Util/TestRunner.h"

bool test_page_protect_roundtrip(void) {
    PageAllocator page = PageAllocatorInit();
    Allocator    *base = ALLOCATOR_OF(&page);

    size  page_bytes = PageAllocatorPageSize(&page);
    void *region     = AllocatorAlloc(base, page_bytes, true);
    if (!region) {
        return false;
    }

    // Write into the region while it's writable.
    u8 *bytes = (u8 *)region;
    bytes[0]  = 0xab;
    bytes[1]  = 0xcd;

    // Drop to PROT_NONE then bring back to RW — we should be able to
    // read what we wrote before. (We don't try to actually trip the
    // protection here; that would need SIGSEGV handling.)
    bool ok = true;
    ok      = ok && PageProtect(region, page_bytes, PAGE_PROT_NONE);
    ok      = ok && PageProtect(region, page_bytes, PAGE_PROT_READ_WRITE);
    ok      = ok && bytes[0] == 0xab && bytes[1] == 0xcd;

    // Also exercise read-only: write before, drop to READ, restore.
    bytes[2] = 0xef;
    ok       = ok && PageProtect(region, page_bytes, PAGE_PROT_READ);
    ok       = ok && bytes[2] == 0xef;
    ok       = ok && PageProtect(region, page_bytes, PAGE_PROT_READ_WRITE);

    AllocatorFree(&page.base, region);
    PageAllocatorDeinit(&page);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting PageProtect tests\n\n");

    TestFunction tests[] = {
        test_page_protect_roundtrip,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "PageProtect");
}
