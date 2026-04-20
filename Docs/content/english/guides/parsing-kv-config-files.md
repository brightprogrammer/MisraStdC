---
title: "Parsing Key-Value Config Files"
date: 2026-04-19
description: "How to use the KvConfig parser for simple application and server configuration."
authors:
  - siddharth-mishra
tags:
  - parser
  - config
  - map
  - strings
---

`KvConfig` is the simple parser you reach for when JSON is unnecessary and environment variables are too flat.

It is meant for the common case of application configuration files:

- `key = value`
- `key: value`
- blank lines
- `#` and `;` comments
- quoted values when whitespace matters

The parsed result is stored in a `Map(Str, Str)`, so the interface stays close to the rest of MisraStdC instead of inventing a separate config object model.

## A Minimal Example

```c
#include <Misra.h>

int main(void) {
    Str      text = StrInitFromZstr(
        "host = localhost\n"
        "port = 8080\n"
        "debug = true\n"
    );
    KvConfig cfg  = KvConfigInit();
    StrIter  si   = StrIterFromStr(text);
    i64      port = 0;
    bool     debug = false;

    si = KvConfigParse(si, &cfg);

    Str *host = KvConfigGet(&cfg, "host");
    KvConfigGetI64(&cfg, "port", &port);
    KvConfigGetBool(&cfg, "debug", &debug);

    WriteFmtLn("host = {}", *host);
    WriteFmtLn("port = {}", port);
    WriteFmtLn("debug = {}", debug);

    KvConfigDeinit(&cfg);
    StrDeinit(&text);
}
```

The important point is that parsing and access are separate concerns:

- `KvConfigParse(...)` reads the file format
- `KvConfigGet(...)` returns the stored string value
- typed helpers like `KvConfigGetI64(...)` and `KvConfigGetBool(...)` convert the stored text when needed

## What Syntax It Accepts

The parser supports both `=` and `:` as separators:

```text
host = localhost
port: 8080
```

Leading and trailing whitespace around keys and unquoted values is ignored.

Comments are supported in two forms:

```text
# full-line comment
workers = 16 ; inline comment
```

Quoted values keep inner whitespace:

```text
path = "/srv/my app"
title = 'hello world'
```

If the same key appears multiple times, the later value replaces the earlier one. That is usually the least surprising behavior for layered config files.

## Why It Uses `Map(Str, Str)`

This is intentional.

The parser is not trying to become a schema system. It gives you a small, predictable configuration representation and lets the application decide what keys mean.

That keeps the parser useful across very different programs:

- small command-line tools
- daemons and services
- game/server config
- parser frontends and analysis tools

It also means config access composes naturally with the rest of the container APIs.

## When To Use This Instead Of JSON

Use `KvConfig` when:

- the file is mostly flat
- you want hand-editable config
- comments matter
- startup configuration is more important than nested data modeling

Use JSON when:

- nested data is essential
- arrays and structured objects are part of the format
- the file is meant for interchange, not just local configuration

So the practical split is simple:

- `KvConfig` for human-edited runtime config
- `JSON` for structured data interchange

## Typed Access Is Deliberately Small

The current typed getters are:

- `KvConfigGetBool(...)`
- `KvConfigGetI64(...)`
- `KvConfigGetF64(...)`

Everything else can still be read as a plain `Str` and interpreted by the application.

That is the right tradeoff for now. A config parser should make common cases easy without pretending it can infer a whole application schema from text.
