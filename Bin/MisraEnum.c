/// file      : bin/misraenum.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
///
/// Tool takes in JSON specification of a C enum and then emits C code for the
/// corresponding enum type, along with functions to convert to and from string
/// value.
///
/// Sample JSON format:
///
/// {
///   "name": "CKeyword",
///   "to_from_str": true,
///   "invalid_enum" : { "name": "C_KEYWORD_UNKNOWN", "str": "unknown", "value": 0},
///   "entries": [
///     { "name": "C_KEYWORD_ALIGNAS", "str": "alignas" },
///     { "name": "C_KEYWORD_ALIGNOF", "str": "alignof" },
///     { "name": "C_KEYWORD_AUTO", "str": "auto" },
///     { "name": "C_KEYWORD_BOOL", "str": "bool" }
///   ]
/// }
///
///

#include <Misra.h>

typedef struct EnumEntry {
    Str name;
    Str str;
    i64 value;
} EnumEntry;

typedef Vec(EnumEntry) EnumEntries;

int main(int argc, char** argv) {
    LogInit(false);

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "USAGE : %s <enum-json-spec> [output-file-name] \n", argc == 0 ? "MisraEnum" : argv[0]);
        return 1;
    }

    const char* input_filename  = argv[1];
    const char* output_filename = NULL;
    if (argc == 3) {
        output_filename = argv[3];
    }

    Str code = StrInit();
    ReadCompleteFile(input_filename, &code.data, &code.length, &code.capacity);

    EnumEntries entries = VecInit();

    StrIter json        = StrIterFromStr(code);
    Str     enum_name   = StrInit();
    bool    to_from_str = false;

    EnumEntry invalid_enum = {0};

    i64 last_value = 0;
    JR_OBJ(json, {
        JR_STR_KV(json, "name", enum_name);
        JR_BOOL_KV(json, "to_from_str", to_from_str);

        JR_OBJ_KV(json, "invalid_enum", {
            JR_STR_KV(json, "name", invalid_enum.name);
            JR_INT_KV(json, "value", invalid_enum.value);
            JR_INT_KV(json, "str", invalid_enum.value);
        });

        if (invalid_enum.name.length) {
            last_value = invalid_enum.value;
        }

        JR_ARR_KV(json, "entries", {
            EnumEntry e = {0};

            JR_OBJ(json, {
                JR_STR_KV(json, "name", e.name);
                JR_INT_KV(json, "value", e.value);
                JR_STR_KV(json, "str", e.str);
            });

            if (!e.name.length) {
                LOG_ERROR("Invalid enum entry in 'entries' array. Entry without name.");
                abort();
            }

            if (!e.value) {
                e.value = last_value++;
            } else {
                last_value = e.value;
            }

            if (to_from_str && !e.str.length) {
                LOG_ERROR("to_from_str is set to true but str value not provided for enum %s", e.name.data);
                abort();
            }

            VecPushBack(&entries, e);
        });
    });

    StrClear(&code);

    // Use a temporary variable for enum name
    StrWriteFmt(&code, "typedef enum {} {{\n", FMT(enum_name.data));

    // last value starts with invalid enum's value
    if (invalid_enum.name.length) {
        last_value = invalid_enum.name.length ? invalid_enum.value : VecFirst(&entries).value;
        StrWriteFmt(&code, "    {} = {},\n", FMT(invalid_enum.name.data), FMT(invalid_enum.value));
    }
    
    // Use VecForeach for iterating over entries
    VecForeach(&entries, e, {
        if (last_value == e.value - 1) {
            StrWriteFmt(&code, "    {},\n", FMT(e.name.data));
        } else {
            StrWriteFmt(&code, "    {} = {},\n", FMT(e.name.data), FMT(e.value));
        }
        last_value = e.value;
    });

    StrWriteFmt(&code, "}} {};\n", FMT(enum_name.data));

    if (to_from_str) {
        // Store string literals in temporary variables
        const char* funcHeader = "///\n"
            "/// Converts given zero-terminated string to {} enum value.\n"
            "///\n"
            "/// zstr[in] : String to be converted back to corresponding enum.\n"
            "///\n"
            "/// SUCCESS : Value of enum\n"
            "/// FAILURE : 0\n"
            "///\n"
            "{} {}FromZstr(const char* zstr) {{\n"
            "    if(!zstr) {{\n"
            "        LOG_ERROR(\"Invalid string provided. Cannot convert to enum.\");\n"
            "        return {};\n"
            "    }}\n";
            
        // Prepare the return value for invalid enum
        const char* invalidEnumName = "0";
        if (invalid_enum.name.length) {
            invalidEnumName = invalid_enum.name.data;
        }
        
        StrWriteFmt(
            &code,
            funcHeader,
            FMT(enum_name.data),
            FMT(enum_name.data),
            FMT(enum_name.data),
            FMT(invalidEnumName)
        );
        
        // Use VecForeach for iterating over entries
        VecForeach(&entries, e, {
            const char* compareTemplate = "    if(ZstrCompareN(\"{}\", zstr, {}) == 0) {{return {};}}\n";
            // Store the length in a variable to avoid taking address of rvalue
            unsigned long long strLength = (unsigned long long)e.str.length;
            StrWriteFmt(
                &code,
                compareTemplate,
                FMT(e.str.data),
                FMT(strLength),
                FMT(e.name.data)
            );
        });
        
        const char* returnTemplate = "    return {};\n}}\n";
        StrWriteFmt(
            &code,
            returnTemplate,
            FMT(invalidEnumName)
        );

        const char* toZstrHeader = "///\n"
            "/// Converts given enum to {} zero-terminated string.\n"
            "///\n"
            "/// e[in] : String to be converted back to corresponding enum.\n"
            "///\n"
            "/// SUCCESS : A zero-terminated char pointer representing corresponding string value of enum\n"
            "/// FAILURE : NULL\n"
            "///\n"
            "const char* {}ToZstr({} e) {{\n"
            "    switch(e) {{\n";
            
        StrWriteFmt(
            &code,
            toZstrHeader,
            FMT(enum_name.data),
            FMT(enum_name.data),
            FMT(enum_name.data)
        );
        
        // Use VecForeach for iterating over entries
        VecForeach(&entries, e, {
            const char* caseTemplate = "        case {} : {{return \"{}\";}}\n";
            StrWriteFmt(&code, caseTemplate, FMT(e.name.data), FMT(e.str.data));
        });
        
        const char* defaultTemplate = "        default: break;\n"
            "    }}\n"
            "    return \"{}\";\n"
            "}}\n";
            
        // Use a static string for NULL to avoid taking address of string literal
        const char* nullStrValue = "NULL";
        const char* nullStr = nullStrValue;
        if (invalid_enum.str.data) {
            nullStr = invalid_enum.str.data;
        }
        
        StrWriteFmt(
            &code,
            defaultTemplate,
            FMT(nullStr)
        );
    }

    if (output_filename) {
        FILE* f = fopen(output_filename, "w");
        fwrite(code.data, 1, code.length, f);
        fclose(f);
    } else {
        puts(code.data);
    }

    StrDeinit(&invalid_enum.name);
    StrDeinit(&invalid_enum.str);
    StrDeinit(&enum_name);
    StrDeinit(&code);

    VecForeach(&entries, e, {
        StrDeinit(&e.name);
        StrDeinit(&e.str);
    });
    VecDeinit(&entries);

    LogDeinit();
    return 0;
}
