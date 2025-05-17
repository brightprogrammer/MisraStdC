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

    WriteFmt("HexQWords :\n{#X2}\n", FMT(code));
    WriteFmt("String :\n{}\n", FMT(code));
    const char *s = "const char strrrr go brrrrrr";
    WriteFmt("Null-terminated String :\n{}\n", FMT(s));

    i32 x = 0;
    StrReadFmt("integer : 1234", "integer : {}", FMT(x));
    WriteFmt("read x = {}", FMT(x));


    LogDeinit();
    return 0;
}
