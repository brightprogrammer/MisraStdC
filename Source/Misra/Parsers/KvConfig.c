#include <Misra/Parsers/KvConfig.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Container/Map/Private.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>


static bool kvconfig_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

static bool kvconfig_is_comment_start(char c) {
    return c == '#' || c == ';';
}

static bool kvconfig_parse_bool_value(const Str *value, bool *out) {
    if (!out) {
        LOG_FATAL("Expected valid bool output pointer");
    }

    ValidateStr(value);

    if (StrCmpZstrIgnoreCase(value, "true") == 0 || StrCmpZstrIgnoreCase(value, "yes") == 0 ||
        StrCmpZstrIgnoreCase(value, "on") == 0 || StrCmpZstrIgnoreCase(value, "1") == 0) {
        *out = true;
        return true;
    }

    if (StrCmpZstrIgnoreCase(value, "false") == 0 || StrCmpZstrIgnoreCase(value, "no") == 0 ||
        StrCmpZstrIgnoreCase(value, "off") == 0 || StrCmpZstrIgnoreCase(value, "0") == 0) {
        *out = false;
        return true;
    }

    return false;
}

static bool kvconfig_parse_i64_value(const Str *value, i64 *out) {
    Zstr      endptr = NULL;
    long long parsed;

    if (!out) {
        LOG_FATAL("Expected valid integer output pointer");
    }

    parsed = ZstrToI64(StrBegin(value), &endptr);

    if (!endptr || endptr == StrBegin(value) || *endptr != '\0') {
        return false;
    }

    *out = (i64)parsed;
    return true;
}

static bool kvconfig_parse_f64_value(const Str *value, f64 *out) {
    Zstr   endptr = NULL;
    double parsed;

    if (!out) {
        LOG_FATAL("Expected valid float output pointer");
    }

    parsed = ZstrToF64(StrBegin(value), &endptr);

    if (!endptr || endptr == StrBegin(value) || *endptr != '\0') {
        return false;
    }

    *out = parsed;
    return true;
}

static StrIter kvconfig_consume_line_end(StrIter si) {
    char c;
    if (StrIterPeek(&si, &c) && c == '\r') {
        StrIterMustNext(&si);
    }
    if (StrIterPeek(&si, &c) && c == '\n') {
        StrIterMustNext(&si);
    }
    return si;
}

u64 KvConfigHash(const void *data, u32 ignored_size) {
    const Str *str  = data;
    u64        hash = 1469598103934665603ULL;
    size       idx;

    (void)ignored_size;
    ValidateStr(str);

    for (idx = 0; idx < StrLen(str); idx++) {
        hash ^= (u64)(unsigned char)StrCharAt(str, idx);
        hash *= 1099511628211ULL;
    }

    return hash;
}

i32 KvConfigCompare(const void *lhs, const void *rhs) {
    const Str *a   = lhs;
    const Str *b   = rhs;
    size       min = 0;
    i32        cmp = 0;

    ValidateStr(a);
    ValidateStr(b);

    min = StrLen(a) < StrLen(b) ? StrLen(a) : StrLen(b);
    cmp = MemCompare(StrBegin(a), StrBegin(b), min);

    if (cmp != 0) {
        return cmp;
    }

    if (StrLen(a) == StrLen(b)) {
        return 0;
    }

    return StrLen(a) < StrLen(b) ? -1 : 1;
}

StrIter KvConfigSkipWhitespace(StrIter si) {
    char c;
    while (StrIterPeek(&si, &c) && kvconfig_is_space(c)) {
        StrIterMustNext(&si);
    }
    return si;
}

StrIter KvConfigSkipLine(StrIter si) {
    char c;
    while (StrIterPeek(&si, &c) && c != '\n') {
        StrIterMustNext(&si);
    }
    return kvconfig_consume_line_end(si);
}

StrIter KvConfigReadKey(StrIter si, Str *key) {
    StrIter saved_si = si;

    if (!key) {
        LOG_FATAL("Expected valid key output string");
    }

    si = KvConfigSkipWhitespace(si);

    char c;
    while (StrIterPeek(&si, &c)) {
        if (c == '=' || c == ':' || kvconfig_is_space(c) || c == '\n' || kvconfig_is_comment_start(c)) {
            break;
        }

        StrPushBack(key, c);
        StrIterMustNext(&si);
    }

    if (StrLen(key) == 0) {
        LOG_ERROR("Expected config key");
        StrClear(key);
        return saved_si;
    }

    return si;
}

StrIter KvConfigReadValue(StrIter si, Str *value) {
    StrIter saved_si = si;

    if (!value) {
        LOG_FATAL("Expected valid value output string");
    }

    si = KvConfigSkipWhitespace(si);

    char c;
    if (!StrIterPeek(&si, &c) || c == '\n' || kvconfig_is_comment_start(c)) {
        return si;
    }

    char quote = c;
    if (quote == '"' || quote == '\'') {
        StrIterMustNext(&si);

        while (StrIterPeek(&si, &c)) {
            if (c == quote) {
                StrIterMustNext(&si);
                return si;
            }

            if (c == '\\') {
                StrIterMustNext(&si);
                if (!StrIterPeek(&si, &c)) {
                    LOG_ERROR("Unexpected end of quoted config value");
                    StrClear(value);
                    return saved_si;
                }

                switch (c) {
                    case 'n' :
                        StrPushBack(value, '\n');
                        break;
                    case 'r' :
                        StrPushBack(value, '\r');
                        break;
                    case 't' :
                        StrPushBack(value, '\t');
                        break;
                    default :
                        // Pass any other escaped character through verbatim.
                        StrPushBack(value, c);
                        break;
                }

                StrIterMustNext(&si);
                continue;
            }

            StrPushBack(value, c);
            StrIterMustNext(&si);
        }

        LOG_ERROR("Missing closing quote in config value");
        StrClear(value);
        return saved_si;
    }

    while (StrIterPeek(&si, &c) && c != '\n') {
        if (kvconfig_is_comment_start(c) && StrLen(value) > 0 &&
            kvconfig_is_space(StrCharAt(value, StrLen(value) - 1))) {
            while (StrLen(value) > 0 && kvconfig_is_space(StrCharAt(value, StrLen(value) - 1))) {
                char dropped = '\0';
                StrPopBack(value, &dropped);
            }
            return si;
        }

        if (kvconfig_is_comment_start(c) && StrLen(value) == 0) {
            return si;
        }

        StrPushBack(value, c);
        StrIterMustNext(&si);
    }

    if (StrLen(value) > 0) {
        Str stripped = StrStrip(value, NULL);
        StrDeinit(value);
        *value = stripped;
    }

    return si;
}

StrIter KvConfigReadPair(StrIter si, Str *key, Str *value) {
    StrIter saved_si = si;

    if (!key || !value) {
        LOG_FATAL("Expected valid key/value outputs");
    }

    si = KvConfigReadKey(si, key);
    if (si.pos == saved_si.pos) {
        return saved_si;
    }

    si = KvConfigSkipWhitespace(si);

    char c;
    if (!StrIterPeek(&si, &c) || (c != '=' && c != ':')) {
        LOG_ERROR("Expected '=' or ':' after config key");
        StrClear(key);
        StrClear(value);
        return saved_si;
    }

    StrIterMustNext(&si);
    si = KvConfigReadValue(si, value);

    if (si.pos == saved_si.pos) {
        StrClear(key);
        StrClear(value);
        return saved_si;
    }

    si = KvConfigSkipWhitespace(si);
    if (!StrIterPeek(&si, &c)) {
        // EOF after value is fine - last line without newline.
        return si;
    }
    if (kvconfig_is_comment_start(c)) {
        si = KvConfigSkipLine(si);
    } else if (c != '\n') {
        LOG_ERROR("Unexpected trailing characters after config value");
        StrClear(key);
        StrClear(value);
        return saved_si;
    } else {
        si = kvconfig_consume_line_end(si);
    }

    return si;
}

StrIter KvConfigParse(StrIter si, KvConfig *cfg) {
    StrIter saved_si = si;

    if (!cfg) {
        LOG_FATAL("Expected valid KvConfig object");
    }

    ValidateMap(cfg);

    char c;
    while (StrIterPeek(&si, &c)) {
        Str     key   = StrInit(cfg->allocator);
        Str     value = StrInit(cfg->allocator);
        StrIter read_si;

        while (StrIterPeek(&si, &c)) {
            if (c == '\n') {
                StrIterMustNext(&si);
                continue;
            }

            if (kvconfig_is_space(c)) {
                si = KvConfigSkipWhitespace(si);
                char c2;
                if (!StrIterPeek(&si, &c2) || c2 == '\n') {
                    si = kvconfig_consume_line_end(si);
                    continue;
                }
            }

            break;
        }

        if (!StrIterPeek(&si, &c)) {
            StrDeinit(&key);
            StrDeinit(&value);
            break;
        }

        if (kvconfig_is_comment_start(c)) {
            si = KvConfigSkipLine(si);
            StrDeinit(&key);
            StrDeinit(&value);
            continue;
        }

        read_si = KvConfigReadPair(si, &key, &value);
        if (read_si.pos == si.pos) {
            StrDeinit(&key);
            StrDeinit(&value);
            return saved_si;
        }

        MapSetOnlyL(cfg, key, value);
        StrDeinit(&key);
        StrDeinit(&value);
        si = read_si;
    }

    return si;
}

Str *kvconfig_get_ptr_str(KvConfig *cfg, const Str *key) {
    if (!cfg || !key) {
        return NULL;
    }

    return map_get_value_ptr(
        GENERIC_MAP(cfg),
        key,
        sizeof(MAP_ENTRY_TYPE(cfg)),
        offsetof(MAP_ENTRY_TYPE(cfg), key),
        sizeof(MAP_KEY_TYPE(cfg)),
        offsetof(MAP_ENTRY_TYPE(cfg), value),
        offsetof(MAP_ENTRY_TYPE(cfg), hash)
    );
}

Str *kvconfig_get_ptr_zstr(KvConfig *cfg, Zstr key) {
    Str  lookup = {0};
    Str *value  = NULL;

    if (!cfg || !key) {
        return NULL;
    }

    lookup = StrInitFromCstr(key, ZstrLen(key), cfg->allocator);
    value  = kvconfig_get_ptr_str(cfg, &lookup);
    StrDeinit(&lookup);
    return value;
}

Str kvconfig_get_str(KvConfig *cfg, const Str *key) {
    Str *value = kvconfig_get_ptr_str(cfg, key);

    if (!value) {
        return StrInit(cfg->allocator);
    }

    return StrInitFromCstr(StrBegin(value), StrLen(value), cfg->allocator);
}

Str kvconfig_get_zstr(KvConfig *cfg, Zstr key) {
    Str *value = kvconfig_get_ptr_zstr(cfg, key);

    if (!value) {
        return StrInit(cfg->allocator);
    }

    return StrInitFromCstr(StrBegin(value), StrLen(value), cfg->allocator);
}

bool kvconfig_contains_str(KvConfig *cfg, const Str *key) {
    return kvconfig_get_ptr_str(cfg, key) != NULL;
}

bool kvconfig_contains_zstr(KvConfig *cfg, Zstr key) {
    return kvconfig_get_ptr_zstr(cfg, key) != NULL;
}

bool kvconfig_get_bool_str(KvConfig *cfg, const Str *key, bool *value) {
    Str *str = kvconfig_get_ptr_str(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_bool_value(str, value);
}

bool kvconfig_get_bool_zstr(KvConfig *cfg, Zstr key, bool *value) {
    Str *str = kvconfig_get_ptr_zstr(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_bool_value(str, value);
}

bool kvconfig_get_i64_str(KvConfig *cfg, const Str *key, i64 *value) {
    Str *str = kvconfig_get_ptr_str(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_i64_value(str, value);
}

bool kvconfig_get_i64_zstr(KvConfig *cfg, Zstr key, i64 *value) {
    Str *str = kvconfig_get_ptr_zstr(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_i64_value(str, value);
}

bool kvconfig_get_f64_str(KvConfig *cfg, const Str *key, f64 *value) {
    Str *str = kvconfig_get_ptr_str(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_f64_value(str, value);
}

bool kvconfig_get_f64_zstr(KvConfig *cfg, Zstr key, f64 *value) {
    Str *str = kvconfig_get_ptr_zstr(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_f64_value(str, value);
}
