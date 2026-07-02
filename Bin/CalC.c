#include <Misra/ParserCombinator.h>
#include <Misra/Std.h>

///
/// CalC -- a small immediate-mode calculator, the running playground for the
/// parser-combinator DSL. It parses and evaluates as it reads (no AST): every
/// rule folds its result straight into a `Num`. Variables live in the grammar
/// context, so a result can be named and reused on a later line.
///
///   line           = assignment | expr
///   assignment     = identifier '=' expr           (stores into ctx->vars)
///   expr           = additive
///   additive       = multiplicative ( ('+'|'-') multiplicative )*   left-assoc
///   multiplicative = unary          ( ('*'|'/'|'%') unary )*        left-assoc
///   unary          = ('-'|'+')? atom
///   atom           = '(' expr ')' | identifier | number
///   number         = digit+ ( '.' digit+ )?        (int, or float if a '.')
///   identifier     = letter+                        (a variable name)
///
/// Whitespace is scannerless: each token consumes trailing whitespace and the
/// top rule skips leading whitespace once. `pi` and `e` are seeded variables.
///

typedef struct Num {
    bool is_float;
    union {
        i64 i;
        f64 f;
    };
} Num;

static Num num_int(i64 v) {
    return (Num) {.is_float = false, .i = v};
}
static Num num_flt(f64 v) {
    return (Num) {.is_float = true, .f = v};
}
static f64 num_f(Num n) {
    return n.is_float ? n.f : (f64)n.i;
}

static Num num_add(Num a, Num b) {
    return (a.is_float || b.is_float) ? num_flt(num_f(a) + num_f(b)) : num_int(a.i + b.i);
}
static Num num_sub(Num a, Num b) {
    return (a.is_float || b.is_float) ? num_flt(num_f(a) - num_f(b)) : num_int(a.i - b.i);
}
static Num num_mul(Num a, Num b) {
    return (a.is_float || b.is_float) ? num_flt(num_f(a) * num_f(b)) : num_int(a.i * b.i);
}
static Num num_div(Num a, Num b) {
    if (a.is_float || b.is_float) {
        f64 d = num_f(b);
        return num_flt(d != 0.0 ? num_f(a) / d : 0.0);
    }
    return num_int(b.i != 0 ? a.i / b.i : 0);
}
static Num num_mod(Num a, Num b) {
    if (a.is_float || b.is_float)
        return num_flt(0.0);
    return num_int(b.i != 0 ? a.i % b.i : 0);
}
static Num num_neg(Num a) {
    return a.is_float ? num_flt(-a.f) : num_int(-a.i);
}

typedef Map(Str, Num) Vars;

///
/// The grammar context threaded through every parser as `ctx`. For CalC it is
/// the variable environment; the map owns its allocator (and deep-copies names),
/// so a later grammar's `ctx` is where an AST root / arena would join it.
///
typedef struct ParserCtx {
    Vars vars;
} ParserCtx;

PcRecognizer(WsChar);
PcRecognizer(Ws);
PcParser(DigitCh, char);
PcParser(LetterCh, char);
PcParser(CharIfExist, char, char);
PcParser(Sym, char, char);
PcParser(SignCh, char);
PcParser(Number, Num);
PcParser(Identifier, Str);
PcParser(VarRef, Num);
PcParser(AddOp, char);
PcParser(MulOp, char);
PcParser(Parenthesized, Num);
PcParser(Atom, Num);
PcParser(Unary, Num);
PcParser(Multiplicative, Num);
PcParser(Additive, Num);
PcParser(Expr, Num);
PcParser(Assignment, Num);
PcParser(Line, Num);
PcParser(Calc, Num);

PcRecognizer(WsChar) {
    PcSatisfyChar(c, c == ' ' || c == '\t' || c == '\n' || c == '\r') {}
}

PcRecognizer(Ws) {
    PcRecognizeMany(WsChar);
}

PcParser(DigitCh, char) {
    PcSatisfyChar(c, c >= '0' && c <= '9') {
        *value = c;
    }
}

PcParser(LetterCh, char) {
    PcSatisfyChar(c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
        *value = c;
    }
}

/// fundamental: match a specific char (the `expect` input), capture it
PcParser(CharIfExist, char, char) {
    PcSatisfyChar(c, c == expect) {
        *value = c;
    }
}

/// a token char: match `expect`, then eat trailing whitespace
PcParser(Sym, char, char) {
    PcSeq() {
        PcMatch(CharIfExist, expect, value);
        PcRecognize(Ws);
    }
}

PcParser(SignCh, char) {
    PcChoice() {
        PcAlt(Sym, '-', value);
        PcAlt(Sym, '+', value);
    }
}

/// number = digit+ ( '.' digit+ )?  -- accumulated char-by-char into a Str, then
/// converted by the library's own parser; int unless a '.' makes it a float
PcParser(Number, Num) {
    char d;
    bool is_float = false;
    PcSeq() {
        StrInitStack(tok, 64) {
            PcMatch(DigitCh, &d);
            StrPushBack(&tok, d);
            PcMany(DigitCh, &d) {
                StrPushBack(&tok, d);
            }
            PcOpt(CharIfExist, '.', &d) {
                is_float = true;
                StrPushBack(&tok, d);
                PcMany(DigitCh, &d) {
                    StrPushBack(&tok, d);
                }
            }
            if (is_float) {
                f64 f = 0;
                StrToF64(&tok, &f, NULL);
                *value = num_flt(f);
            } else {
                u64 u = 0;
                StrToU64(&tok, &u, NULL);
                *value = num_int((i64)u);
            }
        }
        PcRecognize(Ws);
    }
}

/// identifier = letter+ ; the matched letters are appended into the caller's Str
PcParser(Identifier, Str) {
    char l;
    PcSeq() {
        PcMatch(LetterCh, &l);
        StrPushBack(value, l);
        PcMany(LetterCh, &l) {
            StrPushBack(value, l);
        }
        PcRecognize(Ws);
    }
}

/// variable reference: look the name up in the environment; undefined -> reject
PcParser(VarRef, Num) {
    PcSeq() {
        StrInitStack(name, 64) {
            PcMatch(Identifier, &name);
            Num *slot = MapGetFirstPtr(&ctx->vars, name);
            if (!slot)
                PcReject();
            *value = *slot;
        }
    }
}

PcParser(AddOp, char) {
    PcChoice() {
        PcAlt(Sym, '+', value);
        PcAlt(Sym, '-', value);
    }
}

PcParser(MulOp, char) {
    PcChoice() {
        PcAlt(Sym, '*', value);
        PcAlt(Sym, '/', value);
        PcAlt(Sym, '%', value);
    }
}

PcParser(Parenthesized, Num) {
    char paren;
    PcSeq() {
        PcMatch(Sym, '(', &paren);
        PcMatch(Expr, value);
        PcMatch(Sym, ')', &paren);
    }
}

PcParser(Atom, Num) {
    PcChoice() {
        PcAlt(Parenthesized, value);
        PcAlt(VarRef, value);
        PcAlt(Number, value);
    }
}

PcParser(Unary, Num) {
    char sign = '+';
    PcSeq() {
        PcOpt(SignCh, &sign) {}
        PcMatch(Atom, value);
        if (sign == '-')
            *value = num_neg(*value);
    }
}

PcParser(Multiplicative, Num) {
    char op;
    Num  rhs;
    PcSeq() {
        PcMatch(Unary, value);
        PcMany(MulOp, &op) {
            PcMatch(Unary, &rhs);
            *value = (op == '*') ? num_mul(*value, rhs) : (op == '/') ? num_div(*value, rhs) : num_mod(*value, rhs);
        }
    }
}

PcParser(Additive, Num) {
    char op;
    Num  rhs;
    PcSeq() {
        PcMatch(Multiplicative, value);
        PcMany(AddOp, &op) {
            PcMatch(Multiplicative, &rhs);
            *value = (op == '+') ? num_add(*value, rhs) : num_sub(*value, rhs);
        }
    }
}

PcParser(Expr, Num) {
    return PcParse(Additive, value);
}

/// assignment = identifier '=' expr ; evaluate, then bind the name (the map copies it)
PcParser(Assignment, Num) {
    char eq;
    PcSeq() {
        StrInitStack(name, 64) {
            PcMatch(Identifier, &name);
            PcMatch(Sym, '=', &eq);
            PcMatch(Expr, value);
            (void)MapInsertR(&ctx->vars, name, *value);
        }
    }
}

PcParser(Line, Num) {
    PcChoice() {
        PcTryAlt(Assignment, value);
        PcAlt(Expr, value);
    }
}

PcParser(Calc, Num) {
    PcSeq() {
        PcRecognize(Ws);
        PcMatch(Line, value);
    }
}

/// bind a predefined variable; the map deep-copies the key, so the temporary is freed
static void seed(Vars *vars, HeapAllocator *heap, Zstr name, Num v) {
    Str key = StrInitFromCstr(name, ZstrLen(name), heap);
    (void)MapInsertR(vars, key, v);
    StrDeinit(&key);
}

static void eval_line(ParserCtx *ctx, const char *src, u64 len) {
    StrIter        in  = StrIterFromCstr((char *)src, len);
    Num            out = {0};
    PcParserStatus st  = PcRun(Calc, &in, &out);
    if ((st & PC_PARSER_STATUS_SUCCESS) && in.pos == in.length) {
        if (out.is_float)
            LOG_INFO("{}", out.f);
        else
            LOG_INFO("{}", out.i);
    } else {
        LOG_INFO("error at column {}", in.pos + 1);
    }
}

int main(int argc, char **argv) {
    HeapAllocator heap = HeapAllocatorInit();

    ParserCtx ctx = {.vars = MapInitWithDeepCopy(str_hash, str_compare, str_init_copy, str_deinit, NULL, NULL, &heap)};
    seed(&ctx.vars, &heap, "pi", num_flt(3.14159265358979323846));
    seed(&ctx.vars, &heap, "e", num_flt(2.71828182845904523536));

    ArgParse args = ArgParseInit("calc", "an immediate-mode parser-combinator calculator", &heap);
    Zstr     expr = NULL;
    ArgOptional(&args, "-c", "--command", &expr, "evaluate one expression and exit");
    ArgRun rc = ArgParseRun(&args, argc, argv);

    int status = 0;
    if (rc == ARG_RUN_ERROR) {
        status = 1;
    } else if (rc != ARG_RUN_HELP) {
        if (expr != NULL) {
            eval_line(&ctx, expr, ZstrLen(expr));
        } else {
            File fin = FileStdin();
            StrInitStack(line, 4096) {
                for (;;) {
                    StrClear(&line);
                    bool eof = false;
                    for (;;) {
                        char c;
                        i64  n = FileRead(&fin, &c, 1);
                        if (n <= 0) {
                            eof = true;
                            break;
                        }
                        if (c == '\n')
                            break;
                        StrPushBack(&line, c);
                    }
                    if (StrLen(&line) > 0)
                        eval_line(&ctx, StrBegin(&line), StrLen(&line));
                    if (eof)
                        break;
                }
            }
        }
    }

    ArgParseDeinit(&args);
    MapDeinit(&ctx.vars);
    HeapAllocatorDeinit(&heap);
    return status;
}
