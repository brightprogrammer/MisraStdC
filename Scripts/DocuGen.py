import os
import re
import argparse
from pathlib import Path
from datetime import datetime

# Default values
DEFAULT_ROOT_DIR = "Include"
DEFAULT_OUTPUT_DIR = "Docs/content/english/blog"

# Parse command-line arguments
parser = argparse.ArgumentParser(
    description="Generate documentation from comments.")
parser.add_argument("--root", default=DEFAULT_ROOT_DIR,
                    help="Root directory to scan for source files.")
parser.add_argument("--output", default=DEFAULT_OUTPUT_DIR,
                    help="Directory to output generated documentation.")
args = parser.parse_args()

ROOT_DIR = args.root
OUTPUT_DIR = args.output

print(f'Using root dir = {ROOT_DIR}')
print(f'Using output dir = {OUTPUT_DIR}')

os.makedirs(OUTPUT_DIR, exist_ok=True)

# Regular expressions
comment_line_re = re.compile(r'^\s*///\s?(.*)')
param_re = re.compile(r'^(\w+)\[(in|out|in,out)\]\s*:\s*(.+)')
success_re = re.compile(r'^SUCCESS\s*:\s*(.+)')
failure_re = re.compile(r'^FAILURE\s*:\s*(.+)')
usage_start_re = re.compile(r'^USAGE\s*:\s*$')
macro_or_func_re = re.compile(r'^\s*#define\s+(\w+)|^\s*(?:\w+\s+)+(\w+)\s*\(')


def parse_comment_block(lines, next_code_line=None):
    brief_lines = []
    params = []
    usage_lines = []

    success_lines = []
    failure_lines = []
    info_lines = []
    note_lines = []
    warn_lines = []

    section = "brief"
    last_param = None

    for line in lines:
        if not line.strip():
            continue

        if usage_start_re.match(line):
            section = "usage"
            continue

        elif success_match := success_re.match(line):
            section = "success"
            success_lines = [success_match.group(1).strip()]
            continue

        elif failure_match := failure_re.match(line):
            section = "failure"
            failure_lines = [failure_match.group(1).strip()]
            continue

        elif param_match := param_re.match(line):
            section = "params"
            param_name, direction, desc = param_match.groups()
            params.append({
                "name": param_name,
                "direction": direction,
                "desc": desc.strip()
            })
            last_param = params[-1]
            continue

        elif info_match := re.match(r"^///\s*INFO\s*:\s*(.*)", line):
            section = "info"
            info_lines = [info_match.group(1).strip()]
            continue

        elif note_match := re.match(r"^///\s*NOTE\s*:\s*(.*)", line):
            section = "note"
            note_lines = [note_match.group(1).strip()]
            continue

        elif warn_match := re.match(r"^///\s*WARN\s*:\s*(.*)", line):
            section = "warn"
            warn_lines = [warn_match.group(1).strip()]
            continue

        # Handle multiline for each section
        if line.startswith(" "):
            if section == "params" and last_param:
                last_param["desc"] += " " + line.strip()
            elif section == "success":
                success_lines.append(line.strip())
            elif section == "failure":
                failure_lines.append(line.strip())
            elif section == "info":
                info_lines.append(line.strip())
            elif section == "note":
                note_lines.append(line.strip())
            elif section == "warn":
                warn_lines.append(line.strip())
            continue

        if section == "brief":
            brief_lines.append(line.strip())
        elif section == "usage":
            usage_lines.append(line)

    # Detect kind by analyzing the next_code_line (code right after docstring)
    kind = "function"  # default

    if next_code_line:
        code_line = next_code_line.strip()
        # Macro detection: usually starts with #define or macro-like naming
        if code_line.startswith("#define") or re.match(r"^[A-Z0-9_]+\s*\(", code_line):
            kind = "macro"
        # Type detection: struct, enum, union, typedef
        elif any(code_line.startswith(kw) for kw in ("struct ", "enum ", "union ", "typedef ")):
            kind = "type"
        # Function detection: look for typical function pattern (return_type func_name(...))
        else:
            # Optionally, check for function signature using regex:
            func_pattern = re.compile(
                r"^[\w\*\s]+?\s+\**\w+\s*\([^;]*\)\s*;?$")
            if func_pattern.match(code_line):
                kind = "function"
            else:
                # fallback if none matches:
                kind = "function"

    return {
        "brief": " ".join(brief_lines),
        "params": params,
        "usage": "\n".join(usage_lines) if usage_lines else None,
        "success": " ".join(success_lines) if success_lines else None,
        "failure": " ".join(failure_lines) if failure_lines else None,
        "info": " ".join(info_lines) if info_lines else None,
        "note": " ".join(note_lines) if note_lines else None,
        "warn": " ".join(warn_lines) if warn_lines else None,
        "kind": kind,
    }


def write_markdown(symbol_name, doc):
    out_path = Path(OUTPUT_DIR) / f"generated-doc-{symbol_name}.md"
    print(f"Writing: {out_path}")
    with open(out_path, "w") as f:
        # Write front matter
        f.write("---\n")
        f.write(f'title: "{symbol_name}"\n')
        f.write(f'meta_title: "{symbol_name}"\n')
        f.write(f'description: "Documentation for {
                symbol_name} macro or function."\n')
        f.write(f'date: {datetime.now().isoformat()}\n')
        f.write(f"categories: [{doc["kind"]}]\n")
        f.write("tags: [\"documentation\", \"generated\"]\n")
        f.write("draft: false\n")
        f.write("---\n\n")

        # Symbol title
        f.write(f"# <center>`{symbol_name}`</center>\n\n")

        # Description
        f.write("## Description\n\n")
        f.write(doc["brief"] + "\n\n")

        # Parameters
        if doc["params"]:
            f.write("## Parameters\n\n")
            f.write("| Name | Direction | Description |\n")
            f.write("|------|-----------|-------------|\n")
            for p in doc["params"]:
                f.write(f"| `{p['name']}` | {
                        p['direction']} | {p['desc']} |\n")
            f.write("\n")

        # Usage
        if doc["usage"]:
            f.write("## Usage\n\n")
            f.write("```c\n")
            f.write(doc["usage"])
            f.write("\n```\n\n")

        # Success
        if doc["success"]:
            f.write("## Success\n\n" + doc["success"] + "\n\n")

        # Failure
        if doc["failure"]:
            f.write("## Failure\n\n" + doc["failure"] + "\n\n")

        # Info messages
        if doc.get("info"):
            f.write("## Info\n\n" + doc["info"] + "\n\n")

        # Note
        if doc.get("note"):
            f.write("## Note\n\n" + doc["note"] + "\n\n")

        # Warnings
        if doc.get("warn"):
            f.write("## Warning\n\n" + doc["warn"] + "\n\n")


def process_file(filepath):
    with open(filepath, "r") as f:
        lines = f.readlines()

    i = 0
    while i < len(lines):
        if comment_line_re.match(lines[i]):
            comment_block = []
            # Collect comment block lines without '///' prefix
            while i < len(lines) and (m := comment_line_re.match(lines[i])):
                comment_block.append(m.group(1))
                i += 1

            # Skip blank lines after comment block
            while i < len(lines) and not lines[i].strip():
                i += 1

            next_code_line = lines[i].strip() if i < len(lines) else None

            if next_code_line:
                # Pass next_code_line to parse_comment_block for kind detection
                doc = parse_comment_block(
                    comment_block, next_code_line=next_code_line)

                # Extract symbol name (macro or function)
                match = macro_or_func_re.match(next_code_line)
                if match:
                    symbol = match.group(1) or match.group(2)
                    write_markdown(symbol, doc)
        else:
            i += 1


def walk_source_tree(root):
    for dirpath, _, filenames in os.walk(root):
        for filename in filenames:
            if filename.endswith((".c", ".h")):
                process_file(os.path.join(dirpath, filename))


if __name__ == "__main__":
    walk_source_tree(ROOT_DIR)
