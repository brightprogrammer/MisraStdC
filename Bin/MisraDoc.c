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
///         "name" : "MisraStdC",
///         "description" : "A Personal Standard Library",
///         "source_code" : "https://github.com/brightprogrammer/MisraStdC",
///         "maintainers" : [
///             {
///                 "name" : "Siddharth Mishra",
///                 "bio" : "Hey There!",
///                 "username" : "brightprogrammer",
///                 "email" : "admin@brightprogrammer.in",
///                 "socials" : [
///                     { "website" : "https://brightprogrammer.in" },
///                     { "linkedin" : "https://linkedin.com/in/brightprogrammer" },
///                     { "youtube" : "https://youtube.com/@brightprogrammer" },
///                     { "x" : "https://x.com/brightprogramer" }
///                 ]
///             }
///         ],
///         "source_directories" : ["Source", "Include"],
///         "test_directories" : ["Test"],
///         "home_page" : "README.md",
///         "build_dir" : "build/doc"
///     }
/// }
///

#include <Misra.h>

typedef struct {
    Str platform_name;
    Str profile_url;
} Social;
typedef Vec(Social) Socials;

void SocialDeinit(Social* s) {
    if (!s) {
        LOG_ERROR("Invalid social object. Invalid arguments");
        abort();
    }

    StrDeinit(&s->platform_name);
    StrDeinit(&s->profile_url);
}

typedef struct {
    Str     name;
    Str     bio;
    Str     username;
    Str     email;
    Socials socials;
} Maintainer;
typedef Vec(Maintainer) Maintainers;

void MaintainerDeinit(Maintainer* m) {
    if (!m) {
        LOG_ERROR("Invalid maintainer object. Invalid arguments");
        abort();
    }

    StrDeinit(&m->name);
    StrDeinit(&m->bio);
    StrDeinit(&m->username);
    StrDeinit(&m->email);
    VecDeinit(&m->socials);
}

typedef struct {
    Str         name;
    Str         description;
    Str         source_code;
    Str         home_page;
    Str         build_dir;
    Maintainers maintainers;
    Strs        source_directories;
    Strs        test_directories;
} Project;

void ProjectDeinit(Project* p) {
    if (!p) {
        LOG_ERROR("Invalid project object. Invalid arguments");
        abort();
    }

    StrDeinit(&p->name);
    StrDeinit(&p->description);
    StrDeinit(&p->source_code);
    StrDeinit(&p->home_page);
    StrDeinit(&p->build_dir);
    VecDeinit(&p->maintainers);
    VecDeinit(&p->source_directories);
    VecDeinit(&p->test_directories);
}

#define JR_PROJECT(json, proj)                                                                                         \
    do {                                                                                                               \
        Project p            = {0};                                                                                    \
        p.maintainers        = VecInit_T(&p.maintainers);                                                              \
        p.test_directories   = VecInit_T(&p.test_directories);                                                         \
        p.source_directories = VecInit_T(&p.source_directories);                                                       \
        JR_OBJ(json, {                                                                                                 \
            JR_OBJ_KV(json, "project", {                                                                               \
                JR_STR_KV(json, "name", p.name);                                                                       \
                JR_STR_KV(json, "description", p.description);                                                         \
                JR_STR_KV(json, "source_code", p.source_code);                                                         \
                JR_STR_KV(json, "home_page", p.home_page);                                                             \
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
                JR_ARR_KV(json, "maintainers", {                                                                       \
                    JR_OBJ(json, {                                                                                     \
                        Maintainer m = {0};                                                                            \
                        m.socials    = VecInit_T(&m.socials);                                                          \
                        JR_STR_KV(json, "name", m.name);                                                               \
                        JR_STR_KV(json, "bio", m.bio);                                                                 \
                        JR_STR_KV(json, "username", m.username);                                                       \
                        JR_STR_KV(json, "email", m.email);                                                             \
                        JR_ARR_KV(json, "socials", {                                                                   \
                            JR_OBJ(json, {                                                                             \
                                Social s = {0};                                                                        \
                                StrInitCopy(&s.platform_name, &key);                                                   \
                                JR_STR(json, s.profile_url);                                                           \
                                VecPushBack(&m.socials, s);                                                            \
                            });                                                                                        \
                        });                                                                                            \
                        VecPushBack(&p.maintainers, m);                                                                \
                    });                                                                                                \
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

        // recursively explore directories and get files that need documentation
        Strs file_paths = VecInit();
        Scope(&file_paths, VecDeinit, {
            // temporary vector to store all directory paths to explore files in
            Strs dir_paths = VecInit();
            Scope(&dir_paths, VecDeinit, {
                VecMergeAndOwn(&dir_paths, &project.source_directories);
                VecMergeAndOwn(&dir_paths, &project.test_directories);

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

            // TODO: first let's just emit markdown files that contain the whole code
            // not only the documentation. This is to first setup the documentation pipeline
            // and then worry about how to make it look and generate the documentation
        });
    });

    LogDeinit();
    return 0;
}
