#include <Misra/ParserCombinator.h>
#include <Misra/Std/Allocator/Heap.h>
#include <Misra/Std/Log.h>

///
/// Per-grammar context threaded through every parser as `ctx`. This calculator
/// evaluates to a `u64` and allocates nothing, so it only carries the allocator
/// that a real (AST-building) grammar would allocate nodes from -- here it just
/// demonstrates the threading and stands ready for the C parser to grow into.
///
typedef struct ParserCtx {
    /// Where parsers allocate long-lived output (AST nodes). Unused by the u64 calculator.
    Allocator *alloc;
} ParserCtx;

///
/// A tiny arithmetic calculator -- the running example / playground for the DSL,
/// loosest precedence first:
///   expr           = additive
///   additive       = multiplicative ( ('+'|'-') multiplicative )*   left-assoc
///   multiplicative = atom           ( ('*'|'/') atom )*             left-assoc
///   atom           = '(' expr ')' | "pi" | number
///   number         = digit+  (folded into a u64 as we go)
/// "pi" (-> 3) exercises the literal-string matcher. No rule touches StrIter or
/// the status directly -- only the atoms and the block frames do.
///

PcParser(CharIfExist, char, char);
PcParser(Digit, char);
PcParser(Number, u64);
PcParser(Pi, u64);
PcParser(AddOp, char);
PcParser(MulOp, char);
PcParser(Atom, u64);
PcParser(Parenthesized, u64);
PcParser(Multiplicative, u64);
PcParser(Additive, u64);
PcParser(Expr, u64);

/// fundamental: match a specific char (the `expect` input), capture it
PcParser(CharIfExist, char, char) {
    PcSatisfyChar(c, c == expect) {
        *value = c;
    }
}

/// char-class atom via the char predicate
PcParser(Digit, char) {
    PcSatisfyChar(c, c >= '0' && c <= '9') {
        *value = c;
    }
}

/// number = digit+, folded left into a u64 as the run is consumed
PcParser(Number, u64) {
    char d;
    PcSeq() {
        PcMatch(Digit, &d);
        *value = (u64)(d - '0');
        PcMany(Digit, &d) {
            *value = *value * 10 + (u64)(d - '0');
        }
    }
}

/// literal-string atom: the "pi" keyword
PcParser(Pi, u64) {
    PcSatisfyStr("pi") {
        *value = 3;
    }
}

PcParser(AddOp, char) {
    PcChoice() {
        PcAlt(CharIfExist, '+', value);
        PcAlt(CharIfExist, '-', value);
    }
}

PcParser(MulOp, char) {
    PcChoice() {
        PcAlt(CharIfExist, '*', value);
        PcAlt(CharIfExist, '/', value);
    }
}

PcParser(Parenthesized, u64) {
    char paren;
    PcSeq() {
        PcMatch(CharIfExist, '(', &paren);
        PcMatch(Expr, value);
        PcMatch(CharIfExist, ')', &paren);
    }
}

PcParser(Atom, u64) {
    PcChoice() {
        PcTryAlt(Parenthesized, value);
        PcAlt(Pi, value);
        PcAlt(Number, value);
    }
}

PcParser(Multiplicative, u64) {
    char op;
    u64  rhs;
    PcSeq() {
        PcMatch(Atom, value);
        PcMany(MulOp, &op) {
            PcMatch(Atom, &rhs);
            *value = (op == '*') ? (*value * rhs) : (rhs ? *value / rhs : 0);
        }
    }
}

PcParser(Additive, u64) {
    char op;
    u64  rhs;
    PcSeq() {
        PcMatch(Multiplicative, value);
        PcMany(AddOp, &op) {
            PcMatch(Multiplicative, &rhs);
            *value = (op == '+') ? (*value + rhs) : (*value - rhs);
        }
    }
}

PcParser(Expr, u64) {
    return PcParse(Additive, value);
}

int main() {
    HeapAllocator  heap  = HeapAllocatorInit();
    ParserCtx      ctx   = {.alloc = ALLOCATOR_OF(&heap)};
    char           buf[] = "1+2*3+4";
    StrIter        in    = StrIterFromZstr(buf);
    u64            out   = 0;
    PcParserStatus st    = pc_parser_Expr(&in, &ctx, &out);

    if (st & PC_PARSER_STATUS_SUCCESS)
        LOG_INFO("parsed value = {}", out);
    else
        LOG_INFO("parse failed");

    HeapAllocatorDeinit(&heap);
    return 0;
}
