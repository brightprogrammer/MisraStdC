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
///         "source_code" : "https://github.com/brightprogrammer/MisraStdC"
///         "maintainers" : [
///             {
///                 "name" : "Siddharth Mishra",
///                 "bio" : "Hey There!",
///                 "username" : "brightprogrammer",
///                 "email" : "admin@brightprogrammer.in",
///                 "socials" : {
///                     "website" : "https://brightprogrammer.in",
///                     "linkedin" : "https://linkedin.com/in/brightprogrammer",
///                     "youtube" : "https://youtube.com/@brightprogrammer",
///                     "x" : "https://x.com/brightprogramer"
///                 }
///             }
///         ],
///         "source_directories" : ["Source", "Include"],
///         "test_directories" : ["Test"],
///         "home_page" : "README.md"
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
        Project p = {0};                                                                                               \
        JR_OBJ(json, {                                                                                                 \
            JR_OBJ_KV(json, "project", {                                                                               \
                JR_STR_KV(json, "name", p.name);                                                                       \
                JR_STR_KV(json, "description", p.description);                                                         \
                JR_STR_KV(json, "source_code", p.source_code);                                                         \
                JR_STR_KV(json, "home_page", p.home_page);                                                             \
                JR_STR_KV(json, "build_dir", p.build_dir);                                                             \
                JR_ARR_KV(json, "maintainers", {                                                                       \
                    Maintainer m = {0};                                                                                \
                    JR_STR_KV(json, "name", m.name);                                                                   \
                    JR_STR_KV(json, "bio", m.bio);                                                                     \
                    JR_STR_KV(json, "username", m.username);                                                           \
                    JR_STR_KV(json, "email", m.email);                                                                 \
                    JR_ARR_KV(json, "socials", {                                                                       \
                        Social s = {0};                                                                                \
                        JR_STR_KV(json, "platform_name", s.platform_name);                                             \
                        JR_STR_KV(json, "profile_url", s.profile_url);                                                 \
                        VecPushBack(&m.socials, s);                                                                    \
                    });                                                                                                \
                    VecPushBack(&p.maintainers, m);                                                                    \
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

    LogDeinit();
    return 0;
}
