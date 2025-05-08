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
            break;                                                                                                     \
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
        bool expect_comma = false;                                                                                     \
        bool failed       = false;                                                                                     \
                                                                                                                       \
        /* while not at the end of array. */                                                                           \
        while (StrIterPeek(&si) && StrIterPeek(&si) != ']') {                                                          \
            if (expect_comma) {                                                                                        \
                if (StrIterPeek(&si) != ',') {                                                                         \
                    LOG_ERROR("Expected ',' between values in array. Invalid JSON array.");                            \
                    failed = true;                                                                                     \
                    si     = saved_si;                                                                                 \
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
                    failed = true;                                                                                     \
                    si     = saved_si;                                                                                 \
                    break;                                                                                             \
                }                                                                                                      \
                si = read_si;                                                                                          \
            }                                                                                                          \
            si = JSkipWhitespace(si);                                                                                  \
                                                                                                                       \
            /* expect a comma after a successful value read in array */                                                \
            expect_comma = true;                                                                                       \
        }                                                                                                              \
                                                                                                                       \
        /* end of array */                                                                                             \
        if (!failed) {                                                                                                 \
            if (StrIterPeek(&si) != ']') {                                                                             \
                LOG_ERROR("Invalid end of array. Expected ']'.");                                                      \
                failed = true;                                                                                         \
                si     = saved_si;                                                                                     \
                break;                                                                                                 \
            }                                                                                                          \
                                                                                                                       \
            StrIterNext(&si);                                                                                          \
        }                                                                                                              \
    } while (0)


#define JR_OBJ(si, reader)                                                                                             \
    do {                                                                                                               \
        if (!StrIterRemainingLength(&si)) {                                                                            \
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
                    LOG_ERROR("Expected ',' between key/value pairs in object. Invalid JSON object.");                 \
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

#define JR_OBJ_KV(si, k, reader)                                                                                       \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            JR_OBJ(si, reader);                                                                                        \
        }                                                                                                              \
    } while (0)

#define JR_ARR_KV(si, k, reader)                                                                                       \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            JR_ARR(si, reader);                                                                                        \
        }                                                                                                              \
    } while (0)

typedef Vec(Str) Strs;

int main(int argc, char** argv) {
    LogInit(false);

    Str json = StrInitFromZstr(
        "{   \"name\"  :    \"misra\", \"data\":{\"x_axis_val\":-22.24485,\"gname\":\"a random "
        "graph\",\"y_axis_val\":133.455234} ,\"ref\":40, \"strs\":[\"x\", \"ah _ ha\", \"lessa do something\"]}"
    );
    StrIter si = StrIterFromStr(&json);

    struct {
        struct {
            float x;
            float y;
            Str   n;
        } data;
        Str  name;
        int  ref;
        Strs strs;
    } obj = {0};

    obj.strs = (Strs)VecInit();

    JR_OBJ(si, {
        JR_INT_KV(si, "ref", obj.ref);
        JR_OBJ_KV(si, "data", {
            JR_FLT_KV(si, "y_axis_val", obj.data.y);
            JR_FLT_KV(si, "x_axis_val", obj.data.x);
            JR_STR_KV(si, "gname", obj.data.n);
        });
        JR_STR_KV(si, "name", obj.name);
        JR_ARR_KV(si, "strs", {
            Str tmp_s;
            JR_STR(si, tmp_s);
            VecPushBack(&obj.strs, tmp_s);
        });
    });

    printf("Name : %s\n", obj.name.data);
    printf("Ref : %d\n", obj.ref);
    printf("X : %f\n", obj.data.x);
    printf("X : %f\n", obj.data.y);
    printf("N : %s\n", obj.data.n.data);
    printf("strs : [");
    VecForeach(&obj.strs, str, { printf("%s, ", str.data); });
    printf("]\n");

    LogDeinit();
    return 0;
}
