#include <Misra.h>

#define MapItem(KTYPE, VTYPE)                                                                                          \
    struct {                                                                                                           \
        KTYPE key;                                                                                                     \
        VTYPE value;                                                                                                   \
    }

#define Map(KTYPE, VTYPE)                                                                                              \
    struct {                                                                                                           \
        GenericHash       k_hash;                                                                                      \
        GenericCompare    k_cmp;                                                                                       \
        GenericCopyInit   k_init;                                                                                      \
        GenericCopyDeinit k_deinit;                                                                                    \
        GenericCopyInit   v_init;                                                                                      \
        GenericCopyDeinit v_deinit;                                                                                    \
        MapItem(KTYPE, VTYPE) * data;                                                                                  \
        size length;                                                                                                   \
        size capacity;                                                                                                 \
        f32  load_factor;                                                                                              \
    }

#define ValidateMap(m)                                                                                                 \
    do {                                                                                                               \
        if (!(m)->k_hash || (m)->length > (m)->capacity || (m)->load_factor < 0.0001) {                                \
            abort();                                                                                                   \
        }                                                                                                              \
        if ((m)->data) {                                                                                               \
            (void)(m->data[0]);                                                                                        \
        }                                                                                                              \
    } while (0)

// Initializes a Map with all function pointers set to NULL (except hash),
// data to NULL, length and capacity to 0, and sets the hash function and load_factor.
#define MapInit(h, lf)                                                                                                 \
    {.k_hash      = (GenericHash)(void*)(h),                                                                                  \
     .k_cmp       = NULL,                                                                                              \
     .k_init      = NULL,                                                                                              \
     .k_deinit    = NULL,                                                                                              \
     .v_init      = NULL,                                                                                              \
     .v_deinit    = NULL,                                                                                              \
     .data        = NULL,                                                                                              \
     .length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .load_factor = (lf)}

// Initializes a Map, setting the hash function, key comparison function (k_cmp)
// and the load_factor. Other fields are default NULL/0.
#define MapInitWithCmp(h, lf, kmp)                                                                                     \
    {.k_hash      = (GenericHash)(void*)(h),                                                                                  \
     .k_cmp       = (GenericCompare)(void*)(kmp),                                                                             \
     .k_init      = NULL,                                                                                              \
     .k_deinit    = NULL,                                                                                              \
     .v_init      = NULL,                                                                                              \
     .v_deinit    = NULL,                                                                                              \
     .data        = NULL,                                                                                              \
     .length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .load_factor = (lf)}

// Initializes a Map, setting the hash function, deep copy/deinitialization functions
// for both keys and values, and the load_factor. k_cmp is NULL.
#define MapInitWithDeepCopy(h, lf, ki, kd, vi, vd)                                                                     \
    {.k_hash      = (GenericHash)(void*)(h),                                                                                  \
     .k_cmp       = NULL,                                                                                              \
     .k_init      = (GenericCopyInit)(void*)(ki),                                                                             \
     .k_deinit    = (GenericCopyDeinit)(void*)(kd),                                                                           \
     .v_init      = (GenericCopyInit)(void*)(vi),                                                                             \
     .v_deinit    = (GenericCopyDeinit)(void*)(vd),                                                                           \
     .data        = NULL,                                                                                              \
     .length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .load_factor = (lf)}

// Initializes a Map, setting the hash function, key comparison function,
// deep copy/deinitialization functions for both keys and values,
// and the load_factor. This is the most comprehensive initializer.
#define MapInitWithCmpDeepCopy(h, lf, kmp, ki, kd, vi, vd)                                                             \
    {.k_hash      = (GenericHash)(void*)(h),                                                                                  \
     .k_cmp       = (GenericCompare)(void*)(kmp),                                                                             \
     .k_init      = (GenericCopyInit)(void*)(ki),                                                                             \
     .k_deinit    = (GenericCopyDeinit)(void*)(kd),                                                                           \
     .v_init      = (GenericCopyInit)(void*)(vi),                                                                             \
     .v_deinit    = (GenericCopyDeinit)(void*)(vd),                                                                           \
     .data        = NULL,                                                                                              \
     .length      = 0,                                                                                                 \
     .capacity    = 0,                                                                                                 \
     .load_factor = (lf)}

u64 StrHash(Str* s, u64 size) {
    (void)s;
    (void)size;
    return 1;
}

int main(void) {
    Map(Str, Str) map = MapInit(StrHash, 0.5);
    (void)map;

    Str file = StrInit();
    if (ReadCompleteFile("Bin/Demangler/CppNameManglingGrammar", &file.data, &file.length, &file.capacity)) {
        Strs lines = StrSplit(&file, "\n");
        VecForeachPtr(&lines, line, {
            if (StrStartsWithZstr(line, "[.") && StrEndsWithZstr(line, "]")) {
                Str rule_name = StrInit();
                StrReadFmt(line->data, "[.{}]", FMT(rule_name));
                if (rule_name.length) {
                    WriteFmtLn("Got Rule : {}", FMT(rule_name));
                    StrDeinit(&rule_name);
                }
            }
        });

        VecDeinit(&lines);
        VecDeinit(&file);
    } else {
        LOG_ERROR("Failed to read file");
    }
    return 0;
}
