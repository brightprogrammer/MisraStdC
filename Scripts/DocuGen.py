import os
import re
import argparse
from pathlib import Path
from datetime import datetime
from urllib.parse import quote
from pathlib import PurePosixPath

# Default values
# Changed to a list for multiple root dirs
DEFAULT_ROOT_DIRS = ["Include", "Source"]
DEFAULT_OUTPUT_DIR = "Docs/content/english/blog"

# Global store for all parsed symbols and their associated data
# Format: { 'symbol_name': {'name': ..., 'kind': ..., 'doc': ..., 'filepath': ..., 'def_lineno': ..., 'definition_line_content': ...}, ... }
parsed_symbols_map = {}

# Global store for file contents to avoid re-reading
# Format: { 'filepath_str': ['line1', 'line2', ...], ... }
file_contents_map = {}

# Regular expressions (revised for robustness)
comment_line_re = re.compile(r'^\s*///\s?(.*)')
# Regexes for content lines (after '///' stripping), allowing leading spaces
param_re = re.compile(r'^\s*(\w+)\[(in|out|in,out)\]\s*:\s*(.+)')
success_re = re.compile(r'^\s*SUCCESS\s*:\s*(.+)')
failure_re = re.compile(r'^\s*FAILURE\s*:\s*(.+)')
usage_start_re = re.compile(r'^\s*USAGE\s*:\s*$')
info_re = re.compile(r'^\s*INFO\s*:\s*(.+)')
note_re = re.compile(r'^\s*NOTE\s*:\s*(.+)')
warn_re = re.compile(r'^\s*WARN\s*:\s*(.+)')
macro_or_func_re = re.compile(r'^\s*#define\s+(\w+)|^\s*(?:\w+\s+)+(\w+)\s*\(')


def collect_symbols_and_content(filepath_obj: Path):
    filepath_str = str(filepath_obj)
    try:
        with open(filepath_obj, "r", encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
        # Store file content for Pass 2
        file_contents_map[filepath_str] = lines
    except Exception as e:
        print(f"Error reading file {filepath_str}: {e}")
        return

    local_i = 0
    while local_i < len(lines):
        current_line_content = lines[local_i]
        if comment_line_re.match(current_line_content):
            comment_block_lines_content = []

            idx_after_comment = local_i
            while idx_after_comment < len(lines) and \
                    (m := comment_line_re.match(lines[idx_after_comment])):
                comment_block_lines_content.append(m.group(1))
                idx_after_comment += 1

            idx_of_definition_line = idx_after_comment
            while idx_of_definition_line < len(lines) and \
                    not lines[idx_of_definition_line].strip():
                idx_of_definition_line += 1

            if idx_of_definition_line < len(lines):
                next_code_line_content = lines[idx_of_definition_line].strip()
                doc = parse_comment_block(comment_block_lines_content,
                                          next_code_line=next_code_line_content)

                match = macro_or_func_re.match(next_code_line_content)
                if match:
                    symbol_name = match.group(1) or match.group(2)
                    if symbol_name:
                        if symbol_name in parsed_symbols_map:
                            # Basic duplicate handling: log and overwrite.
                            # Consider more sophisticated handling if needed.
                            print(f"Warning: Duplicate symbol '{symbol_name}' detected. "
                                  f"Old: {parsed_symbols_map[symbol_name]['filepath']}:{
                                      parsed_symbols_map[symbol_name]['def_lineno']}, "
                                  f"New: {filepath_str}:{idx_of_definition_line + 1}. Overwriting.")
                        parsed_symbols_map[symbol_name] = {
                            "name": symbol_name,
                            "kind": doc["kind"],
                            "doc": doc,
                            "filepath": filepath_str,
                            "def_lineno": idx_of_definition_line + 1,  # 1-indexed
                            "definition_line_content": next_code_line_content
                        }

            # Advance main loop counter past the processed block
            # If a definition was found, start after it, otherwise after the comment block
            local_i = (idx_of_definition_line + 1) \
                if idx_of_definition_line < len(lines) and next_code_line_content \
                else idx_after_comment
        else:
            local_i += 1


def find_usages(current_symbols_map, all_file_contents_map, context_lines=2):
    symbol_usages_xref = {name: [] for name in current_symbols_map}

    for symbol_name, symbol_data in current_symbols_map.items():
        if not symbol_name:
            continue

        try:
            symbol_regex = re.compile(r'\b' + re.escape(symbol_name) + r'\b')
        except re.error:
            print(f"Warning: Could not compile regex for symbol '{
                  symbol_name}'. Skipping usage scan for it.")
            continue

        for filepath_str, lines in all_file_contents_map.items():
            in_block_comment = False

            for line_num_0_indexed, line_content in enumerate(lines):
                stripped = line_content.strip()

                # Detect start and end of multiline comments
                if "/*" in stripped:
                    in_block_comment = True
                if "*/" in stripped:
                    in_block_comment = False
                    continue  # Skip the line that ends a block comment

                # Skip line if inside block comment or line comment
                if in_block_comment or stripped.startswith("//"):
                    continue

                if symbol_regex.search(line_content):
                    is_definition_line = (
                        filepath_str == symbol_data['filepath'] and
                        (line_num_0_indexed + 1) == symbol_data['def_lineno']
                    )

                    if not is_definition_line:
                        # Extract context
                        start = max(0, line_num_0_indexed - context_lines)
                        end = min(len(lines), line_num_0_indexed +
                                  context_lines + 1)
                        snippet_lines = lines[start:end]

                        # Extra: Skip snippets where all lines are commented
                        if all(l.strip().startswith("//") or '/*' in l or '*/' in l for l in snippet_lines):
                            continue

                        code_snippet = '\n'.join(line.rstrip()
                                                 for line in snippet_lines)

                        symbol_usages_xref[symbol_name].append({
                            "filepath": filepath_str,
                            "lineno": line_num_0_indexed + 1,
                            "code": code_snippet
                        })

    return symbol_usages_xref


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


def write_markdown(symbol_name, symbol_data_item, symbol_usages_list, resolved_output_dir: Path):
    doc = symbol_data_item["doc"]
    # output_file_path is absolute because resolved_output_dir is absolute
    output_file_path = resolved_output_dir / f"generated-doc-{symbol_name}.md"
    markdown_file_dir = output_file_path.parent  # also absolute

    print(f"Writing: {output_file_path}")
    with open(output_file_path, "w", encoding='utf-8') as f:
        f.write("---\n")
        f.write(f'title: "{symbol_name}"\n')
        f.write(f'meta_title: "{symbol_name}"\n')  # Added meta_title
        f.write(f'description: "Documentation for {
                symbol_name} {doc["kind"]}."\n')
        f.write(f'date: {datetime.now().isoformat()}\n')
        f.write(f'categories: ["{doc["kind"].capitalize()}"]\n')
        f.write("tags: [\"documentation\", \"generated\", \"api\"]\n")
        f.write("draft: false\n")
        f.write("---\n\n")

        f.write(f"# <center>`{symbol_name}`</center>\n\n")

        if doc.get("brief"):
            f.write("## Description\n\n")
            f.write(doc["brief"] + "\n\n")

        # Info messages
        if doc.get("info"):
            f.write('{{< notice "info" >}}\n\n' +
                    doc["info"] + '\n\n{{< /notice >}}')

        # Note
        if doc.get("note"):
            f.write('{{< notice "note" >}}\n\n' +
                    doc["note"] + '\n\n{{< /notice >}}')

        # Warnings
        if doc.get("warn"):
            f.write('{{< notice "warning" >}}\n\n' +
                    doc["warn"] + '\n\n{{< /notice >}}')

        if doc.get("params"):
            f.write("## Parameters\n\n")
            f.write("| Name | Direction | Description |\n")
            f.write("|------|-----------|-------------|\n")
            for p in doc["params"]:
                f.write(f"| `{p['name']}` | {
                        p['direction']} | {p['desc']} |\n")
            f.write("\n")

        if doc.get("usage"):
            f.write("## Usage (from documentation)\n\n")
            f.write("```c\n")
            f.write(doc["usage"])
            f.write("\n```\n\n")

        for section_key in ["success", "failure", "info", "note", "warn"]:
            if doc.get(section_key):
                f.write(f"## {section_key.capitalize()}\n\n{
                        doc[section_key]}\n\n")

        f.write("{{< accordion \"Usage examples (Cross-references)\" >}}\n")
        if symbol_usages_list:
            for usage in symbol_usages_list:
                # This is an absolute path
                usage_file_path_obj = Path(usage['filepath'])
                try:
                    # markdown_file_dir is absolute. usage_file_path_obj is absolute.
                    relative_usage_path_str = os.path.relpath(
                        usage_file_path_obj, start=markdown_file_dir)
                    # Ensure forward slashes
                    link_path = Path(relative_usage_path_str).as_posix()
                except ValueError:
                    link_path = Path(usage['filepath']).name
                    print(f"Warning: Could not create relative path for {
                          usage['filepath']} from {markdown_file_dir}. Using filename.")

                # Clean and indent code block
                escaped_lines = usage['code'].splitlines()
                cleaned_lines = ["    " + line.strip()
                                 for line in escaped_lines]
                escaped_code = '\n'.join(cleaned_lines)

                # Convert to a Posix path and remove leading parent references
                p = PurePosixPath(link_path)

                # Remove all leading '..' parts:
                parts = [part for part in p.parts if part != ".."]
                clean_path = "/".join(parts)

                print(clean_path)  # Output: Include/Misra/Std/Utility/StrIter.h

                # Then build your URL:
                github_url = f"https://github.com/brightprogrammer/MisraStdC/blob/master/{
                    clean_path}#L{usage['lineno']}"

                f.write(
                    f'* In [`{Path(usage["filepath"]).name}:{usage["lineno"]}`]({github_url}):\n\n')
                f.write(f"```c\n{escaped_code}\n```\n\n")
        else:
            f.write("No external code usages found in the scanned files.\n")
        f.write("{{< /accordion >}}\n\n")


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


# --- Main Execution ---
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate documentation from C comments with cross-references.")
    parser.add_argument(
        "--root",
        default=DEFAULT_ROOT_DIRS,  # Default is now a list
        nargs='+',  # Allows one or more arguments for --root
        metavar='DIR',
        help=f"Root directory/directories to scan for source files. Can specify multiple. Default: {
            ' '.join(DEFAULT_ROOT_DIRS)}"
    )
    parser.add_argument(
        "--output",
        default=DEFAULT_OUTPUT_DIR,
        help=f"Directory to output generated documentation. Default: {
            DEFAULT_OUTPUT_DIR}"
    )
    args = parser.parse_args()

    ROOT_DIRS_list = args.root  # This will be a list of directory strings
    OUTPUT_DIR_str = args.output

    # Resolve OUTPUT_DIR to an absolute path and create it
    resolved_output_dir = Path(OUTPUT_DIR_str).resolve()
    os.makedirs(resolved_output_dir, exist_ok=True)
    print(f"Output directory: {resolved_output_dir}")

    # --- Pass 1: Collect symbols and file contents ---
    print("\n--- Starting Pass 1: Collecting symbols and file contents ---")
    for root_dir_item_str in ROOT_DIRS_list:
        # Resolve each root dir to absolute
        root_dir_path_obj = Path(root_dir_item_str).resolve()
        if not root_dir_path_obj.is_dir():
            print(f"Warning: Provided root path '{root_dir_item_str}' ({
                  root_dir_path_obj}) is not a valid directory. Skipping.")
            continue
        print(f"Pass 1: Scanning under root directory: {root_dir_path_obj}")
        for dirpath_str, _, filenames in os.walk(root_dir_path_obj):
            current_dirpath_obj = Path(dirpath_str)
            for filename in filenames:
                if filename.endswith((".c", ".h")):
                    # Construct absolute path for the file to be processed
                    abs_filepath_to_process = (
                        current_dirpath_obj / filename).resolve()
                    # print(f"Pass 1: Processing {abs_filepath_to_process}") # Verbose
                    collect_symbols_and_content(abs_filepath_to_process)
    print(f"--- Pass 1 complete. Found {len(parsed_symbols_map)
                                        } unique symbols. Read {len(file_contents_map)} files. ---")

    # --- Pass 2: Find usages for all collected symbols ---
    print("\n--- Starting Pass 2: Finding symbol usages ---")
    symbol_usages_xref_map = find_usages(parsed_symbols_map, file_contents_map)
    usage_count = sum(len(usages)
                      for usages in symbol_usages_xref_map.values())
    print(
        f"--- Pass 2 complete. Found {usage_count} total usage instances. ---")

    # --- Pass 3: Generate Markdown for each symbol ---
    print("\n--- Starting Pass 3: Generating Markdown files ---")
    if not parsed_symbols_map:
        print("No symbols found to document.")
    else:
        for symbol_name_key, symbol_data in parsed_symbols_map.items():
            usages = symbol_usages_xref_map.get(symbol_name_key, [])
            write_markdown(symbol_name_key, symbol_data,
                           usages, resolved_output_dir)

    print("\n--- Documentation generation complete. ---")

    # --- Pass 2: Find usages for all collected symbols ---
    print("\n--- Starting Pass 2: Finding symbol usages ---")
    symbol_usages_xref_map = find_usages(parsed_symbols_map, file_contents_map)
    usage_count = sum(len(usages)
                      for usages in symbol_usages_xref_map.values())
    print(
        f"--- Pass 2 complete. Found {usage_count} total usage instances. ---")

    # --- Pass 3: Generate Markdown for each symbol ---
    print("\n--- Starting Pass 3: Generating Markdown files ---")
    if not parsed_symbols_map:
        print("No symbols found to document.")
    else:
        for symbol_name_key, symbol_data in parsed_symbols_map.items():
            usages = symbol_usages_xref_map.get(symbol_name_key, [])
            write_markdown(symbol_name_key, symbol_data,
                           usages, resolved_output_dir)

    print("\n--- Documentation generation complete. ---")

    # --- Pass 2: Find usages for all collected symbols ---
    print("\n--- Starting Pass 2: Finding symbol usages ---")
    symbol_usages_xref_map = find_usages(parsed_symbols_map, file_contents_map)
    usage_count = sum(len(usages)
                      for usages in symbol_usages_xref_map.values())
    print(
        f"--- Pass 2 complete. Found {usage_count} total usage instances. ---")

    # --- Pass 3: Generate Markdown for each symbol ---
    print("\n--- Starting Pass 3: Generating Markdown files ---")
    if not parsed_symbols_map:
        print("No symbols found to document.")
    else:
        for symbol_name_key, symbol_data in parsed_symbols_map.items():
            usages = symbol_usages_xref_map.get(symbol_name_key, [])
            write_markdown(symbol_name_key, symbol_data,
                           usages, resolved_output_dir)

    print("\n--- Documentation generation complete. ---")

    # --- Pass 2: Find usages for all collected symbols ---
    print("\n--- Starting Pass 2: Finding symbol usages ---")
    symbol_usages_xref_map = find_usages(parsed_symbols_map, file_contents_map)
    usage_count = sum(len(usages)
                      for usages in symbol_usages_xref_map.values())
    print(
        f"--- Pass 2 complete. Found {usage_count} total usage instances. ---")

    # --- Pass 3: Generate Markdown for each symbol ---
    print("\n--- Starting Pass 3: Generating Markdown files ---")
    if not parsed_symbols_map:
        print("No symbols found to document.")
    else:
        for symbol_name_key, symbol_data in parsed_symbols_map.items():
            usages = symbol_usages_xref_map.get(symbol_name_key, [])
            write_markdown(symbol_name_key, symbol_data,
                           usages, resolved_output_dir)

    print("\n--- Documentation generation complete. ---")
