#include <Misra/Parsers/KvConfig.h>
#include <Misra/Std/Container/Map/Private.h>
#include <Misra/Std/Memory.h>
#include <Misra/Std/Log.h>

#include <errno.h>
#include <stdlib.h>

static bool kvconfig_is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

static bool kvconfig_is_line_end(char c) {
    return c == '\n' || c == '\0';
}

static bool kvconfig_is_comment_start(char c) {
    return c == '#' || c == ';';
}

static char kvconfig_ascii_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static bool kvconfig_equals_ignore_case(const Str *value, const char *zstr) {
    size idx = 0;

    ValidateStr(value);

    while (idx < value->length && zstr[idx]) {
        if (kvconfig_ascii_lower(value->data[idx]) != kvconfig_ascii_lower(zstr[idx])) {
            return false;
        }
        idx += 1;
    }

    return idx == value->length && zstr[idx] == '\0';
}

static bool kvconfig_parse_bool_value(const Str *value, bool *out) {
    if (!out) {
        LOG_ERROR("Expected valid bool output pointer");
        return false;
    }

    if (kvconfig_equals_ignore_case(value, "true") || kvconfig_equals_ignore_case(value, "yes") ||
        kvconfig_equals_ignore_case(value, "on") || kvconfig_equals_ignore_case(value, "1")) {
        *out = true;
        return true;
    }

    if (kvconfig_equals_ignore_case(value, "false") || kvconfig_equals_ignore_case(value, "no") ||
        kvconfig_equals_ignore_case(value, "off") || kvconfig_equals_ignore_case(value, "0")) {
        *out = false;
        return true;
    }

    return false;
}

static bool kvconfig_parse_i64_value(const Str *value, i64 *out) {
    char     *endptr = NULL;
    long long parsed;

    if (!out) {
        LOG_ERROR("Expected valid integer output pointer");
        return false;
    }

    errno  = 0;
    parsed = strtoll(value->data, &endptr, 10);

    if (errno != 0 || !endptr || *endptr != '\0') {
        return false;
    }

    *out = (i64)parsed;
    return true;
}

static bool kvconfig_parse_f64_value(const Str *value, f64 *out) {
    char  *endptr = NULL;
    double parsed;

    if (!out) {
        LOG_ERROR("Expected valid float output pointer");
        return false;
    }

    errno  = 0;
    parsed = strtod(value->data, &endptr);

    if (errno != 0 || !endptr || *endptr != '\0') {
        return false;
    }

    *out = parsed;
    return true;
}

static StrIter kvconfig_consume_line_end(StrIter si) {
    if (StrIterPeek(&si) == '\r') {
        StrIterNext(&si);
    }

    if (StrIterPeek(&si) == '\n') {
        StrIterNext(&si);
    }

    return si;
}

u64 KvConfigHash(const void *data, u32 ignored_size) {
    const Str *str  = data;
    u64        hash = 1469598103934665603ULL;
    size       idx;

    (void)ignored_size;
    ValidateStr(str);

    for (idx = 0; idx < str->length; idx++) {
        hash ^= (u64)(unsigned char)str->data[idx];
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

    min = a->length < b->length ? a->length : b->length;
    cmp = MemCompare(a->data, b->data, min);

    if (cmp != 0) {
        return cmp;
    }

    if (a->length == b->length) {
        return 0;
    }

    return a->length < b->length ? -1 : 1;
}

StrIter KvConfigSkipWhitespace(StrIter si) {
    while (StrIterRemainingLength(&si) && kvconfig_is_space(StrIterPeek(&si))) {
        StrIterNext(&si);
    }

    return si;
}

StrIter KvConfigSkipLine(StrIter si) {
    while (StrIterRemainingLength(&si) && !kvconfig_is_line_end(StrIterPeek(&si))) {
        StrIterNext(&si);
    }

    return kvconfig_consume_line_end(si);
}

StrIter KvConfigReadKey(StrIter si, Str *key) {
    StrIter saved_si = si;

    if (!key) {
        LOG_ERROR("Expected valid key output string");
        return si;
    }

    si = KvConfigSkipWhitespace(si);

    while (StrIterRemainingLength(&si)) {
        char c = StrIterPeek(&si);

        if (c == '=' || c == ':' || kvconfig_is_space(c) || kvconfig_is_line_end(c) || kvconfig_is_comment_start(c)) {
            break;
        }

        StrPushBack(key, c);
        StrIterNext(&si);
    }

    if (key->length == 0) {
        LOG_ERROR("Expected config key");
        StrClear(key);
        return saved_si;
    }

    return si;
}

StrIter KvConfigReadValue(StrIter si, Str *value) {
    StrIter saved_si = si;
    char    quote    = '\0';

    if (!value) {
        LOG_ERROR("Expected valid value output string");
        return si;
    }

    si = KvConfigSkipWhitespace(si);

    if (!StrIterRemainingLength(&si) || kvconfig_is_line_end(StrIterPeek(&si)) ||
        kvconfig_is_comment_start(StrIterPeek(&si))) {
        return si;
    }

    quote = StrIterPeek(&si);
    if (quote == '"' || quote == '\'') {
        StrIterNext(&si);

        while (StrIterRemainingLength(&si)) {
            char c = StrIterPeek(&si);

            if (c == quote) {
                StrIterNext(&si);
                return si;
            }

            if (c == '\\') {
                StrIterNext(&si);
                if (!StrIterRemainingLength(&si)) {
                    LOG_ERROR("Unexpected end of quoted config value");
                    StrClear(value);
                    return saved_si;
                }

                c = StrIterPeek(&si);
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
                    case '\\' :
                    case '"' :
                    case '\'' :
                        StrPushBack(value, c);
                        break;
                    default :
                        StrPushBack(value, c);
                        break;
                }

                StrIterNext(&si);
                continue;
            }

            StrPushBack(value, c);
            StrIterNext(&si);
        }

        LOG_ERROR("Missing closing quote in config value");
        StrClear(value);
        return saved_si;
    }

    while (StrIterRemainingLength(&si) && !kvconfig_is_line_end(StrIterPeek(&si))) {
        char c = StrIterPeek(&si);

        if (kvconfig_is_comment_start(c) && value->length > 0 && kvconfig_is_space(value->data[value->length - 1])) {
            while (value->length > 0 && kvconfig_is_space(value->data[value->length - 1])) {
                char dropped = '\0';
                StrPopBack(value, &dropped);
            }
            return si;
        }

        if (kvconfig_is_comment_start(c) && value->length == 0) {
            return si;
        }

        StrPushBack(value, c);
        StrIterNext(&si);
    }

    if (value->length > 0) {
        Str stripped = StrStrip(value, NULL);
        StrDeinit(value);
        *value = stripped;
    }

    return si;
}

StrIter KvConfigReadPair(StrIter si, Str *key, Str *value) {
    StrIter saved_si = si;

    if (!key || !value) {
        LOG_ERROR("Expected valid key/value outputs");
        return si;
    }

    si = KvConfigReadKey(si, key);
    if (si.pos == saved_si.pos) {
        return saved_si;
    }

    si = KvConfigSkipWhitespace(si);

    if (StrIterPeek(&si) != '=' && StrIterPeek(&si) != ':') {
        LOG_ERROR("Expected '=' or ':' after config key");
        StrClear(key);
        StrClear(value);
        return saved_si;
    }

    StrIterNext(&si);
    si = KvConfigReadValue(si, value);

    if (si.pos == saved_si.pos) {
        StrClear(key);
        StrClear(value);
        return saved_si;
    }

    si = KvConfigSkipWhitespace(si);
    if (kvconfig_is_comment_start(StrIterPeek(&si))) {
        si = KvConfigSkipLine(si);
    } else if (!kvconfig_is_line_end(StrIterPeek(&si))) {
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
        LOG_ERROR("Expected valid KvConfig object");
        return si;
    }

    ValidateMap(cfg);

    while (StrIterRemainingLength(&si)) {
        Str     key   = StrInit();
        Str     value = StrInit();
        StrIter read_si;

        while (StrIterRemainingLength(&si)) {
            char c = StrIterPeek(&si);

            if (c == '\n') {
                StrIterNext(&si);
                continue;
            }

            if (kvconfig_is_space(c)) {
                si = KvConfigSkipWhitespace(si);
                if (kvconfig_is_line_end(StrIterPeek(&si))) {
                    si = kvconfig_consume_line_end(si);
                    continue;
                }
            }

            break;
        }

        if (!StrIterRemainingLength(&si)) {
            break;
        }

        if (kvconfig_is_comment_start(StrIterPeek(&si))) {
            si = KvConfigSkipLine(si);
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

Str *KvConfigGetPtr(KvConfig *cfg, const char *key) {
    Str  lookup = {0};
    Str *value  = NULL;

    if (!cfg || !key) {
        return NULL;
    }

    lookup = StrInitFromZstr(key);
    value  = map_get_value_ptr(
        GENERIC_MAP(cfg),
        &lookup,
        sizeof(MAP_ENTRY_TYPE(cfg)),
        offsetof(MAP_ENTRY_TYPE(cfg), key),
        sizeof(MAP_KEY_TYPE(cfg)),
        offsetof(MAP_ENTRY_TYPE(cfg), value),
        offsetof(MAP_ENTRY_TYPE(cfg), hash)
    );
    StrDeinit(&lookup);
    return value;
}

Str KvConfigGet(KvConfig *cfg, const char *key) {
    Str *value = KvConfigGetPtr(cfg, key);

    if (!value) {
        return StrInit();
    }

    return StrInitFromStr(value);
}

bool KvConfigContains(KvConfig *cfg, const char *key) {
    return KvConfigGetPtr(cfg, key) != NULL;
}

bool KvConfigGetBool(KvConfig *cfg, const char *key, bool *value) {
    Str *str = KvConfigGetPtr(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_bool_value(str, value);
}

bool KvConfigGetI64(KvConfig *cfg, const char *key, i64 *value) {
    Str *str = KvConfigGetPtr(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_i64_value(str, value);
}

bool KvConfigGetF64(KvConfig *cfg, const char *key, f64 *value) {
    Str *str = KvConfigGetPtr(cfg, key);

    if (!str) {
        return false;
    }

    return kvconfig_parse_f64_value(str, value);
}
