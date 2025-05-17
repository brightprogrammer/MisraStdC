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

# Regular expressions (revised for robustness)
comment_line_re = re.compile(r'^\s*///\s?(.*)')
# Regexes for content lines (after '///' stripping), allowing leading spaces
param_re = re.compile(r'^\s*(\w+)\[(in|out|in,out)\]\s*:\s*(.+)')
success_re = re.compile(r'^\s*SUCCESS\s*:\s*(.+)')
failure_re = re.compile(r'^\s*FAILURE\s*:\s*(.+)')
usage_start_re = re.compile(r'^\s*USAGE\s*:\s*$')
info_re = re.compile(r'^\s*INFO\s*:\s*(.*)')
note_re = re.compile(r'^\s*NOTE\s*:\s*(.*)')
warn_re = re.compile(r'^\s*WARN\s*:\s*(.*)')
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
    last_param = None  # Tracks the last parameter for multiline descriptions

    for line in lines:  # `lines` are already stripped of '/// ?' prefix by process_file

        # MODIFICATION 1: Skip blank lines only if not in 'usage' section
        if not line.strip() and section != "usage":
            continue
        # If section is "usage" and line is blank (e.g., "" or "   "), it proceeds.

        # Section detection
        if usage_start_re.match(line):
            section = "usage"
            last_param = None  # Reset last_param
            continue  # Don't add "USAGE:" line itself to usage_lines

        elif param_match := param_re.match(line):
            section = "params"
            param_name, direction, desc = param_match.groups()
            current_param = {
                "name": param_name,
                "direction": direction,
                "desc": desc.strip()
            }
            params.append(current_param)
            last_param = current_param  # Set for potential multiline description
            continue

        elif success_match := success_re.match(line):
            section = "success"
            success_lines = [success_match.group(1).strip()]
            last_param = None  # Reset last_param
            continue

        elif failure_match := failure_re.match(line):
            section = "failure"
            failure_lines = [failure_match.group(1).strip()]
            last_param = None  # Reset last_param
            continue

        # MODIFICATION 3: Corrected INFO/NOTE/WARN regex matching
        elif info_match := info_re.match(line):
            section = "info"
            info_lines = [info_match.group(1).strip()]
            last_param = None  # Reset last_param
            continue

        elif note_match := note_re.match(line):
            section = "note"
            note_lines = [note_match.group(1).strip()]
            last_param = None  # Reset last_param
            continue

        elif warn_match := warn_re.match(line):
            section = "warn"
            warn_lines = [warn_match.group(1).strip()]
            last_param = None  # Reset last_param
            continue

        # MODIFICATION 2: Handle multiline continuations for specific sections
        # This block processes lines starting with a space, intended as continuations.
        # It must only 'continue' if the line is consumed by one of these sections.
        if line.startswith(" "):
            if section == "params" and last_param:
                last_param["desc"] += " " + line.strip()
                continue  # Consumed as parameter description continuation
            elif section == "success" and success_lines:  # Check list exists
                success_lines.append(line.strip())
                continue  # Consumed as success description continuation
            elif section == "failure" and failure_lines:
                failure_lines.append(line.strip())
                continue  # Consumed as failure description continuation
            elif section == "info" and info_lines:
                info_lines.append(line.strip())
                continue  # Consumed as info description continuation
            elif section == "note" and note_lines:
                note_lines.append(line.strip())
                continue  # Consumed as note description continuation
            elif section == "warn" and warn_lines:
                warn_lines.append(line.strip())
                continue  # Consumed as warn description continuation
            # If line started with a space but was not a continuation of the above
            # (e.g., it's an indented brief line, or a USAGE code line),
            # it will fall through to the next block.

        # Append line to the current section's content
        if section == "brief":
            # Only add non-empty stripped lines to brief
            if line.strip():
                brief_lines.append(line.strip())
        elif section == "usage":
            usage_lines.append(line)  # Preserve all lines as-is for USAGE code
        # success_lines, failure_lines etc. are handled by their keyword match or multiline continuation.

    # Detect kind by analyzing the next_code_line (code right after docstring)
    kind = "function"  # default
    if next_code_line:
        code_line = next_code_line.strip()
        if code_line.startswith("#define") or re.match(r"^[A-Z0-9_]+\s*\(", code_line):
            kind = "macro"
        elif any(code_line.startswith(kw) for kw in ("struct ", "enum ", "union ", "typedef ")):
            kind = "type"
        else:
            func_pattern = re.compile(
                r"^[\w\*\s]+?\s+\**\w+\s*\([^;]*\)\s*;?$")
            if func_pattern.match(code_line):
                kind = "function"
            # else: kind remains "function" or previous fallback

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
        f.write(f'description: "Documentation for {symbol_name} {
                doc["kind"]}."\n')  # Made kind more specific
        f.write(f'date: {datetime.now().isoformat()}\n')
        # Capitalized category
        f.write(f'categories: ["{doc["kind"].capitalize()}"]\n')
        f.write("tags: [\"documentation\", \"generated\"]\n")
        f.write("draft: false\n")
        f.write("---\n\n")

        # Symbol title
        f.write(f"# <center>`{symbol_name}`</center>\n\n")

        # Description
        if doc["brief"]:  # Check if brief is not empty
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
            # doc["usage"] is already a multi-line string with preserved formatting
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
            comment_block_lines = []  # Renamed for clarity
            # Collect comment block lines, stripping '/// ?' prefix
            while i < len(lines) and (m := comment_line_re.match(lines[i])):
                # m.group(1) is the content after '/// ?'
                comment_block_lines.append(m.group(1))
                i += 1

            # Skip blank lines after comment block (before the code)
            while i < len(lines) and not lines[i].strip():
                i += 1

            next_code_line = lines[i].strip() if i < len(lines) else None

            if next_code_line:
                doc = parse_comment_block(
                    comment_block_lines, next_code_line=next_code_line)

                match = macro_or_func_re.match(next_code_line)
                if match:
                    symbol = match.group(1) or match.group(2)
                    if symbol:  # Ensure symbol was actually captured
                        write_markdown(symbol, doc)
        else:
            i += 1


def walk_source_tree(root):
    for dirpath, _, filenames in os.walk(root):
        for filename in filenames:
            if filename.endswith((".c", ".h")):
                print(f"Processing file: {os.path.join(
                    dirpath, filename)}")  # Added log
                process_file(os.path.join(dirpath, filename))


if __name__ == "__main__":
    walk_source_tree(ROOT_DIR)
    print("Documentation generation complete.")  # Added completion message
