import os
import re
from pathlib import Path
from datetime import datetime

DOC_LINE_PATTERN = re.compile(r'^\s*/// ?(.*)')
DECLARATION_LINE_PATTERN = re.compile(r'^\s*(.+?[* ]+)([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)\s*;')

def parse_doc_block(doc_lines):
    doc = {
        "brief": [],
        "params": [],
        "usage": [],
        "success": None,
        "failure": None,
    }
    current = "brief"
    for line in doc_lines:
        content = line.strip()
        if not content:
            continue
        if content.startswith("param"):
            doc["params"].append(content)
        elif content.startswith("USAGE"):
            current = "usage"
        elif content.startswith("SUCCESS"):
            doc["success"] = content.split(":", 1)[1].strip()
        elif content.startswith("FAILURE"):
            doc["failure"] = content.split(":", 1)[1].strip()
        else:
            if current == "usage":
                doc["usage"].append(content)
            else:
                doc["brief"].append(content)
    return doc

def generate_markdown(symbol, doc, return_type, symbol_type="Function"):
    today = datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ")

    # Infer categories/tags
    categories = [symbol_type]
    tags = [symbol.lower()]
    
    yaml_front_matter = f"""---
title: "{symbol}"
meta_title: "{symbol}"
description: "{symbol_type} :: {symbol}"
date: {today}
categories: {categories}
author: "Siddharth Mishra"
tags: {tags}
draft: false
---

# <center>`{symbol}` - {symbol_type}</center>
"""

    md_body = ""

    if doc["brief"]:
        md_body += "## Description\n" + "\n".join(doc["brief"]) + "\n\n"

    md_body += "## Syntax\n```c\n"
    params = "..." if not return_type else ", ".join([p for p in doc.get("params", [])])
    md_body += f"{return_type.strip()} {symbol}(...);\n```\n\n"

    if doc["params"]:
        md_body += "## Parameters\n"
        for param in doc["params"]:
            md_body += f"- {param}\n"
        md_body += "\n"

    if doc["usage"]:
        md_body += "## Usage Examples\n```c\n" + "\n".join(doc["usage"]) + "\n```\n\n"

    if doc["success"] or doc["failure"]:
        md_body += "## Behavior\n"
        if doc["success"]:
            md_body += f"**Success**: {doc['success']}\n\n"
        if doc["failure"]:
            md_body += f"**Failure**: {doc['failure']}\n\n"

    if return_type:
        md_body += f"**Returns**: `{return_type.strip()}`\n"

    return yaml_front_matter + "\n" + md_body

def extract_docs_from_file(filepath, output_dir):
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    docs = []
    doc_lines = []
    for i, line in enumerate(lines):
        match = DOC_LINE_PATTERN.match(line)
        if match:
            doc_lines.append(match.group(1))
        elif doc_lines:
            # Look for function/type declaration right after doc block
            decl_line = lines[i].strip()
            decl_match = DECLARATION_LINE_PATTERN.match(decl_line)
            if decl_match:
                return_type, name, _ = decl_match.groups()
                doc_data = parse_doc_block(doc_lines)
                docs.append((name, doc_data, return_type))
            doc_lines = []

    # Write each doc to a separate .md file
    for name, doc_data, return_type in docs:
        md_content = generate_markdown(name, doc_data, return_type)
        with open(output_dir / f"{name}.md", "w", encoding='utf-8') as f:
            f.write(md_content)

def process_directory(dir_path, output_dir="Docs/content/english/blog"):
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    for root, _, files in os.walk(dir_path):
        for file in files:
            if file.endswith(".h") or file.endswith(".c"):
                full_path = Path(root) / file
                extract_docs_from_file(full_path, output_dir)

if __name__ == "__main__":
    # Change this to the actual path
    process_directory("Include")
