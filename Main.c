#include <Misra/Parsers/JSON.h>
#include <Misra/Std.h>
#include <Misra/Types.h>

int main(int argc, char** argv) {
    LogInit(false);

    if (argc != 2) {
        fprintf(stderr, "Need file name.\nUSAGE: %s <c-source>\n", argc > 0 ? argv[0] : "misra");
        return 1;
    }

    Str filename = StrInitFromZstr(argv[1]);
    Str code     = StrInit();
    ReadCompleteFile(filename.data, &code.data, &code.length, &code.capacity);

    printf("%s", code.data);

    LogDeinit();
    return 0;
}
