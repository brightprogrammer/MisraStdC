/// file      : bin/misradoc.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// Tool to generate documentation for Misra style of documentation of symbols.
/// Extracts documentation comments from source and generates documentation based on that.
///
/// Configuration settings are provided using a JSON config file.
///
/// {
///     "project" : {
///         "source_directories" : ["Source", "Include"],
///         "test_directories" : ["Test"],
///         "build_dir" : "build/doc"
///     }
/// }
///

#include <Misra.h>

///
/// Run a scoped block and automatically deinitialize the object at the end.
///
/// Executes `scope_body` and ensures `obj_deinit(obj)` is called afterward,
/// regardless of the block's control flow. This is similar to RAII-style
/// resource management in C++ but implemented manually via a macro.
///
/// The object is passed by pointer. It is not copied or moved.
///
/// This macro ensures the object is only evaluated once by capturing it
/// internally using `__typeof__`.
///
/// The memory pointed to by `obj` is **not** cleared after deinitialization;
/// if zeroing is needed, do it inside `obj_deinit`.
///
/// Parameters:
///     obj[in]         : Pointer to the object to manage.
///     obj_deinit[in]  : Function or macro to deinitialize the object.
///     scope_body[in]  : Block of code that uses the object.
///
/// Usage example:
///     MyStruct s = MyStructInit();
///     Scope(&s, MyStructDeinit, {
///         UseStruct(&s);
///     });
///
/// SUCCESS : Always continues execution after scope.
/// FAILURE : Caller must handle errors inside the scoped body.
///
#define Scope(obj, obj_deinit, scope_body)                                                                             \
    do {                                                                                                               \
        __typeof__((obj)) __o_b_j = (obj);                                                                             \
        {scope_body};                                                                                                  \
        obj_deinit(__o_b_j);                                                                                           \
    } while (0)



typedef struct {
    Str  build_dir;
    Strs source_directories;
    Strs test_directories;
} Project;

void ProjectDeinit(Project* p) {
    if (!p) {
        LOG_ERROR("Invalid project object. Invalid arguments");
        abort();
    }

    StrDeinit(&p->build_dir);
    VecDeinit(&p->source_directories);
    VecDeinit(&p->test_directories);
}

#define JR_PROJECT(json, proj)                                                                                         \
    do {                                                                                                               \
        Project p            = {0};                                                                                    \
        p.test_directories   = VecInit_T(&p.test_directories);                                                         \
        p.source_directories = VecInit_T(&p.source_directories);                                                       \
        JR_OBJ(json, {                                                                                                 \
            JR_OBJ_KV(json, "project", {                                                                               \
                JR_STR_KV(json, "build_dir", p.build_dir);                                                             \
                JR_ARR_KV(json, "test_directories", {                                                                  \
                    Str s = StrInit();                                                                                 \
                    JR_STR(json, s);                                                                                   \
                    VecPushBack(&p.test_directories, s);                                                               \
                });                                                                                                    \
                JR_ARR_KV(json, "source_directories", {                                                                \
                    Str s = StrInit();                                                                                 \
                    JR_STR(json, s);                                                                                   \
                    VecPushBack(&p.source_directories, s);                                                             \
                });                                                                                                    \
            });                                                                                                        \
        });                                                                                                            \
        (proj) = p;                                                                                                    \
    } while (0)

typedef enum {
    SYMBOL_TYPE_INVALID = 0,
    SYMBOL_TYPE_FUNCTION,
    SYMBOL_TYPE_VARIABLE,
    SYMBOL_TYPE_MACRO,
    SYMBOL_TYPE_FNMACRO,
    SYMBOL_TYPE_MAX
} SymbolType;

typedef struct {
    Str        name;       ///> Symbol name
    SymbolType type;       ///> Type of symbol
    Str        doc_str;    ///> Documentation string if available.
    Str        file_path;  ///> Path to file where this symbol is defined/declared
    size       line;       ///> Line number in file `file_path` where this symbol exists.
    i64        parent_idx; ///> Index of parent symbol in Symbols vector.
} Symbol;
typedef Vec(Symbol) Symbols;

void GetSymbolsInParameterList(StrIter* si, Symbols* syms) {
    if (!si || !syms) {
        LOG_FATAL("Invald arguments");
    }
}

Symbols GetSymbolsInFile(Str* file_path) {
    if (!file_path) {
        LOG_FATAL("Invalid arguments.");
    }

    Symbols syms = VecInit();

    Str file_contents = StrInit();
    Scope(&file_contents, StrDeinit, {
        if (!ReadCompleteFile(file_path->data, &file_contents.data, &file_contents.length, &file_contents.capacity)) {
            LOG_ERROR("Failed to read \"%s\" source file.", file_path->data);
            return syms;
        }
    });

    return syms;
}

void GenerateDocumentation(Str* file_path, Str* output_dir) {
    if (!file_path || !output_dir) {
        LOG_FATAL("Invalid arguments.");
    }
}

///
/// Give me some directory names, and I'll recursively traverse each of those
/// for you and get you a vector of all file paths in them.
///
/// dir_paths[in,out] : Recursively traverse directories, and add discovered directories to this.
///
/// SUCCESS : `Strs` vector having names of all available files
/// FAILURE : Empty `Strs` vector otherwise.
///
Strs GetFilePathsRecursively(Strs* dir_paths) {
    if (!dir_paths) {
        LOG_ERROR("Invalid arguments.");
        return (Strs) {0};
    }

    Strs file_paths = VecInit();

    // recursively explore directories and get filenames
    VecForeach(dir_paths, dir_name, {
        // keep track of current path we're exploring
        Str current_path = StrInit();
        StrMerge(&current_path, &dir_name);

        SysDirContents dir_contents = SysGetDirContents(dir_name.data);
        Scope(&dir_contents, VecDeinit, {
            VecForeach(&dir_contents, dir_entry, {
                // if it's a directory then store it for exploration later on
                if (dir_entry.type == SYS_DIR_ENTRY_TYPE_DIRECTORY) {
                    // create new directory path relative to current directory search path
                    Str path = StrInit();
                    StrMerge(&path, &current_path);
                    StrPushBack(&path, '/');
                    StrMerge(&path, &dir_entry.name);

                    // store the directory name, ownersip transferred
                    VecPushBack(dir_paths, path);
                } else if (dir_entry.type == SYS_DIR_ENTRY_TYPE_REGULAR_FILE) {
                    // create complete relative file path
                    Str path = StrInit();
                    StrMerge(&path, &current_path);
                    StrPushBack(&path, '/');
                    StrMerge(&path, &dir_entry.name);

                    // store discovered file name, ownersip transferred
                    VecPushBack(&file_paths, path);
                }
                // any other file type is not documented
            });
        });
    });

    return file_paths;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE : %s config.json\n", argc == 0 ? "MisraDoc" : argv[0]);
        return 1;
    }

    LogInit(false);

    const char* config_path = argv[1];

    Project project = {0};
    Scope(&project, ProjectDeinit, {
        // read project config
        Str config = StrInit();
        Scope(&config, StrDeinit, {
            if (!ReadCompleteFile(config_path, &config.data, &config.length, &config.capacity)) {
                LOG_FATAL("Failed to read config file.");
            }
            StrIter json = StrIterFromStr(&config);
            JR_PROJECT(json, project);
        });

        // temporary vector to store all directory paths to explore files in
        Strs dir_paths = VecInit();
        Scope(&dir_paths, VecDeinit, {
            VecMergeAndOwn(&dir_paths, &project.source_directories);
            VecMergeAndOwn(&dir_paths, &project.test_directories);

            // recursively explore directories and get files that need documentation
            Strs file_paths = GetFilePathsRecursively(&dir_paths);
            Scope(&file_paths, VecDeinit, {
                // go over each file and generate corresponding markdown
                VecForeach(&file_paths, file_path, { GenerateDocumentation(&file_path, &project.build_dir); });
            });
        });
    });

    LogDeinit();
    return 0;
}
