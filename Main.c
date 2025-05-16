#include <Misra/Parsers/JSON.h>
#include <Misra/Std.h>
#include <Misra/Std/Io.h>
#include <Misra/Types.h>


int main(int argc, char **argv) {
    LogInit(false);

    if (argc != 2) {
        fprintf(stderr, "Need file name.\nUSAGE: %s <c-source>\n", argc > 0 ? argv[0] : "misra");
        return 1;
    }

    Str filename = StrInitFromZstr(argv[1]);
    Str code     = StrInit();
    ReadCompleteFile(filename.data, &code.data, &code.length, &code.capacity);

    Str out = StrInit();

    StrWriteFmt(&out, "HexQWords :\n{#X8}\n", FMT(code));
    StrWriteFmt(&out, "String :\n{}\n", FMT(code));
    const char *s = "const char strrrr go brrrrrr";
    StrWriteFmt(&out, "Null-terminated String :\n{}\n", FMT(s));
    StrWriteFmt(&out, "A boolean?? :\n{}\n", FMT(LVAL(true)));

    puts(out.data);

    LogDeinit();
    return 0;
}
