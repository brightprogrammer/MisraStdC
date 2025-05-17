#include <Misra/Parsers/JSON.h>
#include <Misra/Std.h>
#include <Misra/Std/Io.h>
#include <Misra/Types.h>

int main(int argc, char **argv) {
    LogInit(false);

    const char *s = "const char strrrr go brrrrrr";
    WriteFmt("Null-terminated String :\n{}\n", FMT(s));

    i32 x = 0;
    StrReadFmt("integer : 1234", "integer : {}", FMT(x));
    WriteFmt("read x = {}\n", FMT(x));

    const char *uname;
    WriteFmt("Tell me your name! format is username = <name>\n");
    ReadFmt("username = {}", FMT(uname));
    if (uname) {
        WriteFmt("is your name {}? I'm smart, I know!\n", FMT(uname));
        FREE(uname);
    }
    ReadFmt("uname = {}", FMT(uname));
    if (uname) {
        WriteFmtLn("is your name {}? I'm smart, I know!", FMT(uname));
        FREE(uname);
    }

    LogDeinit();
    return 0;
}
