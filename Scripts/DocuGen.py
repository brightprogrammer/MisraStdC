import os
import re
import argparse
from pathlib import Path
from datetime import datetime

# Configuration
DEFAULT_ROOT_DIRS = ["."]
DEFAULT_OUTPUT_DIR = "Docs/content/english/blog/api"

# Data Stores
parsed_symbols = {}
file_contents = {}

# Regular Expressions
RE_COMMENT_LINE = re.compile(r'^\s*///\s?(.*)')
RE_PARAM = re.compile(r'^\s*(\w+)\[(in|out|in,out)\]\s*:\s*(.+)')
RE_RETURNS = re.compile(r'^\s*RETURNS?\s*:\s*(.+)')
RE_SUCCESS = re.compile(r'^\s*SUCCESS\s*:\s*(.+)')
RE_FAILURE = re.compile(r'^\s*FAILURE\s*:\s*(.+)')
RE_USAGE_START = re.compile(r'^\s*USAGE\s*:\s*$')
RE_FIELDS_START = re.compile(r'^\s*FIELDS\s*:\s*$')
RE_FIELD = re.compile(r'^\s*-\s*([A-Za-z_]\w*)\s*:\s*(.+)')
RE_INFO = re.compile(r'^\s*INFO\s*:\s*(.+)')
RE_NOTE = re.compile(r'^\s*NOTE\s*:\s*(.+)')
RE_WARN = re.compile(r'^\s*WARN\s*:\s*(.+)')
RE_TAGS = re.compile(r"^\s*TAGS\s*:\s+(.*)")

KIND_PRIORITY = {
    "macro": 3,
    "function": 2,
    "type": 1,
}


def extract_symbol_name(code_line):
    """Extract a documented symbol name from a definition/declaration line."""
    stripped = code_line.strip()
    single_line = re.sub(r'\s+', ' ', stripped)
    if not stripped:
        return None

    macro_match = re.match(r'^\s*#\s*define\s+(\w+)\b', single_line)
    if macro_match:
        return macro_match.group(1), "macro"

    typedef_fn_ptr_match = re.match(
        r'^\s*typedef\b.*?\(\s*\*\s*(\w+)\s*\)\s*\(', single_line)
    if typedef_fn_ptr_match:
        return typedef_fn_ptr_match.group(1), "type"

    typedef_alias_match = re.match(
        r'^\s*typedef\b(?!.*\(\s*\*\s*\w+\s*\)).*\b(\w+)\s*;\s*$',
        single_line,
    )
    if typedef_alias_match:
        return typedef_alias_match.group(1), "type"

    ptr_func_match = re.match(
        r'^\s*(?:[\w]+\s+)*[\w]+\s*\*+\s*(\w+)\s*\([^;]*\)\s*;?\s*$',
        single_line,
    )
    if ptr_func_match:
        return ptr_func_match.group(1), "function"

    func_match = re.match(
        r'^\s*(?:[\w\*]+\s+)+\(?(\w+)\)?\s*\([^;]*\)\s*;?\s*$', single_line)
    if func_match:
        return func_match.group(1), "function"

    return None


def collect_declaration(lines, start_index):
    """Collect a declaration that may span multiple lines until the first semicolon."""
    collected = []
    idx = start_index
    brace_depth = 0

    while idx < len(lines):
        current_line = lines[idx]
        collected.append(current_line.rstrip())
        brace_depth += current_line.count("{")
        brace_depth -= current_line.count("}")
        if ";" in current_line and brace_depth <= 0:
            break
        if "{" in current_line or "}" in current_line or current_line.strip():
            idx += 1
            continue
        break

    return "\n".join(collected).strip()


def should_replace_symbol(old_symbol_data, new_symbol_data):
    """Prefer higher-level public docs when duplicate symbol names are encountered."""
    old_priority = KIND_PRIORITY.get(old_symbol_data["kind"], 0)
    new_priority = KIND_PRIORITY.get(new_symbol_data["kind"], 0)
    return new_priority >= old_priority


def to_repo_relative_path(file_path: Path):
    """Convert an absolute path to a repo-relative POSIX path when possible."""
    try:
        return file_path.resolve().relative_to(Path.cwd().resolve()).as_posix()
    except ValueError:
        return file_path.name


def is_list_item(line: str):
    """Return True when a stripped comment line is a markdown list item."""
    return bool(
        line.startswith(("- ", "* ")) or
        re.match(r'^\d+\.\s+', line)
    )


def format_markdown_lines(lines):
    """Preserve paragraphs and simple lists from comment text."""
    if not lines:
        return None

    blocks = []
    paragraph = []
    list_items = []

    def flush_paragraph():
        if paragraph:
            blocks.append(" ".join(paragraph))
            paragraph.clear()

    def flush_list():
        if list_items:
            blocks.append("\n".join(list_items))
            list_items.clear()

    for raw_line in lines:
        stripped_line = raw_line.strip()

        if not stripped_line:
            flush_paragraph()
            flush_list()
            continue

        if is_list_item(stripped_line):
            flush_paragraph()
            list_items.append(stripped_line)
            continue

        flush_list()
        paragraph.append(stripped_line)

    flush_paragraph()
    flush_list()
    return "\n\n".join(blocks) if blocks else None


def collect_source_files(root_path: Path):
    """Yield C/header files for either a single file root or a directory root."""
    if root_path.is_file():
        if root_path.suffix.lower() in (".c", ".h") and root_path.name != "Private.h":
            yield root_path.resolve()
        return

    if not root_path.is_dir():
        return

    for dirpath_str, _, filenames in os.walk(root_path):
        current_dir = Path(dirpath_str)
        if any(part.lower() == "docs" for part in current_dir.parts):
            print(f"Skipping directory: {current_dir} (contains 'docs')")
            continue
        for filename in filenames:
            if filename.endswith((".c", ".h")) and filename != "Private.h":
                yield (current_dir / filename).resolve()


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
                code_line = collect_declaration(lines, definition_index)
                documentation = parse_comment_block(
                    comment_block, next_code_line=code_line)
                symbol_info = extract_symbol_name(code_line)
                if symbol_info:
                    symbol_name, inferred_kind = symbol_info
                    if symbol_name.startswith("MISRA_PRIV_"):
                        line_index = definition_index + 1
                        continue
                    documentation["kind"] = inferred_kind
                    new_symbol_data = {
                        "name": symbol_name,
                        "kind": documentation["kind"],
                        "doc": documentation,
                        "filepath": file_path_str,
                        "def_lineno": definition_index + 1,
                        "definition_line_content": code_line
                    }
                    if symbol_name in parsed_symbols:
                        if should_replace_symbol(parsed_symbols[symbol_name], new_symbol_data):
                            print(f"Info: Replacing duplicate symbol '{symbol_name}' with "
                                  f"{file_path_str}:{definition_index + 1}.")
                            parsed_symbols[symbol_name] = new_symbol_data
                    else:
                        parsed_symbols[symbol_name] = new_symbol_data
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
    Parses a block of comment lines to extract documentation details, including tags.

    Args:
        comment_lines (list): A list of strings, where each string is a line from the comment block
                                (stripped of the '/// ?' prefix).
        next_code_line (str, optional): The line of code immediately following the comment block.
                                         Used to determine the type of the documented symbol. Defaults to None.

    Returns:
        dict: A dictionary containing the parsed documentation elements, including 'tags'.
    """
    brief = []
    params = []
    usage = []
    returns = []
    success = []
    failure = []
    fields = []
    info = []
    note = []
    warn = []
    tags = []  # New list to store tags

    current_section = "brief"
    last_parameter = None
    last_field = None

    for line in comment_lines:
        stripped_line = line.strip()
        if not stripped_line and current_section != "usage":
            if current_section == "brief":
                brief.append("")
            elif current_section == "returns":
                returns.append("")
            elif current_section == "success":
                success.append("")
            elif current_section == "failure":
                failure.append("")
            elif current_section == "info":
                info.append("")
            elif current_section == "note":
                note.append("")
            elif current_section == "warn":
                warn.append("")
            continue

        if RE_USAGE_START.match(line):
            current_section = "usage"
            last_parameter = None
            last_field = None
            continue
        elif RE_FIELDS_START.match(line):
            current_section = "fields"
            last_parameter = None
            last_field = None
            continue
        elif match := RE_PARAM.match(line):
            current_section = "params"
            param_name, direction, description = match.groups()
            current_param = {"name": param_name,
                             "direction": direction,
                             "desc": description.strip()}
            params.append(current_param)
            last_parameter = current_param
            last_field = None
            continue
        elif match := RE_FIELD.match(line):
            current_section = "fields"
            current_field = {
                "name": match.group(1),
                "desc": match.group(2).strip()
            }
            fields.append(current_field)
            last_parameter = None
            last_field = current_field
            continue
        elif match := RE_RETURNS.match(line):
            current_section = "returns"
            returns.append(match.group(1).strip())
            last_parameter = None
            last_field = None
            continue
        elif match := RE_SUCCESS.match(line):
            current_section = "success"
            success.append(match.group(1).strip())
            last_parameter = None
            last_field = None
            continue
        elif match := RE_FAILURE.match(line):
            current_section = "failure"
            failure.append(match.group(1).strip())
            last_parameter = None
            last_field = None
            continue
        elif match := RE_INFO.match(line):
            current_section = "info"
            info.append(match.group(1).strip())
            last_parameter = None
            last_field = None
            continue
        elif match := RE_NOTE.match(line):
            current_section = "note"
            note.append(match.group(1).strip())
            last_parameter = None
            last_field = None
            continue
        elif match := RE_WARN.match(line):
            current_section = "warn"
            warn.append(match.group(1).strip())
            last_parameter = None
            last_field = None
            continue
        elif match := RE_TAGS.match(line):  # Handle the @tags section
            current_section = "tags"
            tags.extend([tag.strip()
                        for tag in match.group(1).split(',') if tag.strip()])
            last_parameter = None
            last_field = None
            continue

        if line.startswith(" "):
            if current_section == "params" and last_parameter:
                last_parameter["desc"] += " " + stripped_line
                continue
            elif current_section == "fields" and last_field:
                last_field["desc"] += " " + stripped_line
                continue
            elif current_section == "success" and success:
                success.append(stripped_line)
                continue
            elif current_section == "failure" and failure:
                failure.append(stripped_line)
                continue
            elif current_section == "returns" and returns:
                returns.append(stripped_line)
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
        if re.match(r"^\s*#\s*define\b", code) or re.match(r"^[A-Z0-9_]+\s*\(", code):
            symbol_kind = "macro"
        elif any(code.startswith(kw) for kw in ("struct ", "enum ", "union ", "typedef ")):
            symbol_kind = "type"
        else:
            function_pattern = re.compile(
                r"^(?:[\w\*]+\s+)+\(?\w+\)?\s*\([^;]*\)\s*;?$")
            if function_pattern.match(code):
                symbol_kind = "function"

    return {
        "brief": format_markdown_lines(brief),
        "params": params,
        "usage": "\n".join(usage) if usage else None,
        "returns": format_markdown_lines(returns),
        "success": format_markdown_lines(success),
        "failure": format_markdown_lines(failure),
        "fields": fields if fields else None,
        "info": format_markdown_lines(info),
        "note": format_markdown_lines(note),
        "warn": format_markdown_lines(warn),
        "tags": tags if tags else None,  # Include the tags in the result
        "kind": symbol_kind,
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
        # Assuming symbol_data is a dictionary that might contain a 'tags' key
        tags_string = ""
        if "tags" in doc and doc["tags"]:
            tags_string = ", ".join(f'"{tag}"' for tag in doc["tags"])

        markdown_file.write(
            f'tags: [{tags_string if tags_string else ""}]\n')
        markdown_file.write("draft: false\n")
        markdown_file.write("---\n\n")

        if doc.get("brief"):
            markdown_file.write("## Description\n\n")
            markdown_file.write(doc["brief"] + "\n\n")

        markdown_file.write("<!--more-->\n\n")

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

        if doc.get("fields"):
            markdown_file.write("## Fields\n\n")
            markdown_file.write("| Name | Description |\n")
            markdown_file.write("|------|-------------|\n")
            for field in doc["fields"]:
                markdown_file.write(
                    f"| `{field['name']}` | {field['desc']} |\n")
            markdown_file.write("\n\n")

        if doc.get("usage"):
            markdown_file.write("## Usage example (from documentation)\n\n")
            markdown_file.write("```c\n")
            markdown_file.write(doc["usage"])
            markdown_file.write("\n```\n\n")

        if doc.get("returns"):
            markdown_file.write(f"## Returns\n\n{doc['returns']}\n\n")

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
                cleaned_path = to_repo_relative_path(usage_file_path)
                github_link = f"https://github.com/brightprogrammer/MisraStdC/blob/master/{
                    cleaned_path}#L{usage_item['lineno']}"

                indented_code = '\n'.join(
                    ["    " + line.rstrip() for line in usage_item['code'].splitlines()])

                markdown_file.write(
                    f'* In [`{usage_file_path.name}:{usage_item["lineno"]}`]({github_link}):\n\n')
                markdown_file.write(f"```c\n{indented_code}\n```\n\n")
        else:
            markdown_file.write(
                "No external code usages found in the scanned files.\n")
        markdown_file.write("{{< /accordion >}}\n\n")


def process_source_files(root_directories, output_directory):
    """
    Orchestrates the documentation generation process, skipping the 'Docs' directory.

    Args:
        root_directories (list): A list of root directories to scan for source files.
        output_directory (str): The directory to output the generated documentation.
    """
    resolved_output_dir = Path(output_directory).resolve()
    os.makedirs(resolved_output_dir, exist_ok=True)
    print(f"Output directory: {resolved_output_dir}")
    parsed_symbols.clear()
    file_contents.clear()

    print("\n--- Starting Pass 1: Collecting symbols and file contents ---")
    for root_dir_str in root_directories:
        root_path = Path(root_dir_str).resolve()
        if not root_path.exists():
            print(f"Warning: Root path '{root_dir_str}' ({
                  root_path}) does not exist. Skipping.")
            continue
        print(f"Pass 1: Scanning root: {root_path}")
        for file_to_process in collect_source_files(root_path):
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
