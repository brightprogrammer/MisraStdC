import os
import sys
import re

from pathlib import Path, PurePosixPath
from datetime import datetime
from clang.cindex import Index, CursorKind, Config, TypeKind


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
        sections[key] = "".join(sections[key]).strip()

    sections["brief"] = "".join(sections["brief"]).strip()
    return sections


def extract_types(cursor, generic_types):
    """
    Extracts documentation for typedefs, inheriting from generic types.
    """
    generic_type_docs = {}
    results = []

    def get_underlying_type_decl(typedef_cursor):
        """Resolve the underlying struct/union/enum declaration for a typedef"""
        underlying_type = typedef_cursor.underlying_typedef_type
        if underlying_type.kind == TypeKind.ELABORATED:
            return underlying_type.get_declaration()
        return None

    def collect_members(decl_cursor):
        """Collect fields or enum members from a struct/union/enum declaration"""
        members = []
        if decl_cursor.kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
            for child in decl_cursor.get_children():
                if child.kind == CursorKind.FIELD_DECL:
                    members.append({
                        "name": child.spelling,
                        "type": child.type.spelling
                    })
        elif decl_cursor.kind == CursorKind.ENUM_DECL:
            for child in decl_cursor.get_children():
                if child.kind == CursorKind.ENUM_CONSTANT_DECL:
                    members.append({
                        "name": child.spelling,
                        "value": child.enum_value
                    })
        return members

    def visit(node):
        # Handle generic type documentation from #define
        if node.kind == CursorKind.MACRO_DEFINITION:
            for generic_type_name in generic_types:
                if node.spelling == generic_type_name:
                    # Get the raw text of the macro definition and the preceding comment
                    extent = node.extent
                    file_path = Path(extent.start.file.name)
                    with open(file_path, 'r', encoding='utf-8') as f:
                        lines = f.readlines()
                        start_line = extent.start.line - 1
                        comment_lines = []
                        for i in range(start_line - 1, -1, -1):
                            line = lines[i].strip()
                            if line.startswith("///"):
                                comment_lines.insert(0, line)
                            else:
                                break
                        if comment_lines:
                            raw_comment = "\n".join(comment_lines)
                            generic_type_docs[generic_type_name] = parse_comment(
                                raw_comment)
                    break  # Found a matching generic type, no need to check others
            else:
                print(f"")

        # Handle typedef declarations
        elif node.kind == CursorKind.TYPEDEF_DECL:
            typedef_name = node.spelling
            decl = get_underlying_type_decl(node)
            inherited_doc = {}

            if decl and decl.kind in (
                CursorKind.STRUCT_DECL,
                CursorKind.UNION_DECL,
                CursorKind.ENUM_DECL
            ):
                # Check if the underlying type's name looks like a generic type instantiation
                for generic_type_name in generic_types:
                    if decl.type.spelling.startswith(f"{generic_type_name}("):
                        if generic_type_name in generic_type_docs:
                            inherited_doc = generic_type_docs[generic_type_name]
                        break

                # Get documentation from typedef itself
                raw_comment = node.raw_comment
                doc_sections = parse_comment(
                    raw_comment) if raw_comment else {}

                # Merge documentation (typedef-specific overrides generic)
                merged_doc = inherited_doc.copy()
                merged_doc.update(doc_sections)

                location = node.location.file.name if node.location.file else "unknown"

                results.append({
                    "name": typedef_name,
                    "doc": merged_doc,
                    "location": location,
                    "kind": decl.kind,
                    "members": collect_members(decl)
                })

        # Recurse for children
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
    output_path = output_dir / f"generated-doc-{safe_symbol}.md"
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

        md.write("## Description\n\n")
        if doc.get("brief"):
            md.write(doc["brief"] + "\n\n")
        else:
            md.write("No description provided\n\n")

        md.write("<!--more-->\n")

        for notice_type, hugo_notice in [("info", "info"), ("note", "note"), ("warn", "warning")]:
            if doc.get(notice_type):
                md.write(f'\n{{{{< notice "{hugo_notice}" >}}}}\n\n{
                         doc[notice_type]}\n\n{{{{< /notice >}}}}\n')

        # Handle members (fields/enum members)
        kind = type_entry.get("kind")
        members = type_entry.get("members", [])

        if kind in (CursorKind.STRUCT_DECL, CursorKind.UNION_DECL):
            md.write("\n## Fields\n\n")
            if members:
                field_descs = {f["name"]: f["desc"]
                               for f in doc.get("fields", [])}
                md.write("| Name | Type | Description |\n")
                md.write("|------|------|-------------|\n")
                for member in members:
                    name = member.get("name", "")
                    type_ = member.get("type", "")
                    desc = field_descs.get(name, "")
                    md.write(f"| `{name}` | `{type_}` | {
                             desc if desc else "No description provided"} |\n")
                md.write("\n")
            else:
                md.write("No fields.\n\n")
        elif kind == CursorKind.ENUM_DECL:
            md.write("\n## Enum Members\n\n")
            if members:
                enum_descs = {f["name"]: f["desc"]
                              for f in doc.get("fields", [])}
                md.write("| Name | Value | Description |\n")
                md.write("|------|-------|-------------|\n")
                for member in members:
                    name = member.get("name", "")
                    value = member.get("value", "")
                    desc = enum_descs.get(name, "")
                    md.write(f"| `{name}` | `{value}` | {desc} |\n")
                md.write("\n")
            else:
                md.write("No members.\n\n")

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
    if len(sys.argv) < 4:
        print("Usage: python3 generate_type_docs.py <root-directory> <output-directory> <generic-types-file>")
        print("       <generic-types-file> should be a file containing a list of generic type macro names, one per line.")
        sys.exit(1)

    root_dir = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    generic_types_file = Path(sys.argv[3])
    output_dir.mkdir(parents=True, exist_ok=True)

    try:
        with open(generic_types_file, 'r', encoding='utf-8') as f:
            generic_type_names = [line.strip() for line in f if line.strip()]
    except FileNotFoundError:
        print(f"Error: Generic types file not found at {generic_types_file}")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading generic types file: {e}")
        sys.exit(1)

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
        types = extract_types(tu.cursor, generic_type_names)
        for t in types:
            type_name = t['name']
            type_dict[type_name] = t

    print(f"Found {len(type_dict)} documented types.")

    for t in type_dict.values():
        generate_markdown(t, output_dir)


if __name__ == "__main__":
    main()
