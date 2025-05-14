#include <Misra/Parsers/C.h>
#include <Misra/Parsers/JSON.h>
#include <Misra/Std.h>

int main(int argc, char** argv) {
    LogInit(false);

    if (argc != 2) {
        fprintf(stderr, "Need file name.\nUSAGE: %s <c-source>\n", argc > 0 ? argv[0] : "misra");
        return 1;
    }

    Str filename = StrInitFromZstr(argv[1]);
    Str code     = StrInit();
    ReadCompleteFile(filename.data, &code.data, &code.length, &code.capacity);
    StrIter si = StrIterFromStr(&code);

    LogDeinit();
    return 0;
}
