/// file : tests/std/http.leak.c
/// Success-path leak-guard tests for Http: route allocations through an explicit
/// DebugAllocator and assert DebugAllocatorLiveCount(&dbg)==0 after cleanup, to KILL
/// remove_void_call survivors that drop an internal *Deinit on a reachable branch.
///
/// Each survivor is a `cxx_remove_void_call` dropping an internal Str/Vec Deinit
/// that frees storage allocated through the request/response allocator. With the
/// default HeapAllocator (mull build) the leak is invisible; routing every
/// allocation through a DebugAllocator and asserting LiveCount==0 after teardown
/// makes the dropped Deinit observable -> the mutant fails -> KILLED.

#include <Misra.h>
#include <Misra/Parsers/Http.h>
#include <Misra/Std/Allocator/Debug.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Zstr.h>

#include "../Util/TestRunner.h"

// =============================================================================
// HttpHeaderDeinit (L23 key, L24 value): the user-facing header deinit must
// release BOTH key and value storage. Build a header with non-empty key and
// value through the DebugAllocator, deinit it, and require the allocator to be
// drained. Dropping either StrDeinit leaks that field's backing buffer.

// =============================================================================
// http_header_deinit (L32 value): the Vec deep-copy deinit callback runs when a
// Vec(HttpHeader) is torn down. Build a Vec(HttpHeader) wired with the deep-copy
// callbacks, R-push a header into it (the vec deep-copies key+value through the
// allocator), then VecDeinit -> http_header_deinit per entry. Dropping the value
// StrDeinit leaks each deep-copied header value's storage.
//
// VecPushBackR leaves the source `hh` intact (it deep-copies rather than moves),
// so the test deinits the source itself; the only storage left for VecDeinit to
// reclaim is the vec's own copies -- exactly what the L31/L32 callback frees.

bool test_hl_headers_vec_deinit_releases_values(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    HttpHeaders headers = VecInitWithDeepCopy(http_header_init_copy, http_header_deinit, adbg);

    HttpHeader hh = HttpHeaderInit(adbg);
    StrAppendFmt(&hh.key, "X-A-Reasonably-Long-Header-Key-Name");
    StrAppendFmt(&hh.value, "a-reasonably-long-header-value-string");

    bool ok = VecPushBackR(&headers, hh);
    ok      = ok && (VecLen(&headers) == 1);
    // R-push deep-copied into the vec and left `hh` owning its own storage;
    // reclaim the source so only the vec's deep copies remain live.
    HttpHeaderDeinit(&hh);
    ok = ok && (DebugAllocatorLiveCount(&dbg) > 0);

    // VecDeinit runs http_header_deinit on the surviving copy.
    VecDeinit(&headers);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// http_request_parse_zstr success path: the scratch `version` Str (L132) and
// `method` Str (L135) are allocated through the request allocator and freed on
// the SUCCESS branch before the function returns. A request whose method/version
// strings are long enough to heap-allocate leaks that scratch storage if either
// StrDeinit is dropped. The whole parse-then-deinit round-trip must drain to 0.

// =============================================================================
// Header ownership by move: http_request_parse_zstr inserts each parsed header
// into req->headers by move (no deep copy). HttpRequestDeinit frees every
// header's key+value via its per-element loop, so the allocator must drain.
// Before the move-ownership fix the deep-copy push abandoned the local scratch
// header, leaking one key+value pair per parsed header.

// =============================================================================
// HttpRequestDeinit (L191 StrDeinit(&req->url)): the parsed URL is owned by the
// request allocator. After a successful parse the url Str holds heap storage;
// dropping its StrDeinit leaks it even though VecDeinit + MemSet still run.

bool test_hl_request_deinit_releases_url(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // A long URL forces the url Str onto the heap.
    Zstr raw =
        "GET /a-deliberately-long-url-path-to-force-heap-allocation HTTP/1.1\r\n"
        "\r\n";

    HttpRequest req  = HttpRequestInit(adbg);
    Zstr        next = HttpRequestParse(&req, raw);
    bool        ok   = (next != raw) && (StrLen(&req.url) > 0);
    ok               = ok && (DebugAllocatorLiveCount(&dbg) > 0);

    HttpRequestDeinit(&req);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// HttpRespondWithHtml (L414 StrDeinit(&response->body)): before installing the
// new body, the OLD body must be released. If the response already carries a
// (heap-backed) body and HtmlRespond is called, dropping the StrDeinit leaks the
// previous body. Pre-fill the body via one HtmlRespond, then call again.

bool test_hl_respond_with_html_releases_old_body(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    Str first = StrInit(adbg);
    StrAppendFmt(&first, "<h1>first-body-long-enough-to-heap-allocate</h1>");

    HttpResponse response = HttpResponseInit(adbg);
    HttpRespondWithHtml(&response, HTTP_RESPONSE_CODE_OK, &first);

    Str second = StrInit(adbg);
    StrAppendFmt(&second, "<h1>second-body-also-long-enough-to-heap</h1>");
    // Second call must free the first body before installing the second.
    HttpRespondWithHtml(&response, HTTP_RESPONSE_CODE_OK, &second);

    StrDeinit(&first);
    StrDeinit(&second);
    HttpResponseDeinit(&response);
    bool ok = (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}

// =============================================================================
// HttpResponseDeinit (L501 StrDeinit(&response->body)): the response body is
// owned by the response allocator. A non-empty body must be released at deinit;
// dropping its StrDeinit leaks it even though VecDeinit + MemSet still run.

#if FEATURE_FILE
// =============================================================================
// http_respond_with_file_zstr (L431 StrDeinit(&response->body)): before reading
// the file into a fresh body Str, the OLD body must be released. Pre-fill the
// body via HtmlRespond, then serve a file; dropping the StrDeinit leaks the old
// body.

bool test_hl_respond_with_file_releases_old_body(void) {
    DebugAllocator dbg  = DebugAllocatorInit();
    Allocator     *adbg = ALLOCATOR_OF(&dbg);

    // Pre-fill body with a heap-backed string.
    Str first = StrInit(adbg);
    StrAppendFmt(&first, "<h1>old-body-long-enough-to-force-heap-here</h1>");
    HttpResponse response = HttpResponseInit(adbg);
    HttpRespondWithHtml(&response, HTTP_RESPONSE_CODE_OK, &first);
    StrDeinit(&first);

    // Create a temp file with content.
    Str  path    = StrInit(adbg);
    File f       = FileOpenTemp(&path, adbg);
    bool ok      = FileIsOpen(&f);
    Zstr payload = "file-payload-long-enough-to-heap-allocate-body";
    ok           = ok && (FileWrite(&f, payload, ZstrLen(payload)) == (i64)ZstrLen(payload));
    FileClose(&f);

    // Serving the file must free the old (HTML) body before installing the new.
    HttpResponse *res =
        HttpRespondWithFile(&response, HTTP_RESPONSE_CODE_OK, HTTP_CONTENT_TYPE_TEXT_PLAIN, StrBegin(&path));
    ok = ok && (res != NULL);

    FileRemove(StrBegin(&path));
    StrDeinit(&path);
    HttpResponseDeinit(&response);
    ok = ok && (DebugAllocatorLiveCount(&dbg) == 0);

    DebugAllocatorDeinit(&dbg);
    return ok;
}
#endif

int main(void) {
    WriteFmt("[INFO] Starting Http.Leak tests\n\n");

    TestFunction tests[] = {
        test_hl_headers_vec_deinit_releases_values,
        test_hl_request_deinit_releases_url,
        test_hl_respond_with_html_releases_old_body,
#if FEATURE_FILE
        test_hl_respond_with_file_releases_old_body,
#endif
    };

    return run_test_suite(tests, (int)(sizeof(tests) / sizeof(tests[0])), NULL, 0, "Http.Leak");
}
