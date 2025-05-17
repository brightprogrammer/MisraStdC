import os
import re
import argparse
from pathlib import Path
from datetime import datetime
from pathlib import PurePosixPath

# Configuration
DEFAULT_ROOT_DIRS = ["."]
DEFAULT_OUTPUT_DIR = "Docs/content/english/blog"

# Data Stores
parsed_symbols = {}
file_contents = {}

# Regular Expressions
RE_COMMENT_LINE = re.compile(r'^\s*///\s?(.*)')
RE_PARAM = re.compile(r'^\s*(\w+)\[(in|out|in,out)\]\s*:\s*(.+)')
RE_SUCCESS = re.compile(r'^\s*SUCCESS\s*:\s*(.+)')
RE_FAILURE = re.compile(r'^\s*FAILURE\s*:\s*(.+)')
RE_USAGE_START = re.compile(r'^\s*USAGE\s*:\s*$')
RE_INFO = re.compile(r'^\s*INFO\s*:\s*(.+)')
RE_NOTE = re.compile(r'^\s*NOTE\s*:\s*(.+)')
RE_WARN = re.compile(r'^\s*WARN\s*:\s*(.+)')
RE_SYMBOL_DEFINITION = re.compile(
    r'^\s*#define\s+(\w+)|^\s*(?:\w+\s+)+(\w+)\s*\(')


def extract_symbols_and_store_content(file_path: Path):
    """
    Reads a file, extracts documented symbols (macros or functions),
    and stores the file content for later usage analysis.

    Args:
        file_path (Path): The path to the C/H source file.
    """
    file_path_str = str(file_path)
    try:
        with open(file_path, "r", encoding='utf-8', errors='ignore') as file:
            lines = file.readlines()
        file_contents[file_path_str] = lines
    except Exception as error:
        print(f"Error reading file {file_path_str}: {error}")
        return

    line_index = 0
    while line_index < len(lines):
        current_line = lines[line_index]
        if RE_COMMENT_LINE.match(current_line):
            comment_block = []
            comment_index = line_index
            while comment_index < len(lines) and (match := RE_COMMENT_LINE.match(lines[comment_index])):
                comment_block.append(match.group(1))
                comment_index += 1

            definition_index = comment_index
            while definition_index < len(lines) and not lines[definition_index].strip():
                definition_index += 1

            if definition_index < len(lines):
                code_line = lines[definition_index].strip()
                documentation = parse_comment_block(
                    comment_block, next_code_line=code_line)
                symbol_match = RE_SYMBOL_DEFINITION.match(code_line)
                if symbol_match:
                    symbol_name = symbol_match.group(
                        1) or symbol_match.group(2)
                    if symbol_name:
                        if symbol_name in parsed_symbols:
                            print(f"Warning: Duplicate symbol '{symbol_name}' found. "
                                  f"Old: {parsed_symbols[symbol_name]['filepath']}:{
                                      parsed_symbols[symbol_name]['def_lineno']}, "
                                  f"New: {file_path_str}:{definition_index + 1}. Overwriting.")
                        parsed_symbols[symbol_name] = {
                            "name": symbol_name,
                            "kind": documentation["kind"],
                            "doc": documentation,
                            "filepath": file_path_str,
                            "def_lineno": definition_index + 1,
                            "definition_line_content": code_line
                        }
                line_index = definition_index + 1
            else:
                line_index = comment_index
        else:
            line_index += 1


def find_symbol_usages(symbols, all_contents, context_lines=2):
    """
    Finds all occurrences of the documented symbols within the scanned files.

    Args:
        symbols (dict): A dictionary of parsed symbols.
        all_contents (dict): A dictionary containing the content of each scanned file.
        context_lines (int): The number of context lines to include in the usage snippet.

    Returns:
        dict: A dictionary where keys are symbol names and values are lists of usage locations.
    """
    symbol_usages = {name: [] for name in symbols}

    for symbol_name, symbol_data in symbols.items():
        if not symbol_name:
            continue
        try:
            symbol_regex = re.compile(r'\b' + re.escape(symbol_name) + r'\b')
        except re.error:
            print(f"Warning: Could not compile regex for symbol '{
                  symbol_name}'. Skipping usage scan.")
            continue

        for file_path_str, lines in all_contents.items():
            in_block_comment = False
            for line_num, line in enumerate(lines):
                stripped_line = line.strip()

                if "/*" in stripped_line:
                    in_block_comment = True
                if "*/" in stripped_line:
                    in_block_comment = False
                    continue

                if in_block_comment or stripped_line.startswith("//"):
                    continue

                if symbol_regex.search(line):
                    is_definition = (
                        file_path_str == symbol_data['filepath'] and
                        (line_num + 1) == symbol_data['def_lineno']
                    )
                    if not is_definition:
                        start_index = max(0, line_num - context_lines)
                        end_index = min(
                            len(lines), line_num + context_lines + 1)
                        snippet = lines[start_index:end_index]

                        if not all(l.strip().startswith("//") or '/*' in l or '*/' in l for l in snippet):
                            code_snippet = '\n'.join(
                                s.rstrip() for s in snippet)
                            symbol_usages[symbol_name].append({
                                "filepath": file_path_str,
                                "lineno": line_num + 1,
                                "code": code_snippet
                            })
    return symbol_usages


def parse_comment_block(comment_lines, next_code_line=None):
    """
    Parses a block of comment lines to extract documentation details.

    Args:
        comment_lines (list): A list of strings, where each string is a line from the comment block
                             (stripped of the '/// ?' prefix).
        next_code_line (str, optional): The line of code immediately following the comment block.
                                         Used to determine the type of the documented symbol. Defaults to None.

    Returns:
        dict: A dictionary containing the parsed documentation elements.
    """
    brief = []
    params = []
    usage = []
    success = []
    failure = []
    info = []
    note = []
    warn = []

    current_section = "brief"
    last_parameter = None

    for line in comment_lines:
        stripped_line = line.strip()
        if not stripped_line and current_section != "usage":
            continue

        if RE_USAGE_START.match(line):
            current_section = "usage"
            last_parameter = None
            continue
        elif match := RE_PARAM.match(line):
            current_section = "params"
            param_name, direction, description = match.groups()
            current_param = {"name": param_name,
                             "direction": direction, "desc": description.strip()}
            params.append(current_param)
            last_parameter = current_param
            continue
        elif match := RE_SUCCESS.match(line):
            current_section = "success"
            success.append(match.group(1).strip())
            last_parameter = None
            continue
        elif match := RE_FAILURE.match(line):
            current_section = "failure"
            failure.append(match.group(1).strip())
            last_parameter = None
            continue
        elif match := RE_INFO.match(line):
            current_section = "info"
            info.append(match.group(1).strip())
            last_parameter = None
            continue
        elif match := RE_NOTE.match(line):
            current_section = "note"
            note.append(match.group(1).strip())
            last_parameter = None
            continue
        elif match := RE_WARN.match(line):
            current_section = "warn"
            warn.append(match.group(1).strip())
            last_parameter = None
            continue

        if line.startswith(" "):
            if current_section == "params" and last_parameter:
                last_parameter["desc"] += " " + stripped_line
                continue
            elif current_section == "success" and success:
                success.append(stripped_line)
                continue
            elif current_section == "failure" and failure:
                failure.append(stripped_line)
                continue
            elif current_section == "info" and info:
                info.append(stripped_line)
                continue
            elif current_section == "note" and note:
                note.append(stripped_line)
                continue
            elif current_section == "warn" and warn:
                warn.append(stripped_line)
                continue

        if current_section == "brief" and stripped_line:
            brief.append(stripped_line)
        elif current_section == "usage":
            usage.append(line)

    symbol_kind = "function"
    if next_code_line:
        code = next_code_line.strip()
        if code.startswith("#define") or re.match(r"^[A-Z0-9_]+\s*\(", code):
            symbol_kind = "macro"
        elif any(code.startswith(kw) for kw in ("struct ", "enum ", "union ", "typedef ")):
            symbol_kind = "type"
        else:
            function_pattern = re.compile(
                r"^[\w\*\s]+?\s+\**\w+\s*\([^;]*\)\s*;?$")
            if function_pattern.match(code):
                symbol_kind = "function"

    # Regex patterns for symbol extraction
    RE_MACRO_OR_FUNC = re.compile(
        r'^\s*#define\s+(\w+)|'          # Macro pattern
        r'^\s*(?:\w+\s+)+(\w+)\s*\('     # Function pattern
    )
    RE_TYPE_DECL = re.compile(
        # Type declaration
        r'^\s*(?:typedef\s+)?(struct|enum|union)\s*(?:{\s*)?(\w+)?|'
        r'^\s*}\s*(\w+)\s*;'            # Typedef ending pattern
    )

    symbol_name = None
    if next_code_line:
        code = next_code_line.strip()

        if symbol_kind == "macro":
            if match := RE_MACRO_OR_FUNC.match(code):
                symbol_name = match.group(1)

        elif symbol_kind == "function":
            if match := RE_MACRO_OR_FUNC.match(code):
                symbol_name = match.group(2)

        elif symbol_kind == "type":
            # Handle both direct declarations and typedefs
            if match := RE_TYPE_DECL.match(code):
                # Group 2: struct/enum name (non-typedef)
                # Group 3: typedef name (when at end)
                symbol_name = match.group(2) or match.group(3)

    return {
        "brief": " ".join(brief),
        "params": params,
        "usage": "\n".join(usage) if usage else None,
        "success": " ".join(success) if success else None,
        "failure": " ".join(failure) if failure else None,
        "info": " ".join(info) if info else None,
        "note": " ".join(note) if note else None,
        "warn": " ".join(warn) if warn else None,
        "kind": symbol_kind,
        "symbol_name": symbol_name,
    }


def generate_markdown_file(symbol_name, symbol_data, usages, output_dir: Path):
    """
    Generates a Markdown file for a documented symbol.

    Args:
        symbol_name (str): The name of the documented symbol.
        symbol_data (dict): The data associated with the symbol.
        usages (list): A list of usage locations for the symbol.
        output_dir (Path): The directory where the Markdown file should be written.
    """
    doc = symbol_data["doc"]
    output_path = output_dir / f"generated-doc-{symbol_name}.md"
    markdown_dir = output_path.parent

    print(f"Writing: {output_path}")
    with open(output_path, "w", encoding='utf-8') as markdown_file:
        markdown_file.write("---\n")
        markdown_file.write(f'title: "{symbol_name}"\n')
        markdown_file.write(f'meta_title: "{symbol_name}"\n')
        markdown_file.write(f'description: "Documentation for {
                            symbol_name} {doc["kind"]}."\n')
        markdown_file.write(f'date: {datetime.now().isoformat()}\n')
        markdown_file.write(f'categories: ["{doc["kind"].capitalize()}"]\n')
        markdown_file.write(
            f"tags: [\"documentation\", \"generated\", {doc['symbol_name']}]\n")
        markdown_file.write("draft: false\n")
        markdown_file.write("---\n\n")

        markdown_file.write(f"# <center>`{symbol_name}`</center>\n\n")

        if doc.get("brief"):
            markdown_file.write("## Description\n\n")
            markdown_file.write(doc["brief"] + "\n\n")

        markdown_file.write("<!--more-->")

        if doc.get("info"):
            markdown_file.write(
                '{{< notice "info" >}}\n\n' + doc["info"] + '\n\n{{< /notice >}}\n\n')

        if doc.get("note"):
            markdown_file.write(
                '{{< notice "note" >}}\n\n' + doc["note"] + '\n\n{{< /notice >}}\n\n')

        if doc.get("warn"):
            markdown_file.write(
                '{{< notice "warning" >}}\n\n' + doc["warn"] + '\n\n{{< /notice >}}\n\n')

        if doc.get("params"):
            markdown_file.write("## Parameters\n\n")
            markdown_file.write("| Name | Direction | Description |\n")
            markdown_file.write("|------|-----------|-------------|\n")
            for param in doc["params"]:
                markdown_file.write(f"| `{param['name']}` | {
                                    param['direction']} | {param['desc']} |\n")
            markdown_file.write("\n\n")

        if doc.get("usage"):
            markdown_file.write("## Usage example (from documentation)\n\n")
            markdown_file.write("```c\n")
            markdown_file.write(doc["usage"])
            markdown_file.write("\n```\n\n")

        for section in ["success", "failure"]:
            if doc.get(section):
                markdown_file.write(f"## {section.capitalize()}\n\n{
                                    doc[section]}\n\n")

        markdown_file.write("## Usage example (Cross-references)\n\n")
        markdown_file.write(
            "{{< accordion \"Usage examples (Cross-references)\" >}}\n")
        if usages:
            for usage_item in usages:
                usage_file_path = Path(usage_item['filepath'])
                try:
                    relative_path = os.path.relpath(
                        usage_file_path, start=markdown_dir)
                    link_path = Path(relative_path).as_posix()
                except ValueError:
                    link_path = usage_file_path.name
                    print(f"Warning: Could not create relative path for {
                          usage_item['filepath']} from {markdown_dir}. Using filename.")

                posix_path = PurePosixPath(link_path)
                cleaned_parts = [
                    part for part in posix_path.parts if part != ".."]
                cleaned_path = "/".join(cleaned_parts)
                github_link = f"https://github.com/brightprogrammer/MisraStdC/blob/master/{
                    cleaned_path}#L{usage_item['lineno']}"

                indented_code = '\n'.join(
                    ["    " + line.strip() for line in usage_item['code'].splitlines()])

                markdown_file.write(
                    f'* In [`{usage_file_path.name}:{usage_item["lineno"]}`]({github_link}):\n\n')
                markdown_file.write(f"```c\n{indented_code}\n```\n\n")
        else:
            markdown_file.write(
                "No external code usages found in the scanned files.\n")
        markdown_file.write("{{< /accordion >}}\n\n")


def process_source_files(root_directories, output_directory):
    """
    Orchestrates the documentation generation process.

    Args:
        root_directories (list): A list of root directories to scan for source files.
        output_directory (str): The directory to output the generated documentation.
    """
    resolved_output_dir = Path(output_directory).resolve()
    os.makedirs(resolved_output_dir, exist_ok=True)
    print(f"Output directory: {resolved_output_dir}")

    print("\n--- Starting Pass 1: Collecting symbols and file contents ---")
    for root_dir_str in root_directories:
        if "Docs" in root_dir_str:
            continue
        root_path = Path(root_dir_str).resolve()
        if not root_path.is_dir():
            print(f"Warning: Root path '{root_dir_str}' ({
                  root_path}) is not a valid directory. Skipping.")
            continue
        print(f"Pass 1: Scanning under root directory: {root_path}")
        for dirpath_str, _, filenames in os.walk(root_path):
            current_dir = Path(dirpath_str)
            for filename in filenames:
                if filename.endswith((".c", ".h")):
                    file_to_process = (current_dir / filename).resolve()
                    extract_symbols_and_store_content(file_to_process)
    print(f"--- Pass 1 complete. Found {len(parsed_symbols)
                                        } unique symbols. Read {len(file_contents)} files. ---")

    print("\n--- Starting Pass 2: Finding symbol usages ---")
    symbol_usage_map = find_symbol_usages(parsed_symbols, file_contents)
    total_usages = sum(len(usages) for usages in symbol_usage_map.values())
    print(
        f"--- Pass 2 complete. Found {total_usages} total usage instances. ---")

    print("\n--- Starting Pass 3: Generating Markdown files ---")
    if not parsed_symbols:
        print("No symbols found to document.")
    else:
        for symbol_name, symbol_data in parsed_symbols.items():
            usages = symbol_usage_map.get(symbol_name, [])
            generate_markdown_file(
                symbol_name, symbol_data, usages, resolved_output_dir)

    print("\n--- Documentation generation complete. ---")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Generate documentation from C comments with cross-references.")
    parser.add_argument(
        "--root",
        default=DEFAULT_ROOT_DIRS,
        nargs='+',
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

    root_directories_list = args.root
    output_directory_str = args.output

    process_source_files(root_directories_list, output_directory_str)
