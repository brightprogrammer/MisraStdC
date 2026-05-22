#include <Misra/Parsers/JSON.h>
#include <Misra/Std/Zstr.h>
#include <Misra/Std/Allocator/Default.h>

// libc

static StrIter JSkipObject(StrIter si) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    // starting of an object
    char c;
    if (!StrIterPeek(&si, &c) || c != '{') {
        LOG_ERROR("Invalid object start. Expected '{'.");
        return saved_si;
    }
    StrIterMustNext(&si);
    si = JSkipWhitespace(si);

    StrIter read_si;
    bool    expect_comma = false;

    // scratch allocator for the per-iteration `key` Str. Lives across the
    // whole loop; freed before every return below. DefaultAllocator is the
    // right fit -- JSON keys are caller-controlled and unbounded in size,
    // so a stack-backed buffer would impose a truncation limit on parser
    // input.
    DefaultAllocator scratch = DefaultAllocatorInit();

    // while not at the end of object.
    while (StrIterPeek(&si, &c) && c != '}') {
        if (expect_comma) {
            if (c != ',') {
                LOG_ERROR(
                    "Expected ',' between key/value pairs in object. Invalid "
                    "JSON object."
                );
                DefaultAllocatorDeinit(&scratch);
                return saved_si;
            }
            StrIterMustNext(&si); // skip comma
            si = JSkipWhitespace(si);
        }

        Str key = StrInit(&scratch);

        // key start
        read_si = JReadString(si, &key);
        if (read_si.pos == si.pos) {
            LOG_ERROR("Failed to read string key in object. Invalid JSON");
            StrDeinit(&key);
            DefaultAllocatorDeinit(&scratch);
            return saved_si;
        }
        si = read_si;
        si = JSkipWhitespace(si);

        if (!StrIterPeek(&si, &c) || c != ':') {
            LOG_ERROR("Expected ':' after key string. Failed to read JSON");
            StrDeinit(&key);
            DefaultAllocatorDeinit(&scratch);
            return saved_si;
        }
        StrIterMustNext(&si);
        si = JSkipWhitespace(si);

        // skip values within object
        read_si = JSkipValue(si);

        // if still no advancement in read position
        if (read_si.pos == si.pos) {
            LOG_ERROR("Failed to parse value. Invalid JSON.");
            StrDeinit(&key);
            DefaultAllocatorDeinit(&scratch);
            return saved_si;
        }

        LOG_INFO("User skipped reading of '{}' field in JSON object.", key);

        StrDeinit(&key);
        si = read_si;
        si = JSkipWhitespace(si);

        // expect a comma after a successful key-value pair read
        expect_comma = true;
    }

    if (!StrIterPeek(&si, &c) || c != '}') {
        LOG_ERROR("Expected end of object '}' but found '{c}'", c);
        DefaultAllocatorDeinit(&scratch);
        return saved_si;
    }

    StrIterMustNext(&si);
    DefaultAllocatorDeinit(&scratch);
    return si;
}

static StrIter JSkipArray(StrIter si) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    char c;
    if (!StrIterPeek(&si, &c) || c != '[') {
        LOG_ERROR("Invalid array start. Expected '['.");
        return saved_si;
    }
    StrIterMustNext(&si);
    si = JSkipWhitespace(si);

    StrIter read_si;
    bool    expect_comma = false;

    // while not at the end of array.
    while (StrIterPeek(&si, &c) && c != ']') {
        if (expect_comma) {
            if (c != ',') {
                LOG_ERROR("Expected ',' between values in array. Invalid JSON array.");
                return saved_si;
            }
            StrIterMustNext(&si); // skip comma
            si = JSkipWhitespace(si);
        }

        // skip values within array
        read_si = JSkipValue(si);

        // if no advancement in read position
        if (read_si.pos == si.pos) {
            LOG_ERROR("Failed to parse value. Invalid JSON.");
            return saved_si;
        }

        si = read_si;
        si = JSkipWhitespace(si);

        // expect a comma after a successful value read in array
        expect_comma = true;
    }

    // end of array
    if (!StrIterPeek(&si, &c) || c != ']') {
        LOG_ERROR("Invalid end of array. Expected ']'.");
        return saved_si;
    }

    StrIterMustNext(&si);
    return si;
}

StrIter JSkipWhitespace(StrIter si) {
    char c;
    while (StrIterPeek(&si, &c)) {
        switch (c) {
            case ' ' :
            case '\t' :
            case '\r' :
            case '\n' :
                StrIterMustNext(&si);
                break;
            default :
                return si;
        }
    }
    return si;
}

StrIter JReadString(StrIter si, Str *str) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!str) {
        LOG_FATAL("Invalid str object to read into.");
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    // string start
    char c;
    if (StrIterPeek(&si, &c) && c == '"') {
        StrIterMustNext(&si);

        // while a printable character
        while (StrIterPeek(&si, &c)) {
            // three cases
            // - end of string (return)
            // - an escape sequence (processed and appended)
            // - acceptable string character (appended)
            switch (c) {
                // end of string
                case '"' :
                    StrIterMustNext(&si);
                    return si;

                // starting of an escape sequence
                case '\\' :
                    StrIterMustNext(&si);
                    if (!StrIterPeek(&si, &c)) {
                        LOG_ERROR("Unexpected end of string.");
                        StrClear(str);
                        return saved_si;
                    }

                    switch (c) {
                        // escape sequence
                        case '\\' :
                            StrPushBack(str, '\\');
                            StrIterMustNext(&si);
                            break;

                        case '"' :
                            StrPushBack(str, '"');
                            StrIterMustNext(&si);
                            break;

                        case '/' :
                            StrPushBack(str, '/');
                            StrIterMustNext(&si);
                            break;

                        case 'b' :
                            StrPushBack(str, '\b');
                            StrIterMustNext(&si);
                            break;

                        case 'f' :
                            StrPushBack(str, '\f');
                            StrIterMustNext(&si);
                            break;

                        case 'n' :
                            StrPushBack(str, '\n');
                            StrIterMustNext(&si);
                            break;

                        case 'r' :
                            StrPushBack(str, '\r');
                            StrIterMustNext(&si);
                            break;

                        case 't' :
                            StrPushBack(str, '\t');
                            StrIterMustNext(&si);
                            break;

                        // espaced unicode sequence
                        case 'u' :
                            LOG_ERROR(
                                "No unicode support '{.6}'. Unicode sequence will be skipped.",
                                LVAL(si.data + si.pos - 1)
                            );
                            StrIterMustMove(&si, 5);
                            break;

                        default :
                            LOG_ERROR("Invalid JSON object key string.");
                            StrClear(str);
                            return saved_si;
                    }
                    break;

                // default allowed characters
                default :
                    StrPushBack(str, c);
                    StrIterMustNext(&si);
                    break;
            }
        }
    }

    return si;
}

StrIter JReadNumber(StrIter si, Number *num) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!num) {
        LOG_FATAL("Invalid number object.");
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);
    // scratch allocator for the digit-accumulator Str `ns`. JSON numbers
    // are caller-controlled and have no spec-mandated upper length, so a
    // stack-backed buffer is unsafe -- DefaultAllocator is the right fit.
    DefaultAllocator scratch = DefaultAllocatorInit();
    Str              ns      = StrInit(&scratch);

    bool is_neg = false;
    char c;
    if (StrIterPeek(&si, &c) && c == '-') {
        is_neg = true;
        StrIterMustNext(&si);
    }

    bool is_flt             = false;
    bool has_exp            = false;
    bool has_exp_plus_minus = false;
    bool is_parsing         = true;

    while (is_parsing && StrIterPeek(&si, &c)) {
        switch (c) {
            case 'E' :
            case 'e' :
                if (has_exp) {
                    LOG_ERROR("Invalid number. Multiple exponent indicators.");
                    StrDeinit(&ns);
                    DefaultAllocatorDeinit(&scratch);
                    return saved_si;
                }
                has_exp = true;
                is_flt  = true;
                StrPushBack(&ns, c);
                StrIterMustNext(&si);
                break;

            case '.' :
                if (is_flt) {
                    LOG_ERROR("Invalid number. Multiple decimal indicators.");
                    StrDeinit(&ns);
                    DefaultAllocatorDeinit(&scratch);
                    return saved_si;
                }
                is_flt = true;
                StrPushBack(&ns, c);
                StrIterMustNext(&si);
                break;

            case '0' :
            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' :
            case '8' :
            case '9' :
                StrPushBack(&ns, c);
                StrIterMustNext(&si);
                break;

            case '-' :
            case '+' :
                // +/- can only appear after an exponent
                if (!has_exp) {
                    LOG_ERROR(
                        "Invalid number. Exponent sign indicators '+' or '-' "
                        "must appear after exponent 'E' or 'e' indicator."
                    );
                    StrDeinit(&ns);
                    DefaultAllocatorDeinit(&scratch);
                    return saved_si;
                }
                if (has_exp_plus_minus) {
                    LOG_ERROR(
                        "Invalid number. Multiple '+' or '-' in Number. "
                        "Expected only once after 'e' or 'E'."
                    );
                    StrDeinit(&ns);
                    DefaultAllocatorDeinit(&scratch);
                    return saved_si;
                }
                has_exp_plus_minus = true;
                StrPushBack(&ns, c);
                StrIterMustNext(&si);
                break;

            default :
                is_parsing = false;
                break;
        }
    }

    if (!ns.length) {
        LOG_ERROR("Failed to parse number. '{.8}'", LVAL(saved_si.data + saved_si.pos));
        StrDeinit(&ns);
        DefaultAllocatorDeinit(&scratch);
        return saved_si;
    }

    // convert to number
    Zstr end = NULL;
    if (is_flt) {
        num->f = ZstrToF64(ns.data, &end);
    } else {
        num->i = ZstrToI64(ns.data, &end);
    }
    if (end == ns.data) {
        LOG_ERROR("Failed to convert string to number.");
        StrDeinit(&ns);
        DefaultAllocatorDeinit(&scratch);
        return saved_si;
    }

    // negate
    if (is_neg) {
        if (is_flt) {
            num->f *= -1;
        } else {
            num->i *= -1;
        }
    }
    num->is_float = is_flt;

    StrDeinit(&ns);
    DefaultAllocatorDeinit(&scratch);
    return si;
}

StrIter JReadInteger(StrIter si, i64 *val) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!val) {
        LOG_FATAL("Invalid pointer to integer. Don't know where to store.");
    }

    StrIter saved_si = si;
    Number  num;
    si = JReadNumber(si, &num);

    if (si.pos == saved_si.pos) {
        LOG_ERROR("Failed to parse integer number.");
        return saved_si;
    }

    if (num.is_float) {
        LOG_ERROR("Failed to parse integer. Got floating point value.");
        return saved_si;
    }

    *val = num.i;

    return si;
}

StrIter JReadFloat(StrIter si, f64 *val) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!val) {
        LOG_FATAL("Invalid pointer to float. Don't know where to store.");
    }

    StrIter saved_si = si;
    Number  num;
    si = JReadNumber(si, &num);

    if (si.pos == saved_si.pos) {
        LOG_ERROR("Failed to parse floating point number");
        return saved_si;
    }

    if (num.is_float) {
        *val = num.f;
    } else {
        *val = (f64)num.i;
    }

    return si;
}

StrIter JReadBool(StrIter si, bool *b) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!b) {
        LOG_FATAL("Invalid boolean pointer. Don't know where to store.");
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    char c;
    if (StrIterRemainingLength(&si) >= 4) {
        if (StrIterPeek(&si, &c) && c == 't') {
            const char *pos = StrIterPos(&si);
            if (pos && ZstrCompareN(pos, "true", 4) == 0) {
                StrIterMustMove(&si, 4);
                *b = true;
                return si;
            }
            LOG_ERROR("Failed to read boolean value. Expected true. Invalid JSON");
            return saved_si;
        }

        if (StrIterRemainingLength(&si) >= 5) {
            if (StrIterPeek(&si, &c) && c == 'f') {
                const char *pos = StrIterPos(&si);
                if (pos && ZstrCompareN(pos, "false", 5) == 0) {
                    StrIterMustMove(&si, 5);
                    *b = false;
                    return si;
                }
                LOG_ERROR("Failed to read boolean value. Expected false. Invalid JSON");
                return saved_si;
            }
        }

        LOG_ERROR("Failed to parse boolean value. Expected true/false. Invalid JSON");
        return saved_si;
    } else {
        LOG_ERROR(
            "Insufficient string length to parse a boolean value. Unexpected "
            "end of input."
        );
        return saved_si;
    }
}

StrIter JReadNull(StrIter si, bool *is_null) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!is_null) {
        LOG_FATAL("Invalid boolean pointer. Don't know where to store.");
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    *is_null = false;
    char c;
    if (StrIterRemainingLength(&si) >= 4) {
        if (StrIterPeek(&si, &c) && c == 'n') {
            const char *pos = StrIterPos(&si);
            if (pos && ZstrCompareN(pos, "null", 4) == 0) {
                StrIterMustMove(&si, 4);
                *is_null = true;
                return si;
            }
            LOG_ERROR("Failed to read boolean value. Expected null. Invalid JSON");
            return saved_si;
        }

        return saved_si;
    } else {
        LOG_ERROR(
            "Insufficient string length to parse a boolean value. Unexpected "
            "end of input."
        );
        return saved_si;
    }
}

StrIter JSkipValue(StrIter si) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    char c;
    if (!StrIterPeek(&si, &c)) {
        LOG_ERROR("Failed to read value. Invalid JSON");
        return si;
    }

    // check for true/false
    if (c == 't' || c == 'f') {
        StrIter before_si = si;
        bool    b;
        si = JReadBool(si, &b);

        if (si.pos == before_si.pos) {
            LOG_ERROR(
                "Failed to read boolean value. Expected true/false. Invalid "
                "JSON."
            );
            return saved_si;
        }

        return si;
    }

    // check for null
    if (c == 'n') {
        StrIter before_si = si;
        bool    n;
        si = JReadNull(si, &n);

        if (si.pos == before_si.pos) {
            LOG_ERROR(
                "Failed to read boolean value. Expected true/false. Invalid "
                "JSON."
            );
            return saved_si;
        }

        return si;
    }


    // expecting a string
    if (c == '"') {
        StrIter before_si = si;
        // String value is parsed-and-discarded; JSON spec puts no upper
        // bound on string length, so a stack-backed buffer would limit
        // valid input. DefaultAllocator is the right fit here.
        DefaultAllocator scratch = DefaultAllocatorInit();
        Str              s       = StrInit(&scratch);
        si                       = JReadString(si, &s);
        StrDeinit(&s);
        DefaultAllocatorDeinit(&scratch);

        if (si.pos == before_si.pos) {
            LOG_ERROR("Failed to read string value. Expected string. Invalid JSON.");
            return saved_si;
        }

        return si;
    }

    // looks like starting of a number?
    if (c == '-' || (c >= '0' && c <= '9')) {
        StrIter before_si = si;
        Number  num;
        si = JReadNumber(si, &num);

        if (si.pos == before_si.pos) {
            LOG_ERROR("Failed to read number value. Expected a number. Invalid JSON.");
            return saved_si;
        }

        return si;
    }

    // looks like starting of an object
    if (c == '{') {
        StrIter before_si = si;
        si                = JSkipObject(si);

        if (si.pos == before_si.pos) {
            LOG_ERROR("Failed to read object. Expected an object. Invalid JSON.");
            return saved_si;
        }

        return si;
    }

    // looks like starting of an array
    if (c == '[') {
        StrIter before_si = si;
        si                = JSkipArray(si);

        if (si.pos == before_si.pos) {
            LOG_ERROR("Failed to read array. Expected an array. Invalid JSON.");
            return saved_si;
        }

        return si;
    }

    LOG_ERROR("Failed to read value. Invalid JSON");
    return si;
}
