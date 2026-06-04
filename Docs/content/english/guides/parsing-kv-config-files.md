---
title: "Parsing Key-Value Config Files"
date: 2026-04-19
description: "Read simple `key = value` config files with KvConfig."
authors:
  - siddharth-mishra
tags:
  - parser
  - config
  - map
  - strings
---

> **Note**: This post was drafted by an AI assistant under direction from the author. It is not first-hand writing; the design choices it describes are real, the prose explaining them is generated. Treat the technical content as the design talking, and the framing as a translation layer.

`KvConfig` parses flat configuration files like:

```text
host = localhost
port = 8080
debug = true
```

It supports `=` or `:` as separators, `#` and `;` comments, blank lines, and single- or double-quoted values for strings with embedded whitespace. The result is a `Map(Str, Str)` you read by key.

## Example

```c
#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>

int main(void) {
    Scope(alloc, DefaultAllocator) {
        Str text = StrInitFromZstr(
            "host = localhost\n"
            "port = 8080\n"
            "debug = true\n"
        );

        KvConfig cfg = KvConfigInit();
        StrIter  si  = StrIterFromStr(text);
        si = KvConfigParse(si, &cfg);

        Str   host  = KvConfigGet(&cfg, "host");
        i64   port  = 0;
        bool  debug = false;
        KvConfigGetI64(&cfg, "port", &port);
        KvConfigGetBool(&cfg, "debug", &debug);

        WriteFmtLn("host = {}", host);
        WriteFmtLn("port = {}", port);
        WriteFmtLn("debug = {}", debug);

        StrDeinit(&host);
        KvConfigDeinit(&cfg);
        StrDeinit(&text);
    }
}
```

## Reading values

| Function | What it returns |
| --- | --- |
| `KvConfigGet(&cfg, "key")` | A fresh `Str` copy. Caller owns it and must `StrDeinit`. |
| `KvConfigGetPtr(&cfg, "key")` | An internal reference. No allocation, no ownership; valid until the config is mutated or deinit'd. |
| `KvConfigGetI64(&cfg, "key", &out)` | `false` if the key is missing or the value does not parse as `i64`. |
| `KvConfigGetF64(&cfg, "key", &out)` | Same shape for `f64`. |
| `KvConfigGetBool(&cfg, "key", &out)` | Accepts `true` / `false`, `1` / `0`, `yes` / `no`. |

If a key appears more than once in the file, the later line wins.

## When to use it

Pick `KvConfig` for hand-edited, mostly flat config: short daemon settings, CLI defaults, small game configs. If you need nested objects, arrays, or strict schemas, use the `JSON` parser instead.
