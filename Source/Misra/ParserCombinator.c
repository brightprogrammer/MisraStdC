/// file      : ParserCombinator.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// The fundamental byte-reader parsers the DSL delivers out of the box, so a
/// binary grammar composes `PcMatch(PcU32BE, &field)` / `PcMatch(PcI32BE, &f)`
/// without hand-writing a wrapper per field. Each reads one fixed-width field
/// from the `BufIter` cursor at the stated endianness through Buf's direct
/// `BufRead{U,I}*`; a short buffer fails the rule (never aborts). Defined here
/// rather than inline so they live outside every grammar's translation unit --
/// which is why they see the context only as an opaque `struct PcParserCtx *`
/// (unused; a grammar's context is tagged `struct PcParserCtx` so the pointer
/// lines up at the call).

#include <Misra/ParserCombinator.h>
#include <Misra/Std/Container/Buf.h>
#include <Misra/Std/Container/Str.h>
#include <Misra/Std/Io.h>

#define PC_DELIVER_READER(Name, Type, Reader)                                                                          \
    PcParserStatus pc_parser_##Name(BufIter *in, struct PcParserCtx *ctx, Type *value) {                               \
        (void)ctx;                                                                                                     \
        PC_BYTE_ATOM(Reader(in, value));                                                                               \
    }

PC_DELIVER_READER(PcU8, u8, BufReadU8)
PC_DELIVER_READER(PcU16BE, u16, BufReadU16BE)
PC_DELIVER_READER(PcU16LE, u16, BufReadU16LE)
PC_DELIVER_READER(PcU32BE, u32, BufReadU32BE)
PC_DELIVER_READER(PcU32LE, u32, BufReadU32LE)
PC_DELIVER_READER(PcU64BE, u64, BufReadU64BE)
PC_DELIVER_READER(PcU64LE, u64, BufReadU64LE)

PC_DELIVER_READER(PcI8, i8, BufReadI8)
PC_DELIVER_READER(PcI16BE, i16, BufReadI16BE)
PC_DELIVER_READER(PcI16LE, i16, BufReadI16LE)
PC_DELIVER_READER(PcI32BE, i32, BufReadI32BE)
PC_DELIVER_READER(PcI32LE, i32, BufReadI32LE)
PC_DELIVER_READER(PcI64BE, i64, BufReadI64BE)
PC_DELIVER_READER(PcI64LE, i64, BufReadI64LE)

#undef PC_DELIVER_READER

static Zstr pc_level_word(PcReportLevel level) {
    switch (level) {
        case PC_REPORT_ERROR :
            return "error";
        case PC_REPORT_WARN :
            return "warning";
        default :
            return "note";
    }
}

void PcReportsRender(Str *out, Str *src, PcReports *reports) {
    const char *bytes = StrBegin(src);
    u64         n     = StrLen(src);
    for (u64 r = 0; r < VecLen(reports); r++) {
        PcReport rep = VecAt(reports, r);

        // The source line the span sits on, bounded by a newline -- or a NUL, since
        // a parser may terminate a borrowed slice in place -- on either side.
        u64 line_start = rep.start;
        while (line_start > 0 && bytes[line_start - 1] != '\n' && bytes[line_start - 1] != '\0')
            line_start--;
        u64 line_end = rep.start;
        while (line_end < n && bytes[line_end] != '\n' && bytes[line_end] != '\0')
            line_end++;
        u64 span_end = rep.end < line_end ? rep.end : line_end;
        while (span_end > rep.start && (bytes[span_end - 1] == ' ' || bytes[span_end - 1] == '\t'))
            span_end--;

        StrAppendFmt(out, "{}: {}\n", pc_level_word(rep.level), rep.message);
        StrPushBackR(out, ' ');
        StrPushBackR(out, ' ');
        for (u64 c = line_start; c < line_end; c++)
            StrPushBackR(out, bytes[c]);
        StrPushBackR(out, '\n');
        StrPushBackR(out, ' ');
        StrPushBackR(out, ' ');
        for (u64 c = line_start; c < rep.start; c++)
            StrPushBackR(out, ' ');
        for (u64 c = rep.start; c < span_end; c++)
            StrPushBackR(out, '^');
        StrPushBackR(out, '\n');
    }
}
