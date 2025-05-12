/// file      : parsers/json.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Provides an easy to use API to serialize and deserialize JSON to and from
/// C structs.
///

#ifndef MISRA_PARSERS_JSON_H
#define MISRA_PARSERS_JSON_H

#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Types.h>

/* Usage Example :

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

    obj.strs = VecInit_T(&obj.strs);

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

    StrClear(&json);

    JW_OBJ(json, {
        JW_INT_KV(json, "ref", obj.ref);
        JW_OBJ_KV(json, "data", {
            JW_FLT_KV(json, "y_axis_val", obj.data.y);
            JW_FLT_KV(json, "x_axis_val", obj.data.x);
            JW_STR_KV(json, "gname", obj.data.n);
        });
        JW_STR_KV(json, "name", obj.name);
        JW_ARR_KV(json, "strs", obj.strs, s, { JW_STR(json, s); });
    });

    // {"ref":40,"data":{"y_axis_val":133.455231,"x_axis_val":-22.244850,"gname":"a random graph"},"name":"misra","strs":["x","ah _ ha","lessa do something"]}
    printf("%s\n", json.data);
*/

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

///
/// Read a JSON string value from stream and assign to target.
/// The resulting string is dynamically allocated in `Str` format.
///
/// si[in,out] : JSON stream iterator to read from.
/// str[out]   : Destination `Str` to store the string.
///
/// USAGE:
///   JR_STR(si, name);
///
/// SUCCESS : `str` contains the read string
/// FAILURE : `si` updated to failure state on parse error
///
#define JR_STR(si, str)                                                                                                \
    do {                                                                                                               \
        Str my_str = StrInit();                                                                                        \
        si         = JReadString((si), &my_str);                                                                       \
        (str)      = my_str;                                                                                           \
    } while (0)

///
/// Read a string key-value pair if key matches.
///
/// si[in,out] : JSON stream iterator to read from.
/// k[in]      : Expected key name (C-string).
/// str[out]   : Destination `Str` to store the value.
///
/// USAGE:
///   JR_STR_KV(si, "username", user.name);
///
/// SUCCESS : `str` contains the value if key matched
/// FAILURE : No-op if key does not match
///
#define JR_STR_KV(si, k, str)                                                                                          \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            Str my_str = StrInit();                                                                                    \
            si         = JReadString((si), &my_str);                                                                   \
            (str)      = my_str;                                                                                       \
        }                                                                                                              \
    } while (0)

///
/// Read a JSON integer value from stream and assign to target.
///
/// si[in,out] : JSON stream iterator to read from.
/// i[out]     : Integer variable to store the value.
///
/// USAGE:
///   JR_INT(si, count);
///
/// SUCCESS : `i` contains the parsed integer
/// FAILURE : `si` updated to failure state on parse error
///
#define JR_INT(si, i)                                                                                                  \
    do {                                                                                                               \
        i64 my_int = 0;                                                                                                \
        si         = JReadInteger((si), &my_int);                                                                      \
        (i)        = my_int;                                                                                           \
    } while (0)

///
/// Read an integer key-value pair if key matches.
///
/// si[in,out] : JSON stream iterator to read from.
/// k[in]      : Expected key name (C-string).
/// i[out]     : Integer variable to store the value.
///
/// USAGE:
///   JR_INT_KV(si, "ref", obj.ref);
///
/// SUCCESS : `i` contains value if key matched
/// FAILURE : No-op if key does not match
///
#define JR_INT_KV(si, k, i)                                                                                            \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            i64 my_int = 0;                                                                                            \
            si         = JReadInteger((si), &my_int);                                                                  \
            (i)        = my_int;                                                                                       \
        }                                                                                                              \
    } while (0)

///
/// Read a JSON float value from stream and assign to target.
///
/// si[in,out] : JSON stream iterator to read from.
/// f[out]     : Float variable to store the value.
///
/// USAGE:
///   JR_FLT(si, temperature);
///
/// SUCCESS : `f` contains the parsed float
/// FAILURE : `si` updated to failure state on parse error
///
#define JR_FLT(si, f)                                                                                                  \
    do {                                                                                                               \
        f64 my_flt = 0;                                                                                                \
        si         = JReadFloat((si), &my_flt);                                                                        \
        (f)        = my_flt;                                                                                           \
    } while (0)

///
/// Read a float key-value pair if key matches.
///
/// si[in,out] : JSON stream iterator to read from.
/// k[in]      : Expected key name (C-string).
/// f[out]     : Float variable to store the value.
///
/// USAGE:
///   JR_FLT_KV(si, "x_axis_val", obj.data.x);
///
/// SUCCESS : `f` contains value if key matched
/// FAILURE : No-op if key does not match
///
#define JR_FLT_KV(si, k, f)                                                                                            \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            f64 my_flt = 0;                                                                                            \
            si         = JReadFloat((si), &my_flt);                                                                    \
            (f)        = my_flt;                                                                                       \
        }                                                                                                              \
    } while (0)

///
/// Read a JSON boolean value from stream and assign to target.
///
/// si[in,out] : JSON stream iterator to read from.
/// b[out]     : Boolean variable to store the value.
///
/// USAGE:
///   JR_BOOL(si, is_active);
///
/// SUCCESS : `b` contains the parsed boolean
/// FAILURE : `si` updated to failure state on parse error
///
#define JR_BOOL(si, b)                                                                                                 \
    do {                                                                                                               \
        bool my_b = 0;                                                                                                 \
        si        = JReadBool((si), &my_b);                                                                            \
        (b)       = my_b;                                                                                              \
    } while (0)

///
/// Read a boolean key-value pair if key matches.
///
/// si[in,out] : JSON stream iterator to read from.
/// k[in]      : Expected key name (C-string).
/// b[out]     : Boolean variable to store the value.
///
/// USAGE:
///   JR_BOOL_KV(si, "enabled", flag);
///
/// SUCCESS : `b` contains value if key matched
/// FAILURE : No-op if key does not match
///
#define JR_BOOL_KV(si, k, b)                                                                                           \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            bool my_b = 0;                                                                                             \
            si        = JReadBool((si), &my_b);                                                                        \
            (b)       = my_b;                                                                                          \
        }                                                                                                              \
    } while (0)

///
/// Read a JSON array using a custom value reader expression.
///
/// The macro parses a JSON array and calls the user-provided code block for each element.
/// If the value can't be parsed or reader fails to advance the iterator, the value is skipped.
///
/// si[in,out] : Stream iterator to read from.
/// reader     : Code block to read each value.
///
/// USAGE:
///   JR_ARR(si, {
///       MyStruct tmp = {0};
///       JR_OBJ(si, {
///           JR_INT_KV(si, "x", tmp.x);
///           JR_FLT_KV(si, "y", tmp.y);
///       });
///       VecPush(&data.items, tmp);
///   });
///
/// SUCCESS : All values processed or skipped gracefully
/// FAILURE : Logs error and restores `si` on structural or read failure
///
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

///
/// Read a JSON object using a custom field reader expression.
///
/// The macro parses the object and invokes the provided code block for each key-value pair.
/// If the key is not recognized or parsing fails, the value is skipped.
///
/// si[in,out] : Stream iterator to read from.
/// reader     : Code block to handle each key-value pair. Can include JR_*_KV macros.
///
/// USAGE:
///   JR_OBJ(si, {
///       JR_STR_KV(si, "name", obj.name);
///       JR_INT_KV(si, "id", obj.id);
///   });
///
/// SUCCESS : Entire object read or skipped successfully
/// FAILURE : Logs error and restores `si` on structural or read failure
///
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
                    LOG_ERROR("Expected ',' after key/value pairs in object. Invalid JSON object.");                   \
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

///
/// Conditionally parse a JSON object if key matches expected name.
///
/// si[in,out] : Stream iterator to read from.
/// k[in]      : Expected key name (C-string).
/// reader     : Code block to handle key-value pairs in object.
///
/// USAGE:
///   JR_OBJ_KV(si, "config", {
///       JR_BOOL_KV(si, "debug", flags.debug_mode);
///   });
///
/// SUCCESS : Object parsed if key matched
/// FAILURE : No-op if key does not match
///
#define JR_OBJ_KV(si, k, reader)                                                                                       \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            JR_OBJ(si, reader);                                                                                        \
        }                                                                                                              \
    } while (0)

///
/// Conditionally parse a JSON array if key matches expected name.
///
/// si[in,out] : Stream iterator to read from.
/// k[in]      : Expected key name (C-string).
/// reader     : Code block to handle array element parsing.
///
/// USAGE:
///   JR_ARR_KV(si, "list", {
///       JR_INT(si, val);
///       VecPush(&arr, val);
///   });
///
/// SUCCESS : Array parsed if key matched
/// FAILURE : No-op if key does not match
///
#define JR_ARR_KV(si, k, reader)                                                                                       \
    do {                                                                                                               \
        if (!StrCmpCstr(&key, (k))) {                                                                                  \
            JR_ARR(si, reader);                                                                                        \
        }                                                                                                              \
    } while (0)

///
/// Begin a JSON object and write key-value entries using JW_*_KV macros.
/// This macro must be used as a wrapper for other `JW_*_KV` macros to generate a JSON object.
/// Tracks whether commas are needed between entries using an internal flag.
///
/// writer[in] : A block of code containing `JW_*_KV` calls for populating the object.
///
/// USAGE:
///   JW_OBJ(json, {
///       JW_STR_KV(json, "name", obj.name);
///       JW_INT_KV(json, "ref", obj.ref);
///   });
///
/// SUCCESS : Appends a valid JSON object to `json`
/// FAILURE : Does not return on failure (relies on internal string operations)
///
#define JW_OBJ(j, writer)                                                                                              \
    do {                                                                                                               \
        bool ___is_first___ = true;                                                                                    \
        StrPushBack(&(j), '{');                                                                                        \
        {writer};                                                                                                      \
        StrPushBack(&(j), '}');                                                                                        \
    } while (0)

///
/// Write a key and nested object inside an existing JSON object.
/// Should be called inside `JW_OBJ`. Adds commas appropriately based on insertion order.
///
/// j[in,out] : The target string to append to.
/// k[in]     : The key name to use in the JSON object.
/// writer[in]: A block of code using `JW_*_KV` to populate the inner object.
///
/// USAGE:
///   JW_OBJ_KV(json, "config", {
///       JW_INT_KV(json, "timeout", 30);
///   });
///
/// SUCCESS : Appends a nested JSON object under the given key
/// FAILURE : Does not return on failure
///
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

///
/// Write a JSON array from a vector. Each item is rendered using the provided `writer`.
/// Handles inserting commas between elements.
///
/// j[in,out]  : The target string to append to.
/// arr[in]    : A vector to iterate over.
/// item[out]  : Iterator variable for the current item.
/// writer[in] : Code block that appends JSON for each `item`.
///
/// USAGE:
///   JW_ARR(json, some_vec, s, {
///       JW_STR(json, s);
///   });
///
/// SUCCESS : Appends a JSON array to `json`
/// FAILURE : Does not return on failure
///
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

///
/// Write a key and an array value into a JSON object.
/// Intended for use within a `JW_OBJ`. Adds commas automatically.
///
/// j[in,out]  : The target string to append to.
/// k[in]      : Key name for the array.
/// arr[in]    : A vector to iterate over.
/// item[out]  : Iterator variable for the current item.
/// writer[in] : Code block that appends JSON for each `item`.
///
/// USAGE:
///   JW_ARR_KV(json, "tags", tag_vec, tag, {
///       JW_STR(json, tag);
///   });
///
/// SUCCESS : Appends a key-value array pair to `json`
/// FAILURE : Does not return on failure
///
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

///
/// Append an integer value to a JSON string.
///
/// j[in,out] : The target string to append to.
/// i[in]     : Integer value.
///
/// USAGE:
///   JW_INT(json, 42);
///
/// SUCCESS : Appends a numeric value to `json`
/// FAILURE : Does not return on failure
///
#define JW_INT(j, i)                                                                                                   \
    do {                                                                                                               \
        i64 my_int = (i);                                                                                              \
        StrAppendf(&(j), "%lld", my_int);                                                                              \
    } while (0)

///
/// Write a key and integer value to a JSON object.
///
/// j[in,out] : The target string to append to.
/// k[in]     : Key name.
/// i[in]     : Integer value.
///
/// USAGE:
///   JW_INT_KV(json, "count", 5);
///
/// SUCCESS : Appends a key-value integer pair
/// FAILURE : Does not return on failure
///
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

///
/// Append a floating-point value to a JSON string.
///
/// j[in,out] : The target string to append to.
/// f[in]     : Floating-point value.
///
/// USAGE:
///   JW_FLT(json, 3.14);
///
/// SUCCESS : Appends a float value to `json`
/// FAILURE : Does not return on failure
///
#define JW_FLT(j, f)                                                                                                   \
    do {                                                                                                               \
        f64 my_flt = (f);                                                                                              \
        StrAppendf(&(j), "%f", my_flt);                                                                                \
    } while (0)

///
/// Write a key and float value to a JSON object.
///
/// j[in,out] : The target string to append to.
/// k[in]     : Key name.
/// f[in]     : Floating-point value.
///
/// USAGE:
///   JW_FLT_KV(json, "pi", 3.14159);
///
/// SUCCESS : Appends a key-value float pair
/// FAILURE : Does not return on failure
///
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

///
/// Append a string value (quoted) to the JSON.
///
/// j[in,out] : The target string to append to.
/// s[in]     : A `Str` object containing the string.
///
/// USAGE:
///   JW_STR(json, name);
///
/// SUCCESS : Appends a quoted string to `json`
/// FAILURE : Does not return on failure
///
#define JW_STR(j, s)                                                                                                   \
    do {                                                                                                               \
        StrAppendf(&(j), "\"%s\"", (s).data);                                                                          \
    } while (0)

///
/// Write a key and string value into a JSON object.
///
/// j[in,out] : The target string to append to.
/// k[in]     : Key name.
/// s[in]     : A `Str` object containing the value.
///
/// USAGE:
///   JW_STR_KV(json, "username", user.name);
///
/// SUCCESS : Appends a key-value string pair
/// FAILURE : Does not return on failure
///
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

///
/// Append a boolean value to the JSON as a string "true"/"false".
///
/// j[in,out] : The target string to append to.
/// b[in]     : Boolean value.
///
/// USAGE:
///   JW_BOOL(json, true);
///
/// SUCCESS : Appends "true" or "false" as a string
/// FAILURE : Does not return on failure
///
#define JW_BOOL(j, b)                                                                                                  \
    do {                                                                                                               \
        StrAppendf(&(j), "\"%b\"", b);                                                                                 \
    } while (0)

///
/// Write a key and boolean value into a JSON object.
///
/// j[in,out] : The target string to append to.
/// k[in]     : Key name.
/// b[in]     : Boolean value.
///
/// USAGE:
///   JW_BOOL_KV(json, "is_active", user.active);
///
/// SUCCESS : Appends a key-value boolean pair
/// FAILURE : Does not return on failure
///
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
