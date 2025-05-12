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

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "USAGE : %s config.json\n", argc == 0 ? "MisraDoc" : argv[0]);
        return 1;
    }

    LogInit(false);

    const char* config = argv[1];

    Str code = StrInit();
    ReadCompleteFile(config, &code.data, &code.length, &code.capacity);
    StrIter json = StrIterFromStr(&code);

    Project project = {0};
    JR_PROJECT(json, project);

    Strs dir_paths = VecInit();
    VecMergeAndOwn(&dir_paths, &project.source_directories);
    VecMergeAndOwn(&dir_paths, &project.test_directories);

    Strs file_paths = VecInitWithDeepCopy(StrInitCopy, StrDeinit);
    VecForeach(&dir_paths, dir_name, {
        // keep track of current path we're exploring
        Str current_path = StrInit();
        StrMerge(&current_path, &dir_name);

        SysDirContents dir_contents = SysGetDirContents(dir_name.data);
        VecForeach(&dir_contents, dir_entry, {
            // if it's a directory then store it for exploration lateron
            if (dir_entry.type == SYS_DIR_ENTRY_TYPE_DIRECTORY) {
                // create new directory path relative to current directory search path
                Str dir_name = StrInit();
                StrMerge(&dir_name, &current_path);
                StrPushBack(&dir_name, '/');
                StrMerge(&dir_name, &dir_entry.name);

                // store the director name
                VecPushBack(&dir_paths, dir_name);
            } else if (dir_entry.type == SYS_DIR_ENTRY_TYPE_REGULAR_FILE) {
                // create complete relative file path
                Str file_path = StrInit();
                StrMerge(&file_path, &current_path);
                StrPushBack(&file_path, '/');
                StrMerge(&file_path, &dir_entry.name);

                // store discovered file name
                VecPushBack(&file_paths, file_path);
            }
        });
        VecDeinit(&dir_contents);
    });
    VecDeinit(&dir_paths);

    VecForeach(&file_paths, file_path, { puts(file_path.data); });

    LogDeinit();
    return 0;
}
