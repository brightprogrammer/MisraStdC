#ifndef MISRA_PARSERS_JSON_H
#define MISRA_PARSERS_JSON_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Types.h>

///
/// JSON
/// { .. } (object)
/// "key" : "string-value"
/// "key" : number-value
/// "key" : true/false
/// "key" : [array-value]
/// "key" : { object }
/// "key" : null
///

typedef Vec(Str) StrVec;
typedef Vec(i64) Si64Vec;
typedef Vec(i64) F64Vec;

typedef struct {
    bool is_float;
    union {
        f64 f;
        i64 i;
    };
} Number;

typedef StrIter (*JArrayItemReader)(StrIter si, void* data);
typedef StrIter (*JObjectValueReader)(StrIter si, Str* key, void* data);

///
/// Skip whitespace from current reading position.
///
/// si[in] : Reading position to start looking for whitespace
///
/// RETURN : `StrIter`
///
StrIter JSkipWhitespace(StrIter si);

///
/// JReads a string from the given string iterator and stores the result in the provided `Str` object.
///
/// This function parses a string enclosed in double quotes (`"`) and handles escape sequences such as `\\`, `\"`, `\n`, `\t`, etc.
/// The parsed string is stored in the provided `Str` object, and the function will handle both normal characters and escape sequences.
/// Note: Unicode escape sequences (e.g., `\uXXXX`) are not supported in this implementation.
///
/// Parameters:
///   si[in]    : The `StrIter` pointing to the current position in the input string, which will be parsed for a string value.
///   str[out]  : A pointer to a `Str` object that will hold the parsed string. This object will be populated with the result of the parsing.
///
/// Returns:
///   A `StrIter` that points to the next position in the string after the parsed string. If the parsing fails, it will return the original `si` iterator.
///
/// Errors:
///   - Logs an error if the reading position is invalid or exhausted.
///   - Logs an error if the `str` parameter is `NULL`.
///   - Logs an error if there is an invalid escape sequence or an unsupported Unicode sequence.
///   - Logs an error if the string cannot be parsed correctly (e.g., no closing quotation mark).
///
/// Example:
///   StrIter si = some_str_iter;
///   Str str;
///   StrInit(&str);
///   StrIter new_si = JReadString(si, &str);
///   // Use the parsed string in `str` object
///
StrIter JReadString(StrIter si, Str* str);

///
/// JReads a number from the given string iterator and stores the result in the provided `Number` object.
///
/// This function handles parsing integers and floating-point numbers, including those with exponents. It supports both positive and negative values.
/// It parses the number in a manner consistent with the JSON specification for numbers.
///
/// Parameters:
///   si[in]    : The `StrIter` pointing to the current position in the input string, which will be parsed for a number.
///   num[out]  : A pointer to a `Number` object that will hold the parsed number (either an integer or floating-point value).
///               This object will be populated with the result of the parsing, including the number's value and whether it is a floating-point number.
///
/// Returns:
///   A `StrIter` that points to the next position in the string after the parsed number. If the parsing fails, it will return the original `si` iterator.
///
/// Errors:
///   - Logs an error if the reading position is invalid or exhausted.
///   - Logs an error if the `num` parameter is `NULL`.
///   - Logs an error if there is an invalid number format (e.g., multiple decimal points, multiple exponent indicators, invalid characters).
///   - Logs an error if the number is empty after parsing (e.g., no digits found).
///   - Logs an error if the string cannot be converted to a valid number.
///
/// Example:
///   StrIter si = some_str_iter;
///   Number num;
///   StrIter new_si = JReadNumber(si, &num);
///   // Use the parsed number in `num` object
///
StrIter JReadNumber(StrIter si, Number* num);

///
/// Strictly parses an integer from the string, failing if a floating-point value is encountered.
///
/// This function will attempt to parse an integer from the current position of the string iterator. If a floating-point value is encountered, the parsing will fail, and no advancement will be made in the iterator. The parsed integer value will be stored in the `val` parameter.
///
/// Parameters:
///   si[in]   : The `StrIter` pointing to the current position in the input string where the integer should be parsed.
///   val[out] : A pointer to an integer (`i64`) where the parsed value will be stored.
///
/// Returns:
///   A `StrIter` pointing to the next position after the parsed integer, or the original iterator if parsing fails.
///
/// Errors:
///   - Logs an error if the reading position is invalid or exhausted.
///   - Logs an error if the `val` pointer is `NULL`.
///   - Logs an error if a floating-point value is encountered during parsing.
///   - Logs an error if the integer parsing fails for any other reason.
///
/// Example:
///   StrIter si = some_str_iter;
///   i64 value;
///   StrIter new_si = JReadInteger(si, &value);
///   // Use the parsed integer in `value`
///
StrIter JReadInteger(StrIter si, i64* val);

///
/// JReads a floating-point number from the string. If an integer is encountered, it will be converted to a float.
///
/// This function parses a floating-point number from the current position of the string iterator. If an integer is encountered instead, it will be converted into a floating-point value. The parsed value is stored in the `val` parameter.
///
/// Parameters:
///   si[in]   : The `StrIter` pointing to the current position in the input string where the floating-point number should be parsed.
///   val[out] : A pointer to a float (`f64`) where the parsed value will be stored.
///
/// Returns:
///   A `StrIter` pointing to the next position after the parsed floating-point number, or the original iterator if parsing fails.
///
/// Errors:
///   - Logs an error if the reading position is invalid or exhausted.
///   - Logs an error if the `val` pointer is `NULL`.
///   - Logs an error if the floating-point parsing fails for any other reason.
///
/// Example:
///   StrIter si = some_str_iter;
///   f64 value;
///   StrIter new_si = JReadFloat(si, &value);
///   // Use the parsed floating-point number in `value`
///
StrIter JReadFloat(StrIter si, f64* val);

///
/// JRead a boolean value ("true" or "false") from the string.
///
/// This function parses a boolean value from the current position of the string iterator. If the value is "true",
/// the parsed value will be `true`. If the value is "false", it will be parsed as `false`. If anything else is encountered,
/// the parsing fails, and the iterator is returned without advancement. The parsed boolean value is stored in the `b` parameter.
///
/// Parameters:
///   si[in]   : The `StrIter` pointing to the current position in the input string where the boolean value should be parsed.
///   b[out]   : A pointer to a boolean (`bool`) where the parsed value will be stored.
///
/// Returns:
///   A `StrIter` pointing to the next position after the parsed boolean value, or the original iterator if parsing fails.
///
/// Errors:
///   - Logs an error if the reading position is invalid or exhausted.
///   - Logs an error if the `b` pointer is `NULL`.
///   - Logs an error if the expected "true" or "false" value is not found.
///   - Logs an error if the string length is insufficient to parse a boolean value.
///
/// Example:
///   StrIter si = some_str_iter;
///   bool value;
///   StrIter new_si = JReadBool(si, &value);
///   // Use the parsed boolean value in `value`
///
StrIter JReadBool(StrIter si, bool* b);

///
/// JRead a null value from the string.
///
/// This function parses a "null" value from the current position of the string iterator. If the value is "null",
/// the parsed value will be set to `true` in the `is_null` parameter. If anything else is encountered, the parsing fails,
/// and the iterator is returned without advancement. The result will indicate whether a "null" value was found.
///
/// Parameters:
///   si[in]       : The `StrIter` pointing to the current position in the input string where the null value should be parsed.
///   is_null[out] : A pointer to a boolean (`bool`) that will be set to `true` if a "null" value is parsed, `false` otherwise.
///
/// Returns:
///   A `StrIter` pointing to the next position after the parsed "null" value, or the original iterator if parsing fails.
///
/// Errors:
///   - Logs an error if the reading position is invalid or exhausted.
///   - Logs an error if the `is_null` pointer is `NULL`.
///   - Logs an error if the expected "null" value is not found.
///   - Logs an error if the string length is insufficient to parse a "null" value.
///
/// Example:
///   StrIter si = some_str_iter;
///   bool is_null_value;
///   StrIter new_si = CheckNull(si, &is_null_value);
///   // Use the result in `is_null_value`
///
StrIter JReadNull(StrIter si, bool* is_null);

///
/// Skip the value at the current position in the string.
///
/// This function is used to skip over the value at the current reading position in the string. It is primarily
/// used when the `Reader` in `JReadObject` doesn't read a value, allowing for selective skipping of key-value
/// pairs. It supports skipping different JSON value types like `true`, `false`, `null`, strings, numbers,
/// objects, and arrays.
///
/// si[in] : `StrIter`. Iterator to the current position in the string, where the value is to be skipped.
///
/// SUCCESS : Returns the updated string iterator (`StrIter`) after skipping the value.
/// FAILURE : Returns the same value as the provided `si` if an error occurs while skipping the value.
///           The error will be logged with the relevant details.
///
/// Error Cases:
///   - Invalid reading position.
///   - Exhausted string iterator range.
///   - Failed to parse a boolean (`true`, `false`), null, string, number, object, or array.
///   - Invalid JSON value encountered.
///
/// Supported Value Types:
///   - Boolean values: "true", "false".
///   - Null value: "null".
///   - String values: Enclosed in double quotes (`"`).
///   - Numbers: Integer or floating-point numbers.
///   - Objects: Enclosed in curly braces (`{}`).
///   - Arrays: Enclosed in square brackets (`[]`).
///
StrIter JSkipValue(StrIter si);

// ---------------- JR Means JSON Read -------------------

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

#define JR_BOOL(si, b)                                                                                                 \
    do {                                                                                                               \
        bool my_b = 0;                                                                                                 \
        si        = JReadBool((si), &my_b);                                                                            \
        (b)       = my_b;                                                                                              \
    } while (0)

#define JR_BOOL_KV(si, k, b)                                                                                           \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            bool my_b = 0;                                                                                             \
            si        = JReadBool((si), &my_b);                                                                        \
            (b)       = my_f;                                                                                          \
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

// ---------------- JW Means JSON Write -------------------

#define JW_OBJ(j, writer)                                                                                              \
    do {                                                                                                               \
        bool ___is_first___ = true;                                                                                    \
        StrPushBack(&(j), '{');                                                                                        \
        {writer};                                                                                                      \
        StrPushBack(&(j), '}');                                                                                        \
    } while (0)

#define JW_OBJ_KV(j, k, writer)                                                                                        \
    do {                                                                                                               \
        if (___is_first___) {                                                                                          \
            ___is_first___ = false;                                                                                    \
        } else {                                                                                                       \
            StrPushBack(&(j), ',');                                                                                    \
        }                                                                                                              \
        StrAppendf(&(j), "\"%s\":", k);                                                                                \
        JW_OBJ(j, writer);                                                                                             \
    } while (0)

#define JW_ARR(j, arr, item, writer)                                                                                   \
    do {                                                                                                               \
        bool ___is_first___ = true;                                                                                    \
        StrPushBack(&(j), '[');                                                                                        \
        VecForeach(&(arr), item, {                                                                                     \
            if (___is_first___) {                                                                                      \
                ___is_first___ = false;                                                                                \
            } else {                                                                                                   \
                StrPushBack(&(j), ',');                                                                                \
            }                                                                                                          \
            { writer }                                                                                                 \
        });                                                                                                            \
        StrPushBack(&(j), ']');                                                                                        \
    } while (0)

#define JW_ARR_KV(j, k, arr, item, writer)                                                                             \
    do {                                                                                                               \
        if (___is_first___) {                                                                                          \
            ___is_first___ = false;                                                                                    \
        } else {                                                                                                       \
            StrPushBack(&(j), ',');                                                                                    \
        }                                                                                                              \
        StrAppendf(&(j), "\"%s\":", k);                                                                                \
        JW_ARR(j, arr, item, writer);                                                                                  \
    } while (0)

#define JW_INT(j, i)                                                                                                   \
    do {                                                                                                               \
        i64 my_int = (i);                                                                                              \
        StrAppendf(&(j), "%lld", my_int);                                                                              \
    } while (0)

#define JW_INT_KV(j, k, i)                                                                                             \
    do {                                                                                                               \
        if (___is_first___) {                                                                                          \
            ___is_first___ = false;                                                                                    \
        } else {                                                                                                       \
            StrPushBack(&(j), ',');                                                                                    \
        }                                                                                                              \
        StrAppendf(&(j), "\"%s\":", k);                                                                                \
        JW_INT(j, i);                                                                                                  \
    } while (0)

#define JW_FLT(j, f)                                                                                                   \
    do {                                                                                                               \
        f64 my_flt = (f);                                                                                              \
        StrAppendf(&(j), "%f", my_flt);                                                                                \
    } while (0)

#define JW_FLT_KV(j, k, f)                                                                                             \
    do {                                                                                                               \
        if (___is_first___) {                                                                                          \
            ___is_first___ = false;                                                                                    \
        } else {                                                                                                       \
            StrPushBack(&(j), ',');                                                                                    \
        }                                                                                                              \
        StrAppendf(&(j), "\"%s\":", k);                                                                                \
        JW_FLT(j, f);                                                                                                  \
    } while (0)

#define JW_STR(j, s)                                                                                                   \
    do {                                                                                                               \
        StrAppendf(&(j), "\"%s\"", (s).data);                                                                          \
    } while (0)

#define JW_STR_KV(j, k, s)                                                                                             \
    do {                                                                                                               \
        if (___is_first___) {                                                                                          \
            ___is_first___ = false;                                                                                    \
        } else {                                                                                                       \
            StrPushBack(&(j), ',');                                                                                    \
        }                                                                                                              \
        StrAppendf(&(j), "\"%s\":", k);                                                                                \
        JW_STR(j, s);                                                                                                  \
    } while (0)

#define JW_BOOL(j, b)                                                                                                  \
    do {                                                                                                               \
        StrAppendf(&(j), "\"%b\"", b);                                                                                 \
    } while (0)

#define JW_BOOL_KV(j, k, b)                                                                                            \
    do {                                                                                                               \
        if (___is_first___) {                                                                                          \
            ___is_first___ = false;                                                                                    \
        } else {                                                                                                       \
            StrPushBack(&(j), ',');                                                                                    \
        }                                                                                                              \
        StrAppendf(&(j), "\"%s\":", k);                                                                                \
        JW_BOOL(j, b);                                                                                                 \
    } while (0)

#endif // MISRA_PARSERS_JSON_H
