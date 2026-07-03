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
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>

///
/// The cursor type. Text grammars use the default `StrIter` (`Iter(char)`); a byte-oriented
/// grammar sets `#define PC_ITER BufIter` (`Iter(const u8)`) before including this header. The
/// block frames are cursor-agnostic (they only use `in->pos` / `*in` / `IterIndex`); only the
/// atoms are element-specific -- `PcSatisfy*` for characters, the `PcU*` family below for bytes.
///
#ifndef PC_ITER
#    define PC_ITER StrIter
#endif

///
/// A parser combinator does not only need a way to parse a string. It also needs to convert
/// it to a representation that any phase after parsing can use and analyze. This may heavily
/// depend on the language being parsed.
///
/// The mechanism this library uses is a per-grammar context (`PcParserCtx`, see below) threaded
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
    PC_REPORT_INFO,
    PC_REPORT_WARN,
    PC_REPORT_ERROR
} PcReportLevel;

typedef struct PcReport {
    u64           start;   ///< span start, an index into the input (see `IterIndex`)
    u64           end;     ///< span end, exclusive
    PcReportLevel level;
    Zstr          message; ///< the parser's words; the specifics show under the caret
} PcReport;

typedef Vec(PcReport) PcReports;

///
/// PcReportsRender: draw the recorded diagnostics rustc-style into `out`. For each report it appends
/// a `level: message` line, the source line the report spans (bounded by a newline or NUL on either
/// side, so it works on the whole multi-line input), and a caret run under the span. `src` is the
/// parsed input; a grammar only records `PcReport`s and never draws a caret. The caller owns `out`
/// and decides where it goes -- print it, log it, ... -- so the renderer is I/O-free.
///
void PcReportsRender(Str *out, Str *src, PcReports *reports);

///
/// The parser context and its savepoint contract
/// ==============================================
///
/// `PcParserCtx` is NOT defined here. Each grammar (a C parser, a JSON parser, ...) declares its
/// own `typedef struct { ... } PcParserCtx;` before using `PcParser`, and it is threaded through
/// every parser as `ctx`. It carries whatever the grammar accumulates as it parses -- typically an
/// allocator, a `Vec(PcReport) reports;` sink (required by the `PcReport*` macros), and any
/// context-sensitive state the grammar discovers along the way (a symbol table, the set of typedef
/// names, ...). The combinator only forwards `ctx`; it never looks inside it.
///
/// Because a backtracking parser abandons speculative attempts, any state a parser writes into
/// `ctx` while trying an alternative that later fails must be undone -- otherwise a rule that
/// discovered a symbol on a path the parse did not take would leave that symbol behind. The DSL
/// undoes it automatically, provided the grammar supplies a small savepoint contract next to its
/// `PcParserCtx`:
///
///   - a type  `PcParserCtxMark`                                     an opaque saved-state handle.
///   - a func  `PcParserCtxMark PcParserCtxSnapshot(PcParserCtx *ctx)`
///                                                                    capture the current context.
///   - a func  `void PcParserCtxRollback(PcParserCtx *ctx, PcParserCtxMark mark)`
///                                                                    restore a previously captured
///                                                                    context.
///
/// The grammar author writes ONLY the context mutations (bind a symbol, record a type). The author
/// never calls snapshot or rollback -- the combinators do, around every attempt that may be
/// abandoned. The division of labour is: you mutate, the DSL saves and restores.
///
/// Contract -- what the two functions must guarantee
/// -------------------------------------------------
/// Picture the context as a state machine with three states, relative to the most recent mark:
///
///     SETTLED   no mark outstanding; the context is the committed baseline.
///     MARKED    a mark is held and the context still equals it (nothing changed since).
///     DIRTY     a mark is held and the context has changed since it was taken.
///
///                     Snapshot                    (a mutation)
///     SETTLED  ------------------>   MARKED   ------------------->   DIRTY --.
///        ^                             |                              |  <--' (a mutation)
///        |          Rollback(mark)     |          Rollback(mark)      |
///        '-----------------------------+------------------------------'
///
///   - Snapshot does NOT change the context; it returns a mark bound to the current state.
///   - A mutation is the whole CLASS of context-changing operations the grammar performs; every one
///     must be reversible with respect to every outstanding mark.
///   - Rollback(mark) restores the context to exactly what it was when `mark` was taken, however
///     many mutations happened since. A mark may be rolled back to more than once (an alternation
///     rewinds to the same mark once per failing arm), so rollback must be repeatable.
///
/// How the combinators use it during backtracking
/// ----------------------------------------------
/// Before a combinator tries an attempt it might abandon -- an arm of a `PcChoice`, the body of a
/// `PcOpt`, one round of a `PcMatchZeroOrMore`/`PcMatchOneOrMore` -- it takes a mark. If the attempt is abandoned (an arm
/// fails without committing, an optional is absent, a repetition stops), the context is rolled back
/// to that mark, so a failed attempt leaves the context exactly as it found it. If the attempt
/// commits (it consumed input and belongs to the parse), its mutations stay. A grammar therefore
/// reads and writes `ctx` as ordinary imperative code and still parses soundly under backtracking,
/// with no manual save or restore anywhere in the grammar.
///
/// The `reports` sink is deliberately OUTSIDE this contract: diagnostics accumulate monotonically
/// and are never rolled back, so an error explaining why a branch failed survives even after the
/// parser backtracks past it. Snapshot and rollback concern the grammar's semantic state only.
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
/// Used internally by `PcMatch`/`PcAlt`/`PcMatchOneOrMore`/`PcOpt`; call it directly only to delegate one
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
///   PcRecognizer(Name)       -> (PC_ITER *in, PcParserCtx *ctx)               define, no input
///   PcRecognizer(Name, InT)  -> (PC_ITER *in, PcParserCtx *ctx, InT expect)   define, one input
///   PcRecognize(Name)        -> pc_parser_Name(in, ctx)                      call, no input
///   PcRecognize(Name, In)    -> pc_parser_Name(in, ctx, In)                  call, one input
///
/// SUCCESS: The body returns `PC_PARSER_STATUS_SUCCESS` (| `CONSUMED` when it advanced `in`).
/// FAILURE: The body returns `PC_PARSER_STATUS_FAILED` (| `CONSUMED` when it advanced then failed).
///
#define PcRecognizer(...) OVERLOAD(PcRecognizer, __VA_ARGS__)
#define PcRecognizer_1(Name)                                                                                           \
    static inline PcParserStatus PcGenParserName(Name)(PC_ITER * in, PcParserCtx * ctx PC_MAYBE_UNUSED)
#define PcRecognizer_2(Name, InT)                                                                                      \
    static inline PcParserStatus PcGenParserName(Name)(PC_ITER * in, PcParserCtx * ctx PC_MAYBE_UNUSED, InT expect)

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
///   PcParser(Name, BuildT)       -> (PC_ITER *in, PcParserCtx *ctx, BuildT *value)
///   PcParser(Name, InT, BuildT)  -> (PC_ITER *in, PcParserCtx *ctx, InT expect, BuildT *value)
///
/// The names a body reads are `in` (stream), `ctx` (grammar context), `expect` (the input, in
/// the 3-arg form), and `value` (the output). The consumer must have a `PcParserCtx` type in scope.
///
/// SUCCESS: The body returns `PC_PARSER_STATUS_SUCCESS` (or'd with `CONSUMED` when it advanced
///          `in`); the parsed result has been written to `*value`.
/// FAILURE: The body returns `PC_PARSER_STATUS_FAILED` (or'd with `CONSUMED` when it advanced
///          before failing); `*value` is left unspecified.
///
#define PcParser(...) OVERLOAD(PcParser, __VA_ARGS__)
#define PcParser_2(Name, BuildT)                                                                                       \
    static inline PcParserStatus PcGenParserName(Name)(PC_ITER * in, PcParserCtx * ctx PC_MAYBE_UNUSED, BuildT * value)
#define PcParser_3(Name, InT, BuildT)                                                                                  \
    static inline PcParserStatus                                                                                       \
        PcGenParserName(Name)(PC_ITER * in, PcParserCtx * ctx PC_MAYBE_UNUSED, InT expect, BuildT * value)

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
/// PcSeq: run the steps (`PcMatch`/`PcMatchOneOrMore`/...) in order; a failing step returns early. If the
/// body runs to the end, the rule succeeds (consuming whatever the steps consumed).
///
/// SUCCESS: All steps matched; returns SUCCESS with the accumulated CONSUMED bit.
/// FAILURE: A step failed and already returned; control never reaches the frame's success return.
///
#define PcSeq()                                                                                                        \
    for (struct {                                                                                                      \
             PC_ITER start;                                                                                            \
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
             PC_ITER         mark;                                                                                     \
             PcParserCtxMark ctx_mark;                                                                                 \
             PcParserStatus  st;                                                                                       \
             bool            ran, matched, done;                                                                       \
         } pc_ch = {*in, PcParserCtxSnapshot(ctx), 0, false, false, false};                                            \
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
/// PcFailIfNotEof: a `PcSeq` step that succeeds only at end of input. If any input remains, it
/// records `Msg` at the cursor and fails the rule (exactly as `PcReject`, carrying the sequence's
/// consumed bit) -- the "the whole input had to parse" assertion, so a grammar never inspects the
/// cursor by hand. `ctx` must carry a `Vec(PcReport) reports;` sink for the recorded cause.
///
#define PcFailIfNotEof(Msg)                                                                                            \
    do {                                                                                                               \
        char UNPL(pc_eof_) = 0;                                                                                        \
        if (IterPeekAt(in, 0, &UNPL(pc_eof_))) {                                                                       \
            PcReportErrorHere(Msg);                                                                                    \
            return PC_CONSUMED(pc_seq.start) | PC_PARSER_STATUS_FAILED;                                                \
        }                                                                                                              \
    } while (0)

///
/// Record a diagnostic and KEEP GOING -- the greedy alternative to `PcReject`. Appends a
/// `PcReport` (spanning what the current `PcSeq` frame has consumed so far) to `ctx->reports`,
/// then returns nothing, so the rule substitutes a poison value and parsing continues to collect
/// more errors. `ctx` must carry a `Vec(PcReport) reports;`. A `PcSeq` step, like `PcReject`.
///
#define PcReportError(Msg) PC_REPORT(PC_REPORT_ERROR, Msg)
#define PcReportWarn(Msg)  PC_REPORT(PC_REPORT_WARN, Msg)
#define PcReportInfo(Msg)  PC_REPORT(PC_REPORT_INFO, Msg)
#define PC_REPORT(Level, Msg)                                                                                          \
    VecPushBackR(                                                                                                      \
        &ctx->reports,                                                                                                 \
        ((PcReport) {.start = (pc_seq.start).pos, .end = IterIndex(in), .level = (Level), .message = (Msg)})           \
    )

///
/// The `...Here` variants report a diagnostic whose span is the single character at the current
/// cursor, rather than the enclosing `PcSeq` frame. Use them where nothing has been consumed and
/// there is no frame span to point at -- an "expected X"-style error, e.g. inside a `PcChoice`
/// where every arm has rewound. They read only the cursor, so they need no `pc_seq`.
///
#define PcReportErrorHere(Msg) PC_REPORT_HERE(PC_REPORT_ERROR, Msg)
#define PcReportWarnHere(Msg)  PC_REPORT_HERE(PC_REPORT_WARN, Msg)
#define PcReportInfoHere(Msg)  PC_REPORT_HERE(PC_REPORT_INFO, Msg)
#define PC_REPORT_HERE(Level, Msg)                                                                                     \
    VecPushBackR(                                                                                                      \
        &ctx->reports,                                                                                                 \
        ((PcReport) {.start = IterIndex(in), .end = IterIndex(in) + 1, .level = (Level), .message = (Msg)})            \
    )

///
/// PcMatchZeroOrMore: zero-or-more ("any number", including none). Run parser `Name` repeatedly; the body runs
/// once per match. A match that consumed nothing would spin forever, so the loop stops on it (it
/// never aborts and never allocates); the failing/empty iteration is rewound so its bytes are not
/// eaten. Always "succeeds" -- it is a step that simply stops.
///
#define PcMatchZeroOrMore(Name, ...)                                                                                   \
    for (struct {                                                                                                      \
             PC_ITER         mark;                                                                                     \
             PcParserCtxMark ctx_mark;                                                                                 \
         } UNPL(pc_zom_) = {*in, PcParserCtxSnapshot(ctx)};                                                            \
         (UNPL(pc_zom_).mark    = *in,                                                                                 \
         UNPL(pc_zom_).ctx_mark = PcParserCtxSnapshot(ctx),                                                            \
         (PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS) && in->pos != UNPL(pc_zom_).mark.pos) ?               \
             true :                                                                                                    \
             (*in = UNPL(pc_zom_).mark, PcParserCtxRollback(ctx, UNPL(pc_zom_).ctx_mark), false);)

///
/// PcMatchOneOrMore: one-or-more. Like `PcMatchZeroOrMore`, but at least one match is required: if the first attempt does
/// not match, the whole rule fails (reporting the sequence's consumed bit, exactly as a failing
/// `PcMatch` would), so it is a `PcSeq` step. The body runs once per match, the first included --
/// folding the common "match one, then any more" shape into a single step.
///
#define PcMatchOneOrMore(Name, ...)                                                                                    \
    for (struct {                                                                                                      \
             PC_ITER         mark;                                                                                     \
             PcParserCtxMark ctx_mark;                                                                                 \
             bool            done;                                                                                     \
         } UNPL(pc_oom1_) = {*in, PcParserCtxSnapshot(ctx), false};                                                    \
         !UNPL(pc_oom1_).done;                                                                                         \
         UNPL(pc_oom1_).done = true)                                                                                   \
        if ((UNPL(pc_oom1_).mark     = *in,                                                                            \
             UNPL(pc_oom1_).ctx_mark = PcParserCtxSnapshot(ctx),                                                       \
             !((PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS) && in->pos != UNPL(pc_oom1_).mark.pos)))        \
            return (                                                                                                   \
                *in = UNPL(pc_oom1_).mark,                                                                             \
                PcParserCtxRollback(ctx, UNPL(pc_oom1_).ctx_mark),                                                     \
                PC_CONSUMED(pc_seq.start) | PC_PARSER_STATUS_FAILED                                                    \
            );                                                                                                         \
        else                                                                                                           \
            for (struct {                                                                                              \
                     PC_ITER         mark;                                                                             \
                     PcParserCtxMark ctx_mark;                                                                         \
                     bool            first;                                                                            \
                 } UNPL(pc_oomN_) = {*in, PcParserCtxSnapshot(ctx), true};                                             \
                 UNPL(pc_oomN_).first ?                                                                                \
                     true :                                                                                            \
                     (UNPL(pc_oomN_).mark    = *in,                                                                    \
                     UNPL(pc_oomN_).ctx_mark = PcParserCtxSnapshot(ctx),                                               \
                     (PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS) && in->pos != UNPL(pc_oomN_).mark.pos ?   \
                          true :                                                                                       \
                          (*in = UNPL(pc_oomN_).mark, PcParserCtxRollback(ctx, UNPL(pc_oomN_).ctx_mark), false));      \
                 UNPL(pc_oomN_).first = false)

///
/// PcOpt: zero-or-one. Try parser `Name`; if it matches, run the body once (the parsed value is
/// available through whatever output pointer was passed). If it does not match, rewind and skip
/// the body. Never fails the sequence -- it is a step that is simply optional. Like `PcMatchOneOrMore`, it
/// is one `for` statement with its bookkeeping in the loop scope, so it nests, sits next to other
/// steps on the same line, and takes an unbraced body without a dangling-`else` surprise.
///
#define PcOpt(Name, ...)                                                                                               \
    for (struct {                                                                                                      \
             PC_ITER         mark;                                                                                     \
             PcParserCtxMark ctx_mark;                                                                                 \
             bool            ran;                                                                                      \
         } UNPL(pc_opt_) = {*in, PcParserCtxSnapshot(ctx), false};                                                     \
         !UNPL(pc_opt_).ran &&                                                                                         \
         ((UNPL(pc_opt_).ran = true),                                                                                  \
          (PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS) ?                                                    \
              true :                                                                                                   \
              (*in = UNPL(pc_opt_).mark, PcParserCtxRollback(ctx, UNPL(pc_opt_).ctx_mark), false));)

///
/// PcRecognizeZeroOrMore: the recognizer-flavoured `PcMatchZeroOrMore` -- a whole recognizer body that runs a
/// sub-recognizer zero-or-more times ("any number", including none) and then returns success.
/// Terminal (it owns the `return`), so it is the entire body of a "recognize zero-or-more of one
/// thing" parser (e.g. whitespace), not a mid-sequence step. A non-consuming match stops the loop
/// and is rewound, so it can never spin. Overloaded 1-arg / 2-arg like `PcRecognize`.
///
#define PcRecognizeZeroOrMore(...)        OVERLOAD(PcRecognizeZeroOrMore, __VA_ARGS__)
#define PcRecognizeZeroOrMore_1(Name)     PC_RECOGNIZE_REPEAT(PcRecognize_1(Name), 0)
#define PcRecognizeZeroOrMore_2(Name, In) PC_RECOGNIZE_REPEAT(PcRecognize_2(Name, In), 0)

///
/// PcRecognizeOneOrMore: the one-or-more counterpart of `PcRecognizeZeroOrMore` -- the whole body of a
/// "recognize one-or-more of one thing" recognizer. Identical, except it returns failure when it
/// matched nothing at all. Terminal, like `PcRecognizeZeroOrMore`.
///
#define PcRecognizeOneOrMore(...)        OVERLOAD(PcRecognizeOneOrMore, __VA_ARGS__)
#define PcRecognizeOneOrMore_1(Name)     PC_RECOGNIZE_REPEAT(PcRecognize_1(Name), 1)
#define PcRecognizeOneOrMore_2(Name, In) PC_RECOGNIZE_REPEAT(PcRecognize_2(Name, In), 1)
#define PC_RECOGNIZE_REPEAT(call, min_one)                                                                             \
    do {                                                                                                               \
        PC_ITER UNPL(pc_rm_start) = *in;                                                                               \
        for (struct {                                                                                                  \
                 PC_ITER         mark;                                                                                 \
                 PcParserCtxMark ctx_mark;                                                                             \
             } UNPL(pc_rm_) = {*in, PcParserCtxSnapshot(ctx)};                                                         \
             (UNPL(pc_rm_).mark    = *in,                                                                              \
             UNPL(pc_rm_).ctx_mark = PcParserCtxSnapshot(ctx),                                                         \
             ((call) & PC_PARSER_STATUS_SUCCESS) && in->pos != UNPL(pc_rm_).mark.pos) ?                                \
                 true :                                                                                                \
                 (*in = UNPL(pc_rm_).mark, PcParserCtxRollback(ctx, UNPL(pc_rm_).ctx_mark), false);)                   \
            ;                                                                                                          \
        if ((min_one) && in->pos == UNPL(pc_rm_start).pos)                                                             \
            return PC_PARSER_STATUS_FAILED;                                                                            \
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
    ((void)(pc_ch.done || ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ?                        \
                               ((pc_ch.matched = pc_ch.done = true)) :                                                 \
                               ((pc_ch.st & PC_PARSER_STATUS_CONSUMED) ?                                               \
                                    (pc_ch.done = true, false) :                                                       \
                                    (*in = pc_ch.mark, PcParserCtxRollback(ctx, pc_ch.ctx_mark), false)))))

#define PcTryAlt(Name, ...)                                                                                            \
    ((void)(pc_ch.done || ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ?                        \
                               ((pc_ch.matched = pc_ch.done = true)) :                                                 \
                               (*in = pc_ch.mark, PcParserCtxRollback(ctx, pc_ch.ctx_mark), false))))

#define PcAltThen(Name, ...)                                                                                           \
    if (!pc_ch.done && ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ?                           \
                            ((pc_ch.matched = pc_ch.done = true)) :                                                    \
                            ((pc_ch.st & PC_PARSER_STATUS_CONSUMED) ?                                                  \
                                 (pc_ch.done = true, false) :                                                          \
                                 (*in = pc_ch.mark, PcParserCtxRollback(ctx, pc_ch.ctx_mark), false))))

#define PcTryAltThen(Name, ...)                                                                                        \
    if (!pc_ch.done && ((pc_ch.st = PcParse(Name, __VA_ARGS__)) & PC_PARSER_STATUS_SUCCESS ?                           \
                            ((pc_ch.matched = pc_ch.done = true)) :                                                    \
                            (*in = pc_ch.mark, PcParserCtxRollback(ctx, pc_ch.ctx_mark), false)))

///
/// PcElse: the fallback arm of a `PcChoice`. Its body runs iff NO arm matched (every arm failed
/// cleanly, consuming nothing), and it marks the choice handled so the rule succeeds with whatever
/// the body writes to the output (typically a report plus a poison value). Put it last. A rule
/// uses it so it never has to name `pc_ch`.
///
#define PcElse() if (!pc_ch.done && (pc_ch.matched = pc_ch.done = true))

///
/// PcRecover: recovery skip. Advance the stream until the current character satisfies `IsSync` (a
/// resynchronization boundary) or the stream ends; `Var` is the loop-scoped peeked char. Pairs
/// with a report and a poison value to recover from a syntax error and carry on, so one input
/// surfaces more than just the first structural break.
///
#define PcRecover(Var, IsSync)                                                                                         \
    for (char Var = 0; IterPeekAt(in, 0, &Var) && !(IsSync);)                                                          \
    IterMove(in, 1)

///
/// PcCaptureUntil: capture the run of input from the cursor up to (not including) the first char
/// satisfying `IsStop`, or end of input. Binds `PtrOut` to a `Zstr` at the run's start and `LenOut`
/// (a `u64`) to its byte length -- a borrowed `(ptr, len)` view the rule does what it wants with:
/// parse it to a number, copy it into an owned `Str`, or store it as-is. The DSL owns the cursor the
/// whole time, so the rule never touches `in`. `Var` is the loop-scoped peeked char (as in
/// `PcRecover`). The cursor is left AT the stop char -- consume it separately if the grammar must
/// move past it.
///
#define PcCaptureUntil(Var, IsStop, PtrOut, LenOut)                                                                    \
    do {                                                                                                               \
        u64 UNPL(pc_cap_) = IterIndex(in);                                                                             \
        for (char Var = 0; IterPeekAt(in, 0, &Var) && !(IsStop);)                                                      \
            IterMove(in, 1);                                                                                           \
        *(PtrOut) = (Zstr)IterDataAt(in, UNPL(pc_cap_));                                                               \
        *(LenOut) = IterIndex(in) - UNPL(pc_cap_);                                                                     \
    } while (0)

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
            else if (!(IterPeekAt(in, 0, &Var) && (Pred)))                                                             \
                return PC_PARSER_STATUS_FAILED;                                                                        \
            else if ((IterMove(in, 1), true))

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
        if (!IterPeekAt(in, (i64)UNPL(pc_ss_i), &UNPL(pc_ss_c)) || UNPL(pc_ss_c) != UNPL(pc_ss_exp)[UNPL(pc_ss_i)]) {  \
            UNPL(pc_ss_ok) = false;                                                                                    \
            break;                                                                                                     \
        }                                                                                                              \
    }                                                                                                                  \
    if (UNPL(pc_ss_ok))                                                                                                \
        IterMustMove(in, (i64)UNPL(pc_ss_len));                                                                        \
    for (bool UNPL(pc_ss_ran) = false;; UNPL(pc_ss_ran) = true)                                                        \
        if (!UNPL(pc_ss_ok))                                                                                           \
            return PC_PARSER_STATUS_FAILED;                                                                            \
        else if (UNPL(pc_ss_ran))                                                                                      \
            return PC_PARSER_STATUS_SUCCESS | PC_PARSER_STATUS_CONSUMED;                                               \
        else

///
/// Binary fields -- for a grammar with `#define PC_ITER BufIter`. The fixed-width readers are NOT
/// written per grammar: `PcU8`/`PcI8`, `Pc{U,I}16BE`/`LE`, `Pc{U,I}32BE`/`LE`, `Pc{U,I}64BE`/`LE`
/// are fundamental parsers delivered out of the box (declared here, defined in ParserCombinator.c)
/// and used through `PcMatch` / `PcAlt` -- e.g. `PcMatch(PcI32BE, &field)`. Each reads one field
/// from the cursor at the stated endianness and signedness; a short buffer fails the rule. Magic is
/// `PcSatisfyStr`, not a reader.
///
/// The delivered parsers live outside the grammar's translation unit, so they see the context only
/// as an opaque `struct PcParserCtx` (which they ignore) -- so a grammar MUST define its context as
/// a tagged `typedef struct PcParserCtx { ... } PcParserCtx;`, so the opaque pointer lines up.
///
struct PcParserCtx;
PcParserStatus pc_parser_PcU8(BufIter *in, struct PcParserCtx *ctx, u8 *value);
PcParserStatus pc_parser_PcU16BE(BufIter *in, struct PcParserCtx *ctx, u16 *value);
PcParserStatus pc_parser_PcU16LE(BufIter *in, struct PcParserCtx *ctx, u16 *value);
PcParserStatus pc_parser_PcU32BE(BufIter *in, struct PcParserCtx *ctx, u32 *value);
PcParserStatus pc_parser_PcU32LE(BufIter *in, struct PcParserCtx *ctx, u32 *value);
PcParserStatus pc_parser_PcU64BE(BufIter *in, struct PcParserCtx *ctx, u64 *value);
PcParserStatus pc_parser_PcU64LE(BufIter *in, struct PcParserCtx *ctx, u64 *value);
PcParserStatus pc_parser_PcI8(BufIter *in, struct PcParserCtx *ctx, i8 *value);
PcParserStatus pc_parser_PcI16BE(BufIter *in, struct PcParserCtx *ctx, i16 *value);
PcParserStatus pc_parser_PcI16LE(BufIter *in, struct PcParserCtx *ctx, i16 *value);
PcParserStatus pc_parser_PcI32BE(BufIter *in, struct PcParserCtx *ctx, i32 *value);
PcParserStatus pc_parser_PcI32LE(BufIter *in, struct PcParserCtx *ctx, i32 *value);
PcParserStatus pc_parser_PcI64BE(BufIter *in, struct PcParserCtx *ctx, i64 *value);
PcParserStatus pc_parser_PcI64LE(BufIter *in, struct PcParserCtx *ctx, i64 *value);

///
/// PC_BYTE_ATOM: the terminal body of a fundamental byte reader (the delivered parsers and
/// `PcSkipBytes`) -- read/advance or fail the rule; on success return success|consumed.
///
#define PC_BYTE_ATOM(call)                                                                                             \
    do {                                                                                                               \
        if (!(call))                                                                                                   \
            return PC_PARSER_STATUS_FAILED;                                                                            \
        return PC_PARSER_STATUS_SUCCESS | PC_PARSER_STATUS_CONSUMED;                                                   \
    } while (0)

///
/// PcSkipBytes: advance `N` bytes; a short buffer fails. TERMINAL, the body of a recognizer.
///
#define PcSkipBytes(N) PC_BYTE_ATOM(IterMove(in, (i64)(N)))

///
/// PcExpect: a `PcSeq` step that runs a RECOGNIZER (no output) and fails the rule if it did not
/// match -- the recognizer twin of `PcMatch`, for fixed markers (a magic recognizer, a skip).
///
#define PcExpect(...)                                                                                                  \
    do {                                                                                                               \
        if (!(PcRecognize(__VA_ARGS__) & PC_PARSER_STATUS_SUCCESS))                                                    \
            return PC_CONSUMED(pc_seq.start) | PC_PARSER_STATUS_FAILED;                                                \
    } while (0)

///
/// PcMatchExactlyN: run parser `Name` exactly `N` times as a `PcSeq` step, binding `Idx` (a `u64`)
/// to the iteration index for the body to read -- the counted, index-bearing sibling of
/// `PcMatchZeroOrMore` (as `VecForeachIdx` is to `VecForeach`). Any of the `N` failing fails the
/// rule (with the sequence's consumed bit). The body runs after each match.
///
#define PcMatchExactlyN(N, Idx, Name, ...)                                                                             \
    for (u64 Idx = 0; Idx < (u64)(N); Idx++)                                                                           \
        if (!(PcParse(Name, __VA_ARGS__) & PC_PARSER_STATUS_SUCCESS))                                                  \
            return PC_CONSUMED(pc_seq.start) | PC_PARSER_STATUS_FAILED;                                                \
        else

///
/// PcRecognizeExactlyN: run recognizer `Name` exactly `N` times -- the counted sibling of
/// `PcRecognizeZeroOrMore`. TERMINAL (the whole recognizer body); any failure fails.
///
#define PcRecognizeExactlyN(...)           OVERLOAD(PcRecognizeExactlyN, __VA_ARGS__)
#define PcRecognizeExactlyN_2(N, Name)     PC_RECOGNIZE_EXACTLY((N), PcRecognize_1(Name))
#define PcRecognizeExactlyN_3(N, Name, In) PC_RECOGNIZE_EXACTLY((N), PcRecognize_2(Name, In))
#define PC_RECOGNIZE_EXACTLY(N, call)                                                                                  \
    do {                                                                                                               \
        for (u64 UNPL(pc_rxn_) = 0; UNPL(pc_rxn_) < (u64)(N); UNPL(pc_rxn_)++)                                         \
            if (!((call) & PC_PARSER_STATUS_SUCCESS))                                                                  \
                return PC_PARSER_STATUS_FAILED;                                                                        \
        return PC_PARSER_STATUS_SUCCESS | PC_PARSER_STATUS_CONSUMED;                                                   \
    } while (0)

#endif // MISRA_PARSER_COMBINATOR
