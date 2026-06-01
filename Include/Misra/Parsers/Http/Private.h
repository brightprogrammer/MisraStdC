/// file      : parsers/http/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/Http namespace. Not part of
/// the documented public surface; the PascalCase macros in
/// <Misra/Parsers/Http.h> are the only intended call shape. The two
/// container-callback helpers (`http_header_deinit`, `http_header_init_copy`)
/// are referenced from public HttpRequestInit / HttpResponseInit macros so
/// they must remain visible at the use site.

#ifndef MISRA_PARSERS_HTTP_PRIVATE_H
#define MISRA_PARSERS_HTTP_PRIVATE_H

#include <Misra/Std/Allocator.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HttpRequest    HttpRequest;
typedef struct HttpResponse   HttpResponse;
typedef enum   HttpResponseCode HttpResponseCode;
typedef enum   HttpContentType  HttpContentType;

void http_header_deinit(void *header, const Allocator *alloc);
bool http_header_init_copy(void *dst, const void *src, const Allocator *alloc);
Zstr http_request_parse_zstr(HttpRequest *req, Zstr in);
Zstr http_request_parse_str(HttpRequest *req, const Str *in);
#if FEATURE_FILE
HttpResponse *http_respond_with_file_zstr(
    HttpResponse    *response,
    HttpResponseCode status,
    HttpContentType  content_type,
    Zstr             filepath
);
HttpResponse *http_respond_with_file_str(
    HttpResponse    *response,
    HttpResponseCode status,
    HttpContentType  content_type,
    const Str       *filepath
);
#endif
Str http_response_serialize(const HttpResponse *response, Allocator *alloc);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_HTTP_PRIVATE_H
