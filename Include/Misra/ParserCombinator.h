/// file      : misra/parsercombinator.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Parser combinator provides arbitrary parser creation capabilities.
///
///   Definition:
///     A parser combinator is a high order function over parsers that takes
///     a sequence (in the sense that ordering may matter) of parsers and returns
///     a new parser that is derived from the provided parsers.
///
///   Parser:
///     type Parser a = String -> Result (a, String)
///     A parser takes a string and returns a value of type a, and the remaining string
///     left to parse.
///
///   Algebraic interfaces:
///     - Functor: Take the result of a parser and transform it without changing the parser
///         itself. Like parsing a digit sequence and converting it to Int or Float.
///     - Applicative: Run parsers in sequence where later parsers dont depend on
///         earlier parsers. [Context-Free nature]
///     - Monad: Run parsers in sequence where later parser depends on the earlier
///         parser's value. [Context-Sensitive nature]
///     - Alternative: Choice and repetition.
///

/// Last year I was tasked to write a C++ demangler in pure C and was getting paid to do it.
/// I knew this person, and was like a mentor to me, I failed them miserably. The parser I wrote
/// was PEG (as I learn now), which was basically a top-down recursive-descent left-to-right parser that I built using
/// very basic macro-defined parser combinators. I really respect this person, and I lost
/// their trust and respect, which hurts whenever my internal dialogue reminds me of it.
/// If I get this right this time, this part of my code is my homage to them.
///
/// There were macros to define parser rules and then to use the rules. There were macros
/// to help you parse in a sequence or alternation and consume the input along the way.
///
/// The parser was in the end designed in such a way that it would parse the input as it
/// is consumed, zero tokenization. It was able to automatically backtrack because of it's
/// design, and I was very proud of it. A few things came biting me in the ass very badly
/// later down the line :
/// - [Heavy macro usage], in the sense that macro was expanding to multiple lines
///     and was not dispatching code to handler functions. The issue with this is
///     that debugging is a real PITA, because debuggers dont expand macros.
///     Essentially, single-stepping code in GDB to watch the parser run was hell!
/// - [Left-recursive] nature of the C++ name mangling grammar itself. Left recursive
///     grammar is a real hell if you are writing a recursive descent parser and you
///     let this nature of grammar slip away from your eyes. There are multiple normalized
///     forms that can  help you but it's still a pain, for very large grammar (that C++ name
///     mangling grammar already is) is a real pain.
/// - [Backtracking] parsers have issue with `Alternative` combinators. They try the next
///     in alternation when the first one fails. Libraries like Parsec in Haskell split this
///     into two behaviors :
///     - If parsing the very first in an alternation fails and it failed by not cosuming a single
///       token, then do not parse anything in the alternative sequence at all.
///     - You can force the alternation parsing anyhow by a `try` keyword that forces the combinator
///       to continue trying alternatives.
///     I believe this would've solved a big chunk of my issues.
/// - [Ordering of alternatives] mattered in my parser. Sometimes I wanted the parser to take
///     a longer parsing route, but the way I wrote the grammar using combinators made the parser
///     take the first smallest route it was able to parse. This created a lot of debugging time
///     which was already hard.
/// - Normalized grammars are hard to make any sense of! If possible, normalize parts of the grammars.
///     Depending on how long each rule is, the number of rules blow up and stop making any sense.
///
/// Now, I'm standing on the same cliff, and this time I want to make a different decision.
///
/// I'm using an LLM to learn how other languages and libraries have achieved the same thing.
/// I know this is possible, and I just have to make some tweaks in the way I designed my
/// combinators the last time. The approach this time is to try to imitate how functional
/// languages achieve this same thing. The functional part can be emulated using macros
/// as code-generators hopefully.
///
/// On the [Heavy macro usage] point, I know that I use lots of macros in this library.
/// That is mainly because I know the real reason debuggability vanishes is not because
/// it's a macro, but because I used macros in a way that it hid the implementation.
/// In this library I made sure that the macros are very thin wrappers and are essentially
/// code-dispatchers first and then code-generators. This ensures that most of the buggy
/// remains debuggable through a function that actually handles the implementation.
///

#ifndef MISRA_PARSER_COMBINATOR_H
#define MISRA_PARSER_COMBINATOR_H

#include <Misra/Types.h>
#include <Misra/Std/Utility/StrIter.h>
#include <Misra/Std/Zstr.h>

///
/// A parser combinator does not only need a way to parse a string. It also needs to convert
/// it to a representation that any phase after parsing can use and analyze. This may heavily
/// depend on the language being parsed.
///
/// The mechanism this library uses is a per-grammar context (`ParserCtx`, see below) threaded
/// through every parser. The grammar author places their in-progress structure (an AST root,
/// symbol tables, ...) and an allocator on it, so parsers can build long-lived output as the
/// input is consumed -- immediate-mode parsing with no separate tree-walking pass.
///

///
/// This is the design philosophy behind the macros below:
///
/// - A parser is an ordinary `static inline` function. `PcParser` writes its signature so it
///   is steppable in a debugger; the only thing a grammar body ever calls into is another
///   parser (through `PcMatch`/`PcAlt`/...), never opaque combinator glue.
/// - `StrIter` (the input stream) and `PcParserStatus` (the result) are internal constructs.
///   They appear only inside the block frames, the arms, and the two atoms -- a grammar rule
///   built on top of these never names either one.
/// - Scannerless: parsers consume the source character-by-character through a `StrIter`; there
///   is no tokenization pass.
///

///
/// Status returned by every parser. Two independent bits: whether the parse succeeded, and
/// whether any input was consumed (which stays meaningful even when the parse later fails,
/// because it decides commit-vs-backtrack in an enclosing choice). Grammar bodies never read
/// or write this directly; the macros construct and inspect it.
///
typedef u32 PcParserStatus;

enum {
    /// The parse did not succeed.
    PC_PARSER_STATUS_FAILED = 0,
    /// The parse succeeded.
    PC_PARSER_STATUS_SUCCESS = 1,
    /// Input was advanced (matters for backtracking even on a failed parse).
    PC_PARSER_STATUS_CONSUMED = 1u << 1
};

///
/// Diagnostics. Error reporting is a shared model, not per-grammar: a parser records what it has
/// to say (a level + a span into the input + its own words) and stays out of the rendering. The
/// grammar author never draws a caret or computes a column -- a renderer turns these into output.
/// Reporting does NOT unwind (it is not "raise"): a rule records a `PcReport`, substitutes a
/// placeholder/poison value, and keeps parsing, so one input yields as many errors as possible.
///
typedef enum {
    REPORT_INFO,
    REPORT_WARN,
    REPORT_ERROR
} ReportLevel;

typedef struct PcReport {
    u64         start;   ///< span start, an index into the input (see `IterIndex`)
    u64         end;     ///< span end, exclusive
    ReportLevel level;
    Zstr        message; ///< the parser's words; the specifics show under the caret
} PcReport;

///
/// ParserCtx is NOT defined here. Each grammar (a C parser, a JSON parser, ...) declares its
/// own `typedef struct { ... } ParserCtx;` before using `PcParser`, and threads a pointer to
/// it through every parser as `ctx`. It carries whatever the grammar needs to build output as
/// it parses, typically:
///   - an `Allocator *` the parsers allocate AST nodes from (giving them a lifetime beyond the
///     parse),
///   - a `Vec(PcReport) reports;` sink the `PcReport*` macros append to (required by those
///     macros), and
///   - the growing root of the AST / a symbol table for context-sensitive rules.
/// The combinator only forwards `ctx`; it never looks inside it.
///

/// Mangled name of the parser function for rule `Name`.
#define PcGenParserName(Name) pc_parser_##Name

/// Portable "may be unused" attribute for the `ctx` parameter: not every parser needs the
/// context (recognizers, pure arithmetic, ...), and those should not warn.
#if defined(__GNUC__) || defined(__clang__)
#    define PC_MAYBE_UNUSED __attribute__((unused))
#else
#    define PC_MAYBE_UNUSED
#endif

///
/// Invoke an output-producing parser by name, threading the stream `in` and context `ctx`
/// automatically. Overloaded by arity, mirroring `PcParser` -- the last argument is the output:
///
///   PcParse(Name, Out)      -> pc_parser_Name(in, ctx, Out)
///   PcParse(Name, In, Out)  -> pc_parser_Name(in, ctx, In, Out)
///
/// Used internally by `PcMatch`/`PcAlt`/`PcMany`/`PcOpt`; call it directly only to delegate one
/// rule wholesale to another (`return PcParse(Other, value);`). Because an output parser always
/// carries the output argument, this family never sees an empty argument list -- so there is no
/// `__VA_OPT__` here (the recognizer family below has none either, for the same reason: every
/// arity writes its call explicitly).
///
#define PcParse(...)             OVERLOAD(PcParse, __VA_ARGS__)
#define PcParse_2(Name, Out)     PcGenParserName(Name)(in, ctx, Out)
#define PcParse_3(Name, In, Out) PcGenParserName(Name)(in, ctx, In, Out)

///
/// The recognizer family: parsers that produce NO output -- they only succeed/fail (and consume),
/// the "validator" style. Identical to the `PcParser`/`PcParse` family with the trailing output
/// slot removed, so the arities shift down by one: 1-arg (no input) or 2-arg (an input to match).
///
///   PcRecognizer(Name)       -> (StrIter *in, ParserCtx *ctx)               define, no input
///   PcRecognizer(Name, InT)  -> (StrIter *in, ParserCtx *ctx, InT expect)   define, one input
///   PcRecognize(Name)        -> pc_parser_Name(in, ctx)                      call, no input
///   PcRecognize(Name, In)    -> pc_parser_Name(in, ctx, In)                  call, one input
///
/// SUCCESS: The body returns `PC_PARSER_STATUS_SUCCESS` (| `CONSUMED` when it advanced `in`).
/// FAILURE: The body returns `PC_PARSER_STATUS_FAILED` (| `CONSUMED` when it advanced then failed).
///
#define PcRecognizer(...) OVERLOAD(PcRecognizer, __VA_ARGS__)
#define PcRecognizer_1(Name)                                                                                           \
    static inline PcParserStatus PcGenParserName(Name)(StrIter * in, ParserCtx * ctx PC_MAYBE_UNUSED)
#define PcRecognizer_2(Name, InT)                                                                                      \
    static inline PcParserStatus PcGenParserName(Name)(StrIter * in, ParserCtx * ctx PC_MAYBE_UNUSED, InT expect)

#define PcRecognize(...)        OVERLOAD(PcRecognize, __VA_ARGS__)
#define PcRecognize_1(Name)     PcGenParserName(Name)(in, ctx)
#define PcRecognize_2(Name, In) PcGenParserName(Name)(in, ctx, In)

///
/// Run a parser from OUTSIDE a parser body -- the entry point a driver (a REPL, a CLI) uses to
/// kick off the top rule, so it never spells the mangled `pc_parser_<Name>` by hand. `PcParse`
/// threads the ambient stream `in` for a rule; a driver owns its own stream, so it passes a
/// pointer to it explicitly. `ctx` is still taken from the enclosing scope. The remaining args
/// (the outputs) forward to the parser call.
///
///   PcRun(Name, &in, &out)  -> pc_parser_Name(&in, ctx, &out)
///
#define PcRun(Name, InPtr, ...) PcGenParserName(Name)(InPtr, ctx, __VA_ARGS__)

///
/// Define (or, when followed by `;`, forward-declare) a parser. Overloaded by arity:
///
///   PcParser(Name, BuildT)       -> (StrIter *in, ParserCtx *ctx, BuildT *value)
///   PcParser(Name, InT, BuildT)  -> (StrIter *in, ParserCtx *ctx, InT expect, BuildT *value)
///
/// The names a body reads are `in` (stream), `ctx` (grammar context), `expect` (the input, in
/// the 3-arg form), and `value` (the output). The consumer must have a `ParserCtx` type in scope.
///
/// SUCCESS: The body returns `PC_PARSER_STATUS_SUCCESS` (or'd with `CONSUMED` when it advanced
///          `in`); the parsed result has been written to `*value`.
/// FAILURE: The body returns `PC_PARSER_STATUS_FAILED` (or'd with `CONSUMED` when it advanced
///          before failing); `*value` is left unspecified.
///
#define PcParser(...) OVERLOAD(PcParser, __VA_ARGS__)
#define PcParser_2(Name, BuildT)                                                                                       \
    static inline PcParserStatus PcGenParserName(Name)(StrIter * in, ParserCtx * ctx PC_MAYBE_UNUSED, BuildT * value)
#define PcParser_3(Name, InT, BuildT)                                                                                  \
    static inline PcParserStatus                                                                                       \
        PcGenParserName(Name)(StrIter * in, ParserCtx * ctx PC_MAYBE_UNUSED, InT expect, BuildT * value)

///
/// The consumed bit for a parser, derived from the stream position against a snapshot: a parse
/// that rewound leaves `pos` unchanged (not consumed); one that committed leaves it advanced
/// (consumed). This is why backtracking accounts for itself -- no separate flag is threaded.
///
#define PC_CONSUMED(start) (in->pos != (start).pos ? PC_PARSER_STATUS_CONSUMED : 0u)

///
/// Block frames -- the whole body of a rule is one of these (or a bare `PcSatisfy*`/delegation).
/// Each scopes its bookkeeping in the `for`-init struct so frames nest and sit side by side
/// without name clashes, and each owns the rule's `return`.
///
/// PcSeq: run the steps (`PcMatch`/`PcMany`/...) in order; a failing step returns early. If the
/// body runs to the end, the rule succeeds (consuming whatever the steps consumed).
///
/// SUCCESS: All steps matched; returns SUCCESS with the accumulated CONSUMED bit.
/// FAILURE: A step failed and already returned; control never reaches the frame's success return.
///
#define PcSeq()                                                                                                        \
    for (struct {                                                                                                      \
             StrIter start;                                                                                            \
             bool    ran;                                                                                              \
         } pc_seq = {*in, false};                                                                                      \
         ;                                                                                                             \
         pc_seq.ran = true)                                                                                            \
        if (pc_seq.ran)                                                                                                \
            return PC_CONSUMED(pc_seq.start) | PC_PARSER_STATUS_SUCCESS;                                               \
        else

///
/// PcChoice: try the arms (`PcAlt`/`PcTryAlt`/...) top to bottom; the first that matches wins and
/// its value is the rule's value. If none match, the rule fails.
///
/// SUCCESS: An arm matched; returns SUCCESS with CONSUMED reflecting the arm.
/// FAILURE: No arm matched; returns FAILED, CONSUMED set if the rule advanced before failing
///          (a committed arm that consumed then failed) -- driving the caller's commit-vs-backtrack.
///
#define PcChoice()                                                                                                     \
    for (struct {                                                                                                      \
             StrIter        mark;                                                                                      \
             PcParserStatus st;                                                                                        \
             bool           ran, matched, done;                                                                        \
         } pc_ch = {*in, 0, false, false, false};                                                                      \
         ;                                                                                                             \
         pc_ch.ran = true)                                                                                             \
        if (pc_ch.ran)                                                                                                 \
            return (pc_ch.matched ? PC_PARSER_STATUS_SUCCESS : PC_PARSER_STATUS_FAILED) | PC_CONSUMED(pc_ch.mark);     \
        else

///
/// Sequence steps (used inside `PcSeq`). Both forward their extra args straight to the parser
/// call, so the parser's own signature validates arity and types at the call site.
///
/// PcMatch: run parser `Name`; on failure, fail the whole rule (returning with the sequence's
/// consumed bit). On success, continue with the next step.
///
#define PcMatch(Name, ...)                                                                                             \
    do {                                                                                                               \
        if (!(PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS))                                                  \
            return PC_CONSUMED(pc_seq.start) | PC_PARSER_STATUS_FAILED;                                                \
    } while (0)

///
/// PcReject: fail the current rule outright from its body -- the escape hatch for a
/// context-sensitive rejection no combinator can express (an undefined variable, a name that is
/// not a typedef, ...). Reports the sequence's consumed bit, exactly as a failing `PcMatch` would,
/// so an enclosing choice commits/backtracks correctly. A `PcSeq` step, like `PcMatch`; a rule
/// uses it so it never has to name the consumed bit or the status itself.
///
#define PcReject() return PC_CONSUMED(pc_seq.start) | PC_PARSER_STATUS_FAILED

///
/// Record a diagnostic and KEEP GOING -- the greedy alternative to `PcReject`. Appends a
/// `PcReport` (spanning what the current `PcSeq` frame has consumed so far) to `ctx->reports`,
/// then returns nothing, so the rule substitutes a poison value and parsing continues to collect
/// more errors. `ctx` must carry a `Vec(PcReport) reports;`. A `PcSeq` step, like `PcReject`.
///
#define PcReportError(Msg) PC_REPORT(REPORT_ERROR, Msg)
#define PcReportWarn(Msg)  PC_REPORT(REPORT_WARN, Msg)
#define PcReportInfo(Msg)  PC_REPORT(REPORT_INFO, Msg)
#define PC_REPORT(Level, Msg)                                                                                          \
    VecPushBack(                                                                                                       \
        &ctx->reports,                                                                                                 \
        ((PcReport) {.start = (pc_seq.start).pos, .end = IterIndex(in), .level = (Level), .message = (Msg)})           \
    )

///
/// PcMany: zero-or-more. Run parser `Name` repeatedly; the body runs once per match. A match
/// that consumed nothing would spin forever, so the loop stops on it (it never aborts and never
/// allocates); the failing/empty iteration is rewound so its bytes are not eaten. Always
/// "succeeds" -- it is a step that simply stops.
///
#define PcMany(Name, ...)                                                                                              \
    for (StrIter UNPL(pc_many_) = *in;                                                                                 \
         (UNPL(pc_many_) = *in,                                                                                        \
         (PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS) && in->pos != UNPL(pc_many_).pos) ?                   \
             true :                                                                                                    \
             ((*in = UNPL(pc_many_)), false);)

///
/// PcOpt: zero-or-one. Try parser `Name`; if it matches, run the body once (the parsed value is
/// available through whatever output pointer was passed). If it does not match, rewind and skip
/// the body. Never fails the sequence -- it is a step that is simply optional. Like `PcMany`, it
/// is one `for` statement with its bookkeeping in the loop scope, so it nests, sits next to other
/// steps on the same line, and takes an unbraced body without a dangling-`else` surprise.
///
#define PcOpt(Name, ...)                                                                                               \
    for (struct {                                                                                                      \
             StrIter mark;                                                                                             \
             bool    ran;                                                                                              \
         } UNPL(pc_opt_) = {*in, false};                                                                               \
         !UNPL(pc_opt_).ran &&                                                                                         \
         ((UNPL(pc_opt_).ran = true),                                                                                  \
          (PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS) ? true : (*in = UNPL(pc_opt_).mark, false));)

///
/// PcRecognizeMany: the recognizer-flavoured `PcMany` -- a whole recognizer body that runs a
/// sub-recognizer zero-or-more times and then returns success. Terminal (it owns the `return`), so
/// it is the entire body of a "recognize zero-or-more of one thing" parser (e.g. whitespace), not
/// a mid-sequence step. Same empty-match guard as `PcMany`: a non-consuming match stops the loop
/// and is rewound, so it can never spin. Overloaded 1-arg / 2-arg like `PcRecognize`.
///
#define PcRecognizeMany(...)        OVERLOAD(PcRecognizeMany, __VA_ARGS__)
#define PcRecognizeMany_1(Name)     PC_RECOGNIZE_MANY(PcRecognize_1(Name))
#define PcRecognizeMany_2(Name, In) PC_RECOGNIZE_MANY(PcRecognize_2(Name, In))
#define PC_RECOGNIZE_MANY(call)                                                                                        \
    do {                                                                                                               \
        StrIter UNPL(pc_rm_start) = *in;                                                                               \
        for (StrIter UNPL(pc_rm_mark) = *in;                                                                           \
             (UNPL(pc_rm_mark) = *in, ((call) & PC_PARSER_STATUS_SUCCESS) && in->pos != UNPL(pc_rm_mark).pos) ?        \
                 true :                                                                                                \
                 ((*in = UNPL(pc_rm_mark)), false);)                                                                   \
            ;                                                                                                          \
        return PC_CONSUMED(UNPL(pc_rm_start)) | PC_PARSER_STATUS_SUCCESS;                                              \
    } while (0)

///
/// Choice arms (used inside `PcChoice`). All forward their extra args to the parser call.
///
/// `PcAlt`/`PcTryAlt` are BODILESS: the matched parser's output IS the arm's value. Each is a
/// complete statement, so writing a `{ ... }` after one is a compile error.
/// `PcAltThen`/`PcTryAltThen` take a build body, run on a match, to transform the parsed value.
///
/// Plain arms COMMIT: if the arm consumed input then failed, the choice fails (consumed) rather
/// than trying later arms. `Try` arms BACKTRACK: any failure rewinds and the next arm is tried.
///
#define PcAlt(Name, ...)                                                                                               \
    ((void)(pc_ch.done ||                                                                                              \
            ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ?                                      \
                 ((pc_ch.matched = pc_ch.done = true)) :                                                               \
                 ((pc_ch.st & PC_PARSER_STATUS_CONSUMED) ? (pc_ch.done = true, false) : (*in = pc_ch.mark, false)))))

#define PcTryAlt(Name, ...)                                                                                            \
    ((void)(pc_ch.done || ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ?                        \
                               ((pc_ch.matched = pc_ch.done = true)) :                                                 \
                               (*in = pc_ch.mark, false))))

#define PcAltThen(Name, ...)                                                                                           \
    if (!pc_ch.done &&                                                                                                 \
        ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ?                                          \
             ((pc_ch.matched = pc_ch.done = true)) :                                                                   \
             ((pc_ch.st & PC_PARSER_STATUS_CONSUMED) ? (pc_ch.done = true, false) : (*in = pc_ch.mark, false))))

#define PcTryAltThen(Name, ...)                                                                                        \
    if (!pc_ch.done &&                                                                                                 \
        ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ? ((pc_ch.matched = pc_ch.done = true)) :  \
                                                                              (*in = pc_ch.mark, false)))

///
/// Atoms -- the only place `StrIter` and `PcParserStatus` are handled directly. Every fundamental
/// parser (a char class, a keyword, ...) is written on top of one of these, so it never pokes the
/// stream or builds a status by hand.
///
/// PcSatisfyChar: match one char, bound as `Var`, for which `Pred` holds; run the build body,
/// then succeed (consuming the char). If `Pred` is false at the current position, fail without
/// consuming. `Var` is loop-scoped (it does not leak into the parser body).
///
/// SUCCESS: `Pred(Var)` held; the char is consumed, the body has run; returns SUCCESS|CONSUMED.
/// FAILURE: `Pred(Var)` did not hold (or there was no char); nothing consumed; returns FAILED.
///
#define PcSatisfyChar(Var, Pred)                                                                                       \
    for (char Var = 0;;)                                                                                               \
        for (bool UNPL(pc_sc_ran) = false;; UNPL(pc_sc_ran) = true)                                                    \
            if (UNPL(pc_sc_ran))                                                                                       \
                return PC_PARSER_STATUS_SUCCESS | PC_PARSER_STATUS_CONSUMED;                                           \
            else if (!(StrIterPeek(in, &Var) && (Pred)))                                                               \
                return PC_PARSER_STATUS_FAILED;                                                                        \
            else if ((StrIterMove(in, 1), true))

///
/// PcSatisfyStr: match the literal string `Expect` (a `Zstr`; pass `StrBegin(&s)` for a `Str`) at
/// the current position. The whole string is peeked before committing, so a mismatch consumes
/// nothing (a clean failure, usable in a choice without a commit surprise); a full match consumes
/// it and runs the build body.
///
/// SUCCESS: `Expect` was present; it is consumed, the body has run; returns SUCCESS|CONSUMED.
/// FAILURE: `Expect` was not present; nothing consumed; returns FAILED.
///
#define PcSatisfyStr(Expect)                                                                                           \
    Zstr UNPL(pc_ss_exp) = (Expect);                                                                                   \
    u64  UNPL(pc_ss_len) = ZstrLen(UNPL(pc_ss_exp));                                                                   \
    bool UNPL(pc_ss_ok)  = true;                                                                                       \
    for (u64 UNPL(pc_ss_i) = 0; UNPL(pc_ss_i) < UNPL(pc_ss_len); UNPL(pc_ss_i)++) {                                    \
        char UNPL(pc_ss_c);                                                                                            \
        if (!StrIterPeekAt(in, (i64)UNPL(pc_ss_i), &UNPL(pc_ss_c)) ||                                                  \
            UNPL(pc_ss_c) != UNPL(pc_ss_exp)[UNPL(pc_ss_i)]) {                                                         \
            UNPL(pc_ss_ok) = false;                                                                                    \
            break;                                                                                                     \
        }                                                                                                              \
    }                                                                                                                  \
    if (UNPL(pc_ss_ok))                                                                                                \
        StrIterMustMove(in, (i64)UNPL(pc_ss_len));                                                                     \
    for (bool UNPL(pc_ss_ran) = false;; UNPL(pc_ss_ran) = true)                                                        \
        if (!UNPL(pc_ss_ok))                                                                                           \
            return PC_PARSER_STATUS_FAILED;                                                                            \
        else if (UNPL(pc_ss_ran))                                                                                      \
            return PC_PARSER_STATUS_SUCCESS | PC_PARSER_STATUS_CONSUMED;                                               \
        else

#endif // MISRA_PARSER_COMBINATOR
