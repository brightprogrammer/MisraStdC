#include <Misra.h>

int main(void) {
    Str file = StrInit();
    if (ReadCompleteFile("Bin/Demangler/CppNameManglingGrammar", &file.data, &file.length, &file.capacity)) {
        Strs lines = StrSplit(&file, "\n");
        
        // Use the fixed VecForeachPtr macro
        VecForeachPtr(&lines, line, {
            if (StrStartsWithZstr(line, "[.") && StrEndsWithZstr(line, "]")) {
                Str rule_name = StrInit();
                StrReadFmt(line->data, "[.{}]", FMT(rule_name));
                
                if (rule_name.length) {
                    WriteFmtLn("Got Rule : {}", FMT(rule_name));
                    StrDeinit(&rule_name);
                }
            }
        });

        VecDeinit(&lines);
        VecDeinit(&file);
    } else {
        LOG_ERROR("Failed to read file");
    }
    return 0;
}
