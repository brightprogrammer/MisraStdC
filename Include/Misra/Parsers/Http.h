/// file      : Http.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// HTTP/1.1 request + response primitives. Pure parsing / serialization;
/// no socket dependency. The serializer renders a response into a `Str`
/// which the caller is free to write to any transport (sockets, files,
/// in-memory mocks).

#ifndef MISRA_PARSERS_HTTP_H
#define MISRA_PARSERS_HTTP_H

#include <Misra/Std.h>
#include <Misra/Types.h>

typedef enum HttpRequestMethod {
    HTTP_REQUEST_METHOD_UNKNOWN = 0,
    HTTP_REQUEST_METHOD_GET,
    HTTP_REQUEST_METHOD_POST,
    HTTP_REQUEST_METHOD_DELETE,
    HTTP_REQUEST_METHOD_PUT,
    HTTP_REQUEST_METHOD_PATCH,
    HTTP_REQUEST_METHOD_HEAD,
    HTTP_REQUEST_METHOD_OPTIONS,
    HTTP_REQUEST_METHOD_CONNECT,
    HTTP_REQUEST_METHOD_TRACE,
} HttpRequestMethod;

///
/// A single `Key: Value` HTTP header. `key` and `value` are both `Str`
/// objects that own their backing storage through their stored
/// allocator. When kept inside a `Vec(HttpHeader)`, the deep-copy
/// callbacks installed via `HttpHeaderInit` handle duplication and
/// cleanup automatically.
///
typedef struct HttpHeader {
    Str key;
    Str value;
} HttpHeader;

///
/// Initialize an empty `HttpHeader`. Inside a `Scope` the allocator
/// argument may be omitted (uses `MisraScope`).
///
#define HttpHeaderInit(...)         MISRA_OVERLOAD(HttpHeaderInit, __VA_ARGS__)
#define HttpHeaderInit_0()          HttpHeaderInit_1(MisraScope)
#define HttpHeaderInit_1(alloc_ptr) ((HttpHeader) {.key = StrInit_1(alloc_ptr), .value = StrInit_1(alloc_ptr)})

typedef Vec(HttpHeader) HttpHeaders;

///
/// User-facing deinit. Releases the backing storage owned by
/// `header->key` and `header->value`, then zeros the struct.
///
/// SUCCESS : Returns to the caller. `*header` is zeroed.
/// FAILURE : Function cannot fail. NULL `header` is a no-op.
///
void HttpHeaderDeinit(HttpHeader *header);

///
/// Container-callback shape of the same operation, matching
/// `GenericCopyDeinit`. Plumb this into `VecInitWithDeepCopy` so a
/// `Vec(HttpHeader)` automatically deinits each entry on removal.
///
/// SUCCESS : Returns to the caller. `*(HttpHeader *)header` is zeroed.
/// FAILURE : Function cannot fail. NULL `header` is a no-op.
///
void http_header_deinit(void *header, const Allocator *alloc);

///
/// Container-callback for deep copy. Used as the `copy_init` half of a
/// deeply-copying `Vec(HttpHeader)`.
///
/// SUCCESS : Returns `true`. `*(HttpHeader *)dst` is a deep copy of
///           `*(const HttpHeader *)src` allocated through `alloc`.
/// FAILURE : Returns `false` on allocator OOM. `*(HttpHeader *)dst` is
///           left zeroed.
///
bool http_header_init_copy(void *dst, const void *src, const Allocator *alloc);

///
/// Find a header by key (case-sensitive zero-terminated comparison).
///
/// headers[in] : Caller's `Vec(HttpHeader)` to search.
/// key[in]     : Key to look up.
///
/// SUCCESS : Returns a pointer to the matching header inside the
///           vector. The pointer is valid until `*headers` is mutated
///           or deinitialized.
/// FAILURE : Returns `NULL` if no header matches; `*headers` is
///           unchanged.
///
HttpHeader *http_headers_find_zstr(HttpHeaders *headers, Zstr key);
HttpHeader *http_headers_find_str(HttpHeaders *headers, const Str *key);
#define HttpHeadersFind(headers, key)                                                                                                                        \
    _Generic((key), Str *: http_headers_find_str, const Str *: http_headers_find_str, char *: http_headers_find_zstr, const char *: http_headers_find_zstr)( \
        (headers),                                                                                                                                           \
        (key)                                                                                                                                                \
    )

typedef enum HttpResponseCode {
    HTTP_RESPONSE_CODE_INVALID = 0,

    HTTP_RESPONSE_CODE_CONTINUE            = 100,
    HTTP_RESPONSE_CODE_SWITCHING_PROTOCOLS = 101,
    HTTP_RESPONSE_CODE_PROCESSING          = 102,
    HTTP_RESPONSE_CODE_EARLY_HINTS         = 103,

    HTTP_RESPONSE_CODE_OK                            = 200,
    HTTP_RESPONSE_CODE_CREATED                       = 201,
    HTTP_RESPONSE_CODE_ACCEPTED                      = 202,
    HTTP_RESPONSE_CODE_NON_AUTHORITATIVE_INFORMATION = 203,
    HTTP_RESPONSE_CODE_NO_CONTENT                    = 204,
    HTTP_RESPONSE_CODE_RESET_CONTENT                 = 205,
    HTTP_RESPONSE_CODE_PARTIAL_CONTENT               = 206,
    HTTP_RESPONSE_CODE_MULTI_STATUS                  = 207,
    HTTP_RESPONSE_CODE_ALREADY_REPORTED              = 208,
    HTTP_RESPONSE_CODE_IM_USED                       = 226,

    HTTP_RESPONSE_CODE_MULTIPLE_CHOICES   = 300,
    HTTP_RESPONSE_CODE_MOVED_PERMANENTLY  = 301,
    HTTP_RESPONSE_CODE_FOUND              = 302,
    HTTP_RESPONSE_CODE_SEE_OTHER          = 303,
    HTTP_RESPONSE_CODE_NOT_MODIFIED       = 304,
    HTTP_RESPONSE_CODE_USE_PROXY          = 305,
    HTTP_RESPONSE_CODE_TEMPORARY_REDIRECT = 307,
    HTTP_RESPONSE_CODE_PERMANENT_REDIRECT = 308,

    HTTP_RESPONSE_CODE_BAD_REQUEST                     = 400,
    HTTP_RESPONSE_CODE_UNAUTHORIZED                    = 401,
    HTTP_RESPONSE_CODE_PAYMENT_REQUIRED                = 402,
    HTTP_RESPONSE_CODE_FORBIDDEN                       = 403,
    HTTP_RESPONSE_CODE_NOT_FOUND                       = 404,
    HTTP_RESPONSE_CODE_METHOD_NOT_ALLOWED              = 405,
    HTTP_RESPONSE_CODE_NOT_ACCEPTABLE                  = 406,
    HTTP_RESPONSE_CODE_PROXY_AUTHENTICATION_REQUIRED   = 407,
    HTTP_RESPONSE_CODE_REQUEST_TIMEOUT                 = 408,
    HTTP_RESPONSE_CODE_CONFLICT                        = 409,
    HTTP_RESPONSE_CODE_GONE                            = 410,
    HTTP_RESPONSE_CODE_LENGTH_REQUIRED                 = 411,
    HTTP_RESPONSE_CODE_PRECONDITION_FAILED             = 412,
    HTTP_RESPONSE_CODE_PAYLOAD_TOO_LARGE               = 413,
    HTTP_RESPONSE_CODE_URI_TOO_LONG                    = 414,
    HTTP_RESPONSE_CODE_UNSUPPORTED_MEDIA_TYPE          = 415,
    HTTP_RESPONSE_CODE_RANGE_NOT_SATISFIABLE           = 416,
    HTTP_RESPONSE_CODE_EXPECTATION_FAILED              = 417,
    HTTP_RESPONSE_CODE_IM_A_TEAPOT                     = 418,
    HTTP_RESPONSE_CODE_MISDIRECTED_REQUEST             = 421,
    HTTP_RESPONSE_CODE_UNPROCESSABLE_ENTITY            = 422,
    HTTP_RESPONSE_CODE_LOCKED                          = 423,
    HTTP_RESPONSE_CODE_FAILED_DEPENDENCY               = 424,
    HTTP_RESPONSE_CODE_TOO_EARLY                       = 425,
    HTTP_RESPONSE_CODE_UPGRADE_REQUIRED                = 426,
    HTTP_RESPONSE_CODE_PRECONDITION_REQUIRED           = 428,
    HTTP_RESPONSE_CODE_TOO_MANY_REQUESTS               = 429,
    HTTP_RESPONSE_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE = 431,
    HTTP_RESPONSE_CODE_UNAVAILABLE_FOR_LEGAL_REASONS   = 451,

    HTTP_RESPONSE_CODE_INTERNAL_SERVER_ERROR           = 500,
    HTTP_RESPONSE_CODE_NOT_IMPLEMENTED                 = 501,
    HTTP_RESPONSE_CODE_BAD_GATEWAY                     = 502,
    HTTP_RESPONSE_CODE_SERVICE_UNAVAILABLE             = 503,
    HTTP_RESPONSE_CODE_GATEWAY_TIMEOUT                 = 504,
    HTTP_RESPONSE_CODE_HTTP_VERSION_NOT_SUPPORTED      = 505,
    HTTP_RESPONSE_CODE_VARIANT_ALSO_NEGOTIATES         = 506,
    HTTP_RESPONSE_CODE_INSUFFICIENT_STORAGE            = 507,
    HTTP_RESPONSE_CODE_LOOP_DETECTED                   = 508,
    HTTP_RESPONSE_CODE_NOT_EXTENDED                    = 510,
    HTTP_RESPONSE_CODE_NETWORK_AUTHENTICATION_REQUIRED = 511,
} HttpResponseCode;

typedef enum HttpContentType {
    HTTP_CONTENT_TYPE_INVALID = 0,

    HTTP_CONTENT_TYPE_TEXT_HTML,
    HTTP_CONTENT_TYPE_TEXT_PLAIN,
    HTTP_CONTENT_TYPE_TEXT_CSS,
    HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT,
    HTTP_CONTENT_TYPE_TEXT_CSV,

    HTTP_CONTENT_TYPE_APPLICATION_JSON,
    HTTP_CONTENT_TYPE_APPLICATION_XML,
    HTTP_CONTENT_TYPE_APPLICATION_JAVASCRIPT,
    HTTP_CONTENT_TYPE_APPLICATION_PDF,
    HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM,
    HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED,
    HTTP_CONTENT_TYPE_APPLICATION_ZIP,
    HTTP_CONTENT_TYPE_APPLICATION_MS_EXCEL,
    HTTP_CONTENT_TYPE_APPLICATION_OPENXML_SPREADSHEET,
    HTTP_CONTENT_TYPE_APPLICATION_LD_JSON,
    HTTP_CONTENT_TYPE_APPLICATION_GRAPHQL,
    HTTP_CONTENT_TYPE_APPLICATION_FONT_WOFF,

    HTTP_CONTENT_TYPE_IMAGE_JPEG,
    HTTP_CONTENT_TYPE_IMAGE_PNG,
    HTTP_CONTENT_TYPE_IMAGE_GIF,
    HTTP_CONTENT_TYPE_IMAGE_BMP,
    HTTP_CONTENT_TYPE_IMAGE_WEBP,
    HTTP_CONTENT_TYPE_IMAGE_SVG_XML,

    HTTP_CONTENT_TYPE_AUDIO_MPEG,
    HTTP_CONTENT_TYPE_AUDIO_OGG,
    HTTP_CONTENT_TYPE_AUDIO_WAV,

    HTTP_CONTENT_TYPE_VIDEO_MP4,
    HTTP_CONTENT_TYPE_VIDEO_OGG,
    HTTP_CONTENT_TYPE_VIDEO_WEBM,

    HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA,
    HTTP_CONTENT_TYPE_MULTIPART_BYTERANGES,

    HTTP_CONTENT_TYPE_FONT_WOFF,
    HTTP_CONTENT_TYPE_FONT_WOFF2,
} HttpContentType;

///
/// Parsed HTTP request. Carries the allocator that owns `url` and
/// `headers`; all sub-allocations route through the same handle.
///
typedef struct HttpRequest {
    Allocator        *allocator;
    HttpRequestMethod method;
    Str               url;
    HttpHeaders       headers;
} HttpRequest;

///
/// Initialize an empty `HttpRequest`. Inside a `Scope` the allocator
/// argument may be omitted (uses `MisraScope`).
///
#define HttpRequestInit(...) MISRA_OVERLOAD(HttpRequestInit, __VA_ARGS__)
#define HttpRequestInit_0()  HttpRequestInit_1(MisraScope)
#define HttpRequestInit_1(alloc_ptr)                                                                                   \
    ((HttpRequest) {.allocator = ALLOCATOR_OF(alloc_ptr),                                                              \
                    .method    = HTTP_REQUEST_METHOD_UNKNOWN,                                                          \
                    .url       = StrInit_1(alloc_ptr),                                                                 \
                    .headers   = VecInitWithDeepCopy_3(http_header_init_copy, http_header_deinit, alloc_ptr)})

///
/// Parse an HTTP/1.1 request out of `in` into `req`. `req` must already
/// be initialized with `HttpRequestInit(...)` so the parser has an
/// allocator to write into.
///
/// SUCCESS : Returns a pointer past the parsed request line + headers
///           (start of the body).
/// FAILURE : Returns `in` unchanged when the input is malformed.
///
Zstr http_request_parse_zstr(HttpRequest *req, Zstr in);
Zstr http_request_parse_str(HttpRequest *req, const Str *in);
#define HttpRequestParse(req, in)                                                                                                                               \
    _Generic((in), Str *: http_request_parse_str, const Str *: http_request_parse_str, char *: http_request_parse_zstr, const char *: http_request_parse_zstr)( \
        (req),                                                                                                                                                  \
        (in)                                                                                                                                                    \
    )

///
/// Release storage owned by `req` and zero the struct. Safe to call on
/// a partially-parsed request.
///
/// SUCCESS : Returns to the caller. `*req` is zeroed.
/// FAILURE : Function cannot fail. NULL `req` is a no-op.
///
void HttpRequestDeinit(HttpRequest *req);

///
/// HTTP response under construction. Same allocator-ownership story as
/// `HttpRequest`.
///
typedef struct HttpResponse {
    Allocator       *allocator;
    HttpContentType  content_type;
    HttpResponseCode status_code;
    HttpHeaders      headers;
    Str              body;
} HttpResponse;

#define HttpResponseInit(...) MISRA_OVERLOAD(HttpResponseInit, __VA_ARGS__)
#define HttpResponseInit_0()  HttpResponseInit_1(MisraScope)
#define HttpResponseInit_1(alloc_ptr)                                                                                  \
    ((HttpResponse) {.allocator    = ALLOCATOR_OF(alloc_ptr),                                                          \
                     .content_type = HTTP_CONTENT_TYPE_INVALID,                                                        \
                     .status_code  = HTTP_RESPONSE_CODE_INVALID,                                                       \
                     .headers      = VecInitWithDeepCopy_3(http_header_init_copy, http_header_deinit, alloc_ptr),      \
                     .body         = StrInit_1(alloc_ptr)})

///
/// Wire-format lookup tables.
///
/// SUCCESS : Returns the canonical HTTP/1.1 reason-phrase string for
///           the given code / content type. The pointer is to static
///           storage and is valid for the lifetime of the program.
/// FAILURE : Returns `"Unknown"` for codes / content types outside the
///           recognised enum range. Cannot fail.
///
Zstr HttpResponseCodeToZstr(HttpResponseCode code);
Zstr HttpContentTypeToZstr(HttpContentType content_type);

///
/// Populate `response` as an HTML reply. The body is a deep copy of
/// `html` allocated through `response->allocator`.
///
/// SUCCESS : Returns `response` with `status_code`, `content_type`, and
///           `body` updated.
/// FAILURE : Does not return - aborts on NULL arguments.
///
HttpResponse *HttpRespondWithHtml(HttpResponse *response, HttpResponseCode status, const Str *html);

#if FEATURE_FILE
///
/// Populate `response` from a file on disk. The file's bytes are read
/// through `response->allocator`. Only available when the `file`
/// feature is enabled.
///
/// SUCCESS : Returns `response` with body filled.
/// FAILURE : Returns NULL on I/O or allocation failure.
///
HttpResponse *
    HttpRespondWithFile(HttpResponse *response, HttpResponseCode status, HttpContentType content_type, Zstr filepath);
#endif

///
/// Serialize `response` to its on-wire HTTP/1.1 form. Caller owns the
/// returned `Str` and must deinit it. The result is exactly the bytes
/// that should land on the transport (sockets / files / etc.) — no
/// transport call is made.
///
/// SUCCESS : Returns a populated `Str`.
/// FAILURE : Returns an empty `Str` and logs the failing condition
///           (unknown response code, unknown content type, etc.).
///
Str http_response_serialize(const HttpResponse *response, Allocator *alloc);
#define HttpResponseSerialize(...)               MISRA_OVERLOAD(HttpResponseSerialize, __VA_ARGS__)
#define HttpResponseSerialize_1(response)        http_response_serialize((response), MisraScope)
#define HttpResponseSerialize_2(response, alloc) http_response_serialize((response), ALLOCATOR_OF(alloc))

///
/// Release storage owned by `response` and zero the struct.
///
/// SUCCESS : Returns to the caller. `*response` is zeroed.
/// FAILURE : Function cannot fail. NULL `response` is a no-op.
///
void HttpResponseDeinit(HttpResponse *response);

#endif // MISRA_PARSERS_HTTP_H
