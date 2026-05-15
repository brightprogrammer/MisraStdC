#include <Misra/Parsers/Http.h>
#include <Misra/Std/Allocator/Default.h>
#include <Misra/Std/Log.h>


#include "../Util/TestRunner.h"

bool test_http_request_parse_get_with_headers(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    const char *raw =
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: misra-test/1.0\r\n"
        "\r\n"
        "body-bytes";

    HttpRequest req  = HttpRequestInit(alloc_base);
    const char *next = HttpRequestParse(&req, raw);

    bool ok = (next != raw) && (req.method == HTTP_REQUEST_METHOD_GET) && (req.url.length == 11) &&
              (ZstrCompare(req.url.data, "/index.html") == 0) && (req.headers.length == 2) &&
              (ZstrCompare(next, "body-bytes") == 0);

    HttpHeader *host = HttpHeadersFind(&req.headers, "Host");
    ok               = ok && host && ZstrCompare(host->value.data, "example.com") == 0;

    HttpRequestDeinit(&req);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

bool test_http_response_serialize_html(void) {
    DefaultAllocator alloc      = DefaultAllocatorInit();
    Allocator       *alloc_base = ALLOCATOR_OF(&alloc);

    HttpResponse response = HttpResponseInit(alloc_base);
    Str          body     = StrInit(alloc_base);
    StrWriteFmt(&body, "<h1>hi</h1>");
    HttpRespondWithHtml(&response, HTTP_RESPONSE_CODE_OK, &body);
    StrDeinit(&body);

    Str wire = HttpResponseSerialize(&response, alloc_base);

    // Spot-check the wire format:
    //   - starts with "HTTP/1.1 200 OK"
    //   - includes a Content-Type: text/html
    //   - includes a Content-Length: 11 (length of "<h1>hi</h1>")
    //   - ends with the body
    bool ok = wire.length > 0 && ZstrFindSubstring(wire.data, "HTTP/1.1 200 OK\r\n") == wire.data &&
              ZstrFindSubstring(wire.data, "Content-Type: text/html\r\n") != NULL &&
              ZstrFindSubstring(wire.data, "Content-Length: 11\r\n") != NULL &&
              ZstrFindSubstring(wire.data, "\r\n\r\n<h1>hi</h1>") != NULL;

    StrDeinit(&wire);
    HttpResponseDeinit(&response);
    DefaultAllocatorDeinit(&alloc);
    return ok;
}

int main(void) {
    WriteFmt("[INFO] Starting Http tests\n\n");

    TestFunction tests[] = {
        test_http_request_parse_get_with_headers,
        test_http_response_serialize_html,
    };

    return run_test_suite(tests, sizeof(tests) / sizeof(tests[0]), NULL, 0, "Http");
}
