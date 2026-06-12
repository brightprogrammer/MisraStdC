#include <Misra/Parsers/Http.h>
#include <Misra/Std/File.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>
#include <Misra/Sys/Dir.h>


#include "../Util/TestRunner.h"

bool test_http_request_parse_get_with_headers(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Zstr raw =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: misra-test/1.0\r\n"
        "\r\n"
        "body-bytes";

    HttpRequest req  = HttpRequestInit(alloc_base);
    Zstr        next = HttpRequestParse(&req, raw);

    bool ok = (next != raw) && (req.method == HTTP_REQUEST_METHOD_GET) && (StrLen(&req.url) == 11) &&
              (ZstrCompare(StrBegin(&req.url), "/index.html") == 0) && (VecLen(&req.headers) == 2) &&
              (ZstrCompare(next, "body-bytes") == 0);

    HttpHeader *host = HttpHeadersFind(&req.headers, "Host");
    ok               = ok && host && ZstrCompare(StrBegin(&host->value), "example.com") == 0;

    HttpRequestDeinit(&req);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_http_response_serialize_html(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    HttpResponse response = HttpResponseInit(alloc_base);
    Str          body     = StrInit(alloc_base);
    StrAppendFmt(&body, "<h1>hi</h1>");
    HttpRespondWithHtml(&response, HTTP_RESPONSE_CODE_OK, &body);
    StrDeinit(&body);

    Str wire = HttpResponseSerialize(&response, alloc_base);

    // Spot-check the wire format:
    //   - starts with "HTTP/1.1 200 OK"
    //   - includes a Content-Type: text/html
    //   - includes a Content-Length: 11 (length of "<h1>hi</h1>")
    //   - ends with the body
    bool ok = StrLen(&wire) > 0 && ZstrFindSubstring(StrBegin(&wire), "HTTP/1.1 200 OK\r\n") == StrBegin(&wire) &&
              ZstrFindSubstring(StrBegin(&wire), "Content-Type: text/html\r\n") != NULL &&
              ZstrFindSubstring(StrBegin(&wire), "Content-Length: 11\r\n") != NULL &&
              ZstrFindSubstring(StrBegin(&wire), "\r\n\r\n<h1>hi</h1>") != NULL;

    StrDeinit(&wire);
    HttpResponseDeinit(&response);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Each HTTP/1.1 method spelling must classify to its own enum value
// (caller reads req.method to dispatch). Only GET was covered before.
bool test_http_request_parse_all_methods(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    struct {
        Zstr              raw;
        HttpRequestMethod method;
    } cases[] = {
        {    "GET / HTTP/1.1\r\n\r\n",     HTTP_REQUEST_METHOD_GET},
        {   "POST / HTTP/1.1\r\n\r\n",    HTTP_REQUEST_METHOD_POST},
        { "DELETE / HTTP/1.1\r\n\r\n",  HTTP_REQUEST_METHOD_DELETE},
        {    "PUT / HTTP/1.1\r\n\r\n",     HTTP_REQUEST_METHOD_PUT},
        {  "PATCH / HTTP/1.1\r\n\r\n",   HTTP_REQUEST_METHOD_PATCH},
        {   "HEAD / HTTP/1.1\r\n\r\n",    HTTP_REQUEST_METHOD_HEAD},
        {"OPTIONS / HTTP/1.1\r\n\r\n", HTTP_REQUEST_METHOD_OPTIONS},
        {"CONNECT / HTTP/1.1\r\n\r\n", HTTP_REQUEST_METHOD_CONNECT},
        {  "TRACE / HTTP/1.1\r\n\r\n",   HTTP_REQUEST_METHOD_TRACE},
    };

    bool ok = true;
    for (u64 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        HttpRequest req  = HttpRequestInit(alloc_base);
        Zstr        next = HttpRequestParse(&req, cases[i].raw);
        ok               = ok && (next != cases[i].raw) && (req.method == cases[i].method);
        HttpRequestDeinit(&req);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A request whose method is not a known HTTP method is rejected: the
// parser returns the input pointer unchanged (FAILURE contract).
bool test_http_request_parse_rejects_unknown_method(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Zstr cases[] = {
        "GETX / HTTP/1.1\r\n\r\n",
        "PUTT / HTTP/1.1\r\n\r\n",
        "BREW / HTTP/1.1\r\n\r\n",
    };

    bool ok = true;
    for (u64 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        HttpRequest req  = HttpRequestInit(alloc_base);
        Zstr        next = HttpRequestParse(&req, cases[i]);
        ok               = ok && (next == cases[i]) && (req.method == HTTP_REQUEST_METHOD_UNKNOWN);
        HttpRequestDeinit(&req);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A near-miss that shares a prefix with a real method but has the wrong
// length must NOT be accepted as that method (the length guards matter).
bool test_http_request_method_length_exact(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Zstr cases[] = {
        "GE / HTTP/1.1\r\n\r\n",
        "POSTX / HTTP/1.1\r\n\r\n",
    };

    bool ok = true;
    for (u64 i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        HttpRequest req  = HttpRequestInit(alloc_base);
        Zstr        next = HttpRequestParse(&req, cases[i]);
        ok               = ok && (next == cases[i]) && (req.method == HTTP_REQUEST_METHOD_UNKNOWN);
        HttpRequestDeinit(&req);
    }

    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// The parser caps per-message header count at 100 (RFC 7230 § 3.2.5).
// The cap check is `VecLen(&req->headers) >= 100`. A `>=`->`>` mutant
// would let one extra header through. Build a request with exactly 101
// headers: the real parser rejects (returns `in` unchanged, having
// already accumulated the 100-header cap), while a `>`-mutant would
// accept it and advance the cursor.
bool test_ht_header_count_cap_rejects_over_limit(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str raw = StrInit(alloc_base);
    StrAppendFmt(&raw, "GET / HTTP/1.1\r\n");
    // 101 headers -- one past the cap of 100.
    for (u64 i = 0; i < 101; i++) {
        StrAppendFmt(&raw, "X-H{}: v\r\n", i);
    }
    StrAppendFmt(&raw, "\r\n");

    HttpRequest req  = HttpRequestInit(alloc_base);
    Zstr        in   = StrBegin(&raw);
    Zstr        next = HttpRequestParse(&req, in);

    // Real code rejects: cursor returned unchanged (== in).
    bool ok = (next == in);

    HttpRequestDeinit(&req);
    StrDeinit(&raw);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// A request with exactly 100 headers (at the cap) must still parse
// successfully -- the boundary is inclusive on the accept side. This
// pins the lower edge so the cap can't be tightened off-by-one.
bool test_ht_header_count_cap_accepts_at_limit(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str raw = StrInit(alloc_base);
    StrAppendFmt(&raw, "GET / HTTP/1.1\r\n");
    for (u64 i = 0; i < 100; i++) {
        StrAppendFmt(&raw, "X-H{}: v\r\n", i);
    }
    StrAppendFmt(&raw, "\r\n");

    HttpRequest req  = HttpRequestInit(alloc_base);
    Zstr        in   = StrBegin(&raw);
    Zstr        next = HttpRequestParse(&req, in);

    bool ok = (next != in) && (VecLen(&req.headers) == 100);

    HttpRequestDeinit(&req);
    StrDeinit(&raw);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// HttpRespondWithFile must copy the caller's status code and content
// type verbatim into the response, and read the file body. Mutants that
// substitute `status`/`content_type` with a literal, or that flip the
// `FileReadAndClose(...) < 0` success test, would corrupt the response.
// Serve a NON-empty file and assert all three.
bool test_ht_respond_with_file_sets_fields(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path = StrInit(alloc_base);
    File f    = FileOpenTemp(&path, alloc_base);
    bool ok   = FileIsOpen(&f);

    Zstr payload = "hello-body";
    ok           = ok && (FileWrite(&f, payload, ZstrLen(payload)) == (i64)ZstrLen(payload));
    FileClose(&f);

    HttpResponse  response = HttpResponseInit(alloc_base);
    HttpResponse *res =
        HttpRespondWithFile(&response, HTTP_RESPONSE_CODE_OK, HTTP_CONTENT_TYPE_TEXT_PLAIN, StrBegin(&path));

    ok = ok && (res != NULL) && (response.status_code == HTTP_RESPONSE_CODE_OK) &&
         (response.content_type == HTTP_CONTENT_TYPE_TEXT_PLAIN) && (StrLen(&response.body) == ZstrLen(payload)) &&
         (ZstrCompare(StrBegin(&response.body), payload) == 0);

    FileRemove(StrBegin(&path));
    StrDeinit(&path);
    HttpResponseDeinit(&response);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Serving an EXISTING but EMPTY file must succeed (read returns 0
// bytes; the success test is `< 0`, so 0 passes). A `< 0`->`<= 0`
// mutant would treat a zero-byte read as failure and return NULL.
bool test_ht_respond_with_file_empty_succeeds(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    Str  path = StrInit(alloc_base);
    File f    = FileOpenTemp(&path, alloc_base);
    bool ok   = FileIsOpen(&f);
    FileClose(&f); // leave it empty

    HttpResponse  response = HttpResponseInit(alloc_base);
    HttpResponse *res =
        HttpRespondWithFile(&response, HTTP_RESPONSE_CODE_OK, HTTP_CONTENT_TYPE_TEXT_PLAIN, StrBegin(&path));

    ok = ok && (res != NULL) && (response.status_code == HTTP_RESPONSE_CODE_OK) && (StrLen(&response.body) == 0);

    FileRemove(StrBegin(&path));
    StrDeinit(&path);
    HttpResponseDeinit(&response);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

// Serving a NON-existent file must fail: FileReadAndClose returns < 0,
// so the responder returns NULL. Pins the failure edge of the same
// comparison (and the `< 0`->`>= 0` mutant, which would call a
// successful read a failure, is killed by the success tests above).
bool test_ht_respond_with_file_missing_fails(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    HttpResponse  response = HttpResponseInit(alloc_base);
    HttpResponse *res      = HttpRespondWithFile(
        &response,
        HTTP_RESPONSE_CODE_OK,
        HTTP_CONTENT_TYPE_TEXT_PLAIN,
        "no_such_file_zzz_misra_test"
    );

    bool ok = (res == NULL);

    HttpResponseDeinit(&response);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Http tests\n\n");

    TestFunction tests[] = {
        test_http_request_parse_get_with_headers,
        test_http_response_serialize_html,
        test_http_request_parse_all_methods,
        test_http_request_parse_rejects_unknown_method,
        test_http_request_method_length_exact,
        test_ht_header_count_cap_rejects_over_limit,
        test_ht_header_count_cap_accepts_at_limit,
        test_ht_respond_with_file_sets_fields,
        test_ht_respond_with_file_empty_succeeds,
        test_ht_respond_with_file_missing_fails,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Http");
}
