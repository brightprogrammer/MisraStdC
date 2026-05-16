#include <Misra.h>
#include <Misra/Std/Allocator/Default.h>

int main(void) {
    DefaultAllocator alloc = DefaultAllocatorInit();

    Str file = StrInit(&alloc);
    if (ReadCompleteFile(
            "Bin/Demangler/CppNameManglingGrammar",
            &file.data,
            &file.length,
            &file.capacity,
            &alloc.base
        )) {
        Strs lines = StrSplit(&file, "\n");

        // Use the fixed VecForeachPtr macro
        VecForeachPtr(&lines, line) {
            if (StrStartsWithZstr(line, "[.") && StrEndsWithZstr(line, "]")) {
                Str rule_name = StrInit(&alloc);
                StrReadFmt(line->data, "[.{}]", rule_name);

                if (rule_name.length) {
                    WriteFmtLn("Got Rule : {}", rule_name);
                    StrDeinit(&rule_name);
                }
            }
        }

        VecDeinit(&lines);
        VecDeinit(&file);
    } else {
        LOG_ERROR("Failed to read file");
    }

    DefaultAllocatorDeinit(&alloc);
    return 0;
}
