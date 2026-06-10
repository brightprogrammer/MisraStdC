#!/usr/bin/env python3
"""Local mutation-survivor triage web tool for MisraStdC.

A stdlib-only HTTP server + single-page UI that lets a human triage mull
mutation survivors with full context (source, git blame, an honest coverage
hint) and ignore / un-ignore them by writing to the real ledger
``Conventions/mull-ignores.toml``.

The survivor accounting is NOT re-implemented here: this server imports and
reuses the canonical engine from ``Scripts/mull-filter.py`` -- ``parse_report``,
``compute_true_survivors`` (dedupe + per-source-file intersection),
``LedgerEntry`` / ``load_ledger`` and the ``region_hash`` normalisation -- so
the UI and the ``mull-filter.py --gate`` enforcement can never diverge.

CLI::

    python3 Scripts/mull-triage-server.py \
        [--port 8765] [--root .] \
        [--ledger Conventions/mull-ignores.toml] [REPORT_GLOB ...]

Reports default to ``build_mull/**/mutation-*.txt`` (recursive). Binds
127.0.0.1 only. On every state-bearing request the reports + ledger are
re-loaded so the UI always reflects on-disk edits.
"""

import argparse
import glob as globmod
import hashlib
import http.server
import json
import os
import re
import signal
import socketserver
import subprocess
import sys
import time
import urllib.parse

# Reuse the canonical filter engine. The server lives in Scripts/ next to it.
_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import importlib

mull_filter = importlib.import_module("mull-filter")

parse_report = mull_filter.parse_report
compute_true_survivors = mull_filter.compute_true_survivors
LedgerEntry = mull_filter.LedgerEntry
load_ledger = mull_filter.load_ledger
load_toml = mull_filter.load_toml
region_hash = mull_filter.region_hash
select_region = mull_filter.select_region


# --------------------------------------------------------------------------
# Report discovery + survivor / ledger loading (re-done per request)
# --------------------------------------------------------------------------
def discover_reports(root, globs):
    """Resolve the report globs (recursive) into a sorted, de-duped file list."""
    paths = []
    seen = set()
    for g in globs:
        pat = g if os.path.isabs(g) else os.path.join(root, g)
        for p in sorted(globmod.glob(pat, recursive=True)):
            ap = os.path.abspath(p)
            if ap not in seen and os.path.isfile(ap):
                seen.add(ap)
                paths.append(ap)
    return paths


def load_survivors(report_paths):
    """Parse every report and return the deduped, per-file-intersected TRUE
    survivor list (canonical engine). Also returns ``reports_meta`` so the
    coverage hint can name which suites ran a given mutant."""
    parsed = []
    parsed_with_path = []
    for rp in report_paths:
        with open(rp, "r", encoding="utf-8") as f:
            pr = parse_report(f.read())
        parsed.append(pr)
        parsed_with_path.append((rp, pr))
    true_survivors, _ = compute_true_survivors(parsed)
    return true_survivors, parsed_with_path


def survivor_id(sv):
    """Stable string key: file:line:col:mutator (file as reported)."""
    return "%s:%d:%d:%s" % (sv.file, sv.line, sv.col, sv.mutator or "?")


def survivor_desc(sv):
    """Human-readable mutation description from the report's 'Survived: <desc>'
    line (the first line of the survivor's block)."""
    if not sv.block:
        return ""
    first = sv.block[0].rstrip("\n")
    m = mull_filter.SURVIVOR_RE.match(first)
    if m:
        return (m.group("desc") or "").strip()
    return ""


def todo_key(file, line, col, mutator):
    """Normalised (file-basename, line, col, mutator) worklist key. Reported
    survivor files and stored todo files may differ in abs-vs-rel form, so the
    file part is compared by basename only (like the ledger's matcher)."""
    return (os.path.basename(file), int(line), int(col), mutator or "?")


def load_todos(path):
    """Parse the [[todo]] worklist. A missing or empty file is an empty
    worklist (never an error). Returns the list of raw todo dicts."""
    if not path or not os.path.isfile(path):
        return []
    try:
        doc = load_todo_doc(path)
    except OSError:
        return []
    return doc.get("todo", [])


class TriageState:
    """A per-request snapshot: TRUE survivors + LIVE/stale ledger classification
    + the To-Fix worklist. Re-built each request so the UI reflects edits
    immediately."""

    def __init__(self, root, ledger, report_paths, todo):
        self.root = root
        self.ledger = ledger
        self.todo = todo
        self.report_paths = report_paths
        self.survivors, self.parsed_with_path = load_survivors(report_paths)
        self.by_id = {survivor_id(sv): sv for sv in self.survivors}

        # Ledger -> LIVE vs stale (resolve recomputes region_hash vs the tree).
        self.entries = load_ledger(ledger, root)
        self.live = []
        self.stale = []
        for e in self.entries:
            if e.resolve():
                self.live.append(e)
            else:
                self.stale.append(e)

        # Worklist -> {key: raw todo dict} for O(1) (file,line,col,mutator) match.
        self.todos = load_todos(todo)
        self.todo_by_key = {}
        for raw in self.todos:
            k = todo_key(raw.get("file", ""), raw.get("line", 0),
                         raw.get("col", 0), raw.get("mutator"))
            self.todo_by_key[k] = raw

    def todo_for(self, sv):
        """The raw todo dict matching this survivor's key, or None."""
        return self.todo_by_key.get(
            todo_key(sv.file, sv.line, sv.col, sv.mutator))

    def status_for(self, sv):
        """Status precedence: a LIVE ledger entry ('ignored:<id>') wins over a
        worklist entry ('tofix') over 'remaining'. Returns (status, hit) where
        hit is the matching LedgerEntry for ignored, else None."""
        hit = next((e for e in self.live if e.matches(sv)), None)
        if hit is not None:
            return "ignored:%s" % hit.id, hit
        if self.todo_for(sv) is not None:
            return "tofix", None
        return "remaining", None

    def summary(self):
        total = len(self.survivors)
        ignored = 0
        remaining = 0
        tofix = 0
        by_file = {}
        for sv in self.survivors:
            status, _ = self.status_for(sv)
            rel = relpath(self.root, sv.file)
            slot = by_file.setdefault(rel, {"total": 0, "ignored": 0,
                                            "remaining": 0, "tofix": 0})
            slot["total"] += 1
            if status.startswith("ignored"):
                ignored += 1
                slot["ignored"] += 1
            elif status == "tofix":
                tofix += 1
                slot["tofix"] += 1
            else:
                remaining += 1
                slot["remaining"] += 1
        return {"total": total, "ignored": ignored, "remaining": remaining,
                "tofix": tofix, "by_file": by_file}


# --------------------------------------------------------------------------
# Source / blame / component helpers
# --------------------------------------------------------------------------
def relpath(root, file):
    """Best-effort repo-relative path for a (possibly absolute) reported file."""
    ap = os.path.abspath(file)
    rp = os.path.abspath(root)
    if ap.startswith(rp + os.sep):
        return os.path.relpath(ap, rp)
    return file


def component_of(file):
    """Component name from the source file: Vec.c -> 'Vec', Map.c -> 'Map'."""
    base = os.path.basename(file)
    return base[:-2] if base.endswith(".c") else os.path.splitext(base)[0]


def read_source_lines(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read().splitlines()


def abs_source(root, file):
    """Resolve a reported (abs or rel) source path to a readable absolute path."""
    if os.path.isabs(file) and os.path.isfile(file):
        return file
    cand = os.path.join(root, relpath(root, file))
    if os.path.isfile(cand):
        return cand
    return file


def source_context(root, sv, span=6):
    """~12 lines around the target line, each {n, text, is_target}."""
    path = abs_source(root, sv.file)
    try:
        lines = read_source_lines(path)
    except OSError:
        return []
    target = sv.line
    lo = max(1, target - span)
    hi = min(len(lines), target + span)
    out = []
    for n in range(lo, hi + 1):
        out.append({"n": n, "text": lines[n - 1], "is_target": n == target})
    return out


def enclosing_function(root, sv):
    """Best-effort name of the C function containing the target line: scan
    upward for a top-level definition signature ``type name(`` at column 0."""
    path = abs_source(root, sv.file)
    try:
        lines = read_source_lines(path)
    except OSError:
        return None
    sig = re.compile(r"^[A-Za-z_].*\b([A-Za-z_]\w*)\s*\(")
    name = None
    for i in range(min(sv.line, len(lines)) - 1, -1, -1):
        line = lines[i]
        if not line or line[0] in " \t":
            continue
        stripped = line.rstrip()
        if stripped.endswith(";") or stripped.endswith(","):
            continue
        m = sig.match(line)
        if m:
            # Exclude obvious non-functions (control keywords at col 0 are rare
            # in this style, but guard anyway).
            cand = m.group(1)
            if cand not in ("if", "for", "while", "switch", "return", "sizeof"):
                name = cand
                break
    return name


def git_blame(root, file, line):
    """Run `git blame --porcelain -L line,line` for the target line and parse
    {commit, author, date, summary}; returns None if blame is unavailable."""
    rel = relpath(root, file)
    try:
        out = subprocess.run(
            ["git", "-C", root, "blame", "--porcelain",
             "-L", "%d,%d" % (line, line), "--", rel],
            capture_output=True, text=True, check=True,
        ).stdout
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None
    if not out:
        return None
    blame: dict[str, str | None] = {
        "commit": None, "author": None, "date": None, "summary": None}
    lines = out.splitlines()
    if lines:
        blame["commit"] = lines[0].split()[0][:12] if lines[0] else None
    author_time = None
    for ln in lines:
        if ln.startswith("author "):
            blame["author"] = ln[len("author "):].strip()
        elif ln.startswith("author-time "):
            try:
                author_time = int(ln[len("author-time "):].strip())
            except ValueError:
                author_time = None
        elif ln.startswith("summary "):
            blame["summary"] = ln[len("summary "):].strip()
    if author_time is not None:
        import time
        blame["date"] = time.strftime("%Y-%m-%d", time.gmtime(author_time))
    return blame


def coverage_hint(state, sv):
    """HONEST best-effort coverage context. We cannot link coverage
    instrumentation, so we cannot name the failing test functions; we report
    the component, the suites that ran this mutant and did NOT catch it, and the
    related test files on disk where a killing assertion would go."""
    comp = component_of(sv.file)
    # Suites (report names) that list this exact survivor key -> they executed
    # the mutant and did not catch it.
    suites = []
    for rp, parsed in state.parsed_with_path:
        for s, _ in parsed:
            if s is not None and s.key == sv.key:
                name = os.path.basename(rp)
                if name not in suites:
                    suites.append(name)
                break
    # Related test files on disk: Tests/**/<Component>*.c and <Component>.*.c.
    test_files = []
    seen = set()
    patterns = [
        os.path.join(state.root, "Tests", "**", comp + "*.c"),
        os.path.join(state.root, "Tests", "**", comp + ".*.c"),
    ]
    for pat in patterns:
        for p in sorted(globmod.glob(pat, recursive=True)):
            rel = relpath(state.root, p)
            if rel not in seen:
                seen.add(rel)
                test_files.append(rel)
    label = (
        "no suite currently catches this mutant; coverage isn't available to "
        "pinpoint which tests execute the line -- these are the suites/files "
        "where a killing assertion would go."
    )
    return {
        "component": comp,
        "label": label,
        "suites_ran_not_caught": suites,
        "related_test_files": test_files,
    }


def stanza_fields(entry):
    """Ledger stanza fields for the detail view (id, selector, bucket, ...)."""
    raw = entry.raw
    selector = None
    if "function" in raw:
        selector = {"type": "function", "function": raw["function"]}
    elif "lines" in raw:
        selector = {"type": "lines", "lines": raw["lines"]}
    elif "pattern" in raw:
        selector = {"type": "pattern", "pattern": raw["pattern"]}
    return {
        "id": entry.id,
        "selector": selector,
        "mutator": raw.get("mutator"),
        "bucket": raw.get("bucket"),
        "rationale": raw.get("rationale"),
        "author": raw.get("author"),
        "created": raw.get("created"),
        "region_hash": raw.get("region_hash"),
    }


# --------------------------------------------------------------------------
# Ledger writing (fixed-schema TOML; stdlib has no writer)
# --------------------------------------------------------------------------
def today():
    """Today's date from the system via `date +%F` (never hardcoded)."""
    try:
        out = subprocess.run(["date", "+%F"], capture_output=True,
                             text=True, check=True).stdout.strip()
        if re.fullmatch(r"\d{4}-\d{2}-\d{2}", out):
            return out
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    import time
    return time.strftime("%Y-%m-%d")


def _toml_quote(s):
    """Quote a TOML basic string (the ledger uses double-quoted strings)."""
    s = s.replace("\\", "\\\\").replace('"', '\\"')
    s = s.replace("\n", "\\n").replace("\t", "\\t").replace("\r", "\\r")
    return '"' + s + '"'


def existing_ids(ledger):
    try:
        doc = load_toml(ledger)
    except OSError:
        return set()
    return {e.get("id") for e in doc.get("ignore", []) if e.get("id")}


def make_id(component, file, selector, ids):
    """Generate a unique id <COMPONENT>-<SHORTHASH>, falling back to -NNN."""
    comp = component.upper()
    basis = "%s|%s|%s" % (file, json.dumps(selector, sort_keys=True),
                          today())
    short = hashlib.sha256(basis.encode("utf-8")).hexdigest()[:6].upper()
    cand = "%s-%s" % (comp, short)
    if cand not in ids:
        return cand
    n = 1
    while True:
        cand = "%s-%03d" % (comp, n)
        if cand not in ids:
            return cand
        n += 1


def serialize_stanza(fields):
    """Emit one [[ignore]] stanza exactly as the filter expects."""
    out = ["[[ignore]]"]
    out.append("id = %s" % _toml_quote(fields["id"]))
    out.append("file = %s" % _toml_quote(fields["file"]))
    sel = fields["selector"]
    if sel["type"] == "function":
        out.append("function = %s" % _toml_quote(sel["function"]))
    elif sel["type"] == "lines":
        a, b = sel["lines"]
        out.append("lines = [%d, %d]" % (a, b))
    elif sel["type"] == "pattern":
        out.append("pattern = %s" % _toml_quote(sel["pattern"]))
    if fields.get("mutator"):
        out.append("mutator = %s" % _toml_quote(fields["mutator"]))
    out.append("bucket = %s" % _toml_quote(fields["bucket"]))
    out.append("rationale = %s" % _toml_quote(fields["rationale"]))
    out.append("author = %s" % _toml_quote(fields["author"]))
    out.append("created = %s" % _toml_quote(fields["created"]))
    out.append("region_hash = %s" % _toml_quote(fields["region_hash"]))
    return "\n".join(out) + "\n"


def append_stanza(ledger, stanza_text):
    """Append a stanza to the ledger, preserving all existing content."""
    with open(ledger, "r", encoding="utf-8") as f:
        existing = f.read()
    sep = "" if existing.endswith("\n\n") else (
        "\n" if existing.endswith("\n") else "\n\n")
    with open(ledger, "w", encoding="utf-8") as f:
        f.write(existing + sep + stanza_text)


# A stanza spans from a '[[ignore]]' line to the next '[[ignore]]' or EOF.
_IGNORE_HEADER = re.compile(r"^\s*\[\[ignore\]\]\s*$")
_ID_LINE = re.compile(r'^\s*id\s*=\s*"([^"]*)"')


def remove_stanza(ledger, target_id):
    """Parse the ledger into a preamble + stanza blocks, drop the block whose
    id matches, and rewrite. Never corrupts other stanzas. Returns True if a
    stanza was removed."""
    with open(ledger, "r", encoding="utf-8") as f:
        lines = f.readlines()

    # Split into preamble (before first [[ignore]]) and stanza blocks.
    blocks = []
    preamble = []
    cur = None
    for ln in lines:
        if _IGNORE_HEADER.match(ln):
            if cur is not None:
                blocks.append(cur)
            cur = [ln]
        elif cur is None:
            preamble.append(ln)
        else:
            cur.append(ln)
    if cur is not None:
        blocks.append(cur)

    kept = []
    removed = False
    for blk in blocks:
        bid = None
        for ln in blk:
            m = _ID_LINE.match(ln)
            if m:
                bid = m.group(1)
                break
        if bid == target_id and not removed:
            removed = True
            continue
        kept.append(blk)

    if not removed:
        return False

    out = "".join(preamble).rstrip("\n")
    parts = ["".join(blk).rstrip("\n") for blk in kept]
    text = out
    for p in parts:
        text = (text + "\n\n" + p) if text else p
    if text and not text.endswith("\n"):
        text += "\n"
    with open(ledger, "w", encoding="utf-8") as f:
        f.write(text)
    return True


# --------------------------------------------------------------------------
# To-Fix worklist (fixed-schema TOML; same loader/writer style as the ledger)
# --------------------------------------------------------------------------
def load_todo_doc(path):
    """Parse the worklist TOML into {"todo": [ {...}, ... ]}. Reuses tomllib
    when available, else a minimal reader for this fixed [[todo]] shape."""
    if mull_filter.tomllib is not None:
        with open(path, "rb") as f:
            doc = mull_filter.tomllib.load(f)
        doc.setdefault("todo", [])
        return doc
    data = {"todo": []}
    cur = None
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if line == "[[todo]]":
                cur = {}
                data["todo"].append(cur)
                continue
            if "=" not in line or cur is None:
                continue
            key, _, val = line.partition("=")
            cur[key.strip()] = mull_filter._toml_value(
                val.split("#", 1)[0].strip())
    return data


def todo_region_hash(root, file, line):
    """region_hash of the single target line via mull-filter's region_hash on a
    lines=[line,line] selector. Returns the 'sha256:<hex>' string, or None if
    the line cannot be resolved in the current tree."""
    try:
        region = select_region(root, {"file": relpath(root, file),
                                       "lines": [line, line]})
    except (ValueError, FileNotFoundError, OSError):
        return None
    return region_hash(region)


def serialize_todo(fields):
    """Emit one [[todo]] stanza with the fixed worklist schema."""
    out = ["[[todo]]"]
    out.append("file = %s" % _toml_quote(fields["file"]))
    out.append("line = %d" % int(fields["line"]))
    out.append("col = %d" % int(fields["col"]))
    out.append("mutator = %s" % _toml_quote(fields.get("mutator") or "?"))
    if fields.get("note"):
        out.append("note = %s" % _toml_quote(fields["note"]))
    out.append("created = %s" % _toml_quote(fields["created"]))
    out.append("region_hash = %s" % _toml_quote(fields["region_hash"]))
    return "\n".join(out) + "\n"


def append_todo(path, stanza_text):
    """Append a [[todo]] stanza, preserving all existing content (incl. the
    header comment). Creates the file if missing."""
    existing = ""
    if os.path.isfile(path):
        with open(path, "r", encoding="utf-8") as f:
            existing = f.read()
    if not existing:
        sep = ""
    elif existing.endswith("\n\n"):
        sep = ""
    elif existing.endswith("\n"):
        sep = "\n"
    else:
        sep = "\n\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(existing + sep + stanza_text)


# A todo stanza spans from a '[[todo]]' line to the next '[[todo]]' or EOF.
_TODO_HEADER = re.compile(r"^\s*\[\[todo\]\]\s*$")
_TODO_FIELD = re.compile(r'^\s*(\w+)\s*=\s*(.+?)\s*$')


def _todo_block_key(block):
    """Extract the (basename, line, col, mutator) key from a stanza block."""
    file = line = col = mutator = None
    for ln in block:
        m = _TODO_FIELD.match(ln)
        if not m:
            continue
        k, v = m.group(1), mull_filter._toml_value(v_strip(m.group(2)))
        if k == "file":
            file = v
        elif k == "line":
            line = v
        elif k == "col":
            col = v
        elif k == "mutator":
            mutator = v
    if file is None or line is None or col is None:
        return None
    return todo_key(file, line, col, mutator)


def v_strip(val):
    """Strip an inline comment from a TOML value the same way the loaders do."""
    return val.split("#", 1)[0].strip()


def remove_todo(path, file, line, col, mutator):
    """Remove the [[todo]] stanza matching the (file,line,col,mutator) key,
    preserving every other stanza and the preamble. Returns True if removed."""
    if not os.path.isfile(path):
        return False
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    blocks = []
    preamble = []
    cur = None
    for ln in lines:
        if _TODO_HEADER.match(ln):
            if cur is not None:
                blocks.append(cur)
            cur = [ln]
        elif cur is None:
            preamble.append(ln)
        else:
            cur.append(ln)
    if cur is not None:
        blocks.append(cur)

    target = todo_key(file, line, col, mutator)
    kept = []
    removed = False
    for blk in blocks:
        if not removed and _todo_block_key(blk) == target:
            removed = True
            continue
        kept.append(blk)

    if not removed:
        return False

    out = "".join(preamble).rstrip("\n")
    parts = ["".join(blk).rstrip("\n") for blk in kept]
    text = out
    for p in parts:
        text = (text + "\n\n" + p) if text else p
    if text and not text.endswith("\n"):
        text += "\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)
    return True


# --------------------------------------------------------------------------
# HTTP server
# --------------------------------------------------------------------------
class Config:
    root = "."
    ledger = ""
    todo = ""
    report_globs = []


class Handler(http.server.BaseHTTPRequestHandler):
    server_version = "MullTriage/1.0"

    # -- helpers --
    def _state(self):
        paths = discover_reports(Config.root, Config.report_globs)
        return TriageState(Config.root, Config.ledger, paths, Config.todo)

    def _send_json(self, obj, code=200):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_html(self, html, code=200):
        body = html.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _read_body(self):
        length = int(self.headers.get("Content-Length", 0) or 0)
        if length <= 0:
            return {}
        raw = self.rfile.read(length)
        try:
            return json.loads(raw.decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            return {}

    def log_message(self, format, *args):  # quieter logging
        sys.stderr.write("[triage] " + (format % args) + "\n")

    # -- routing --
    def do_GET(self):
        parsed = urllib.parse.urlsplit(self.path)
        path = parsed.path
        if path == "/" or path == "/index.html":
            return self._send_html(INDEX_HTML)
        if path == "/api/survivors":
            return self._api_survivors()
        if path.startswith("/api/survivor/"):
            sid = urllib.parse.unquote(path[len("/api/survivor/"):])
            return self._api_survivor(sid)
        return self._send_json({"error": "not found"}, 404)

    def do_POST(self):
        parsed = urllib.parse.urlsplit(self.path)
        path = parsed.path
        if path == "/api/ignore":
            return self._api_ignore()
        if path == "/api/unignore":
            return self._api_unignore()
        if path == "/api/tofix":
            return self._api_tofix()
        if path == "/api/untofix":
            return self._api_untofix()
        return self._send_json({"error": "not found"}, 404)

    # -- API handlers --
    def _api_survivors(self):
        st = self._state()
        items = []
        for sv in st.survivors:
            status, _ = st.status_for(sv)
            items.append({
                "id": survivor_id(sv),
                "file": relpath(st.root, sv.file),
                "line": sv.line,
                "col": sv.col,
                "mutator": sv.mutator,
                "status": status,
            })
        return self._send_json({"summary": st.summary(), "items": items})

    def _api_survivor(self, sid):
        st = self._state()
        sv = st.by_id.get(sid)
        if sv is None:
            return self._send_json({"error": "unknown survivor id"}, 404)
        status, hit = st.status_for(sv)
        detail = {
            "id": sid,
            "file": relpath(st.root, sv.file),
            "line": sv.line,
            "col": sv.col,
            "mutator": sv.mutator,
            "mutation": survivor_desc(sv),
            "status": status,
            "source_context": source_context(st.root, sv),
            "git_blame": git_blame(st.root, sv.file, sv.line),
            "coverage_hint": coverage_hint(st, sv),
            "enclosing_function": enclosing_function(st.root, sv),
            "stanza": stanza_fields(hit) if hit is not None else None,
            "todo": None,
        }
        raw_todo = st.todo_for(sv)
        if raw_todo is not None:
            stored = raw_todo.get("region_hash")
            current = todo_region_hash(st.root, sv.file, sv.line)
            detail["todo"] = {
                "note": raw_todo.get("note"),
                "created": raw_todo.get("created"),
                "region_hash": stored,
                "current_region_hash": current,
                # not stale == the stored hash still matches the current line.
                "matches": (current is not None and current == stored),
                "stale": (current is None or current != stored),
            }
        return self._send_json(detail)

    def _api_ignore(self):
        body = self._read_body()
        file = body.get("file")
        selector = body.get("selector") or {}
        bucket = body.get("bucket")
        rationale = body.get("rationale")
        author = body.get("author") or "triage-ui"
        mutator = body.get("mutator")

        # Validation: bucket B/C only (A never allowed), rationale non-empty.
        if bucket not in ("B", "C"):
            return self._send_json(
                {"error": 'bucket must be "B" or "C" (A is never allowed)'},
                400)
        if not rationale or not str(rationale).strip():
            return self._send_json({"error": "rationale is required"}, 400)
        if not file:
            return self._send_json({"error": "file is required"}, 400)

        rel = relpath(Config.root, file)
        sel_type = selector.get("type")
        if sel_type == "function":
            if not selector.get("function"):
                return self._send_json(
                    {"error": "function selector needs a function name"}, 400)
            sel = {"type": "function", "function": selector["function"]}
            raw_sel = {"file": rel, "function": selector["function"]}
        elif sel_type == "lines":
            ln = selector.get("lines")
            if (not isinstance(ln, list) or len(ln) != 2):
                return self._send_json(
                    {"error": "lines selector needs [start, end]"}, 400)
            sel = {"type": "lines", "lines": [int(ln[0]), int(ln[1])]}
            raw_sel = {"file": rel, "lines": [int(ln[0]), int(ln[1])]}
        else:
            return self._send_json(
                {"error": 'selector.type must be "lines" or "function"'}, 400)

        # Compute region_hash via the canonical filter logic against the tree.
        try:
            region = select_region(Config.root, raw_sel)
        except (ValueError, FileNotFoundError, OSError) as exc:
            return self._send_json(
                {"error": "selector does not resolve: %s" % exc}, 400)
        rhash = region_hash(region)

        ids = existing_ids(Config.ledger)
        new_id = make_id(component_of(file), rel, sel, ids)
        fields = {
            "id": new_id,
            "file": rel,
            "selector": sel,
            "mutator": mutator if (mutator and str(mutator).strip()) else None,
            "bucket": bucket,
            "rationale": str(rationale).strip(),
            "author": str(author).strip() or "triage-ui",
            "created": today(),
            "region_hash": rhash,
        }
        append_stanza(Config.ledger, serialize_stanza(fields))
        # Cross-flow: a survivor that was in the To-Fix worklist moves into the
        # ledger -- it must NOT remain in both. Drop any matching todo stanza.
        # The survivor key for the worklist is the reported (file,line,col,
        # mutator); resolve it via the live survivor that this ignore targets.
        st = self._state()
        for sv in st.survivors:
            if (relpath(Config.root, sv.file) == rel
                    and st.todo_for(sv) is not None):
                hit = next((e for e in st.live if e.matches(sv)), None)
                if hit is not None and hit.id == new_id:
                    remove_todo(Config.todo, sv.file, sv.line, sv.col,
                                sv.mutator)
        return self._send_json({"ok": True, "id": new_id})

    def _api_unignore(self):
        body = self._read_body()
        eid = body.get("id")
        if not eid:
            return self._send_json({"error": "id is required"}, 400)
        removed = remove_stanza(Config.ledger, eid)
        if not removed:
            return self._send_json({"error": "no stanza with that id"}, 404)
        return self._send_json({"ok": True})

    def _api_tofix(self):
        body = self._read_body()
        sid = body.get("id")
        note = body.get("note")
        if not sid:
            return self._send_json({"error": "id is required"}, 400)
        st = self._state()
        sv = st.by_id.get(sid)
        if sv is None:
            return self._send_json({"error": "unknown survivor id"}, 404)
        status, _ = st.status_for(sv)
        if status.startswith("ignored"):
            return self._send_json(
                {"error": "survivor is ignored; un-ignore it first"}, 400)

        rhash = todo_region_hash(Config.root, sv.file, sv.line)
        if rhash is None:
            return self._send_json(
                {"error": "target line does not resolve in the tree"}, 400)
        note_s = str(note).strip() if note and str(note).strip() else None

        # Idempotent-ish: if already in the worklist, replace it (update note).
        existing = st.todo_for(sv)
        if existing is not None:
            remove_todo(Config.todo, sv.file, sv.line, sv.col, sv.mutator)
        fields = {
            "file": relpath(Config.root, sv.file),
            "line": sv.line,
            "col": sv.col,
            "mutator": sv.mutator,
            "note": note_s,
            "created": today(),
            "region_hash": rhash,
        }
        append_todo(Config.todo, serialize_todo(fields))
        return self._send_json({"ok": True})

    def _api_untofix(self):
        body = self._read_body()
        sid = body.get("id")
        if not sid:
            return self._send_json({"error": "id is required"}, 400)
        st = self._state()
        sv = st.by_id.get(sid)
        if sv is None:
            return self._send_json({"error": "unknown survivor id"}, 404)
        removed = remove_todo(Config.todo, sv.file, sv.line, sv.col,
                              sv.mutator)
        if not removed:
            return self._send_json({"error": "not in the worklist"}, 404)
        return self._send_json({"ok": True})


class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


# --------------------------------------------------------------------------
# Single-page UI (inline HTML/CSS/JS, no external deps)
# --------------------------------------------------------------------------
INDEX_HTML = r"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Mull Survivor Triage</title>
<style>
  :root {
    --bg:#0f1115; --panel:#171a21; --panel2:#1d212b; --border:#2a2f3a;
    --fg:#d7dbe0; --muted:#8a93a3; --accent:#6aa0ff; --remain:#e0a04d;
    --ignored:#5fb87a; --tofix:#f0b429; --target:#3a2f12; --danger:#e06c75;
    --mono:'SFMono-Regular',Consolas,'Liberation Mono',Menlo,monospace;
  }
  * { box-sizing:border-box; }
  body { margin:0; background:var(--bg); color:var(--fg);
    font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;
    font-size:14px; }
  header { padding:10px 16px; background:var(--panel);
    border-bottom:1px solid var(--border); display:flex; align-items:center;
    gap:18px; flex-wrap:wrap; }
  header h1 { font-size:15px; margin:0; font-weight:600; }
  .counts { display:flex; gap:14px; }
  .count { font-variant-numeric:tabular-nums; }
  .count b { font-size:16px; }
  .count.total b { color:var(--accent); }
  .count.remaining b { color:var(--remain); }
  .count.tofix b { color:var(--tofix); }
  .count.ignored b { color:var(--ignored); }
  main { display:flex; height:calc(100vh - 49px); }
  #left { width:42%; min-width:340px; border-right:1px solid var(--border);
    display:flex; flex-direction:column; }
  #filters { padding:8px 12px; background:var(--panel);
    border-bottom:1px solid var(--border); display:flex; gap:8px;
    flex-wrap:wrap; align-items:center; }
  #filters select, #filters input { background:var(--panel2);
    color:var(--fg); border:1px solid var(--border); border-radius:4px;
    padding:4px 6px; font-size:13px; }
  #list { overflow:auto; flex:1; }
  .row { padding:7px 12px; border-bottom:1px solid var(--border);
    cursor:pointer; display:flex; align-items:center; gap:8px; }
  .row:hover { background:var(--panel2); }
  .row.sel { background:#243049; }
  .row .loc { font-family:var(--mono); font-size:12px; flex:1;
    white-space:nowrap; overflow:hidden; text-overflow:ellipsis; }
  .row .mut { font-family:var(--mono); font-size:11px; color:var(--muted); }
  .badge { font-size:10px; padding:2px 6px; border-radius:10px;
    font-weight:600; text-transform:uppercase; letter-spacing:.04em; }
  .badge.remaining { background:#3a2f12; color:var(--remain); }
  .badge.tofix { background:#3a3112; color:var(--tofix); }
  .badge.ignored { background:#16331f; color:var(--ignored); }
  .badge.stale { background:#3a1212; color:var(--danger); }
  #right { flex:1; overflow:auto; padding:16px 20px; }
  #right h2 { font-size:14px; margin:0 0 4px; font-family:var(--mono); }
  .sub { color:var(--muted); font-size:12px; margin-bottom:14px; }
  .card { background:var(--panel); border:1px solid var(--border);
    border-radius:6px; padding:12px 14px; margin-bottom:14px; }
  .card h3 { margin:0 0 8px; font-size:12px; text-transform:uppercase;
    letter-spacing:.05em; color:var(--muted); }
  pre.src { margin:0; font-family:var(--mono); font-size:12px; line-height:1.5;
    overflow:auto; }
  .ln { display:flex; }
  .ln .n { width:46px; text-align:right; padding-right:12px;
    color:var(--muted); user-select:none; }
  .ln .t { white-space:pre; }
  .ln.target { background:var(--target); }
  .ln.target .n { color:var(--remain); font-weight:700; }
  .annot { color:var(--remain); font-style:italic; }
  .blame { font-family:var(--mono); font-size:12px; color:var(--muted); }
  .cov-label { color:var(--remain); font-size:12px; margin-bottom:8px;
    line-height:1.5; }
  .cov ul { margin:4px 0 0; padding-left:18px; }
  .cov li { font-family:var(--mono); font-size:12px; }
  label { display:block; font-size:12px; color:var(--muted); margin:8px 0 3px; }
  select, textarea, input[type=text] { width:100%; background:var(--panel2);
    color:var(--fg); border:1px solid var(--border); border-radius:4px;
    padding:6px 8px; font-size:13px; font-family:inherit; }
  textarea { min-height:70px; resize:vertical; font-family:inherit; }
  .radios { display:flex; gap:16px; margin-top:4px; }
  .radios label { display:flex; align-items:center; gap:6px; margin:0;
    color:var(--fg); cursor:pointer; }
  .radios input { width:auto; }
  button { background:var(--accent); color:#08111f; border:0; border-radius:5px;
    padding:8px 14px; font-size:13px; font-weight:600; cursor:pointer;
    margin-top:12px; }
  button.danger { background:var(--danger); color:#1a0c0e; }
  button:disabled { opacity:.5; cursor:not-allowed; }
  .err { color:var(--danger); font-size:12px; margin-top:8px; }
  .kv { font-size:12px; line-height:1.7; }
  .kv b { color:var(--fg); }
  .empty { color:var(--muted); padding:40px; text-align:center; }
  code { font-family:var(--mono); background:var(--panel2); padding:1px 4px;
    border-radius:3px; }
</style>
</head>
<body>
<header>
  <h1>Mull Survivor Triage</h1>
  <div class="counts">
    <span class="count total">total <b id="c-total">-</b></span>
    <span class="count remaining">remaining <b id="c-remaining">-</b></span>
    <span class="count tofix">to-fix <b id="c-tofix">-</b></span>
    <span class="count ignored">ignored <b id="c-ignored">-</b></span>
  </div>
</header>
<main>
  <div id="left">
    <div id="filters">
      <select id="f-status">
        <option value="all">all</option>
        <option value="remaining" selected>remaining</option>
        <option value="tofix">to fix</option>
        <option value="ignored">ignored</option>
      </select>
      <select id="f-file"><option value="">all files</option></select>
      <select id="f-mutator"><option value="">all mutators</option></select>
      <input id="f-text" type="text" placeholder="filter text..." size="14">
    </div>
    <div id="list"></div>
  </div>
  <div id="right"><div class="empty">Select a survivor to triage.</div></div>
</main>
<script>
let ITEMS = [], SUMMARY = null, SELECTED = null;

function esc(s){ return String(s==null?'':s)
  .replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }

async function getJSON(u){ const r=await fetch(u); return r.json(); }
async function postJSON(u,b){
  const r=await fetch(u,{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify(b)});
  return {ok:r.ok, status:r.status, data:await r.json()};
}

function statusKind(s){
  if (s && s.startsWith('ignored')) return 'ignored';
  if (s === 'tofix') return 'tofix';
  return 'remaining';
}

async function loadList(keepSel){
  const d = await getJSON('/api/survivors');
  ITEMS = d.items; SUMMARY = d.summary;
  document.getElementById('c-total').textContent = SUMMARY.total;
  document.getElementById('c-ignored').textContent = SUMMARY.ignored;
  document.getElementById('c-remaining').textContent = SUMMARY.remaining;
  document.getElementById('c-tofix').textContent = SUMMARY.tofix;
  populateFilter('f-file', [...new Set(ITEMS.map(i=>i.file))].sort());
  populateFilter('f-mutator',
    [...new Set(ITEMS.map(i=>i.mutator).filter(Boolean))].sort());
  renderList();
  if (keepSel && SELECTED && ITEMS.some(i=>i.id===SELECTED))
    loadDetail(SELECTED);
}

function populateFilter(id, vals){
  const sel = document.getElementById(id);
  const cur = sel.value;
  const head = sel.querySelector('option').outerHTML;
  sel.innerHTML = head + vals.map(v=>
    `<option value="${esc(v)}">${esc(v)}</option>`).join('');
  if ([...sel.options].some(o=>o.value===cur)) sel.value = cur;
}

function filtered(){
  const st=document.getElementById('f-status').value;
  const ff=document.getElementById('f-file').value;
  const fm=document.getElementById('f-mutator').value;
  const tx=document.getElementById('f-text').value.toLowerCase();
  return ITEMS.filter(i=>{
    if (st!=='all' && statusKind(i.status)!==st) return false;
    if (ff && i.file!==ff) return false;
    if (fm && i.mutator!==fm) return false;
    if (tx){
      const hay=(i.file+':'+i.line+' '+(i.mutator||'')+' '+i.status).toLowerCase();
      if (!hay.includes(tx)) return false;
    }
    return true;
  });
}

function kindLabel(kind){ return kind==='tofix' ? 'to fix' : kind; }

function renderList(){
  const rows = filtered().sort((a,b)=>
    a.file.localeCompare(b.file) || a.line-b.line || a.col-b.col);
  const html = rows.map(i=>{
    const kind = statusKind(i.status);
    return `<div class="row ${i.id===SELECTED?'sel':''}" data-id="${esc(i.id)}">
      <span class="loc">${esc(i.file.split('/').pop())}:${i.line}</span>
      <span class="mut">${esc(i.mutator||'?')}</span>
      <span class="badge ${kind}">${kindLabel(kind)}</span></div>`;
  }).join('');
  const list = document.getElementById('list');
  list.innerHTML = html || '<div class="empty">No survivors match.</div>';
  list.querySelectorAll('.row').forEach(r=>
    r.onclick=()=>{ SELECTED=r.dataset.id; renderList(); loadDetail(SELECTED); });
}

function srcHtml(ctx, mutation){
  return '<pre class="src">'+ctx.map(l=>{
    let line = `<div class="ln ${l.is_target?'target':''}">`+
      `<span class="n">${l.n}</span><span class="t">${esc(l.text)}</span></div>`;
    if (l.is_target && mutation)
      line += `<div class="ln"><span class="n"></span>`+
        `<span class="t annot">  ↳ mutation: ${esc(mutation)}</span></div>`;
    return line;
  }).join('')+'</pre>';
}

function blameHtml(b){
  if (!b) return '<div class="blame">blame unavailable</div>';
  return `<div class="blame">added in ${esc(b.commit||'?')} by `+
    `${esc(b.author||'?')} on ${esc(b.date||'?')}: ${esc(b.summary||'')}</div>`;
}

function covHtml(c){
  if (!c) return '';
  const suites = (c.suites_ran_not_caught||[]).map(s=>`<li>${esc(s)}</li>`).join('');
  const files = (c.related_test_files||[]).map(s=>`<li>${esc(s)}</li>`).join('');
  return `<div class="card cov"><h3>Coverage hint &mdash; component ${esc(c.component)}</h3>
    <div class="cov-label">${esc(c.label)}</div>
    <div><b>Suites that ran this mutant (did not catch it):</b>
      <ul>${suites||'<li>(none)</li>'}</ul></div>
    <div style="margin-top:8px"><b>Related test files on disk:</b>
      <ul>${files||'<li>(none)</li>'}</ul></div></div>`;
}

function ignoreForm(d){
  const fn = d.enclosing_function;
  return `<div class="card"><h3>Ignore this survivor</h3>
    <label>Bucket</label>
    <select id="ig-bucket">
      <option value="B">B &mdash; strategy / performance</option>
      <option value="C">C &mdash; equivalent mutant</option>
    </select>
    <label>Granularity</label>
    <div class="radios">
      <label><input type="radio" name="gran" value="lines" checked>
        this line (${d.line})</label>
      <label><input type="radio" name="gran" value="function" ${fn?'':'disabled'}>
        this function ${fn?'('+esc(fn)+')':'(unknown)'}</label>
    </div>
    <label>Rationale (required)</label>
    <textarea id="ig-rationale" placeholder="why is this survivor accepted, in caller-contract terms"></textarea>
    <label>Author</label>
    <input id="ig-author" type="text" value="triage-ui">
    <button id="ig-submit">Ignore</button>
    <div class="err" id="ig-err"></div></div>`;
}

function tofixForm(){
  return `<div class="card"><h3>Mark as &ldquo;To Fix&rdquo;</h3>
    <div class="cov-label">A genuine bucket-A gap that needs a killing test.
      This moves it out of Remaining into the To-Fix worklist
      (Conventions/mull-todo.toml); it does NOT suppress the survivor.</div>
    <label>Note (optional &mdash; why / what to fix)</label>
    <input id="tf-note" type="text" placeholder="e.g. assert retrievability after collision">
    <button id="tf-submit">To Fix</button>
    <div class="err" id="tf-err"></div></div>`;
}

function todoHtml(t){
  const staleBadge = t.stale
    ? ' <span class="badge stale">stale</span>' : '';
  const staleNote = t.stale
    ? `<div class="cov-label">region_hash no longer matches the current line
        &mdash; the code moved; re-check this entry.</div>` : '';
  return `<div class="card"><h3>To-Fix worklist entry${staleBadge}</h3>
    ${staleNote}
    <div class="kv">
      <div><b>note:</b> ${t.note?esc(t.note):'<i>(none)</i>'}</div>
      <div><b>created:</b> ${esc(t.created||'?')}</div>
      <div><b>region_hash:</b> <code>${esc(t.region_hash||'?')}</code></div>
    </div>
    <button class="danger" id="tf-remove">Un-mark (To Fix)</button>
    <div class="err" id="tf-rerr"></div></div>`;
}

function stanzaHtml(s){
  const selTxt = s.selector ? (s.selector.type==='function'
    ? 'function '+s.selector.function
    : s.selector.type==='lines'
      ? 'lines '+JSON.stringify(s.selector.lines)
      : 'pattern '+s.selector.pattern) : '?';
  return `<div class="card"><h3>Ledger stanza</h3>
    <div class="kv">
      <div><b>id:</b> <code>${esc(s.id)}</code></div>
      <div><b>bucket:</b> ${esc(s.bucket)}</div>
      <div><b>selector:</b> ${esc(selTxt)}</div>
      ${s.mutator?`<div><b>mutator:</b> ${esc(s.mutator)}</div>`:''}
      <div><b>author:</b> ${esc(s.author)}</div>
      <div><b>created:</b> ${esc(s.created)}</div>
      <div><b>rationale:</b> ${esc(s.rationale)}</div>
    </div>
    <button class="danger" id="un-submit">Un-ignore</button>
    <div class="err" id="un-err"></div></div>`;
}

async function loadDetail(id){
  const d = await getJSON('/api/survivor/'+encodeURIComponent(id));
  const right = document.getElementById('right');
  if (d.error){ right.innerHTML='<div class="empty">'+esc(d.error)+'</div>'; return; }
  const kind = statusKind(d.status);
  let html = `<h2>${esc(d.file)}:${d.line}:${d.col}</h2>
    <div class="sub">mutator <code>${esc(d.mutator||'?')}</code> &middot;
      <span class="badge ${kind}">${kindLabel(kind)}</span></div>
    <div class="card"><h3>Source context</h3>${srcHtml(d.source_context, d.mutation)}</div>
    <div class="card"><h3>Git blame (target line)</h3>${blameHtml(d.git_blame)}</div>
    ${covHtml(d.coverage_hint)}`;
  if (kind==='ignored' && d.stanza){
    html += stanzaHtml(d.stanza);
  } else if (kind==='tofix'){
    // To-Fix: show the worklist entry + un-mark, and still offer Ignore
    // (which moves it from to-fix to ignored).
    if (d.todo) html += todoHtml(d.todo);
    html += ignoreForm(d);
  } else {
    // Remaining: offer both Ignore and To-Fix.
    html += ignoreForm(d);
    html += tofixForm();
  }
  right.innerHTML = html;

  // Wire the un-ignore button when present.
  if (kind==='ignored' && d.stanza){
    document.getElementById('un-submit').onclick = async ()=>{
      const r = await postJSON('/api/unignore', {id:d.stanza.id});
      if (!r.ok){ document.getElementById('un-err').textContent =
        r.data.error||'failed'; return; }
      await loadList(true);
    };
    return;
  }

  // Wire the un-mark (To Fix) button when present.
  if (kind==='tofix'){
    document.getElementById('tf-remove').onclick = async ()=>{
      const r = await postJSON('/api/untofix', {id:d.id});
      if (!r.ok){ document.getElementById('tf-rerr').textContent =
        r.data.error||'failed'; return; }
      await loadList(true);
    };
  }

  // Wire the To-Fix submit button when present (remaining state only).
  const tfBtn = document.getElementById('tf-submit');
  if (tfBtn){
    tfBtn.onclick = async ()=>{
      const errEl = document.getElementById('tf-err'); errEl.textContent='';
      const note = document.getElementById('tf-note').value;
      const r = await postJSON('/api/tofix', {id:d.id, note});
      if (!r.ok){ errEl.textContent = r.data.error||'failed'; return; }
      await loadList(true);
    };
  }

  // Wire the Ignore form (present for remaining AND tofix states).
  {
    document.getElementById('ig-submit').onclick = async ()=>{
      const errEl = document.getElementById('ig-err'); errEl.textContent='';
      const bucket = document.getElementById('ig-bucket').value;
      const rationale = document.getElementById('ig-rationale').value;
      const author = document.getElementById('ig-author').value;
      const gran = document.querySelector('input[name=gran]:checked').value;
      let selector;
      if (gran==='function') selector={type:'function', function:d.enclosing_function};
      else selector={type:'lines', lines:[d.line, d.line]};
      if (!rationale.trim()){ errEl.textContent='rationale is required'; return; }
      const r = await postJSON('/api/ignore',
        {file:d.file, selector, mutator:d.mutator, bucket, rationale, author});
      if (!r.ok){ errEl.textContent = r.data.error||'failed'; return; }
      await loadList(true);
    };
  }
}

['f-status','f-file','f-mutator'].forEach(id=>
  document.getElementById(id).addEventListener('change', renderList));
document.getElementById('f-text').addEventListener('input', renderList);
loadList();
</script>
</body>
</html>
"""


# --------------------------------------------------------------------------
# Auto-restart supervisor (--reload)
# --------------------------------------------------------------------------
def _watched_files():
    """Source files whose change should trigger a server restart: this server
    and the mull-filter engine it imports. Resolved to real absolute paths."""
    here = os.path.realpath(os.path.abspath(__file__))
    files = [here]
    flt = os.path.join(os.path.dirname(here), "mull-filter.py")
    flt = os.path.realpath(flt)
    if flt not in files and os.path.isfile(flt):
        files.append(flt)
    return files


def _mtimes(files):
    """Snapshot {path: mtime} for the watched files (missing files -> None)."""
    snap = {}
    for f in files:
        try:
            snap[f] = os.path.getmtime(f)
        except OSError:
            snap[f] = None
    return snap


def _spawn_child(child_args):
    """Launch the actual server as a child subprocess (no --reload)."""
    env = dict(os.environ)
    env["_MULL_TRIAGE_RELOAD_CHILD"] = "1"
    return subprocess.Popen([sys.executable, os.path.abspath(__file__),
                             *child_args], env=env)


def _stop_child(proc):
    """Terminate the child gracefully (SIGTERM, then SIGKILL after ~5s)."""
    if proc.poll() is not None:
        return
    try:
        proc.terminate()
    except OSError:
        return
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        try:
            proc.kill()
        except OSError:
            pass
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            pass


def supervise(child_args, port, watched):
    """Run as a supervisor: spawn the server child, watch source mtimes, and
    restart on change. If the child crashes on its own, wait for the next
    change before relaunching (no busy loop). Ctrl-C stops the child cleanly."""
    names = ", ".join(os.path.relpath(f) for f in watched)
    sys.stderr.write(
        "[reload] watching %s; server on http://127.0.0.1:%d "
        "(auto-restart on change)\n" % (names, port))

    proc = _spawn_child(child_args)
    snap = _mtimes(watched)
    waiting_after_crash = False

    # Ctrl-C (SIGINT) / SIGTERM: stop the child and exit cleanly. A flag set
    # from the handler is the robust path -- it works whether the signal lands
    # inside time.sleep or anywhere else in the loop.
    stopping = {"flag": False}

    def _on_signal(*_):
        stopping["flag"] = True

    signal.signal(signal.SIGINT, _on_signal)
    signal.signal(signal.SIGTERM, _on_signal)

    try:
        while not stopping["flag"]:
            time.sleep(0.5)
            if stopping["flag"]:
                break
            cur = _mtimes(watched)
            changed = next((f for f in watched if cur[f] != snap[f]), None)

            if changed is not None:
                snap = cur
                sys.stderr.write(
                    "[reload] change detected in %s; restarting...\n"
                    % os.path.relpath(changed))
                # If the child crashed we are not serving; otherwise stop it
                # gracefully before rebinding the same port.
                if not waiting_after_crash:
                    _stop_child(proc)
                proc = _spawn_child(child_args)
                waiting_after_crash = False
                continue

            if not waiting_after_crash and proc.poll() is not None:
                sys.stderr.write(
                    "[reload] server exited on its own (code %s); "
                    "waiting for next change before restart...\n"
                    % proc.returncode)
                waiting_after_crash = True
    except KeyboardInterrupt:
        stopping["flag"] = True

    sys.stderr.write("\n[reload] shutting down\n")
    _stop_child(proc)
    return 0


# --------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------
def main(argv=None):
    p = argparse.ArgumentParser(
        description="Local mull mutation-survivor triage web tool.")
    conv_dir = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "Conventions")
    default_ledger = os.path.join(conv_dir, "mull-ignores.toml")
    default_todo = os.path.join(conv_dir, "mull-todo.toml")
    p.add_argument("--port", type=int, default=8765)
    p.add_argument("--root", default=".")
    p.add_argument("--ledger", default=default_ledger)
    p.add_argument("--todo", default=default_todo,
                   help="path to the To-Fix worklist (mull-todo.toml)")
    p.add_argument("--reload", action="store_true",
                   help="supervise mode: auto-restart the server when its "
                        "source (this file or mull-filter.py) changes")
    p.add_argument("report_globs", nargs="*",
                   default=["build_mull/**/mutation-*.txt"],
                   help="report glob(s) (recursive); "
                        "default build_mull/**/mutation-*.txt")
    raw_argv = list(sys.argv[1:] if argv is None else argv)
    args = p.parse_args(argv)

    # Supervisor mode: do not serve here. Spawn a child that runs the real
    # server (no --reload) and restart it when watched source files change.
    is_child = os.environ.get("_MULL_TRIAGE_RELOAD_CHILD") == "1"
    if args.reload and not is_child:
        child_args = [a for a in raw_argv if a != "--reload"]
        return supervise(child_args, args.port, _watched_files())

    Config.root = os.path.abspath(args.root)
    Config.ledger = (args.ledger if os.path.isabs(args.ledger)
                     else os.path.join(Config.root, args.ledger))
    Config.todo = (args.todo if os.path.isabs(args.todo)
                   else os.path.join(Config.root, args.todo))
    Config.report_globs = args.report_globs or ["build_mull/**/mutation-*.txt"]

    httpd = Server(("127.0.0.1", args.port), Handler)
    n = len(discover_reports(Config.root, Config.report_globs))
    sys.stderr.write(
        "mull-triage-server: http://127.0.0.1:%d  "
        "(root=%s, ledger=%s, %d report file(s))\n"
        % (args.port, Config.root, Config.ledger, n))
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        sys.stderr.write("\nshutting down\n")
    finally:
        httpd.shutdown()
        httpd.server_close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
