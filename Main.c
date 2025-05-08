#include <Misra/Parsers/JSON.h>
#include <Misra/Std.h>
#include <Misra/Types.h>

#define JR_STR(si, str)                                                                                                \
    do {                                                                                                               \
        Str my_str = StrInit();                                                                                        \
        si         = JReadString((si), &my_str);                                                                       \
        (str)      = my_str;                                                                                           \
    } while (0)

#define JR_STR_KV(si, k, str)                                                                                          \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            Str my_str = StrInit();                                                                                    \
            si         = JReadString((si), &my_str);                                                                   \
            (str)      = my_str;                                                                                       \
        }                                                                                                              \
    } while (0)


#define JR_INT(si, i)                                                                                                  \
    do {                                                                                                               \
        i64 my_int = 0;                                                                                                \
        si         = JReadInteger((si), &my_int);                                                                      \
        (i)        = my_int;                                                                                           \
    } while (0)

#define JR_INT_KV(si, k, i)                                                                                            \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            i64 my_int = 0;                                                                                            \
            si         = JReadInteger((si), &my_int);                                                                  \
            (i)        = my_int;                                                                                       \
        }                                                                                                              \
    } while (0)


#define JR_FLT(si, f)                                                                                                  \
    do {                                                                                                               \
        f64 my_flt = 0;                                                                                                \
        si         = JReadFloat((si), &my_flt);                                                                        \
        (f)        = my_flt;                                                                                           \
    } while (0)

#define JR_FLT_KV(si, k, f)                                                                                            \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            f64 my_flt = 0;                                                                                            \
            si         = JReadFloat((si), &my_flt);                                                                    \
            (f)        = my_flt;                                                                                       \
        }                                                                                                              \
    } while (0)


#define JR_ARR(si, reader)                                                                                             \
    do {                                                                                                               \
        if (!StrIterRemainingLength(&si)) {                                                                            \
            LOG_ERROR("String iterator exhausted range. Nothing more left to read.");                                  \
            return si;                                                                                                 \
        }                                                                                                              \
                                                                                                                       \
        StrIter saved_si = si;                                                                                         \
        si               = JSkipWhitespace(si);                                                                        \
                                                                                                                       \
        /* starting of an object */                                                                                    \
        if (StrIterPeek(&si) != '[') {                                                                                 \
            LOG_ERROR("Invalid array start. Expected '['.");                                                           \
            si = saved_si;                                                                                             \
            break;                                                                                                     \
        }                                                                                                              \
        StrIterNext(&si);                                                                                              \
        si = JSkipWhitespace(si);                                                                                      \
                                                                                                                       \
        StrIter read_si;                                                                                               \
        bool    expect_comma = false;                                                                                  \
                                                                                                                       \
        /* while not at the end of array. */                                                                           \
        while (StrIterPeek(&si) && StrIterPeek(&si) != ']') {                                                          \
            if (expect_comma) {                                                                                        \
                if (StrIterPeek(&si) != ',') {                                                                         \
                    LOG_ERROR(                                                                                         \
                        "Expected ',' between values in array. Invalid JSON "                                          \
                        "array."                                                                                       \
                    );                                                                                                 \
                    si = saved_si;                                                                                     \
                    break;                                                                                             \
                }                                                                                                      \
                StrIterNext(&si); /* skip comma */                                                                     \
                si = JSkipWhitespace(si);                                                                              \
            }                                                                                                          \
                                                                                                                       \
            /* try reading using user provided reader */                                                               \
            StrIter si_before_read = si;                                                                               \
            { reader }                                                                                                 \
                                                                                                                       \
            /* if no advancement in read position */                                                                   \
            if (si_before_read.pos == si.pos) {                                                                        \
                /* skip the value */                                                                                   \
                StrIter read_si = JSkipValue(si);                                                                      \
                                                                                                                       \
                /* if still no advancement in read position */                                                         \
                if (read_si.pos == si.pos) {                                                                           \
                    LOG_ERROR("Failed to parse value. Invalid JSON.");                                                 \
                    StrDeinit(&key);                                                                                   \
                    si = saved_si;                                                                                     \
                    break;                                                                                             \
                }                                                                                                      \
            }                                                                                                          \
                                                                                                                       \
            si = read_si;                                                                                              \
            si = JSkipWhitespace(si);                                                                                  \
                                                                                                                       \
            /* expect a comma after a successful value read in array */                                                \
            expect_comma = true;                                                                                       \
        }                                                                                                              \
                                                                                                                       \
        /* end of array */                                                                                             \
        if (StrIterPeek(&si) != ']') {                                                                                 \
            LOG_ERROR("Invalid end of array. Expected ']'.");                                                          \
            si = saved_si;                                                                                             \
            break;                                                                                                     \
        }                                                                                                              \
                                                                                                                       \
        StrIterNext(&si);                                                                                              \
        return si;                                                                                                     \
    } while (0)


#define JR_OBJ(si, reader)                                                                                             \
    do {                                                                                                               \
        if (!StrIterRemainingLength(&si)) {                                                                            \
            LOG_ERROR("String iterator exhausted range. Nothing more left to read.");                                  \
            break;                                                                                                     \
        }                                                                                                              \
                                                                                                                       \
        StrIter saved_si = si;                                                                                         \
        si               = JSkipWhitespace(si);                                                                        \
                                                                                                                       \
        /* starting of an object */                                                                                    \
        if (StrIterPeek(&si) != '{') {                                                                                 \
            LOG_ERROR("Invalid object start. Expected '{'.");                                                          \
            si = saved_si;                                                                                             \
            break;                                                                                                     \
        }                                                                                                              \
        StrIterNext(&si);                                                                                              \
        si = JSkipWhitespace(si);                                                                                      \
                                                                                                                       \
        StrIter read_si;                                                                                               \
        bool    expect_comma = false;                                                                                  \
        bool    failed       = false;                                                                                  \
                                                                                                                       \
        /* while not at the end of object. */                                                                          \
        while (StrIterPeek(&si) && StrIterPeek(&si) != '}') {                                                          \
            if (expect_comma) {                                                                                        \
                if (StrIterPeek(&si) != ',') {                                                                         \
                    LOG_ERROR(                                                                                         \
                        "Expected ',' between key/value pairs in object. "                                             \
                        "Invalid JSON object."                                                                         \
                    );                                                                                                 \
                    failed = true;                                                                                     \
                    si     = saved_si;                                                                                 \
                    break;                                                                                             \
                }                                                                                                      \
                StrIterNext(&si); /* skip comma */                                                                     \
                si = JSkipWhitespace(si);                                                                              \
            }                                                                                                          \
                                                                                                                       \
                                                                                                                       \
            Str key = StrInit();                                                                                       \
                                                                                                                       \
            /* key start */                                                                                            \
            read_si = JReadString(si, &key);                                                                           \
            if (read_si.pos == si.pos) {                                                                               \
                LOG_ERROR("Failed to read string key in object. Invalid JSON");                                        \
                StrDeinit(&key);                                                                                       \
                failed = true;                                                                                         \
                si     = saved_si;                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
                                                                                                                       \
            si = read_si;                                                                                              \
            si = JSkipWhitespace(si);                                                                                  \
                                                                                                                       \
                                                                                                                       \
            if (StrIterPeek(&si) != ':') {                                                                             \
                LOG_ERROR("Expected ':' after key string. Failed to read JSON");                                       \
                StrDeinit(&key);                                                                                       \
                failed = true;                                                                                         \
                si     = saved_si;                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
            StrIterNext(&si);                                                                                          \
            si = JSkipWhitespace(si);                                                                                  \
                                                                                                                       \
                                                                                                                       \
            /* try reading using user provided reader */                                                               \
            StrIter si_before_read = si;                                                                               \
            { reader }                                                                                                 \
                                                                                                                       \
            /* if no advancement in read position */                                                                   \
            if (si_before_read.pos == si.pos) {                                                                        \
                /* skip the value */                                                                                   \
                StrIter read_si = JSkipValue(si);                                                                      \
                                                                                                                       \
                                                                                                                       \
                /* if still no advancement in read position */                                                         \
                if (read_si.pos == si.pos) {                                                                           \
                    LOG_ERROR("Failed to parse value. Invalid JSON.");                                                 \
                    StrDeinit(&key);                                                                                   \
                    failed = true;                                                                                     \
                    si     = saved_si;                                                                                 \
                    break;                                                                                             \
                }                                                                                                      \
                                                                                                                       \
                LOG_INFO("User skipped reading of '%s' field in JSON object.", key.data);                              \
                si = read_si;                                                                                          \
            }                                                                                                          \
                                                                                                                       \
            StrDeinit(&key);                                                                                           \
            si = JSkipWhitespace(si);                                                                                  \
                                                                                                                       \
                                                                                                                       \
            /* expect a comma after a successful key-value pair read */                                                \
            expect_comma = true;                                                                                       \
        }                                                                                                              \
                                                                                                                       \
        if (!failed) {                                                                                                 \
            if (StrIterPeek(&si) != '}') {                                                                             \
                LOG_ERROR("Expected end of object '}' but found '%c'", StrIterPeek(&si));                              \
                failed = true;                                                                                         \
                si     = saved_si;                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
                                                                                                                       \
            StrIterNext(&si);                                                                                          \
        }                                                                                                              \
    } while (0)

int main(int argc, char** argv) {
    LogInit(false);

    Str     json = StrInitFromZstr("{   \"name\"  :    \"misra\",\"ref\":40}");
    StrIter si   = StrIterFromStr(&json);

    struct {
        Str name;
        int ref;
    } obj = {0};

    JR_OBJ(si, {
        JR_STR_KV(si, "name", obj.name);
        JR_INT_KV(si, "ref", obj.ref);
    });

    printf("Name : %s\n", obj.name.data);
    printf("Ref : %d\n", obj.ref);

    LogDeinit();
    return 0;
}
