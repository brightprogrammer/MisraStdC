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

typedef struct Project {
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
        p.test_directories   = VecInitT(p.test_directories);                                                           \
        p.source_directories = VecInitT(p.source_directories);                                                         \
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
/// internally using `TYPE_OF`.
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
        TYPE_OF((obj)) __o_b_j = (obj);                                                                                \
        {scope_body};                                                                                                  \
        obj_deinit(__o_b_j);                                                                                           \
    } while (0)

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
            StrIter json = StrIterFromStr(config);
            JR_PROJECT(json, project);
        });

        // recursively explore directories and get files that need documentation
        Strs file_paths = VecInit();
        Scope(&file_paths, VecDeinit, {
            // temporary vector to store all directory paths to explore files in
            Strs dir_paths = VecInitWithDeepCopy(NULL, StrDeinit);
            Scope(&dir_paths, VecDeinit, {
                VecMerge(&dir_paths, &project.source_directories);
                VecMerge(&dir_paths, &project.test_directories);

                // recursively explore directories and get filenames
                VecForeach(&dir_paths, dir_name, {
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
                                VecPushBack(&dir_paths, path);
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
            });

            // go over each file and generate corresponding markdown
            VecForeach(&file_paths, file_path, {
                Str file_contents = StrInit();
                Scope(&file_contents, StrDeinit, {
                    if (!ReadCompleteFile(
                            file_path.data,
                            &file_contents.data,
                            &file_contents.length,
                            &file_contents.capacity
                        )) {
                        LOG_ERROR("Failed to read \"%s\" source file.", file_path.data);
                        continue;
                    }

                    Str output_path = StrInit();
                    Scope(&output_path, StrDeinit, {
                        StrMerge(&output_path, &file_path);
                        LOG_INFO("%s", output_path.data);
                        StrReplaceZstr(&output_path, "/", "-", -1);
                        LOG_INFO("%s", output_path.data);

                        Str md_code = StrInit();
                        Scope(&md_code, StrDeinit, {
                            StrAppendf(
                                &md_code,
                                "---\n"
                                "title: \"%s\"\n"
                                "meta_title: \"%s\"\n"
                                "description: \"Documentation for %s\"\n"
                                "date: 2025-05-12T05:00:00Z\n"
                                "# image: \"/images/image-placeholder.png\"\n"
                                "categories: [\"Vec\", \"Macro\", \"Generic\"]\n"
                                "author: \"Siddharth Mishra\"\n"
                                "tags: [\"vec\", \"macro\", \"generic\"]\n"
                                "draft: false\n"
                                "---\n"
                                "```c\n",
                                output_path.data,
                                output_path.data,
                                output_path.data
                            );
                            StrMerge(&md_code, &file_contents);
                            StrAppendf(&md_code, "\n```");

                            // complete relative file path
                            StrPushFront(&output_path, '/');
                            LOG_INFO("%s", output_path.data);
                            StrPushFrontCstr(&output_path, project.build_dir.data, project.build_dir.length);
                            LOG_INFO("%s", output_path.data);
                            StrReplaceZstr(&output_path, ".c", ".md", 1);
                            StrReplaceZstr(&output_path, ".h", ".md", 1);
                            LOG_INFO("%s\n\n", output_path.data);


                            // dump code to output path
                            FILE* f = fopen(output_path.data, "w");
                            Scope(f, fclose, { fwrite(md_code.data, 1, md_code.length, f); });
                        });
                    });
                });
            });
        });
    });

    LogDeinit();
    return 0;
}
