
import os
import sys
import re

# Make sure we're using correct env before tring to import clang
sys.path.append("C:\\Python313\\Lib\\site-packages")

from pathlib import Path, PurePosixPath
from datetime import datetime
from clang.cindex import Index, CursorKind, Config

# CONFIGURATION
PROJECT_ROOT = Path(".").resolve()
OUTPUT_DIR = Path("Docs/content/english/blog").resolve()
LIBCLANG_PATH = "C:/clang/bin/libclang.dll"  # Adjust to your system

Config.set_library_file(LIBCLANG_PATH)

def parse_comment(raw):
    """
    Parse structured comment sections from /// comment block.

    Sections handled:
    - brief description (before any section header)
    - FIELDS: - name: description
    - INFO:, NOTE:, WARN:
    """
    if not raw:
        return {}

    sections = {
        "brief": [],
        "fields": [],
        "info": [],
        "note": [],
        "warn": [],
    }

    current_section = "brief"
    lines = [line.lstrip('/').strip() for line in raw.strip().splitlines()]

    for line in lines:
        upper_line = line.upper()
        if upper_line == "FIELDS:":
            current_section = "fields"
            continue
        elif upper_line == "INFO:":
            current_section = "info"
            continue
        elif upper_line == "NOTE:":
            current_section = "note"
            continue
        elif upper_line == "WARN:" or upper_line == "WARNING:":
            current_section = "warn"
            continue

        if current_section == "fields":
            # Expect format: - name: description
            if line.startswith('-'):
                try:
                    name, desc = line[1:].split(':', 1)
                    sections["fields"].append({
                        "name": name.strip(),
                        "desc": desc.strip()
                    })
                except ValueError:
                    # Malformed line, ignore or handle as needed
                    pass
        else:
            sections[current_section].append(line)

    # Join multi-line sections
    for key in sections:
        if key == "fields":
            continue
        sections[key] = "\n".join(sections[key]).strip()

    sections["brief"] = "\n".join(sections["brief"]).strip()
    return sections


def extract_types(cursor):
    """
    Recursively walk AST and extract typedef'd structs/unions/enums with their documentation.

    Returns list of dicts:
    {
        "name": type_name,
        "doc": { ... parsed comment sections ... },
        "location": source file path (str),
    }
    """
    results = []

    def get_comment_for_node(node):
        raw_comment = node.raw_comment
        if raw_comment:
            return raw_comment
        # Fallback: try typedef parent comment if exists
        parent = node.semantic_parent
        if parent and parent.kind == CursorKind.TYPEDEF_DECL:
            return parent.raw_comment
        return None

    def visit(node):
        # Process only typedef'd struct/union/enum
        if node.kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL, CursorKind.ENUM_DECL):
            # Determine the typedef alias name
            typedef_name = None
            parent = node.semantic_parent
            if parent and parent.kind == CursorKind.TYPEDEF_DECL:
                typedef_name = parent.spelling

            if not typedef_name:
                # Try children for typedef decl (rare)
                for c in node.get_children():
                    if c.kind == CursorKind.TYPEDEF_DECL:
                        typedef_name = c.spelling
                        break

            # Final type name to document
            type_name = typedef_name or node.spelling

            if type_name:
                raw_comment = get_comment_for_node(node)
                doc_sections = parse_comment(raw_comment) if raw_comment else {}

                location = node.location.file.name if node.location.file else "unknown"
                results.append({
                    "name": type_name,
                    "doc": doc_sections,
                    "location": location,
                })

        # recurse children
        for c in node.get_children():
            visit(c)

    visit(cursor)
    return results


def generate_markdown(type_entry, output_dir: Path):
    """
    Generate markdown file for a single type_entry dict.
    """
    # Sanitize symbol name for filename
    symbol_name = type_entry["name"]
    safe_symbol = re.sub(r'[\\/*?:"<>|() \t]', '_', symbol_name)
    
    doc = type_entry["doc"]
    output_path = output_dir / f"generated-doc-{safe_symbol}.md"  # Use safe symbol here
    markdown_dir = output_path.parent

    print(f"Writing: {output_path}")
    with open(output_path, "w", encoding='utf-8') as md:
        md.write("---\n")
        md.write(f'title: "{symbol_name}"\n')
        md.write(f'meta_title: "{symbol_name}"\n')
        md.write(f'description: "Documentation for {symbol_name} type."\n')
        md.write(f'date: {datetime.now().isoformat()}\n')
        md.write(f'categories: ["Type"]\n')
        md.write(f'tags: []\n')
        md.write("draft: false\n")
        md.write("---\n\n")

        md.write(f"# <center>`{symbol_name}`</center>\n\n")

        if doc.get("brief"):
            md.write("## Description\n\n")
            md.write(doc["brief"] + "\n\n")

        md.write("<!--more-->")

        for notice_type, hugo_notice in [("info", "info"), ("note", "note"), ("warn", "warning")]:
            if doc.get(notice_type):
                md.write(f'\n{{{{< notice "{hugo_notice}" >}}}}\n\n{doc[notice_type]}\n\n{{{{< /notice >}}}}\n')

        if doc.get("fields"):
            md.write("## Fields\n\n")
            md.write("| Name | Description |\n")
            md.write("|------|-------------|\n")
            for f in doc["fields"]:
                md.write(f"| `{f['name']}` | {f['desc']} |\n")
            md.write("\n\n")

        if doc.get("usage"):
            md.write("## Usage example (from documentation)\n\n")
            md.write("```c\n")
            md.write(doc["usage"])
            md.write("\n```\n\n")

        for section in ["success", "failure"]:
            if doc.get(section):
                md.write(f"## {section.capitalize()}\n\n{doc[section]}\n\n")

        md.write("## Usage example (Cross-references)\n\n")
        md.write("{{< accordion \"Usage examples (Cross-references)\" >}}\n")
        md.write("No external code usages found in the scanned files.\n")
        md.write("{{< /accordion >}}\n\n")


def main():
    if len(sys.argv) < 3:
        print("Usage: python3 generate_type_docs.py <root-directory> <output-directory>")
        sys.exit(1)

    root_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    output_dir.mkdir(parents=True, exist_ok=True)

    # Collect all C and header files recursively
    c_files = []
    for path in root_dir.rglob('*'):
        if path.is_file() and path.suffix.lower() in ('.c', '.h'):
            c_files.append(path)
    c_files = sorted(c_files)

    index = Index.create()
    type_dict = {}

    for file_path in c_files:
        print(f"Processing {file_path}...")
        try:
            tu = index.parse(str(file_path), args=["-x", "c", "-std=c11"])
        except Exception as e:
            print(f"Error parsing {file_path}: {e}")
            continue
        types = extract_types(tu.cursor)
        for t in types:
            type_name = t['name']
            # Overwrite existing entries; adjust logic if needed
            type_dict[type_name] = t

    print(f"Found {len(type_dict)} documented types.")

    for t in type_dict.values():
        generate_markdown(t, output_dir)


if __name__ == "__main__":
    main()
