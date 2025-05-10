/// file      : bin/misraenum.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
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

#include <Misra/Parsers/JSON.h>
#include <Misra/Std.h>

typedef struct {
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

    StrIter json        = StrIterFromStr(&code);
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


    StrAppendf(&code, "typedef enum %s {\n", enum_name.data);

    // last value starts with invalid enum's value
    if (invalid_enum.name.length) {
        last_value = invalid_enum.name.length ? invalid_enum.value : VecFirst(&entries).value;
        StrAppendf(&code, "    %s = %lld,\n", invalid_enum.name.data, invalid_enum.value);
    }
    VecForeach(&entries, e, {
        if (last_value == e.value - 1) {
            StrAppendf(&code, "    %s,\n", e.name.data);
        } else {
            StrAppendf(&code, "    %s = %lld,\n", e.name.data, e.value);
        }
        last_value = e.value;
    });

    StrAppendf(&code, "} %s;\n", enum_name.data);

    if (to_from_str) {
        StrAppendf(
            &code,
            "///\n"
            "/// Converts given zero-terminated string to %s enum value.\n"
            "///\n"
            "/// zstr[in] : String to be converted back to corresponding enum.\n"
            "///\n"
            "/// SUCCESS : Value of enum\n"
            "/// FAILURE : 0\n"
            "///\n"
            "%s %sFromZstr(const char* zstr) {\n"
            "    if(!zstr) {\n"
            "        LOG_ERROR(\"Invalid string provided. Cannot convert to enum.\");\n"
            "        return %s;\n"
            "    }\n",
            enum_name.data,
            enum_name.data,
            enum_name.data,
            invalid_enum.name.length ? invalid_enum.name.data : "0"
        );
        VecForeach(&entries, e, {
            StrAppendf(
                &code,
                "    if(!strncmp(\"%s\", zstr, %zu)) {return %s;}\n",
                e.str.data,
                e.str.length,
                e.name.data
            );
        });
        StrAppendf(
            &code,
            "    return %s;\n"
            "}\n",
            invalid_enum.name.length ? invalid_enum.name.data : "0"
        );

        StrAppendf(
            &code,
            "///\n"
            "/// Converts given enum to %s zero-terminated string.\n"
            "///\n"
            "/// e[in] : String to be converted back to corresponding enum.\n"
            "///\n"
            "/// SUCCESS : A zero-terminated char pointer representing corresponding string value of enum\n"
            "/// FAILURE : NULL\n"
            "///\n"
            "const char* %sToZstr(%s e) {\n"
            "    switch(e) {\n",
            enum_name.data,
            enum_name.data,
            enum_name.data
        );
        VecForeach(&entries, e, {
            StrAppendf(&code, "        case %s : {return \"%s\";}\n", e.name.data, e.str.data);
        });
        StrAppendf(
            &code,
            "        default: break;\n"
            "    }\n"
            "    return \"%s\";\n"
            "}\n",
            invalid_enum.str.data ? invalid_enum.str.data : "NULL"
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
