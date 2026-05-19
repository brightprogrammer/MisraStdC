/// file      : Http.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// HTTP/1.1 request + response runtime helpers. Pure parse / serialize;
/// the transport layer (sockets / files / etc.) is the caller's problem.

#include <Misra/Parsers/Http.h>

#if FEATURE_FILE
#    include <Misra/Std/File.h>
#endif

// ---------------------------------------------------------------------------
// HttpHeader
// ---------------------------------------------------------------------------

void HttpHeaderDeinit(HttpHeader *header) {
    if (!header) {
        LOG_FATAL("invalid arguments");
    }
    StrDeinit(&header->key);
    StrDeinit(&header->value);
    MemSet(header, 0, sizeof(*header));
}

void http_header_deinit(void *header_ptr, const Allocator *alloc) {
    (void)alloc; // each Str carries its own allocator handle
    if (!header_ptr) {
        return;
    }
    HttpHeader *header = (HttpHeader *)header_ptr;
    StrDeinit(&header->key);
    StrDeinit(&header->value);
    MemSet(header, 0, sizeof(*header));
}

bool http_header_init_copy(void *dst_ptr, const void *src_ptr, const Allocator *alloc) {
    if (!dst_ptr || !src_ptr || !alloc) {
        LOG_FATAL("invalid arguments");
    }
    HttpHeader       *dst = (HttpHeader *)dst_ptr;
    const HttpHeader *src = (const HttpHeader *)src_ptr;

    dst->key   = StrInit((Allocator *)alloc);
    dst->value = StrInit((Allocator *)alloc);

    if (!StrInitCopy(&dst->key, &src->key)) {
        StrDeinit(&dst->key);
        return false;
    }
    if (!StrInitCopy(&dst->value, &src->value)) {
        StrDeinit(&dst->key);
        StrDeinit(&dst->value);
        return false;
    }
    return true;
}

HttpHeader *HttpHeadersFind(HttpHeaders *headers, const char *key) {
    if (!headers || !key) {
        LOG_ERROR("invalid arguments");
        return NULL;
    }
    VecForeachPtr(headers, header) {
        if (0 == ZstrCompare(header->key.data, key)) {
            return header;
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// HttpRequest
// ---------------------------------------------------------------------------

static HttpRequestMethod http_request_method_from_str(const Str *mstr) {
    if (!mstr || !mstr->data) {
        return HTTP_REQUEST_METHOD_UNKNOWN;
    }
    if (0 == ZstrCompareN(mstr->data, "GET", 3) && mstr->length == 3)
        return HTTP_REQUEST_METHOD_GET;
    if (0 == ZstrCompareN(mstr->data, "POST", 4) && mstr->length == 4)
        return HTTP_REQUEST_METHOD_POST;
    if (0 == ZstrCompareN(mstr->data, "DELETE", 6) && mstr->length == 6)
        return HTTP_REQUEST_METHOD_DELETE;
    if (0 == ZstrCompareN(mstr->data, "PUT", 3) && mstr->length == 3)
        return HTTP_REQUEST_METHOD_PUT;
    if (0 == ZstrCompareN(mstr->data, "PATCH", 5) && mstr->length == 5)
        return HTTP_REQUEST_METHOD_PATCH;
    if (0 == ZstrCompareN(mstr->data, "HEAD", 4) && mstr->length == 4)
        return HTTP_REQUEST_METHOD_HEAD;
    if (0 == ZstrCompareN(mstr->data, "OPTIONS", 7) && mstr->length == 7)
        return HTTP_REQUEST_METHOD_OPTIONS;
    if (0 == ZstrCompareN(mstr->data, "CONNECT", 7) && mstr->length == 7)
        return HTTP_REQUEST_METHOD_CONNECT;
    if (0 == ZstrCompareN(mstr->data, "TRACE", 5) && mstr->length == 5)
        return HTTP_REQUEST_METHOD_TRACE;
    return HTTP_REQUEST_METHOD_UNKNOWN;
}

const char *HttpRequestParse(HttpRequest *req, const char *in) {
    if (!req || !req->allocator || !in) {
        LOG_FATAL("invalid arguments");
    }

    Allocator  *alloc   = req->allocator;
    const char *cursor  = in;
    Str         method  = StrInit(alloc);
    Str         version = StrInit(alloc);

    StrReadFmt(cursor, "{} {} {}\r\n", method, req->url, version);
    if (cursor == in) {
        LOG_ERROR("http request parse failed: invalid request line");
        StrDeinit(&method);
        StrDeinit(&version);
        return in;
    }

    if (0 != ZstrCompareN(version.data, "HTTP/1.1", 8)) {
        LOG_ERROR("invalid/unsupported HTTP version");
        StrDeinit(&method);
        StrDeinit(&version);
        return in;
    }
    StrDeinit(&version);

    req->method = http_request_method_from_str(&method);
    StrDeinit(&method);
    if (req->method == HTTP_REQUEST_METHOD_UNKNOWN) {
        LOG_ERROR("invalid http request method");
        return in;
    }

    while (true) {
        const char *line_start = cursor;

        if (0 == ZstrCompareN(cursor, "\r\n", 2)) {
            cursor += 2;
            break;
        }

        HttpHeader hh = HttpHeaderInit(alloc);
        StrReadFmt(cursor, "{}: {}\r\n", hh.key, hh.value);
        if (cursor == line_start) {
            LOG_ERROR("failed to parse header line");
            HttpHeaderDeinit(&hh);
            return in;
        }

        if (!VecPushBackR(&req->headers, hh)) {
            HttpHeaderDeinit(&hh);
            LOG_ERROR("failed to push header");
            return in;
        }
    }

    return cursor;
}

void HttpRequestDeinit(HttpRequest *req) {
    if (!req) {
        LOG_FATAL("invalid arguments");
    }
    StrDeinit(&req->url);
    VecDeinit(&req->headers);
    MemSet(req, 0, sizeof(*req));
}

// ---------------------------------------------------------------------------
// HttpResponse: lookup tables
// ---------------------------------------------------------------------------

const char *HttpResponseCodeToZstr(HttpResponseCode code) {
    switch (code) {
        case HTTP_RESPONSE_CODE_CONTINUE :
            return "100 Continue";
        case HTTP_RESPONSE_CODE_SWITCHING_PROTOCOLS :
            return "101 Switching Protocols";
        case HTTP_RESPONSE_CODE_PROCESSING :
            return "102 Processing";
        case HTTP_RESPONSE_CODE_EARLY_HINTS :
            return "103 Early Hints";
        case HTTP_RESPONSE_CODE_OK :
            return "200 OK";
        case HTTP_RESPONSE_CODE_CREATED :
            return "201 Created";
        case HTTP_RESPONSE_CODE_ACCEPTED :
            return "202 Accepted";
        case HTTP_RESPONSE_CODE_NON_AUTHORITATIVE_INFORMATION :
            return "203 Non-Authoritative Information";
        case HTTP_RESPONSE_CODE_NO_CONTENT :
            return "204 No Content";
        case HTTP_RESPONSE_CODE_RESET_CONTENT :
            return "205 Reset Content";
        case HTTP_RESPONSE_CODE_PARTIAL_CONTENT :
            return "206 Partial Content";
        case HTTP_RESPONSE_CODE_MULTI_STATUS :
            return "207 Multi-Status";
        case HTTP_RESPONSE_CODE_ALREADY_REPORTED :
            return "208 Already Reported";
        case HTTP_RESPONSE_CODE_IM_USED :
            return "226 IM Used";
        case HTTP_RESPONSE_CODE_MULTIPLE_CHOICES :
            return "300 Multiple Choices";
        case HTTP_RESPONSE_CODE_MOVED_PERMANENTLY :
            return "301 Moved Permanently";
        case HTTP_RESPONSE_CODE_FOUND :
            return "302 Found";
        case HTTP_RESPONSE_CODE_SEE_OTHER :
            return "303 See Other";
        case HTTP_RESPONSE_CODE_NOT_MODIFIED :
            return "304 Not Modified";
        case HTTP_RESPONSE_CODE_USE_PROXY :
            return "305 Use Proxy";
        case HTTP_RESPONSE_CODE_TEMPORARY_REDIRECT :
            return "307 Temporary Redirect";
        case HTTP_RESPONSE_CODE_PERMANENT_REDIRECT :
            return "308 Permanent Redirect";
        case HTTP_RESPONSE_CODE_BAD_REQUEST :
            return "400 Bad Request";
        case HTTP_RESPONSE_CODE_UNAUTHORIZED :
            return "401 Unauthorized";
        case HTTP_RESPONSE_CODE_PAYMENT_REQUIRED :
            return "402 Payment Required";
        case HTTP_RESPONSE_CODE_FORBIDDEN :
            return "403 Forbidden";
        case HTTP_RESPONSE_CODE_NOT_FOUND :
            return "404 Not Found";
        case HTTP_RESPONSE_CODE_METHOD_NOT_ALLOWED :
            return "405 Method Not Allowed";
        case HTTP_RESPONSE_CODE_NOT_ACCEPTABLE :
            return "406 Not Acceptable";
        case HTTP_RESPONSE_CODE_PROXY_AUTHENTICATION_REQUIRED :
            return "407 Proxy Authentication Required";
        case HTTP_RESPONSE_CODE_REQUEST_TIMEOUT :
            return "408 Request Timeout";
        case HTTP_RESPONSE_CODE_CONFLICT :
            return "409 Conflict";
        case HTTP_RESPONSE_CODE_GONE :
            return "410 Gone";
        case HTTP_RESPONSE_CODE_LENGTH_REQUIRED :
            return "411 Length Required";
        case HTTP_RESPONSE_CODE_PRECONDITION_FAILED :
            return "412 Precondition Failed";
        case HTTP_RESPONSE_CODE_PAYLOAD_TOO_LARGE :
            return "413 Payload Too Large";
        case HTTP_RESPONSE_CODE_URI_TOO_LONG :
            return "414 URI Too Long";
        case HTTP_RESPONSE_CODE_UNSUPPORTED_MEDIA_TYPE :
            return "415 Unsupported Media Type";
        case HTTP_RESPONSE_CODE_RANGE_NOT_SATISFIABLE :
            return "416 Range Not Satisfiable";
        case HTTP_RESPONSE_CODE_EXPECTATION_FAILED :
            return "417 Expectation Failed";
        case HTTP_RESPONSE_CODE_IM_A_TEAPOT :
            return "418 I'm a teapot";
        case HTTP_RESPONSE_CODE_MISDIRECTED_REQUEST :
            return "421 Misdirected Request";
        case HTTP_RESPONSE_CODE_UNPROCESSABLE_ENTITY :
            return "422 Unprocessable Entity";
        case HTTP_RESPONSE_CODE_LOCKED :
            return "423 Locked";
        case HTTP_RESPONSE_CODE_FAILED_DEPENDENCY :
            return "424 Failed Dependency";
        case HTTP_RESPONSE_CODE_TOO_EARLY :
            return "425 Too Early";
        case HTTP_RESPONSE_CODE_UPGRADE_REQUIRED :
            return "426 Upgrade Required";
        case HTTP_RESPONSE_CODE_PRECONDITION_REQUIRED :
            return "428 Precondition Required";
        case HTTP_RESPONSE_CODE_TOO_MANY_REQUESTS :
            return "429 Too Many Requests";
        case HTTP_RESPONSE_CODE_REQUEST_HEADER_FIELDS_TOO_LARGE :
            return "431 Request Header Fields Too Large";
        case HTTP_RESPONSE_CODE_UNAVAILABLE_FOR_LEGAL_REASONS :
            return "451 Unavailable For Legal Reasons";
        case HTTP_RESPONSE_CODE_INTERNAL_SERVER_ERROR :
            return "500 Internal Server Error";
        case HTTP_RESPONSE_CODE_NOT_IMPLEMENTED :
            return "501 Not Implemented";
        case HTTP_RESPONSE_CODE_BAD_GATEWAY :
            return "502 Bad Gateway";
        case HTTP_RESPONSE_CODE_SERVICE_UNAVAILABLE :
            return "503 Service Unavailable";
        case HTTP_RESPONSE_CODE_GATEWAY_TIMEOUT :
            return "504 Gateway Timeout";
        case HTTP_RESPONSE_CODE_HTTP_VERSION_NOT_SUPPORTED :
            return "505 HTTP Version Not Supported";
        case HTTP_RESPONSE_CODE_VARIANT_ALSO_NEGOTIATES :
            return "506 Variant Also Negotiates";
        case HTTP_RESPONSE_CODE_INSUFFICIENT_STORAGE :
            return "507 Insufficient Storage";
        case HTTP_RESPONSE_CODE_LOOP_DETECTED :
            return "508 Loop Detected";
        case HTTP_RESPONSE_CODE_NOT_EXTENDED :
            return "510 Not Extended";
        case HTTP_RESPONSE_CODE_NETWORK_AUTHENTICATION_REQUIRED :
            return "511 Network Authentication Required";
        default :
            return NULL;
    }
}

const char *HttpContentTypeToZstr(HttpContentType type) {
    switch (type) {
        case HTTP_CONTENT_TYPE_TEXT_PLAIN :
            return "text/plain";
        case HTTP_CONTENT_TYPE_TEXT_HTML :
            return "text/html";
        case HTTP_CONTENT_TYPE_TEXT_CSS :
            return "text/css";
        case HTTP_CONTENT_TYPE_TEXT_JAVASCRIPT :
            return "text/javascript";
        case HTTP_CONTENT_TYPE_TEXT_CSV :
            return "text/csv";
        case HTTP_CONTENT_TYPE_APPLICATION_JSON :
            return "application/json";
        case HTTP_CONTENT_TYPE_APPLICATION_XML :
            return "application/xml";
        case HTTP_CONTENT_TYPE_APPLICATION_JAVASCRIPT :
            return "application/javascript";
        case HTTP_CONTENT_TYPE_APPLICATION_PDF :
            return "application/pdf";
        case HTTP_CONTENT_TYPE_APPLICATION_ZIP :
            return "application/zip";
        case HTTP_CONTENT_TYPE_APPLICATION_OCTET_STREAM :
            return "application/octet-stream";
        case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED :
            return "application/x-www-form-urlencoded";
        case HTTP_CONTENT_TYPE_APPLICATION_MS_EXCEL :
            return "application/vnd.ms-excel";
        case HTTP_CONTENT_TYPE_APPLICATION_OPENXML_SPREADSHEET :
            return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
        case HTTP_CONTENT_TYPE_APPLICATION_LD_JSON :
            return "application/ld+json";
        case HTTP_CONTENT_TYPE_APPLICATION_GRAPHQL :
            return "application/graphql";
        case HTTP_CONTENT_TYPE_APPLICATION_FONT_WOFF :
            return "application/font-woff";
        case HTTP_CONTENT_TYPE_IMAGE_JPEG :
            return "image/jpeg";
        case HTTP_CONTENT_TYPE_IMAGE_PNG :
            return "image/png";
        case HTTP_CONTENT_TYPE_IMAGE_GIF :
            return "image/gif";
        case HTTP_CONTENT_TYPE_IMAGE_BMP :
            return "image/bmp";
        case HTTP_CONTENT_TYPE_IMAGE_WEBP :
            return "image/webp";
        case HTTP_CONTENT_TYPE_IMAGE_SVG_XML :
            return "image/svg+xml";
        case HTTP_CONTENT_TYPE_AUDIO_MPEG :
            return "audio/mpeg";
        case HTTP_CONTENT_TYPE_AUDIO_OGG :
            return "audio/ogg";
        case HTTP_CONTENT_TYPE_AUDIO_WAV :
            return "audio/wav";
        case HTTP_CONTENT_TYPE_VIDEO_MP4 :
            return "video/mp4";
        case HTTP_CONTENT_TYPE_VIDEO_WEBM :
            return "video/webm";
        case HTTP_CONTENT_TYPE_VIDEO_OGG :
            return "video/ogg";
        case HTTP_CONTENT_TYPE_MULTIPART_FORM_DATA :
            return "multipart/form-data";
        case HTTP_CONTENT_TYPE_MULTIPART_BYTERANGES :
            return "multipart/byteranges";
        case HTTP_CONTENT_TYPE_FONT_WOFF :
            return "font/woff";
        case HTTP_CONTENT_TYPE_FONT_WOFF2 :
            return "font/woff2";
        default :
            return NULL;
    }
}

// ---------------------------------------------------------------------------
// HttpResponse: builders + serialization
// ---------------------------------------------------------------------------

HttpResponse *HttpRespondWithHtml(HttpResponse *response, HttpResponseCode status, const Str *html) {
    if (!response || !response->allocator || !html) {
        LOG_FATAL("invalid arguments");
    }
    response->status_code  = status;
    response->content_type = HTTP_CONTENT_TYPE_TEXT_HTML;
    StrDeinit(&response->body);
    response->body = StrDup(html, response->allocator);
    return response;
}

#if FEATURE_FILE
HttpResponse *HttpRespondWithFile(
    HttpResponse    *response,
    HttpResponseCode status,
    HttpContentType  content_type,
    const char      *filepath
) {
    if (!response || !response->allocator || !filepath) {
        LOG_FATAL("invalid arguments");
    }
    response->status_code  = status;
    response->content_type = content_type;
    StrDeinit(&response->body);
    response->body = StrInit(response->allocator);
    if (!ReadCompleteFile(
            filepath,
            &response->body.data,
            &response->body.length,
            &response->body.capacity,
            response->allocator
        )) {
        LOG_ERROR("failed to read file: {}", filepath);
        return NULL;
    }
    return response;
}
#endif

Str http_response_serialize(const HttpResponse *response, Allocator *alloc) {
    Str out = StrInit(alloc);

    if (!response) {
        LOG_ERROR("HttpResponseSerialize: response is NULL");
        return out;
    }

    const char *response_code = HttpResponseCodeToZstr(response->status_code);
    if (!response_code) {
        LOG_ERROR("HttpResponseSerialize: invalid/unknown response code {}", (u32)response->status_code);
        return out;
    }
    const char *content_type = HttpContentTypeToZstr(response->content_type);
    if (!content_type) {
        LOG_ERROR("HttpResponseSerialize: invalid/unknown content type {}", (u32)response->content_type);
        return out;
    }

    StrAppendFmt(
        &out,
        "HTTP/1.1 {}\r\n"
        "Content-Type: {}\r\n"
        "Content-Length: {}\r\n",
        response_code,
        content_type,
        response->body.length
    );

    VecForeachPtr(&response->headers, header) {
        StrAppendFmt(&out, "{}: {}\r\n", header->key, header->value);
    }

    StrAppendFmt(&out, "\r\n");

    if (response->body.length) {
        u64 head = out.length;
        if (!StrReserve(&out, head + response->body.length + 1)) {
            LOG_ERROR("HttpResponseSerialize: failed to reserve buffer");
            StrDeinit(&out);
            return StrInit(alloc);
        }
        MemCopy(out.data + head, response->body.data, response->body.length);
        out.length           = head + response->body.length;
        out.data[out.length] = 0;
    }
    return out;
}

void HttpResponseDeinit(HttpResponse *response) {
    if (!response) {
        LOG_FATAL("invalid arguments");
    }
    StrDeinit(&response->body);
    VecDeinit(&response->headers);
    MemSet(response, 0, sizeof(*response));
}
