#include <Misra/ParserCombinator.h>
#include <Misra/Std.h>

///
/// CalC -- a small immediate-mode calculator, the running playground for the
/// parser-combinator DSL. It parses and evaluates as it reads (no AST): every
/// rule folds its result straight into a `Num`. Variables live in the grammar
/// context, and errors are collected greedily: a broken sub-expression records a
/// `PcReport`, poisons its value, and parsing continues, so one line reports as
/// many problems as it has.
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

typedef struct Num {
    bool is_float;
    bool is_error; ///< poison: a broken sub-expression; propagates, never aborts
    union {
        i64 i;
        f64 f;
    };
} Num;

static Num num_int(i64 v) {
    return (Num) {.i = v};
}
static Num num_flt(f64 v) {
    return (Num) {.is_float = true, .f = v};
}
static Num num_poison(void) {
    return (Num) {.is_error = true};
}
static f64 num_f(Num n) {
    return n.is_float ? n.f : (f64)n.i;
}
static bool num_is_zero(Num n) {
    return n.is_float ? (n.f == 0.0) : (n.i == 0);
}

static Num num_add(Num a, Num b) {
    if (a.is_error || b.is_error)
        return num_poison();
    return (a.is_float || b.is_float) ? num_flt(num_f(a) + num_f(b)) : num_int(a.i + b.i);
}
static Num num_sub(Num a, Num b) {
    if (a.is_error || b.is_error)
        return num_poison();
    return (a.is_float || b.is_float) ? num_flt(num_f(a) - num_f(b)) : num_int(a.i - b.i);
}
static Num num_mul(Num a, Num b) {
    if (a.is_error || b.is_error)
        return num_poison();
    return (a.is_float || b.is_float) ? num_flt(num_f(a) * num_f(b)) : num_int(a.i * b.i);
}
static Num num_div(Num a, Num b) {
    if (a.is_error || b.is_error || num_is_zero(b))
        return num_poison();
    return (a.is_float || b.is_float) ? num_flt(num_f(a) / num_f(b)) : num_int(a.i / b.i);
}
static Num num_mod(Num a, Num b) {
    if (a.is_error || b.is_error || num_is_zero(b) || a.is_float || b.is_float)
        return num_poison();
    return num_int(a.i % b.i);
}
static Num num_neg(Num a) {
    if (a.is_error)
        return num_poison();
    return a.is_float ? num_flt(-a.f) : num_int(-a.i);
}

typedef struct Binding {
    Str name;
    Num value;
} Binding;
typedef Vec(Binding) Bindings;

static void binding_deinit(void *copy, const Allocator *alloc) {
    (void)alloc;
    StrDeinit(&((Binding *)copy)->name);
}
typedef Vec(PcReport) Reports;

///
/// The grammar context threaded through every parser as `ctx`: the variable
/// environment plus the diagnostics sink the `PcReport*` macros append to. Both
/// are containers that carry their own allocator.
///
typedef struct PcParserCtx {
    Bindings vars;
    Reports  reports;
} PcParserCtx;

typedef struct {
    u64 len;
} PcParserCtxMark;

static PcParserCtxMark PcParserCtxSnapshot(PcParserCtx *ctx) {
    return (PcParserCtxMark) {.len = VecLen(&ctx->vars)};
}

static void PcParserCtxRollback(PcParserCtx *ctx, PcParserCtxMark mark) {
    VecResize(&ctx->vars, mark.len);
}

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
    PcRecognizeZeroOrMore(WsChar);
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

/// number = digit+ ( '.' digit+ )?  -- accumulated into a Str, converted by the
/// library parser; a value that does not fit is reported and poisoned, not aborted
PcParser(Number, Num) {
    char d;
    bool is_float = false;
    PcSeq() {
        StrInitStack(tok, 64) {
            PcMatchOneOrMore(DigitCh, &d) {
                StrPushBack(&tok, d);
            }
            PcOpt(CharIfExist, '.', &d) {
                is_float = true;
                StrPushBack(&tok, d);
                PcMatchZeroOrMore(DigitCh, &d) {
                    StrPushBack(&tok, d);
                }
            }
            if (is_float) {
                f64 f = 0;
                if (StrToF64(&tok, &f, NULL))
                    *value = num_flt(f);
                else {
                    PcReportError("malformed number");
                    *value = num_poison();
                }
            } else {
                u64 u = 0;
                if (StrToU64(&tok, &u, NULL))
                    *value = num_int((i64)u);
                else {
                    PcReportError("number does not fit in an integer");
                    *value = num_poison();
                }
            }
        }
        PcRecognize(Ws);
    }
}

/// identifier = letter+ ; the matched letters are appended into the caller's Str
PcParser(Identifier, Str) {
    char l;
    PcSeq() {
        PcMatchOneOrMore(LetterCh, &l) {
            StrPushBack(value, l);
        }
        PcRecognize(Ws);
    }
}

/// variable reference: look the name up; undefined -> report and poison (keep going)
PcParser(VarRef, Num) {
    PcSeq() {
        StrInitStack(name, 64) {
            PcMatch(Identifier, &name);
            Num *slot = NULL;
            VecForeachPtrReverse(&ctx->vars, b) if (StrCmp(&b->name, &name) == 0) {
                slot = &b->value;
                break;
            }
            if (slot)
                *value = *slot;
            else {
                PcReportError("undefined variable");
                *value = num_poison();
            }
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
        PcElse() {
            PcReportErrorHere("expected an operand");
            PcRecover(c, c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == ')');
            *value = num_poison();
        }
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
        PcMatchZeroOrMore(MulOp, &op) {
            PcMatch(Unary, &rhs);
            bool fresh = !value->is_error && !rhs.is_error;
            if (fresh && (op == '/' || op == '%') && num_is_zero(rhs)) {
                PcReportError("division by zero");
                *value = num_poison();
            } else if (fresh && op == '%' && (value->is_float || rhs.is_float)) {
                PcReportError("modulo needs integer operands");
                *value = num_poison();
            } else
                *value = (op == '*') ? num_mul(*value, rhs) : (op == '/') ? num_div(*value, rhs) : num_mod(*value, rhs);
        }
    }
}

PcParser(Additive, Num) {
    char op;
    Num  rhs;
    PcSeq() {
        PcMatch(Multiplicative, value);
        PcMatchZeroOrMore(AddOp, &op) {
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
            if (!value->is_error) {
                Binding b = {.name = StrInitFromStr(&name, VecAllocator(&ctx->vars)), .value = *value};
                VecPushBack(&ctx->vars, b);
            }
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
static void seed(Bindings *vars, HeapAllocator *heap, Zstr name, Num v) {
    Binding b = {.name = StrInitFromCstr(name, ZstrLen(name), heap), .value = v};
    VecPushBack(vars, b);
}

static Zstr level_word(PcReportLevel level) {
    switch (level) {
        case PC_REPORT_ERROR :
            return "error";
        case PC_REPORT_WARN :
            return "warning";
        default :
            return "note";
    }
}

/// draw each report rustc-style: level + message, the source line, then carets
/// under its span. Parsers never touch this -- they only record a `PcReport`.
static void render(Str *src, Reports *reports) {
    const char *bytes = StrBegin(src);
    for (u64 r = 0; r < VecLen(reports); r++) {
        PcReport rep = VecAt(reports, r);
        u64      end = rep.end;
        while (end > rep.start && (bytes[end - 1] == ' ' || bytes[end - 1] == '\t'))
            end--;
        WriteFmtLn("{}: {}", level_word(rep.level), rep.message);
        WriteFmtLn("  {}", *src);
        StrInitStack(caret, 512) {
            char space = ' ', hat = '^';
            for (u64 c = 0; c < rep.start; c++)
                StrPushBackR(&caret, space);
            for (u64 c = rep.start; c < end; c++)
                StrPushBackR(&caret, hat);
            WriteFmtLn("  {}", caret);
        }
    }
}

static void eval_line(PcParserCtx *ctx, Str *line) {
    VecClear(&ctx->reports);
    StrIter        in  = StrIterFromStr(*line);
    Num            out = {0};
    PcParserStatus st  = PcRun(Calc, &in, &out);
    if (VecLen(&ctx->reports) > 0)
        render(line, &ctx->reports);
    else if ((st & PC_PARSER_STATUS_SUCCESS) && !StrIterRemainingLength(&in)) {
        if (out.is_float)
            WriteFmtLn("{}", out.f);
        else
            WriteFmtLn("{}", out.i);
    } else
        WriteFmtLn("error at column {}", StrIterIndex(&in) + 1);
}

int main(int argc, char **argv) {
    HeapAllocator heap = HeapAllocatorInit();

    PcParserCtx ctx = {
        .vars    = VecInitWithDeepCopy(NULL, binding_deinit, &heap),
        .reports = VecInit(&heap),
    };
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
            Str cmd = StrInitFromCstr(expr, ZstrLen(expr), &heap);
            eval_line(&ctx, &cmd);
            StrDeinit(&cmd);
        } else {
            File fin = FileStdin();
            StrInitStack(line, 4096) {
                for (;;) {
                    WriteFmt("> ");
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
                        eval_line(&ctx, &line);
                    if (eof) {
                        WriteFmtLn("");
                        break;
                    }
                }
            }
        }
    }

    ArgParseDeinit(&args);
    VecDeinit(&ctx.reports);
    VecDeinit(&ctx.vars);
    HeapAllocatorDeinit(&heap);
    return status;
}
