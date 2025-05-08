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
/// Strictly parse a JSON object.
///
/// This function parses a JSON object by reading key-value pairs. Each key is expected to be a string,
/// and the corresponding value is processed using the provided `Reader` function. The object must be enclosed
/// in curly braces `{}` and key-value pairs should be separated by commas.
///
/// si[in]    : `StrIter`. Iterator to the string being parsed.
/// Reader[in]: Function pointer to a custom key-value reader that will process the key and value from the object.
///             This function is expected to return an updated `StrIter` based on the key and value.
/// data[in]  : Pointer to any user-specific data passed to the `Reader` function.
///
/// SUCCESS : Returns the updated string iterator (`StrIter`) after parsing the object.
/// FAILURE : Returns the same value as the provided `si` if an error occurs during parsing.
///           The error will be logged with the relevant details.
///
/// Error Cases:
///   - Invalid reading position.
///   - Exhausted string iterator range.
///   - Missing or incorrect `Reader` function.
///   - Invalid object start (`{`).
///   - Missing or incorrect colon (`:`) between key and value.
///   - Unexpected comma or missing comma between key-value pairs.
///   - Invalid JSON format.
///   - Invalid or missing object end (`}`).
///
StrIter JReadObject(StrIter si, JObjectValueReader Reader, void* data);

///
/// Reads a JSON array from a string iterator.
///
/// This function parses a JSON array from the input string and optionally uses a user-provided
/// `Reader` function to handle each individual element. If no `Reader` is provided, it will skip
/// over each value using a generic JSON value skipper.
///
/// Parameters:
///   si[in]       : The input `StrIter` pointing to the beginning of the array.
///   Reader[in]   : Optional function to read/process each array element.
///                  If `NULL`, elements will be skipped using `JSkipValue`.
///   data[in,out] : Optional user-provided data passed to the `Reader` function.
///
/// Returns:
///   A `StrIter` pointing just past the end of the array (`]`) if successful,
///   or the original iterator on error (no advancement).
///
StrIter JReadArray(StrIter si, JArrayItemReader Reader, void* data);

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
/// Appends a boolean value to a JSON string.
///
/// This function appends `true` or `false` directly into the given JSON buffer.
/// Intended for use when writing JSON arrays or raw values.
///
/// Parameters:
///   json[in,out]   : Pointer to the destination JSON `Str` buffer.
///   value[in]      : Boolean value to append (`true` or `false`).
///   has_comma[in]  : If `true`, a terminal comma is added after the value.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteBool(Str* json, bool value, bool has_comma);

///
/// Appends a `null` value to a JSON string.
///
/// This function appends the literal `null` to the given JSON buffer.
/// Intended for use when writing JSON arrays or raw values.
///
/// Parameters:
///   json[in,out]   : Pointer to the destination JSON `Str` buffer.
///   has_comma[in]  : If `true`, a terminal comma is added after the value.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteNull(Str* json, bool has_comma);

///
/// Appends a string value to a JSON string.
///
/// This function appends a quoted JSON string value (e.g., `"value"`) to the given buffer.
/// Intended for use when writing JSON arrays or raw values.
///
/// Parameters:
///   json[in,out]   : Pointer to the destination JSON `Str` buffer.
///   value[in]      : The string value to append (must be non-null and non-empty).
///   has_comma[in]  : If `true`, a terminal comma is added after the value.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteString(Str* json, const char* value, bool has_comma);

///
/// Appends a numeric value to a JSON string.
///
/// This function appends an integer or floating-point number to the JSON buffer,
/// depending on the `is_float` flag. Intended for use when writing JSON arrays
/// or raw values.
///
/// Parameters:
///   json[in,out]   : Pointer to the destination JSON `Str` buffer.
///   is_float[in]   : If `true`, appends `float_val`; otherwise, appends `int_val`.
///   int_val[in]    : Integer value to append if `is_float` is `false`.
///   float_val[in]  : Floating-point value to append if `is_float` is `true`.
///   has_comma[in]  : If `true`, a terminal comma is added after the value.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteNumber(Str* json, bool is_float, i64 int_val, f64 float_val, bool has_comma);

///
/// Appends an array of strings to a JSON string.
///
/// This function adds a field to the JSON string containing an array of strings.
/// It loops through the provided string vector (`strvec`) and appends each string to the array.
///
/// Parameters:
///   json[in,out]     : Pointer to the destination `Str` JSON buffer.
///   strvec[in]       : A vector of strings to be written to the JSON string (must be non-null).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the array—used when appending multiple key-value pairs
///                      to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, `NULL` on error.
///
/// Notes:
///   - The array is written as a JSON array, with each string enclosed in double quotes (`"string"`).
///   - The `has_comma` parameter determines whether a comma is added after the array field. This is useful when appending
///     multiple fields to a JSON object.
///
Str* JWriteStrArray(Str* json, StrVec* strvec, bool has_comma);

///
/// Appends an array of numbers to a JSON string.
///
/// This function adds a field to the JSON string containing an array of either integer or floating-point values.
/// The type of numbers to write (integer or float) is determined by the `is_float` parameter. If `is_float` is `true`,
/// the function writes values from the `fvec` (float vector), otherwise, it writes from the `ivec` (integer vector).
///
/// Parameters:
///   json[in,out]     : Pointer to the destination `Str` JSON buffer.
///   is_float[in]     : A boolean flag that determines whether the numbers are floats (`true`) or integers (`false`).
///   ivec[in]         : A vector of integers to be written to the JSON string (only used if `is_float` is `false`).
///   fvec[in]         : A vector of floating-point numbers to be written to the JSON string (only used if `is_float` is `true`).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the array—used when appending multiple key-value pairs
///                      to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, `NULL` on error.
///
/// Notes:
///   - If `is_float` is `true`, `fvec` must be non-null and `ivec` should be null.
///   - If `is_float` is `false`, `ivec` must be non-null and `fvec` should be null.
///   - If both vectors are provided or both are null, the function will log an error and return `NULL`.
///
Str* JWriteNumberArray(Str* json, bool is_float, Si64Vec* ivec, F64Vec* fvec, bool has_comma);

/// Str* json,
/// Str* (*Writer) (Str* str, void* object, bool has_comma),
/// void* object,
/// bool  has_comma
#define JWriteObjectArray(json, Writer, object_arr, has_comma)                                     \
    do {                                                                                           \
        if (!(json)) {                                                                             \
            LOG_ERROR("Invalid arguments.");                                                       \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        if (!Writer) {                                                                             \
            LOG_ERROR("Invalid object writer provided.");                                          \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        if (!(object_arr)) {                                                                       \
            LOG_ERROR("Invalid object array.");                                                    \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        StrPushBack((json), '[');                                                                  \
        VecForeachPtrIdx((object_arr), item_ptr, idx, {                                            \
            bool _has_comma = idx != (object_arr)->length - 1;                                     \
            Writer((json), item_ptr, _has_comma);                                                  \
        });                                                                                        \
        StrPushBack((json), ']');                                                                  \
                                                                                                   \
        if (has_comma) {                                                                           \
            StrPushBack((json), ',');                                                              \
        }                                                                                          \
                                                                                                   \
        return json;                                                                               \
    } while (0)

///
/// Appends a boolean field to a JSON string.
///
/// This function adds a `"field_name": true` or `"field_name": false` pair to the given JSON string.
///
/// Parameters:
///   json[in,out]     : Pointer to the destination `Str` JSON buffer.
///   field_name[in]   : The name of the field (must be non-null and non-empty).
///   value[in]        : Boolean value to write (`true` or `false`).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the
///                      field—used when appending multiple key-value pairs to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteBoolKV(Str* json, const char* field_name, bool value, bool has_comma);

///
/// Appends a null field to a JSON string.
///
/// This function adds a `"field_name": null` pair to the given JSON string.
///
/// Parameters:
///   json[in,out]     : Pointer to the destination `Str` JSON buffer.
///   field_name[in]   : The name of the field (must be non-null and non-empty).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the
///                      field—used when appending multiple key-value pairs to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteNullKV(Str* json, const char* field_name, bool has_comma);

///
/// Appends a string field to a JSON string.
///
/// This function adds a `"field_name": "value"` pair to the given JSON string.
///
/// Parameters:
///   json[in,out]     : Pointer to the destination `Str` JSON buffer.
///   field_name[in]   : The name of the field (must be non-null and non-empty).
///   value[in]        : The string value to write (must be non-null and non-empty).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the
///                      field—used when appending multiple key-value pairs to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteStringKV(Str* json, const char* field_name, const char* value, bool has_comma);

///
/// Append a number field to a JSON string.
///
/// This function appends a `"field_name": number_value` pair to the given `Str` JSON string.
///
/// Parameters:
///   json[in,out]      : Pointer to the destination JSON `Str` buffer.
///   field_name[in]    : The name of the field (must be non-empty).
///   is_float[in]      : Indicates whether the number is a float or integer.
///   int_val[in]       : The integer value (used if `is_float` is false).
///   float_val[in]     : The float value (used if `is_float` is true).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the
///                      field—used when appending multiple key-value pairs to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, `NULL` on error.
///
Str* JWriteNumberKV(
    Str*        json,
    const char* field_name,
    bool        is_float,
    i64         int_val,
    f64         float_val,
    bool        has_comma
);

///
/// Appends an integer field to a JSON string.
///
/// This is a convenience wrapper over `JWriteNumberKV` for writing integer values.
///
/// Parameters:
///   json[in,out]      : Pointer to the destination JSON `Str` buffer.
///   field_name[in]    : Name of the field (must be non-null and non-empty).
///   int_val[in]       : Integer value to write.
///   has_comma[in]     : Whether to append a trailing comma.
///
/// Returns:
///   Updated `Str*` if successful, `NULL` on error.
///
Str* JWriteIntegerKV(Str* json, const char* field_name, i64 int_val, bool has_comma);

///
/// Appends a floating-point field to a JSON string.
///
/// This is a convenience wrapper over `JWriteNumberKV` for writing float values.
///
/// Parameters:
///   json[in,out]      : Pointer to the destination JSON `Str` buffer.
///   field_name[in]    : Name of the field (must be non-null and non-empty).
///   float_val[in]     : Floating-point value to write.
///   has_comma[in]     : Whether to append a trailing comma.
///
/// Returns:
///   Updated `Str*` if successful, `NULL` on error.
///
Str* JWriteFloatKV(Str* json, const char* field_name, f64 float_val, bool has_comma);

///
/// Appends a string array field to a JSON string.
///
/// This function adds a `"field_name": ["str1", "str2", ...]` pair to the given JSON string.
///
/// Parameters:
///   json[in,out]     : Pointer to the destination `Str` JSON buffer.
///   field_name[in]   : The name of the field (must be non-null and non-empty).
///   strvec[in]       : Vector of strings to serialize (must be non-null).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the
///                      field—used when appending multiple key-value pairs to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteStrArrayKV(Str* json, const char* field_name, StrVec* strvec, bool has_comma);

///
/// Appends a number array field to a JSON string.
///
/// This function adds a `"field_name": [num1, num2, ...]` pair to the given JSON string,
/// where the numbers are either integers or floating-point values.
///
/// Parameters:
///   json[in,out]     : Pointer to the destination `Str` JSON buffer.
///   field_name[in]   : The name of the field (must be non-null and non-empty).
///   is_float[in]     : A boolean flag indicating whether the numbers are floats (`true`)
///                      or integers (`false`).
///   ivec[in]         : Vector of integers to serialize (must be non-null if `is_float` is `false`).
///   fvec[in]         : Vector of floats to serialize (must be non-null if `is_float` is `true`).
///   has_comma[in]    : If `true`, a terminal comma will be inserted after the
///                      field—used when appending multiple key-value pairs to a JSON object.
///
/// Returns:
///   Pointer to the updated `Str` if successful, or `NULL` on error.
///
Str* JWriteNumberArrayKV(
    Str*        json,
    const char* field_name,
    bool        is_float,
    Si64Vec*    ivec,
    F64Vec*     fvec,
    bool        has_comma
);

///
/// Appends an integer array field to a JSON string.
///
/// This is a convenience wrapper over `JWriteNumberArrayKV` for writing arrays of integers.
///
/// Parameters:
///   json[in,out]      : Pointer to the destination JSON `Str` buffer.
///   field_name[in]    : Name of the field (must be non-null and non-empty).
///   ivec[in]          : Vector of integers to serialize (must be non-null).
///   has_comma[in]     : Whether to append a trailing comma.
///
/// Returns:
///   Updated `Str*` if successful, `NULL` on error.
///
Str* JWriteIntegerArrayKV(Str* json, const char* field_name, Si64Vec* ivec, bool has_comma);

///
/// Appends a float array field to a JSON string.
///
/// This is a convenience wrapper over `JWriteNumberArrayKV` for writing arrays of floats.
///
/// Parameters:
///   json[in,out]      : Pointer to the destination JSON `Str` buffer.
///   field_name[in]    : Name of the field (must be non-null and non-empty).
///   fvec[in]          : Vector of floats to serialize (must be non-null).
///   has_comma[in]     : Whether to append a trailing comma.
///
/// Returns:
///   Updated `Str*` if successful, `NULL` on error.
///
Str* JWriteFloatArrayKV(Str* json, const char* field_name, F64Vec* fvec, bool has_comma);

/// Str* json,
/// const char* field_name
/// Str* (*Writer) (Str* str, void* object, bool has_comma),
/// void* object,
/// bool  has_comma
#define JWriteObjectArrayKV(json, field_name, Writer, object_arr, has_comma)                       \
    do {                                                                                           \
        if (!(json)) {                                                                             \
            LOG_ERROR("Invalid arguments.");                                                       \
            break;                                                                                 \
        }                                                                                          \
        if (!(field_name) || !strlen(field_name)) {                                                \
            LOG_ERROR("Invalid field name.");                                                      \
            break;                                                                                 \
        }                                                                                          \
        if (!(Writer)) {                                                                           \
            LOG_ERROR("Invalid object writer provided.");                                          \
            break;                                                                                 \
        }                                                                                          \
        if (!(object_arr)) {                                                                       \
            LOG_ERROR("Invalid object array.");                                                    \
            break;                                                                                 \
        }                                                                                          \
                                                                                                   \
        StrAppendf((json), "\"%s\":[", (field_name));                                              \
        VecForeachPtrIdx((object_arr), item_ptr, idx, {                                            \
            bool _has_comma = idx != (object_arr)->length - 1;                                     \
            Writer((json), item_ptr, _has_comma);                                                  \
        });                                                                                        \
        StrPushBack((json), ']');                                                                  \
                                                                                                   \
        if (has_comma) {                                                                           \
            StrPushBack((json), ',');                                                              \
        }                                                                                          \
                                                                                                   \
        return (json);                                                                             \
    } while (0)

#endif // MISRA_PARSERS_JSON_H
