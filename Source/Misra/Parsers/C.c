/// file      : parsers/c.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// A C parser, to parse C code into an AST
///

#include <Misra/Parsers/C.h>
#include <Misra/Std/Log.h>
#include <Misra/Std/Utility.h>

// TODO: in strncmp where we're using MIN2, make sure to ceheck for sufficient remaining length


#define NEXT(si, c)                                                                                                    \
    do {                                                                                                               \
        StrIterNext(si);                                                                                               \
        c = StrIterPeek(si);                                                                                           \
    } while (0);

#define PEEK(si, c)                                                                                                    \
    do {                                                                                                               \
        c = StrIterPeek(si);                                                                                           \
    } while (0);

bool SkipWS(StrIter* si) {
    if (!StrIterRemainingLength(si)) {
        return false;
    }

    while (StrIterPeek(si)) {
        switch (StrIterPeek(si)) {
            case ' ' :
            case '\t' :
            case '\r' :
            case '\n' :
            case '\b' :
            case '\v' :
            case '\f' :
                StrIterNext(si);
                break;
            default :
                true;
        }
    }
    return true;
}

bool cReadKeyword(StrIter* si, Keyword* k) {
    if (!si || !k) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    size rl = StrIterRemainingLength(si);
#define MATCH(s, e)                                                                                                    \
    if (strncmp(si->data + si->pos, s, MIN2(sizeof(s) - 1, rl)) == 0) {                                                \
        StrIterMove(si, sizeof(s) - 1);                                                                                \
        *k = e;                                                                                                        \
        return true;                                                                                                   \
    }

    MATCH("alignas", KEYWORD_ALIGNAS);
    MATCH("_Alignas", KEYWORD_ALIGNAS);
    MATCH("alignof", KEYWORD_ALIGNOF);
    MATCH("_Alignof", KEYWORD_ALIGNAS);
    MATCH("auto", KEYWORD_AUTO);
    MATCH("bool", KEYWORD_BOOL);
    MATCH("_Bool", KEYWORD_BOOL);
    MATCH("break", KEYWORD_BREAK);
    MATCH("case", KEYWORD_CASE);
    MATCH("char", KEYWORD_CHAR);
    MATCH("const", KEYWORD_CONST);
    MATCH("constexpr", KEYWORD_CONSTEXPR);
    MATCH("continue", KEYWORD_CONTINUE);
    MATCH("default", KEYWORD_DEFAULT);
    MATCH("do", KEYWORD_DO);
    MATCH("double", KEYWORD_DOUBLE);
    MATCH("else", KEYWORD_ELSE);
    MATCH("enum", KEYWORD_ENUM);
    MATCH("extern", KEYWORD_EXTERN);
    MATCH("false", KEYWORD_FALSE);
    MATCH("float", KEYWORD_FLOAT);
    MATCH("for", KEYWORD_FOR);
    MATCH("goto", KEYWORD_GOTO);
    MATCH("if", KEYWORD_IF);
    MATCH("inline", KEYWORD_INLINE);
    MATCH("int", KEYWORD_INT);
    MATCH("long", KEYWORD_LONG);
    MATCH("nullptr", KEYWORD_NULLPTR);
    MATCH("register", KEYWORD_REGISTER);
    MATCH("restrict", KEYWORD_RESTRICT);
    MATCH("return", KEYWORD_RETURN);
    MATCH("short", KEYWORD_SHORT);
    MATCH("signed", KEYWORD_SIGNED);
    MATCH("sizeof", KEYWORD_SIZEOF);
    MATCH("static", KEYWORD_STATIC);
    MATCH("static_assert", KEYWORD_STATIC_ASSERT);
    MATCH("_Static_assert", KEYWORD_STATIC_ASSERT);
    MATCH("struct", KEYWORD_STRUCT);
    MATCH("switch", KEYWORD_SWITCH);
    MATCH("thread_local", KEYWORD_THREAD_LOCAL);
    MATCH("_Thread_local", KEYWORD_THREAD_LOCAL);
    MATCH("true", KEYWORD_TRUE);
    MATCH("typedef", KEYWORD_TYPEDEF);
    MATCH("typeof", KEYWORD_TYPEOF);
    MATCH("typeof_unqual", KEYWORD_TYPEOF_UNQUAL);
    MATCH("union", KEYWORD_UNION);
    MATCH("unsigned", KEYWORD_UNSIGNED);
    MATCH("void", KEYWORD_VOID);
    MATCH("volatile", KEYWORD_VOLATILE);
    MATCH("while", KEYWORD_WHILE);
    MATCH("_Atomic", KEYWORD_ATOMIC);
    MATCH("_BitInt", KEYWORD_BITINT);
    MATCH("_Complex", KEYWORD_COMPLEX);
    MATCH("_Decimal128", KEYWORD_DECIMAL128);
    MATCH("_Decimal32", KEYWORD_DECIMAL32);
    MATCH("_Decimal64", KEYWORD_DECIMAL64);
    MATCH("_Generic", KEYWORD_GENERIC);
    MATCH("_Imaginary", KEYWORD_IMAGINARY);
    MATCH("_Noreturn", KEYWORD_NORETURN);

#undef MATCH
    *si = saved_si;
    return false;
}

bool cReadIdentifier(StrIter* si, Str* id) {
    if (!si) {
        LOG_FATAL("Invalid arguments");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    char c = StrIterPeek(si);

    // identifier cannot start with a digit
    if (IS_DIGIT(c)) {
        *si = saved_si;
        return false;
    }

    StrClear(id);

    while (IS_ALPHA_NUMERIC(c) || (c == '_')) {
        StrPushBack(id, c);
        StrIterNext(si);
        c = StrIterPeek(si);
    }

    if (id->length) {
        return true;
    } else {
        *si = saved_si;
        return false;
    }
}

bool cReadIntegerSuffix(StrIter* si, IntegerSuffix* suffix) {
    if (!si || !suffix) {
        LOG_FATAL("Invalid arguments");
    }

    size rl = StrIterRemainingLength(si);
#define MATCH(s, e)                                                                                                    \
    if (strncmp(si->data + si->pos, s, MIN2(sizeof(s) - 1, rl)) == 0) {                                                \
        StrIterMove(si, sizeof(s) - 1);                                                                                \
        *suffix = e;                                                                                                   \
        return true;                                                                                                   \
    }

    MATCH("wb", INTEGER_SUFFIX_WB);
    MATCH("WB", INTEGER_SUFFIX_WB);
    MATCH("l", INTEGER_SUFFIX_L);
    MATCH("L", INTEGER_SUFFIX_L);
    MATCH("ll", INTEGER_SUFFIX_LL);
    MATCH("LL", INTEGER_SUFFIX_LL);
    MATCH("u", INTEGER_SUFFIX_U);
    MATCH("U", INTEGER_SUFFIX_U);

    MATCH("wbu", INTEGER_SUFFIX_WBU);
    MATCH("WBu", INTEGER_SUFFIX_WBU);
    MATCH("lu", INTEGER_SUFFIX_LU);
    MATCH("Lu", INTEGER_SUFFIX_LU);
    MATCH("llu", INTEGER_SUFFIX_LLU);
    MATCH("LLu", INTEGER_SUFFIX_LLU);

    MATCH("wbU", INTEGER_SUFFIX_WBU);
    MATCH("WBU", INTEGER_SUFFIX_WBU);
    MATCH("lU", INTEGER_SUFFIX_LU);
    MATCH("LU", INTEGER_SUFFIX_LU);
    MATCH("llU", INTEGER_SUFFIX_LLU);
    MATCH("LLU", INTEGER_SUFFIX_LLU);

    MATCH("Uwb", INTEGER_SUFFIX_UWB);
    MATCH("UWB", INTEGER_SUFFIX_UWB);
    MATCH("Ul", INTEGER_SUFFIX_UL);
    MATCH("UL", INTEGER_SUFFIX_UL);
    MATCH("Ull", INTEGER_SUFFIX_ULL);
    MATCH("ULL", INTEGER_SUFFIX_ULL);

    MATCH("uwb", INTEGER_SUFFIX_UWB);
    MATCH("uWB", INTEGER_SUFFIX_UWB);
    MATCH("ul", INTEGER_SUFFIX_UL);
    MATCH("uL", INTEGER_SUFFIX_UL);
    MATCH("ull", INTEGER_SUFFIX_ULL);
    MATCH("uLL", INTEGER_SUFFIX_ULL);

#undef MATCH
    *suffix = INTEGER_SUFFIX_NONE;
    return false;
}

bool cReadInteger(StrIter* si, Integer* v) {
    if (!si || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    StrIter saved_si = *si;
    SkipWS(si);

    Str vs = StrInit();

    u32 base = 0;

    char c = StrIterPeek(si);
    if (IS_DIGIT(c)) {
        if (c != '0') {
            base = 10;
        } else {
            base = 8;
        }

        StrPushBack(&vs, c);
        StrIterNext(si);
        c = StrIterPeek(si);

        if (base == 8) {
            if (c == 'x' || c == 'X') {
                base = 16;
                StrPushBack(&vs, c);
                StrIterNext(si);
                c = StrIterPeek(si);
            } else if (c == 'b' || c == 'B') {
                base = 2;
                StrIterNext(si);
                c = StrIterPeek(si);
            } else if (!IS_OCT(c)) {
                LOG_ERROR("Invalid integer constant prefix \".*%s\".", 2, si->data - 2);
                StrDeinit(&vs);
                *si = saved_si;
                return false;
            }
        }
    } else {
        *si = saved_si;
        StrDeinit(&vs);
        return false;
    }

    switch (base) {
        case 2 : {
            while (c == '0' || c == '1' || c == '\'') {
                if (c != '\'') {
                    StrPushBack(&vs, c);
                }
                StrIterNext(si);
                c = StrIterPeek(si);
            }
            break;
        }

        case 8 : {
            while (IS_OCT(c) || c == '\'') {
                if (c != '\'') {
                    StrPushBack(&vs, c);
                }
                StrIterNext(si);
                c = StrIterPeek(si);
            }
            break;
        }

        case 10 : {
            while (IS_DIGIT(c) || c == '\'') {
                if (c != '\'') {
                    StrPushBack(&vs, c);
                }
                StrIterNext(si);
                c = StrIterPeek(si);
            }
            break;
        }

        case 16 : {
            while (IS_HEX(c) || c == '\'') {
                if (c != '\'') {
                    StrPushBack(&vs, c);
                }
                StrIterNext(si);
                c = StrIterPeek(si);
            }
            break;
        }

        default :
            LOG_FATAL("Unreachable code reached.");
    }

    if (vs.length) {
        char* end = NULL;
        v->i      = strtoll(vs.data, &end, base);
        if (end == vs.data) {
            LOG_ERROR(
                "Failed to read integer. Tried to convert \".*%s\" to integer. Guessed base was %u",
                vs.length,
                vs.data,
                base
            );
            StrDeinit(&vs);
            *si = saved_si;
            return false;
        }
        StrDeinit(&vs);

        // optional suffix
        cReadIntegerSuffix(si, &v->suffix);

        return true;
    } else {
        *si = saved_si;
        return false;
    }
    return false;
}

bool cReadFloatSuffix(StrIter* si, FloatSuffix* suffix) {
    if (!si || !suffix) {
        LOG_FATAL("Invalid arguments");
    }

    size rl = StrIterRemainingLength(si);
#define MATCH(s, e)                                                                                                    \
    if (strncmp(si->data + si->pos, s, MIN2(sizeof(s) - 1, rl)) == 0) {                                                \
        StrIterMove(si, sizeof(s) - 1);                                                                                \
        *suffix = e;                                                                                                   \
        return true;                                                                                                   \
    }

    MATCH("f", FLOAT_SUFFIX_F);
    MATCH("F", FLOAT_SUFFIX_F);

    MATCH("l", FLOAT_SUFFIX_L);
    MATCH("L", FLOAT_SUFFIX_L);

    MATCH("df", FLOAT_SUFFIX_DF);
    MATCH("DF", FLOAT_SUFFIX_DF);

    MATCH("dd", FLOAT_SUFFIX_DD);
    MATCH("DD", FLOAT_SUFFIX_DD);

    MATCH("dl", FLOAT_SUFFIX_DL);
    MATCH("DL", FLOAT_SUFFIX_DL);

#undef MATCH
    *suffix = FLOAT_SUFFIX_NONE;
    return false;
}

bool cReadFloat(StrIter* si, Float* v) {
    if (!si || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    StrIter saved_si = *si;
    SkipWS(si);

    Str vs = StrInit();

    char c = 0;
    PEEK(si, c);

    bool is_hex = true;
    if (c == '0') {
        StrPushBack(&vs, c);
        NEXT(si, c);

        if (c == 'x' || c == 'X') {
            StrPushBack(&vs, c);
            NEXT(si, c);
        }

        is_hex = true;
    }

    bool has_dec = false;
    if (c == '.') {
        has_dec = true;
        StrPushBack(&vs, c);
        NEXT(si, c);
    }

    while ((is_hex && IS_HEX(c)) || (!is_hex && IS_DIGIT(c))) {
        StrPushBack(&vs, c);
        NEXT(si, c);

        if (c == '.') {
            if (has_dec) {
                LOG_ERROR("Invalid floating point value.");
                *si = saved_si;
                StrDeinit(&vs);
                return false;
            }

            has_dec = true;
            StrPushBack(&vs, c);
            NEXT(si, c);
        }
    }

    if ((is_hex && (c == 'p' || c == 'P')) || (!is_hex && (c == 'e' || c == 'E'))) {
        StrPushBack(&vs, c);
        NEXT(si, c);

        if (c == '+' || c == '-') {
            StrPushBack(&vs, c);
            NEXT(si, c);
        }

        while (IS_DIGIT(c)) {
            StrPushBack(&vs, c);
            NEXT(si, c);
        }
    }

    if (vs.length) {
        char* end = NULL;
        v->f      = strtold(vs.data, &end);
        if (end == vs.data) {
            LOG_ERROR("Failed to read float. Tried to convert \".*%s\" to long double", vs.length, vs.data);
            StrDeinit(&vs);
            *si = saved_si;
            return false;
        }
        StrDeinit(&vs);

        // optional suffix
        cReadFloatSuffix(si, &v->suffix);

        // A floating suffix df, dd, dl, DF, DD, or DL shall not be used in a
        // hexadecimal floating constant.
        if (is_hex) {
            switch (v->suffix) {
                case FLOAT_SUFFIX_DD :
                case FLOAT_SUFFIX_DL :
                case FLOAT_SUFFIX_DF :
                    LOG_ERROR("Invalid suffix used with hexadecimal float constant.");
                    *si = saved_si;
                    return false;
                default :
                    break;
            }
        }

        return true;
    } else {
        *si = saved_si;
        return false;
    }

    return true;
}

bool cReadEnumConstant(StrIter* si, Str* v) {
    if (!si || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    StrIter saved_si = *si;
    SkipWS(si);
    // TODO:
    return true;
}

// reads a character without it's enclosing double quotes '' or it's encoding-prefix
// meant to be used in cReadChar or cReadStringLiteral
bool cReadCharLiteral(StrIter* si, Char* v) {
    if (!si || !v) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;

    char c = 0;
    PEEK(si, c);

    if (c == '\'' || c == '\n') {
        LOG_ERROR("Invalid or empty character sequence.");
        *si = saved_si;
        return false;
    }

#define SET_CHAR(v, cv)                                                                                                \
    do {                                                                                                               \
        switch ((v)->prefix) {                                                                                         \
            case CHAR_PREFIX_NONE :                                                                                    \
                (v)->c = (cv);                                                                                         \
                break;                                                                                                 \
            case CHAR_PREFIX_U8 :                                                                                      \
                (v)->u8 = (cv);                                                                                        \
                break;                                                                                                 \
            case CHAR_PREFIX_U16 :                                                                                     \
                (v)->u16 = (cv);                                                                                       \
                break;                                                                                                 \
            case CHAR_PREFIX_L :                                                                                       \
            case CHAR_PREFIX_U32 :                                                                                     \
                (v)->u32 = (cv);                                                                                       \
                break;                                                                                                 \
        }                                                                                                              \
    } while (0)

    // possible escape sequence
    if (c == '\\') {
        NEXT(si, c);
        switch (c) {
            case '\'' :
                SET_CHAR(v, '\'');
                NEXT(si, c);
                return true;
            case '\"' :
                SET_CHAR(v, '\"');
                NEXT(si, c);
                return true;
            case '\\' :
                SET_CHAR(v, '\\');
                NEXT(si, c);
                return true;
            case 'a' :
                SET_CHAR(v, '\a');
                NEXT(si, c);
                return true;
            case 'b' :
                SET_CHAR(v, '\b');
                NEXT(si, c);
                return true;
            case 'f' :
                SET_CHAR(v, '\f');
                NEXT(si, c);
                return true;
            case 'n' :
                SET_CHAR(v, '\n');
                NEXT(si, c);
                return true;
            case 'r' :
                SET_CHAR(v, '\r');
                NEXT(si, c);
                return true;
            case 't' :
                SET_CHAR(v, '\t');
                NEXT(si, c);
                return true;
            case 'v' :
                SET_CHAR(v, '\v');
                NEXT(si, c);
                return true;
            case 'u' : {
                NEXT(si, c);
                Str  h = StrInit();
                size i = 0;
                for (i = 0; IS_HEX(c) && i < 4; i++) {
                    StrPushBack(&h, c);
                    NEXT(si, c);
                }
                if (i < 4) {
                    LOG_ERROR("Failed to convert 16-bit unicode escape sequence to character.");
                    StrDeinit(&h);
                    *si = saved_si;
                    return false;
                }
                char* end = NULL;
                v->prefix = CHAR_PREFIX_U16;
                v->u16    = strtoul(h.data, &end, 16);
                if (end == h.data) {
                    LOG_ERROR("Failed to convert 16-bit unicode escape sequence to character.");
                    StrDeinit(&h);
                    *si = saved_si;
                    return false;
                }
                StrDeinit(&h);
                return true;
            }
            case 'U' : {
                NEXT(si, c);
                Str  h = StrInit();
                size i = 0;
                for (i = 0; IS_HEX(c) && i < 8; i++) {
                    StrPushBack(&h, c);
                    NEXT(si, c);
                }
                if (i < 8) {
                    LOG_ERROR("Failed to convert 32-bit unicode escape sequence to character.");
                    StrDeinit(&h);
                    *si = saved_si;
                    return false;
                }
                char* end = NULL;
                v->prefix = CHAR_PREFIX_U32;
                v->u32    = strtoul(h.data, &end, 16);
                if (end == h.data) {
                    LOG_ERROR("Failed to convert 32-bit unicode escape sequence to character.");
                    StrDeinit(&h);
                    *si = saved_si;
                    return false;
                }
                if (v->u32 > 0x10FFFF) {
                    LOG_ERROR("Unicode code point exceeds valid range.");
                    *si = saved_si;
                    StrDeinit(&h);
                    return false;
                }
                StrDeinit(&h);
                return true;
            }
            case 'x' : {
                NEXT(si, c);
                Str h = StrInit();
                for (size i = 0; IS_HEX(c) && i < 8; i++) {
                    StrPushBack(&h, c);
                    NEXT(si, c);
                }
                if (h.length == 0) {
                    LOG_ERROR("Empty hex escape-sequence in character.");
                    StrDeinit(&h);
                    *si = saved_si;
                    return false;
                }
                char* end = NULL;
                v->prefix = CHAR_PREFIX_U32;
                v->u32    = strtoul(h.data, &end, 16);
                if (end == h.data) {
                    LOG_ERROR("Failed to convert hex escape sequence to character.");
                    StrDeinit(&h);
                    *si = saved_si;
                    return false;
                }
                StrDeinit(&h);
                return true;
            }
            case '0' :
            case '1' :
            case '2' :
            case '3' :
            case '4' :
            case '5' :
            case '6' :
            case '7' : {
                Str o = StrInit();
                StrPushBack(&o, c);
                NEXT(si, c);
                for (size i = 0; IS_OCT(c) && i < 2; i++) {
                    StrPushBack(&o, c);
                    NEXT(si, c);
                }
                char* end = NULL;
                v->prefix = CHAR_PREFIX_U32;
                v->u32    = strtol(o.data, &end, 8);
                if (end == o.data) {
                    LOG_ERROR("Failed to convert octal escape sequence to character.");
                    StrDeinit(&o);
                    *si = saved_si;
                    return false;
                }
                StrDeinit(&o);
                return true;
            }

            default :
                LOG_ERROR("Invalid standard character escape.");
                *si = saved_si;
                return false;
        }
    } else {
        SET_CHAR(v, c);
        NEXT(si, c);
        return true;
    }
}

bool cReadCharPrefix(StrIter* si, CharPrefix* prefix) {
    if (!si || !prefix) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    char c = 0;
    PEEK(si, c);

    switch (c) {
        case 'u' : {
            NEXT(si, c);
            if (c == '8') {
                *prefix = CHAR_PREFIX_U8;
                NEXT(si, c);
            } else {
                *prefix = CHAR_PREFIX_U16;
            }
            return true;
        }
        case 'U' : {
            *prefix = CHAR_PREFIX_U32;
            NEXT(si, c);
            return true;
        }
        case 'L' : {
            *prefix = CHAR_PREFIX_L;
            NEXT(si, c);
            return true;
        }
        default :
            *prefix = CHAR_PREFIX_NONE;
            *si     = saved_si;
            return false;
    }
}

// reads a single character along with it's encoding prefix and double quotes ''
bool cReadChar(StrIter* si, Char* v) {
    if (!si || !v) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    // optioanl character literal
    cReadCharPrefix(si, &v->prefix);

    char c = 0;
    PEEK(si, c);

    if (c == '\'') {
        NEXT(si, c);

        if (!cReadCharLiteral(si, v)) {
            LOG_ERROR("Failed to read char literal.");
            *si = saved_si;
            return false;
        }

        PEEK(si, c);
        if (c != '\'') {
            LOG_ERROR("Unexpected end of character sequence. Expected a \"'\", got %c.", c);
            *si = saved_si;
            return false;
        }
        NEXT(si, c);
    }

    return true;
}

bool cReadPredefinedConstant(StrIter* si, PredefinedConstant* v) {
    if (!si || !v) {
        LOG_FATAL("Invalid arguments.");
    }
    StrIter saved_si = *si;
    SkipWS(si);

    size rl = StrIterRemainingLength(si);
    if (!strncmp(si->data + si->pos, "false", MIN2(5, rl))) {
        *v = PREDEFINED_CONSTANT_FALSE;
        StrIterMove(si, 5);
        return true;
    } else if (!strncmp(si->data + si->pos, "true", MIN2(4, rl))) {
        *v = PREDEFINED_CONSTANT_TRUE;
        StrIterMove(si, 4);
        return true;
    } else if (!strncmp(si->data + si->pos, "nullptr", MIN2(7, rl)) ||
               !strncmp(si->data + si->pos, "NULL", MIN2(4, rl))) {
        *v = PREDEFINED_CONSTANT_NULL;
        StrIterMove(si, 7);
        return true;
    } else {
        *si = saved_si;
        return false;
    }

    return true;
}

bool cReadStringLiteral(StrIter* si, StringLiteral* sl) {
    if (!si || !sl) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    // optional prefix
    cReadCharPrefix(si, &sl->prefix);

    char c = 0;
    PEEK(si, c);

    sl->s = VecInit_T(&sl->s);
    if (c == '"') {
        NEXT(si, c)

        // keep reading as many character literals as possible
        // before encountering end of string
        Char cl   = {0};
        cl.prefix = sl->prefix;
        while (c != '"' && cReadCharLiteral(si, &cl)) {
            VecPushBack(&sl->s, cl);
            PEEK(si, c);
            cl.prefix = sl->prefix;
        }

        if (c == '"') {
            StrIterNext(si);
            return true;
        } else {
            VecClear(&sl->s);
            *si = saved_si;
            return false;
        }
    } else {
        return false;
    }
}

bool cReadConstant(StrIter* si, Constant* c) {
    if (!si || !c) {
        LOG_FATAL("Invalid arguments.");
    }

    if (cReadInteger(si, &c->i) || cReadFloat(si, &c->f) || cReadEnumConstant(si, &c->e) || cReadChar(si, &c->c) ||
        cReadPredefinedConstant(si, &c->p)) {
        return true;
    }

    return false;
}

bool cReadPunctuator(StrIter* si, Punctuator* p) {
    if (!si || !p) {
        LOG_FATAL("Invalid arguments");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    size rl = StrIterRemainingLength(si);
#define MATCH(s, e)                                                                                                    \
    if (strncmp(si->data + si->pos, s, MIN2(sizeof(s) - 1, rl)) == 0) {                                                \
        StrIterMove(si, sizeof(s) - 1);                                                                                \
        *p = e;                                                                                                        \
        return true;                                                                                                   \
    }

    // 4-char punctuators
    MATCH("%:%:", PUNCTUATOR_HASHHASH); // digraph

    // 3-char punctuators
    MATCH("...", PUNCTUATOR_ELLIPSIS);
    MATCH("<<=", PUNCTUATOR_LSHIFTASSIGN);
    MATCH(">>=", PUNCTUATOR_RSHIFTASSIGN);

    // 2-char punctuators
    MATCH("->", PUNCTUATOR_ARROW);
    MATCH("++", PUNCTUATOR_PLUSPLUS);
    MATCH("--", PUNCTUATOR_MINUSMINUS);
    MATCH("<<", PUNCTUATOR_LSHIFT);
    MATCH(">>", PUNCTUATOR_RSHIFT);
    MATCH("<=", PUNCTUATOR_LE);
    MATCH(">=", PUNCTUATOR_GE);
    MATCH("==", PUNCTUATOR_EQ);
    MATCH("!=", PUNCTUATOR_NE);
    MATCH("&&", PUNCTUATOR_LOGAND);
    MATCH("||", PUNCTUATOR_LOGOR);
    MATCH("::", PUNCTUATOR_COLONCOLON);
    MATCH("*=", PUNCTUATOR_MULASSIGN);
    MATCH("/=", PUNCTUATOR_DIVASSIGN);
    MATCH("%=", PUNCTUATOR_MODASSIGN);
    MATCH("+=", PUNCTUATOR_PLUSASSIGN);
    MATCH("-=", PUNCTUATOR_MINUSASSIGN);
    MATCH("&=", PUNCTUATOR_ANDASSIGN);
    MATCH("^=", PUNCTUATOR_XORASSIGN);
    MATCH("|=", PUNCTUATOR_ORASSIGN);
    MATCH("##", PUNCTUATOR_HASHHASH); // standard version
    MATCH("<:", PUNCTUATOR_LBRACKET); // digraph
    MATCH(":>", PUNCTUATOR_RBRACKET); // digraph
    MATCH("<%", PUNCTUATOR_LBRACE);   // digraph
    MATCH("%>", PUNCTUATOR_RBRACE);   // digraph
    MATCH("%:", PUNCTUATOR_HASH);     // digraph

    // 1-char punctuators
    MATCH("[", PUNCTUATOR_LBRACKET);
    MATCH("]", PUNCTUATOR_RBRACKET);
    MATCH("(", PUNCTUATOR_LPAREN);
    MATCH(")", PUNCTUATOR_RPAREN);
    MATCH("{", PUNCTUATOR_LBRACE);
    MATCH("}", PUNCTUATOR_RBRACE);
    MATCH(".", PUNCTUATOR_DOT);
    MATCH("&", PUNCTUATOR_AND);
    MATCH("*", PUNCTUATOR_STAR);
    MATCH("+", PUNCTUATOR_PLUS);
    MATCH("-", PUNCTUATOR_MINUS);
    MATCH("~", PUNCTUATOR_TILDE);
    MATCH("!", PUNCTUATOR_BANG);
    MATCH("/", PUNCTUATOR_DIV);
    MATCH("%", PUNCTUATOR_MOD);
    MATCH("<", PUNCTUATOR_LT);
    MATCH(">", PUNCTUATOR_GT);
    MATCH("^", PUNCTUATOR_XOR);
    MATCH("|", PUNCTUATOR_OR);
    MATCH("?", PUNCTUATOR_QUESTION);
    MATCH(":", PUNCTUATOR_COLON);
    MATCH(";", PUNCTUATOR_SEMICOLON);
    MATCH("=", PUNCTUATOR_ASSIGN);
    MATCH(",", PUNCTUATOR_COMMA);
    MATCH("#", PUNCTUATOR_HASH);

#undef MATCH

    *si = saved_si;
    return false;
}

bool cReadHeaderName(StrIter* si, HeaderName* h) {
    if (!si || !h) {
        LOG_FATAL("Invalid arguments");
    }

    StrIter saved_si = *si;
    char    c        = 0;

    PEEK(si, c);

    char close = 0;

    if (c == '"') {
        close       = '"';
        h->is_local = true;
    } else if (c == '<') {
        close       = '>';
        h->is_local = false;
    } else {
        *si = saved_si;
        return false;
    }

    NEXT(si, c);

    h->name = StrInit();
    while (c != close || c != '\n') {
        StrPushBack(&h->name, c);
        NEXT(si, c);
    }

    if (c != close) {
        LOG_ERROR("Unexpected end of header name.");
        *si = saved_si;
        return false;
    }
    NEXT(si, c);

    if (!h->name.length) {
        LOG_ERROR("Invalid empty header name.");
        *si = saved_si;
        return false;
    }

    return true;
}

bool cReadPpNumber(StrIter* si, PpNumber* pn) {
    if (!si || !pn) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    char c = 0;
    PEEK(si, c);
    *pn = StrInit();

    bool has_dec = false;
    if (c == '.') {
        has_dec = true;
        StrPushBack(pn, c);
        NEXT(si, c);
    }

    while (IS_DIGIT(c) || (pn->length && IS_ALPHA(c))) {
        StrPushBack(pn, c);
        NEXT(si, c);

        if (c == '\'') {
            NEXT(si, c);
        }

        if (strchr("eEpP", c)) {
            StrPushBack(pn, c);
            NEXT(si, c);
            if (c == '+' || c == '-') {
                StrPushBack(pn, c);
                NEXT(si, c);
            } else {
                LOG_ERROR("Invalid pre-processing number. Expected a sign '+' or '-' after any of \"eEpP\"");
                *si = saved_si;
                return false;
            }
        }

        if (c == '.') {
            if (has_dec) {
                LOG_ERROR("Invalid pre-processing number. '.' appears more than once.");
                *si = saved_si;
                return false;
            }
            StrPushBack(pn, c);
            NEXT(si, c);
            has_dec = true;
        }
    }

    if (!pn->length || (has_dec && pn->length == 1)) {
        LOG_ERROR("Invalid pre-processing number.");
        *si = saved_si;
        return false;
    }

    return true;
}

bool cReadComment(StrIter* si, Str* cs) {
    if (!si || !cs) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    char c = 0;
    PEEK(si, c);

    *cs = StrInit();
    if (c == '/') {
        StrPushBack(cs, c);
        NEXT(si, c);
        if (c == '/') {
            StrPushBack(cs, c);
            NEXT(si, c);
            while (c && c != '\n') {
                StrPushBack(cs, c);
                NEXT(si, c);
            }
            return true;
        } else if (c == '*') {
            StrPushBack(cs, c);
            NEXT(si, c);
            while (c) {
                if (StrIterRemainingLength(si) >= 2 && si->data[si->pos] == '*' && si->data[si->pos + 1] == '/') {
                    StrPushBack(cs, '*');
                    StrPushBack(cs, '/');
                    StrIterMove(si, 2);
                    return true;
                }
                StrPushBack(cs, c);
                NEXT(si, c);
            }
            LOG_ERROR("Unexpected end of multiline comment.");
            StrClear(cs);
            *si = saved_si;
            return false;
        } else {
            *si = saved_si;
            StrClear(cs);
            return false;
        }
    }

    *si = saved_si;
    return false;
}

bool cReadStorageClassSpecifier(StrIter* si, StorageClassSpecifier* sc) {
    if (!si || !sc) {
        LOG_FATAL("Invalid arguments");
    }

#define MATCH(s, e)                                                                                                    \
    if (strncmp(si->data + si->pos, s, MIN2(sizeof(s) - 1, rl)) == 0) {                                                \
        StrIterMove(si, sizeof(s) - 1);                                                                                \
        *sc = e;                                                                                                       \
        return true;                                                                                                   \
    }

    StrIter saved_si = *si;
    SkipWS(si);

    size rl = StrIterRemainingLength(si);
    MATCH("auto", STORAGE_CLASS_SPECIFIER_AUTO);
    MATCH("constexpr", STORAGE_CLASS_SPECIFIER_CONSTEXPR);
    MATCH("extern", STORAGE_CLASS_SPECIFIER_EXTERN);
    MATCH("register", STORAGE_CLASS_SPECIFIER_REGISTER);

#undef MATCH
    *si = saved_si;
    return false;
}

bool cReadExpr(StrIter* si, Expr* e);
bool cReadAssignmentExpr(StrIter* si, Expr* e);
bool cReadGenericAssociationList(StrIter* si, GenericAssociations* assocs);

void ExprDeinit(Expr* e) {
    if (!e) {
        LOG_FATAL("Invalid arguments.");
    }
    // TODO:
}

void ExprDestroy(Expr* e) {
    if (!e) {
        LOG_FATAL("Invalid arguments.");
    }
    ExprDeinit(e);
    memset(e, 0, sizeof(Expr));
}

bool cReadPrimaryExpr(StrIter* si, Expr* e) {
    if (!si || !e) {
        LOG_FATAL("Invalid arguments.");
    }

    StrIter saved_si = *si;
    SkipWS(si);

    char c = 0;
    PEEK(si, c);

    if (c == '(') {
        NEXT(si, c);
        e->in_parens = NEW(Expr);
        e->type      = EXPR_TYPE_IN_PARENS;
        if (cReadExpr(si, e->in_parens)) {
            PEEK(si, c);
            if (c == ')') {
                NEXT(si, c);
                return true;
            } else {
                LOG_ERROR("Unexpected end of expression. Expected ')', got %c", c);
                goto PARSE_FAILED;
            }
        } else {
            LOG_ERROR("Expected expression.");
            goto PARSE_FAILED;
        }
    } else if (c == '_' && StrIterRemainingLength(si) >= 8 && !strncmp(si->data + si->pos, "_Generic", 8)) {
        StrIterMove(si, 8);
        SkipWS(si);
        PEEK(si, c);
        if (c == '(') {
            NEXT(si, c);
            SkipWS(si);
            e->generic_selection.expr = NEW(Expr);
            e->type                   = EXPR_TYPE_GENERIC_SELECTION;
            if (cReadAssignmentExpr(si, e->generic_selection.expr)) {
                SkipWS(si);
                PEEK(si, c);
                if (c == ',') {
                    NEXT(si, c);
                    SkipWS(si);
                    if (cReadGenericAssociationList(si, &e->generic_selection.generic_associations)) {
                        SkipWS(si);
                        PEEK(si, c);
                        if (c == ')') {
                            NEXT(si, c);
                            return true;
                        } else {
                            LOG_ERROR("Expected ')', got '%c'", c);
                            goto PARSE_FAILED;
                        }
                    } else {
                        LOG_ERROR(" Expected a generic association list.");
                        goto PARSE_FAILED;
                    }
                } else {
                    LOG_ERROR("Expected ',', got '%c'", c);
                    goto PARSE_FAILED;
                }
            } else {
                LOG_ERROR("Expected an assignment expression.");
                goto PARSE_FAILED;
            }
        } else {
            LOG_ERROR("Expected '(', got '%c'", c);
            goto PARSE_FAILED;
        }
    } else if (cReadIdentifier(si, &e->identifier)) {
        e->type = EXPR_TYPE_IDENTIFIER;
        return true;
    } else if (cReadConstant(si, &e->constant)) {
        e->type = EXPR_TYPE_CONSTANT;
        return true;
    } else if (cReadStringLiteral(si, &e->string_literal)) {
        e->type = EXPR_TYPE_STRING_LITERAL;
        return true;
    } else {
        *si = saved_si;
        return false;
    }

PARSE_FAILED:
    ExprDeinit(e);
    *si = saved_si;
    return false;
}
