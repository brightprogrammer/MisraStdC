/// file      : Bin/DocGen.c
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// This is free and unencumbered software released into the public domain.
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
#include <Misra/Std/Allocator/Default.h>

typedef struct Project {
    Str  build_dir;
    Strs source_directories;
    Strs test_directories;
} Project;

static void project_deinit(Project *p) {
    if (!p) {
        LOG_FATAL("Invalid project object. Invalid arguments");
    }
    StrDeinit(&p->build_dir);
    VecForeach(&p->source_directories, s) {
        StrDeinit(&s);
    };
    VecForeach(&p->test_directories, s) {
        StrDeinit(&s);
    };
    VecDeinit(&p->source_directories);
    VecDeinit(&p->test_directories);
}

// Walk a JSON object; for each key invoke on_key(key, &value_si, ctx).
static StrIter
    parse_object_keys(StrIter si, Allocator *alloc, void (*on_key)(Str *key, StrIter *value_si, void *ctx), void *ctx) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }
    StrIter saved_si = si;
    si               = JSkipWhitespace(si);
    if (StrIterPeek(&si) != '{') {
        LOG_ERROR("Invalid object start. Expected '{'.");
        return saved_si;
    }
    StrIterNext(&si);
    si = JSkipWhitespace(si);

    bool expect_comma = false;
    while (StrIterPeek(&si) && StrIterPeek(&si) != '}') {
        if (expect_comma) {
            if (StrIterPeek(&si) != ',') {
                LOG_ERROR("Expected ',' between key/value pairs.");
                return saved_si;
            }
            StrIterNext(&si);
            si = JSkipWhitespace(si);
        }

        Str     key     = StrInit(alloc);
        StrIter read_si = JReadString(si, &key);
        if (read_si.pos == si.pos) {
            LOG_ERROR("Failed to read key.");
            StrDeinit(&key);
            return saved_si;
        }
        si = read_si;
        si = JSkipWhitespace(si);
        if (StrIterPeek(&si) != ':') {
            LOG_ERROR("Expected ':' after key.");
            StrDeinit(&key);
            return saved_si;
        }
        StrIterNext(&si);
        si = JSkipWhitespace(si);

        StrIter si_before = si;
        on_key(&key, &si, ctx);
        if (si.pos == si_before.pos) {
            si = JSkipValue(si);
        }
        StrDeinit(&key);
        si           = JSkipWhitespace(si);
        expect_comma = true;
    }
    if (StrIterPeek(&si) == '}') {
        StrIterNext(&si);
    }
    return si;
}

static StrIter parse_string_array(StrIter si, Strs *out, Allocator *alloc) {
    if (!StrIterRemainingLength(&si)) {
        return si;
    }
    StrIter saved_si = si;
    si               = JSkipWhitespace(si);
    if (StrIterPeek(&si) != '[') {
        LOG_ERROR("Invalid array start. Expected '['.");
        return saved_si;
    }
    StrIterNext(&si);
    si = JSkipWhitespace(si);

    bool expect_comma = false;
    while (StrIterPeek(&si) && StrIterPeek(&si) != ']') {
        if (expect_comma) {
            if (StrIterPeek(&si) != ',') {
                LOG_ERROR("Expected ',' between array values.");
                return saved_si;
            }
            StrIterNext(&si);
            si = JSkipWhitespace(si);
        }
        Str     s  = StrInit(alloc);
        StrIter rs = JReadString(si, &s);
        if (rs.pos == si.pos) {
            StrDeinit(&s);
            si = JSkipValue(si);
        } else {
            si = rs;
            VecPushBack(out, s);
        }
        si           = JSkipWhitespace(si);
        expect_comma = true;
    }
    if (StrIterPeek(&si) == ']') {
        StrIterNext(&si);
    }
    return si;
}

typedef struct ProjCtx {
    Project   *p;
    Allocator *alloc;
} ProjCtx;

static void project_on_key(Str *key, StrIter *value_si, void *vctx) {
    ProjCtx *ctx = (ProjCtx *)vctx;
    if (!StrCmpZstr(key, "build_dir")) {
        Str s     = StrInit(ctx->alloc);
        *value_si = JReadString(*value_si, &s);
        StrDeinit(&ctx->p->build_dir);
        ctx->p->build_dir = s;
    } else if (!StrCmpZstr(key, "source_directories")) {
        *value_si = parse_string_array(*value_si, &ctx->p->source_directories, ctx->alloc);
    } else if (!StrCmpZstr(key, "test_directories")) {
        *value_si = parse_string_array(*value_si, &ctx->p->test_directories, ctx->alloc);
    }
}

static void top_on_key(Str *key, StrIter *value_si, void *vctx) {
    ProjCtx *ctx = (ProjCtx *)vctx;
    if (!StrCmpZstr(key, "project")) {
        *value_si = parse_object_keys(*value_si, ctx->alloc, project_on_key, ctx);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        File err_ = FileStderr();
        FWriteFmtLn(&err_, "USAGE : {} config.json", argc == 0 ? "docgen" : argv[0]);
        return 1;
    }

    DefaultAllocator alloc = DefaultAllocatorInit();

    const char *config_path = argv[1];

    Project project            = {0};
    project.build_dir          = StrInit(&alloc);
    project.source_directories = VecInitT(project.source_directories, &alloc);
    project.test_directories   = VecInitT(project.test_directories, &alloc);

    // read project config
    Str config = StrInit(&alloc);
    if (!ReadCompleteFile(config_path, &config.data, &config.length, &config.capacity, &alloc.base)) {
        LOG_FATAL("Failed to read config file.");
    }
    StrIter json = StrIterFromStr(config);

    ProjCtx ctx = {.p = &project, .alloc = &alloc.base};
    json        = parse_object_keys(json, &alloc.base, top_on_key, &ctx);
    StrDeinit(&config);

    // recursively explore directories and get files that need documentation
    Strs file_paths = VecInit(&alloc);
    Strs dir_paths  = VecInit(&alloc);

    // Seed dir_paths from project source/test dirs (copy so we own these strings)
    VecForeach(&project.source_directories, src_dir) {
        Str copy = StrInitFromStr(&src_dir, &alloc);
        VecPushBack(&dir_paths, copy);
    };
    VecForeach(&project.test_directories, src_dir) {
        Str copy = StrInitFromStr(&src_dir, &alloc);
        VecPushBack(&dir_paths, copy);
    };

    // recursively explore directories and get filenames
    for (u64 i = 0; i < dir_paths.length; ++i) {
        Str *dir_name = &dir_paths.data[i];

        DirContents dir_contents = DirGetContents(dir_name->data, &alloc.base);
        VecForeach(&dir_contents, dir_entry) {
            if (dir_entry.type == SYS_DIR_ENTRY_TYPE_DIRECTORY) {
                Str path = StrInit(&alloc);
                StrMerge(&path, dir_name);
                StrPushBack(&path, '/');
                StrMerge(&path, &dir_entry.name);
                VecPushBack(&dir_paths, path);
                // VecPushBack may have realloc'd; re-fetch dir_name pointer.
                dir_name = &dir_paths.data[i];
            } else if (dir_entry.type == SYS_DIR_ENTRY_TYPE_REGULAR_FILE) {
                Str path = StrInit(&alloc);
                StrMerge(&path, dir_name);
                StrPushBack(&path, '/');
                StrMerge(&path, &dir_entry.name);
                VecPushBack(&file_paths, path);
            }
        };
        VecForeach(&dir_contents, e) {
            StrDeinit(&e.name);
        };
        VecDeinit(&dir_contents);
    }

    // go over each file and generate corresponding markdown
    VecForeach(&file_paths, file_path) {
        Str file_contents = StrInit(&alloc);
        if (!ReadCompleteFile(
                file_path.data,
                &file_contents.data,
                &file_contents.length,
                &file_contents.capacity,
                &alloc.base
            )) {
            LOG_ERROR("Failed to read \"{}\" source file.", file_path.data);
            StrDeinit(&file_contents);
            continue;
        }

        Str output_path = StrInit(&alloc);
        StrMerge(&output_path, &file_path);
        LOG_INFO("{}", output_path);
        StrReplaceZstr(&output_path, "/", "-", -1);
        LOG_INFO("{}", output_path);

        Str md_code = StrInit(&alloc);
        // Create template strings for StrWriteFmt with escaped braces
        const char *mdHeader =
            "---\n"
            "title: \"{}\"\n"
            "meta_title: \"{}\"\n"
            "description: \"Documentation for {}\"\n"
            "date: 2025-05-12T05:00:00Z\n"
            "# image: \"/images/image-placeholder.png\"\n"
            "categories: [\"Vec\", \"Macro\", \"Generic\"]\n"
            "author: \"Siddharth Mishra\"\n"
            "tags: [\"vec\", \"macro\", \"generic\"]\n"
            "draft: false\n"
            "---\n"
            "```c\n";

        StrWriteFmt(&md_code, mdHeader, output_path.data, output_path.data, output_path.data);
        StrMerge(&md_code, &file_contents);
        StrWriteFmt(&md_code, "\n```");

        // complete relative file path
        StrPushFront(&output_path, '/');
        LOG_INFO("{}", output_path);
        StrPushFrontCstr(&output_path, project.build_dir.data, project.build_dir.length);
        LOG_INFO("{}", output_path);
        StrReplaceZstr(&output_path, ".c", ".md", 1);
        StrReplaceZstr(&output_path, ".h", ".md", 1);
        LOG_INFO("{}\n\n", output_path);

        // dump code to output path
        File f = FileOpen(output_path.data, "w");
        if (FileIsValid(&f)) {
            FileWrite(&f, md_code.data, md_code.length);
            FileClose(&f);
        }

        StrDeinit(&md_code);
        StrDeinit(&output_path);
        StrDeinit(&file_contents);
    };

    VecForeach(&file_paths, p) {
        StrDeinit(&p);
    };
    VecDeinit(&file_paths);
    VecForeach(&dir_paths, p) {
        StrDeinit(&p);
    };
    VecDeinit(&dir_paths);

    project_deinit(&project);

    DefaultAllocatorDeinit(&alloc);
    return 0;
}
