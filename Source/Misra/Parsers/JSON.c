#include <Misra/Parsers/JSON.h>
#include <stdlib.h>

static StrIter JReadObject(StrIter si, StrIter (*Reader)(StrIter si, Str* key, void* data), void* data) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    // starting of an object
    if (StrIterPeek(&si) != '{') {
        LOG_ERROR("Invalid object start. Expected '{'.");
        return saved_si;
    }
    StrIterNext(&si);
    si = JSkipWhitespace(si);

    StrIter read_si;
    bool    expect_comma = false;

    if (!Reader) {
        LOG_INFO(
            "User didn't provide any value reader combinator to read from "
            "object KV. Values will "
            "be skipped."
        );
    }

    // while not at the end of object.
    while (StrIterPeek(&si) && StrIterPeek(&si) != '}') {
        if (expect_comma) {
            if (StrIterPeek(&si) != ',') {
                LOG_ERROR(
                    "Expected ',' between key/value pairs in object. Invalid "
                    "JSON object."
                );
                return saved_si;
            }
            StrIterNext(&si); // skip comma
            si = JSkipWhitespace(si);
        }

        Str key = StrInit();

        // key start
        read_si = JReadString(si, &key);
        if (read_si.pos == si.pos) {
            LOG_ERROR("Failed to read string key in object. Invalid JSON");
            StrDeinit(&key);
            return saved_si;
        }
        si = read_si;
        si = JSkipWhitespace(si);

        if (StrIterPeek(&si) != ':') {
            LOG_ERROR("Expected ':' after key string. Failed to read JSON");
            StrDeinit(&key);
            return saved_si;
        }
        StrIterNext(&si);
        si = JSkipWhitespace(si);

        // try reading using user provided reader
        if (Reader) {
            read_si = Reader(si, &key, data);
        } else {
            read_si = si;
        }

        // if no advancement in read position
        if (read_si.pos == si.pos) {
            // skip the value
            read_si = JSkipValue(si);

            // if still no advancement in read position
            if (read_si.pos == si.pos) {
                LOG_ERROR("Failed to parse value. Invalid JSON.");
                StrDeinit(&key);
                return saved_si;
            }

            LOG_INFO("User skipped reading of '%s' field in JSON object.", key.data);
        }
        StrDeinit(&key);
        si = read_si;
        si = JSkipWhitespace(si);

        // expect a comma after a successful key-value pair read
        expect_comma = true;
    }

    if (StrIterPeek(&si) != '}') {
        LOG_ERROR("Expected end of object '}' but found '%c'", StrIterPeek(&si));
        return saved_si;
    }

    StrIterNext(&si);
    return si;
}

static StrIter JReadArray(StrIter si, StrIter (*Reader)(StrIter si, void* data), void* data) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    // starting of an object
    if (StrIterPeek(&si) != '[') {
        LOG_ERROR("Invalid array start. Expected '['.");
        return saved_si;
    }
    StrIterNext(&si);
    si = JSkipWhitespace(si);

    StrIter read_si;
    bool    expect_comma = false;

    // while not at the end of array.
    while (StrIterPeek(&si) && StrIterPeek(&si) != ']') {
        if (expect_comma) {
            if (StrIterPeek(&si) != ',') {
                LOG_ERROR("Expected ',' between values in array. Invalid JSON array.");
                return saved_si;
            }
            StrIterNext(&si); // skip comma
            si = JSkipWhitespace(si);
        }

        // try reading using user provided reader
        if (Reader) {
            read_si = Reader(si, data);
        } else {
            read_si = JSkipValue(si);
        }

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
    if (StrIterPeek(&si) != ']') {
        LOG_ERROR("Invalid end of array. Expected ']'.");
        return saved_si;
    }

    StrIterNext(&si);
    return si;
}

StrIter JSkipWhitespace(StrIter si) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    while (StrIterRemainingLength(&si) && StrIterPeek(&si)) {
        switch (StrIterPeek(&si)) {
            case ' ' :
            case '\t' :
            case '\r' :
            case '\n' :
                StrIterNext(&si);
                break;
            default :
                return si;
        }
    }

    return si;
}

StrIter JReadString(StrIter si, Str* str) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!str) {
        LOG_ERROR("Invalid str object to read into.");
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    // string start
    if (StrIterRemainingLength(&si) && StrIterPeek(&si) == '"') {
        StrIterNext(&si);

        // while a printable character
        while (StrIterRemainingLength(&si) && StrIterPeek(&si)) {
            // three cases
            // - end of string (return)
            // - an escape sequence (processed and appended)
            // - acceptable string character (appended)
            switch (StrIterPeek(&si)) {
                // end of string
                case '"' :
                    StrIterNext(&si);
                    return si;

                // starting of an escape sequence
                case '\\' :
                    StrIterNext(&si);
                    if (!StrIterRemainingLength(&si)) {
                        LOG_ERROR("Unexpected end of string.");
                        StrClear(str);
                        return saved_si;
                    }

                    switch (StrIterPeek(&si)) {
                        // escape sequence
                        case '\\' :
                            StrPushBack(str, '\\');
                            StrIterNext(&si);
                            break;

                        case '"' :
                            StrPushBack(str, '"');
                            StrIterNext(&si);
                            break;

                        case '/' :
                            StrPushBack(str, '/');
                            StrIterNext(&si);
                            break;

                        case 'b' :
                            StrPushBack(str, '\b');
                            StrIterNext(&si);
                            break;

                        case 'f' :
                            StrPushBack(str, '\f');
                            StrIterNext(&si);
                            break;

                        case 'n' :
                            StrPushBack(str, '\n');
                            StrIterNext(&si);
                            break;

                        case 'r' :
                            StrPushBack(str, '\r');
                            StrIterNext(&si);
                            break;

                        case 't' :
                            StrPushBack(str, '\t');
                            StrIterNext(&si);
                            break;

                        // espaced unicode sequence
                        case 'u' :
                            LOG_ERROR("No unicode support");
                            StrClear(str);
                            return saved_si;

                        default :
                            LOG_ERROR("Invalid JSON object key string.");
                            StrClear(str);
                            return saved_si;
                    }
                    break;

                // default allowed characters
                default :
                    StrPushBack(str, StrIterPeek(&si));
                    StrIterNext(&si);
                    break;
            }
        }
    }

    return si;
}

StrIter JReadNumber(StrIter si, Number* num) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!num) {
        LOG_ERROR("Invalid number object.");
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);
    Str ns           = StrInit();

    bool is_neg = false;
    if (StrIterPeek(&si) == '-') {
        is_neg = true;
        StrIterNext(&si);
    }

    bool is_flt             = false;
    bool has_exp            = false;
    bool has_exp_plus_minus = false;
    bool is_parsing         = true;

    while (is_parsing && StrIterRemainingLength(&si) && StrIterPeek(&si)) {
        switch (StrIterPeek(&si)) {
            case 'E' :
            case 'e' :
                if (is_flt || has_exp) {
                    LOG_ERROR("Invalid number. Multiple exponent indicators.");
                    StrDeinit(&ns);
                    return saved_si;
                }
                has_exp = true;
                is_flt  = true;
                StrPushBack(&ns, StrIterPeek(&si));
                StrIterNext(&si);
                break;

            case '.' :
                if (is_flt) {
                    LOG_ERROR("Invalid number. Multiple decimal indicators.");
                    StrDeinit(&ns);
                    return saved_si;
                }
                is_flt = true;
                StrPushBack(&ns, StrIterPeek(&si));
                StrIterNext(&si);
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
                StrPushBack(&ns, StrIterPeek(&si));
                StrIterNext(&si);
                break;

            case '-' :
            case '+' :
                // +/- can only appear after an exponent
                if (!has_exp || has_exp_plus_minus) {
                    LOG_ERROR(
                        "Invalid number. Exponent sign indicators '+' or '-' "
                        "must appear after exponent 'E' or 'e' indicator."
                    );
                    StrDeinit(&ns);
                    return saved_si;
                }
                has_exp_plus_minus = true;
                StrPushBack(&ns, StrIterPeek(&si));
                StrIterNext(&si);
                break;

            default :
                is_parsing = false;
                break;
        }
    }

    if (!ns.length) {
        LOG_ERROR("Failed to parse number. It's empty!");
        StrDeinit(&ns);
        return saved_si;
    }

    // convert to number
    char* end = NULL;
    if (is_flt) {
        num->f = strtod(ns.data, &end);
    } else {
        num->i = strtoll(ns.data, &end, 10);
    }
    if (end == ns.data) {
        LOG_ERROR("Failed to convert string to number.");
        StrDeinit(&ns);
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
    return si;
}

StrIter JReadInteger(StrIter si, i64* val) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!val) {
        LOG_ERROR("Invalid pointer to integer. Don't know where to store.");
        return si;
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

StrIter JReadFloat(StrIter si, f64* val) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!val) {
        LOG_ERROR("Invalid pointer to float. Don't know where to store.");
        return si;
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

StrIter JReadBool(StrIter si, bool* b) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!b) {
        LOG_ERROR("Invalid boolean pointer. Don't know where to store.");
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    if (StrIterRemainingLength(&si) >= 4) {
        if (StrIterPeek(&si) == 't') {
            if (!strncmp(StrIterPos(&si), "true", 4)) {
                StrIterMove(&si, 4);
                *b = true;
                return si;
            }
            LOG_ERROR("Failed to read boolean value. Expected true. Invalid JSON");
            return saved_si;
        }

        if (StrIterRemainingLength(&si) >= 5) {
            if (StrIterPeek(&si) == 'f') {
                if (!strncmp(StrIterPos(&si), "false", 5)) {
                    StrIterMove(&si, 5);
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

StrIter JReadNull(StrIter si, bool* is_null) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }

    if (!is_null) {
        LOG_ERROR("Invalid boolean pointer. Don't know where to store.");
        return si;
    }

    StrIter saved_si = si;
    si               = JSkipWhitespace(si);

    *is_null = false;
    if (StrIterRemainingLength(&si) >= 4) {
        if (StrIterPeek(&si) == 'n') {
            if (!strncmp(StrIterPos(&si), "null", 4)) {
                StrIterMove(&si, 4);
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

    // check for true/false
    if (StrIterPeek(&si) == 't' || StrIterPeek(&si) == 'f') {
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
    if (StrIterPeek(&si) == 'n') {
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
    if (StrIterPeek(&si) == '"') {
        StrIter before_si = si;
        Str     s         = StrInit();
        si                = JReadString(si, &s);
        StrDeinit(&s);

        if (si.pos == before_si.pos) {
            LOG_ERROR("Failed to read string value. Expected string. Invalid JSON.");
            return saved_si;
        }

        return si;
    }

    // looks like starting of a number?
    if (StrIterPeek(&si) == '-' || (StrIterPeek(&si) >= '0' && StrIterPeek(&si) <= '9')) {
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
    if (StrIterPeek(&si) == '{') {
        StrIter before_si = si;
        si                = JReadObject(si, NULL, NULL);

        if (si.pos == before_si.pos) {
            LOG_ERROR("Failed to read object. Expected an object. Invalid JSON.");
            return saved_si;
        }

        return si;
    }

    // looks like starting of an array
    if (StrIterPeek(&si) == '[') {
        StrIter before_si = si;
        si                = JReadArray(si, NULL, NULL);

        if (si.pos == before_si.pos) {
            LOG_ERROR("Failed to read array. Expected an array. Invalid JSON.");
            return saved_si;
        }

        return si;
    }

    LOG_ERROR("Failed to read value. Invalid JSON");
    return si;
}
