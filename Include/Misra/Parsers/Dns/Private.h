/// file      : parsers/dns/private.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Private snake_case backends for the Parsers/Dns namespace. Not part of
/// the documented public surface; the PascalCase macros in
/// <Misra/Parsers/Dns.h> are the only intended call shape.

#ifndef MISRA_PARSERS_DNS_PRIVATE_H
#define MISRA_PARSERS_DNS_PRIVATE_H

#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Public types (DnsWireBuf, DnsType) are defined in
// <Misra/Parsers/Dns.h>, which includes this header AFTER those
// typedefs. Don't redeclare them here -- it would clash with the
// public definitions.

bool dns_build_query_zstr(DnsWireBuf *out, u16 id, Zstr name, DnsType type);
bool dns_build_query_str(DnsWireBuf *out, u16 id, const Str *name, DnsType type);

#ifdef __cplusplus
}
#endif

#endif // MISRA_PARSERS_DNS_PRIVATE_H
