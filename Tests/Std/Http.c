#include <Misra/Parsers/Http.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>


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

int main(void) {
    WriteFmt("[INFO] Starting Http tests\n\n");

    TestFunction tests[] = {
        test_http_request_parse_get_with_headers,
        test_http_response_serialize_html,
        test_http_request_parse_all_methods,
        test_http_request_parse_rejects_unknown_method,
        test_http_request_method_length_exact,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Http");
}
