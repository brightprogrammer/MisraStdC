#!/usr/bin/env python3
"""Filter accepted mutation survivors out of a mull-runner report.

Mutation testing here is *discovery*, not a score to maximise (see
``Conventions/MULL-DISCOVERY-CONVENTIONS.md``). Some surviving mutants are
accepted by design: bucket-B (strategy / performance) and bucket-C
(equivalent) survivors. This tool reads the report that ``Scripts/mutation.sh``
captures from mull-runner and *removes* the survivors that a human has already
triaged and recorded in the ledger ``Conventions/mull-ignores.toml`` so that
downstream consumers only ever see survivors that still need triage.

Report format
-------------
``Scripts/mutation.sh`` runs ``mull-runner`` with no ``--reporters`` flag, so
mull emits its default *IDE* reporter to stdout, which ``mutation.sh`` tees to
``build_mull/<comp>/mutation-<suite>.txt``. A surviving mutant is reported as a
three-line block::

    /abs/or/rel/path/Map.c:31:39: warning: Survived: <desc> [<mutator>]
    return probe_pressure > 0 && (probe_pressure * 4) >= capacity;
                                          ^

followed by summary lines like ``[info] Mutation score: 75%`` and
``[info] Surviving mutants: 1``. This filter keys off the
``...: warning: Survived: ... [<mutator>]`` line; it carries the file, line,
column and mutator id needed to match a ledger entry.

region_hash normalisation
-------------------------
A ledger entry anchors to a source region via exactly one selector
(``function`` / ``lines`` / ``pattern``). ``region_hash`` is the SHA-256 of
that region's lines after this normalisation, applied per line:

  1. strip leading and trailing whitespace;
  2. collapse every internal run of whitespace to a single space.

The normalised lines are joined with ``\n`` and hashed. For a ``pattern``
selector the region is the SET of source lines matching the regex, taken in
*ascending file (line-number) order* (a deterministic ordering); duplicates are
preserved per occurrence in that order.

Invalidation (the git tie-in)
-----------------------------
Before filtering, every entry's ``region_hash`` is recomputed against the
CURRENT working tree. If it no longer matches, the entry is STALE: it suppresses
nothing, is listed loudly in the summary, and the tool exits non-zero. The hash
-- not any git diff -- is the source of truth; ``--check-range`` only uses the
diff as a fast-path to skip hashing entries in untouched files.

Usage
-----
    mull-filter.py [--ledger PATH] [--root DIR] [--out PATH]
                   [--fail-on-new <base>..<head>] REPORT [REPORT ...]
    mull-filter.py --check-range <base>..<head> [--ledger PATH] [--root DIR]

In the default mode it reads one or more report files, writes the filtered
report to ``--out`` (default ``report.filtered.txt`` next to the first input),
and prints a summary block. ``--check-range`` is the enforcement-hook mode: it
runs only the stale-detection (no report needed) over the entries whose files
changed in ``<base>..<head>`` and exits non-zero if any covered region moved
without its ledger stanza being updated.

CI gating (``--fail-on-new``)
-----------------------------
``--fail-on-new <base>..<head>`` turns the default filter mode into the CI gate.
On top of the unconditional stale / bucket-A non-zero exits, it parses
``git diff --unified=0 <base>..<head>`` for the new-file (``+``) line ranges each
hunk adds/changes, and exits non-zero if any REMAINING (non-ignored, non-stale)
survivor's reported ``(file, line)`` falls inside one of those ranges -- i.e. a
survivor that was *introduced by this change* and has no ledger entry. Survivors
in unchanged code never gate. When the range cannot be resolved (git missing or
an unknown / all-zeroes ref -- e.g. a first push / branch create), this gate is
skipped gracefully and only the stale / bucket-A gates apply; it is never an
error to pass an unresolvable range. The absolute mutation score is NEVER a gate.

Exit codes: 0 = clean; non-zero = a hard ledger error (bucket A / missing
fields), one or more stale entries, or (with ``--fail-on-new``) a survivor newly
introduced in the diff range.
"""

import argparse
import hashlib
import os
import re
import subprocess
import sys

# tomllib lands in the 3.11 stdlib; fall back to a tiny reader for this fixed,
# flat-table-array schema if running on an older interpreter.
try:
    import tomllib  # type: ignore
except ModuleNotFoundError:  # pragma: no cover - exercised only on <3.11
    tomllib = None


def load_toml(path):
    if tomllib is not None:
        with open(path, "rb") as f:
            return tomllib.load(f)
    return _minimal_toml(path)


# --------------------------------------------------------------------------
# Minimal TOML reader (fallback only). Handles the ledger's fixed shape:
#   [[ignore]] tables with string / integer / [int,int] string-list values.
# --------------------------------------------------------------------------
def _minimal_toml(path):
    data = {"ignore": []}
    cur = None
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if line == "[[ignore]]":
                cur = {}
                data["ignore"].append(cur)
                continue
            if "=" not in line or cur is None:
                continue
            key, _, val = line.partition("=")
            key = key.strip()
            val = val.split("#", 1)[0].strip()
            cur[key] = _toml_value(val)
    return data


def _toml_value(val):
    if val.startswith("[") and val.endswith("]"):
        inner = val[1:-1].strip()
        if not inner:
            return []
        parts = [p.strip() for p in inner.split(",")]
        out = []
        for p in parts:
            if (p.startswith('"') and p.endswith('"')) or (
                p.startswith("'") and p.endswith("'")
            ):
                out.append(p[1:-1])
            else:
                out.append(int(p))
        return out
    if (val.startswith('"') and val.endswith('"')) or (
        val.startswith("'") and val.endswith("'")
    ):
        return val[1:-1]
    if re.fullmatch(r"-?\d+", val):
        return int(val)
    return val


# --------------------------------------------------------------------------
# Region selection + hashing
# --------------------------------------------------------------------------
def normalise_line(line):
    """Strip trailing whitespace, then collapse all whitespace runs to one
    space after trimming the ends. A whitespace-only line becomes ""."""
    return re.sub(r"\s+", " ", line.strip())


def normalise_region(lines):
    return "\n".join(normalise_line(l) for l in lines)


def region_hash(lines):
    return "sha256:" + hashlib.sha256(
        normalise_region(lines).encode("utf-8")
    ).hexdigest()


def _read_source(root, rel):
    with open(os.path.join(root, rel), "r", encoding="utf-8") as f:
        # splitlines() drops the line terminators; normalisation is
        # terminator-agnostic anyway.
        return f.read().splitlines()


def select_region(root, entry):
    """Return the list of source lines this entry's selector covers, or raise
    ValueError if the selector cannot resolve in the current tree."""
    src = _read_source(root, entry["file"])

    if "function" in entry:
        return _select_function(src, entry["function"])
    if "lines" in entry:
        start, end = entry["lines"]
        if start < 1 or end > len(src) or start > end:
            raise ValueError(
                "lines=[%d,%d] out of range for %s (%d lines)"
                % (start, end, entry["file"], len(src))
            )
        return src[start - 1 : end]
    if "pattern" in entry:
        rx = re.compile(entry["pattern"])
        # SET of matching lines, ascending file order (line-number order).
        matches = [l for l in src if rx.search(l)]
        if not matches:
            raise ValueError(
                "pattern %r matched no line in %s"
                % (entry["pattern"], entry["file"])
            )
        return matches
    raise ValueError("entry %r has no selector" % entry.get("id"))


def _select_function(src, name):
    """Find a C function definition `name(` (not a call) and return the lines
    from its signature through the matching close brace."""
    sig = re.compile(r"\b" + re.escape(name) + r"\s*\(")
    start = None
    for i, line in enumerate(src):
        if not sig.search(line):
            continue
        stripped = line.lstrip()
        # A definition starts at column 0 (storage class / return type) and is
        # not a call (calls are indented inside a body, end in ';', or are
        # `= name(`). Require the line to be the start of a top-level decl.
        if stripped == line.lstrip() and (
            line[0] not in " \t"
        ) and not stripped.endswith(";"):
            start = i
            break
    if start is None:
        raise ValueError("function %r not found as a definition" % name)
    depth = 0
    seen_brace = False
    for j in range(start, len(src)):
        depth += src[j].count("{") - src[j].count("}")
        if "{" in src[j]:
            seen_brace = True
        if seen_brace and depth == 0:
            return src[start : j + 1]
    raise ValueError("unbalanced braces for function %r" % name)


# --------------------------------------------------------------------------
# Ledger validation
# --------------------------------------------------------------------------
SELECTORS = ("function", "lines", "pattern")


def validate_entry(entry):
    """Raise ValueError on any hard ledger error (caller turns it into a
    non-zero exit). Bucket A is never ignorable."""
    eid = entry.get("id", "<no id>")
    bucket = entry.get("bucket")
    if bucket is None:
        raise ValueError("entry %s: missing 'bucket'" % eid)
    if bucket == "A":
        raise ValueError(
            "entry %s: bucket=\"A\" is a contract gap and is NEVER ignorable "
            "-- fix it with a test, do not add it to the ledger" % eid
        )
    if bucket not in ("B", "C"):
        raise ValueError(
            "entry %s: bucket=%r invalid (only \"B\" or \"C\" allowed)"
            % (eid, bucket)
        )
    if not entry.get("rationale"):
        raise ValueError("entry %s: missing 'rationale'" % eid)
    if not entry.get("file"):
        raise ValueError("entry %s: missing 'file'" % eid)
    present = [s for s in SELECTORS if s in entry]
    if len(present) != 1:
        raise ValueError(
            "entry %s: must have exactly one selector among %s (found %s)"
            % (eid, ", ".join(SELECTORS), present or "none")
        )
    if "region_hash" not in entry:
        raise ValueError("entry %s: missing 'region_hash'" % eid)


# --------------------------------------------------------------------------
# Report parsing
# --------------------------------------------------------------------------
# /path/file.c:LINE:COL: warning: Survived: <desc> [<mutator>]
SURVIVOR_RE = re.compile(
    r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+):\s+warning:\s+Survived:\s+"
    r"(?P<desc>.*?)(?:\s+\[(?P<mutator>[A-Za-z0-9_]+)\])?\s*$"
)


class Survivor:
    __slots__ = ("file", "line", "col", "mutator", "block")

    def __init__(self, file, line, col, mutator, block):
        self.file = file
        self.line = line
        self.col = col
        self.mutator = mutator
        self.block = block  # list of raw report lines (the survivor + context)


def parse_report(text):
    """Yield (Survivor | None, raw_line). A Survivor's block is the warning
    line plus the following non-survivor, non-summary context lines (the
    source echo + caret) so the filter can drop the whole block."""
    lines = text.splitlines(keepends=True)
    out = []
    i = 0
    n = len(lines)
    while i < n:
        m = SURVIVOR_RE.match(lines[i].rstrip("\n"))
        if not m:
            out.append((None, lines[i]))
            i += 1
            continue
        block = [lines[i]]
        j = i + 1
        # Absorb the source-echo + caret context lines that mull prints under a
        # survivor warning, stopping at the next survivor or any [info]/[...]
        # summary line or a blank line that separates blocks.
        while j < n:
            nxt = lines[j].rstrip("\n")
            if SURVIVOR_RE.match(nxt) or nxt.startswith("["):
                break
            block.append(lines[j])
            j += 1
            if nxt.strip() == "":
                break
        sv = Survivor(
            os.path.normpath(m.group("file")),
            int(m.group("line")),
            int(m.group("col")),
            m.group("mutator"),
            block,
        )
        out.append((sv, None))
        i = j
    return out


# --------------------------------------------------------------------------
# Matching survivors to ledger entries
# --------------------------------------------------------------------------
class LedgerEntry:
    def __init__(self, raw, root):
        self.raw = raw
        self.id = raw.get("id", "<no id>")
        self.file = raw["file"]
        self.mutator = raw.get("mutator")
        self.bucket = raw.get("bucket")
        self.stored_hash = raw.get("region_hash")
        self.root = root
        self.stale = False
        self.stale_reason = ""
        self.region_lines = None       # 1-based line numbers covered
        self.current_hash = None

    def resolve(self):
        """Compute current region + hash; mark stale on mismatch or missing
        region. Returns True if the entry is LIVE (usable for suppression)."""
        try:
            lines = select_region(self.root, self.raw)
        except (ValueError, FileNotFoundError, OSError) as exc:
            self.stale = True
            self.stale_reason = "region unresolved: %s" % exc
            return False
        self.current_hash = region_hash(lines)
        self.region_lines = self._line_numbers(lines)
        if self.current_hash != self.stored_hash:
            self.stale = True
            self.stale_reason = "region_hash mismatch (stored %s, current %s)" % (
                self.stored_hash,
                self.current_hash,
            )
            return False
        return True

    def _line_numbers(self, region_lines):
        """Map the selected region back to 1-based line numbers in the file so
        a survivor's reported line can be matched. For function/lines the region
        is contiguous; for pattern it is the set of matching line numbers."""
        src = _read_source(self.root, self.file)
        if "lines" in self.raw:
            start, end = self.raw["lines"]
            return set(range(start, end + 1))
        if "function" in self.raw:
            # contiguous: locate the first region line in the source.
            target = region_lines
            for i in range(len(src) - len(target) + 1):
                if src[i : i + len(target)] == target:
                    return set(range(i + 1, i + 1 + len(target)))
            return set()
        if "pattern" in self.raw:
            rx = re.compile(self.raw["pattern"])
            return {i + 1 for i, l in enumerate(src) if rx.search(l)}
        return set()

    def matches(self, sv):
        """Does this LIVE entry cover the given survivor?"""
        if self.stale:
            return False
        if os.path.basename(sv.file) != os.path.basename(self.file):
            # tolerate abs-vs-rel path differences; compare by basename and
            # require the relative file to be a suffix of the reported path.
            return False
        if not (sv.file == self.file or sv.file.endswith(self.file)
                or sv.file.endswith(os.path.basename(self.file))):
            return False
        if self.mutator and self.mutator != "*":
            pat = "^" + re.escape(self.mutator).replace(r"\*", ".*") + "$"
            if sv.mutator is None or not re.match(pat, sv.mutator):
                return False
        return sv.line in (self.region_lines or set())


# --------------------------------------------------------------------------
# git fast-path
# --------------------------------------------------------------------------
def _split_range(rng):
    """Split "<base>..<head>" into (base, head). A bare ref (no "..") is
    treated as base with head defaulting to HEAD."""
    if ".." in rng:
        base, _, head = rng.partition("..")
        return base, (head or "HEAD")
    return rng, "HEAD"


def _git_run(root, args):
    """Run `git -C root <args>` and return stdout, or None if git is missing
    or the command fails (e.g. an unknown ref)."""
    try:
        return subprocess.run(
            ["git", "-C", root] + args,
            capture_output=True, text=True, check=True,
        ).stdout
    except (subprocess.CalledProcessError, FileNotFoundError):
        return None


def changed_files(root, rng):
    base, head = _split_range(rng)
    out = _git_run(root, ["diff", "--name-only", "%s..%s" % (base, head)])
    if out is None:
        return None  # unknown -> caller must hash everything
    return {line.strip() for line in out.splitlines() if line.strip()}


# Hunk header: @@ -a[,b] +c[,d] @@ -- the "+c,d" side names the new-file lines
# that were added/changed (d defaults to 1 when omitted; d==0 = pure deletion).
HUNK_RE = re.compile(r"^@@ -\d+(?:,\d+)? \+(?P<start>\d+)(?:,(?P<count>\d+))? @@")


def parse_diff_added_ranges(diff_text):
    """Parse a `git diff --unified=0` body into {new_file_path: set(line nums)}
    -- the line numbers ADDED or CHANGED on the new-file (+) side of each hunk.
    Pure deletions (d==0) contribute no new-file lines."""
    added = {}
    cur = None
    for raw in diff_text.splitlines():
        if raw.startswith("+++ "):
            path = raw[4:].strip()
            # strip the conventional "b/" prefix; "/dev/null" -> no file
            if path == "/dev/null":
                cur = None
            else:
                if path.startswith("b/"):
                    path = path[2:]
                cur = os.path.normpath(path)
                added.setdefault(cur, set())
            continue
        m = HUNK_RE.match(raw)
        if not m or cur is None:
            continue
        start = int(m.group("start"))
        count = 1 if m.group("count") is None else int(m.group("count"))
        for ln in range(start, start + count):
            added[cur].add(ln)
    return added


def diff_added_ranges(root, rng):
    """Return {new_file: set(added line nums)} for <base>..<head>, or None if
    git is unavailable / the range cannot be resolved."""
    base, head = _split_range(rng)
    out = _git_run(root, ["diff", "--unified=0", "%s..%s" % (base, head)])
    if out is None:
        return None
    return parse_diff_added_ranges(out)


def _survivor_in_added(sv, added):
    """True iff survivor sv's (file, line) lies in a +range of `added`. Paths
    are matched by suffix to tolerate abs-vs-rel report paths."""
    sv_norm = os.path.normpath(sv.file)
    sv_base = os.path.basename(sv_norm)
    for path, lines in added.items():
        if sv_norm == path or sv_norm.endswith(path) or path.endswith(sv_norm) \
                or os.path.basename(path) == sv_base and sv_norm.endswith(os.path.basename(path)):
            if sv.line in lines:
                return True
    return False


# --------------------------------------------------------------------------
# Driver
# --------------------------------------------------------------------------
def load_ledger(path, root):
    doc = load_toml(path)
    raw_entries = doc.get("ignore", [])
    entries = []
    for raw in raw_entries:
        validate_entry(raw)  # raises -> hard error
        entries.append(LedgerEntry(raw, root))
    return entries


def run_filter(args):
    entries = load_ledger(args.ledger, args.root)

    live = []
    stale = []
    for e in entries:
        if e.resolve():
            live.append(e)
        else:
            stale.append(e)

    inputs = []
    for rp in args.report:
        with open(rp, "r", encoding="utf-8") as f:
            inputs.append((rp, f.read()))

    total_survivors = 0
    ignored_by_id = {}
    kept_lines = []
    remaining_survivors = []  # non-ignored, non-stale survivors still in report

    for rp, text in inputs:
        for sv, raw in parse_report(text):
            if raw is not None:
                kept_lines.append(raw)
                continue
            total_survivors += 1
            hit = next((e for e in live if e.matches(sv)), None)
            if hit is not None:
                ignored_by_id.setdefault(hit.id, 0)
                ignored_by_id[hit.id] += 1
                # drop the whole block (suppressed)
            else:
                kept_lines.extend(sv.block)
                remaining_survivors.append(sv)

    out_path = args.out or os.path.join(
        os.path.dirname(os.path.abspath(inputs[0][0])) if inputs else ".",
        "report.filtered.txt",
    )
    with open(out_path, "w", encoding="utf-8") as f:
        f.writelines(kept_lines)

    ignored_total = sum(ignored_by_id.values())
    remaining = total_survivors - ignored_total

    # --fail-on-new gate (b): of the survivors still in the filtered report,
    # which sit inside a line region added/changed by the diff range? Those are
    # NEW survivors and gate the job. Pre-existing survivors in unchanged code
    # do not. When no range is given (schedule / dispatch -- no diff baseline)
    # this gate is skipped entirely; only stale/bucket-A gate then.
    new_survivors = []
    diff_unavailable = False
    if args.fail_on_new:
        added = diff_added_ranges(args.root, args.fail_on_new)
        if added is None:
            diff_unavailable = True
        else:
            new_survivors = [
                sv for sv in remaining_survivors if _survivor_in_added(sv, added)
            ]

    print("=== mull-filter summary ===")
    print("filtered report : %s" % out_path)
    print("total survivors : %d" % total_survivors)
    print("ignored         : %d" % ignored_total)
    for eid in sorted(ignored_by_id):
        print("    %-24s %d" % (eid, ignored_by_id[eid]))
    print("remaining       : %d" % remaining)
    print("stale ignores   : %d" % len(stale))
    for e in stale:
        print("    !! STALE %-20s %s [%s]" % (e.id, e.stale_reason, e.file))

    if args.fail_on_new:
        if diff_unavailable:
            print(
                "new-in-diff gate: SKIPPED (could not resolve diff range %s -- "
                "git missing or unknown ref); gating on stale/bucket-A only."
                % args.fail_on_new
            )
        else:
            print("new-in-diff range : %s" % args.fail_on_new)
            print("NEW survivors     : %d" % len(new_survivors))
            for sv in new_survivors:
                print(
                    "    !! NEW   %s:%d:%d [%s] -- survivor introduced in this "
                    "change with no ledger entry; triage it (add a test for "
                    "bucket A, or ledger it for B/C)"
                    % (sv.file, sv.line, sv.col, sv.mutator or "?")
                )

    rc = 0
    if stale:
        print(
            "error: %d stale ledger entr%s -- suppressed nothing; update the "
            "ledger stanza(s) above (recompute region_hash) and re-run."
            % (len(stale), "y" if len(stale) == 1 else "ies"),
            file=sys.stderr,
        )
        rc = 1
    if new_survivors:
        print(
            "error: %d survivor(s) newly introduced in %s and not ledgered -- "
            "see the NEW lines above. The mutation gate fails on survivors that "
            "appear in changed code."
            % (len(new_survivors), args.fail_on_new),
            file=sys.stderr,
        )
        rc = 1
    return rc


def run_check_range(args):
    """Enforcement hook: fail if a covered region moved in <base>..<head>
    without its ledger stanza tracking it. Implemented as stale-detection
    over (optionally) the changed-files subset."""
    entries = load_ledger(args.ledger, args.root)
    touched = changed_files(args.root, args.check_range)

    stale = []
    checked = 0
    for e in entries:
        # Fast-path: if we know the changed-file set and this entry's file is
        # untouched, its region cannot have moved -> skip hashing it. The hash
        # remains the source of truth for files we do check.
        if touched is not None and e.file not in touched and not any(
            cf.endswith(e.file) or e.file.endswith(cf) for cf in touched
        ):
            continue
        checked += 1
        if not e.resolve():
            stale.append(e)

    print("=== mull-filter --check-range %s ===" % args.check_range)
    print("entries checked : %d / %d" % (checked, len(entries)))
    print("stale           : %d" % len(stale))
    for e in stale:
        print("    !! STALE %-20s %s [%s]" % (e.id, e.stale_reason, e.file))
    if stale:
        print(
            "error: %d ledger-covered region(s) changed in %s without the "
            "ledger stanza being updated. Re-triage and update region_hash."
            % (len(stale), args.check_range),
            file=sys.stderr,
        )
        return 1
    return 0


def main(argv=None):
    p = argparse.ArgumentParser(description="Filter accepted mull survivors.")
    default_ledger = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
        "Conventions", "mull-ignores.toml",
    )
    p.add_argument("report", nargs="*", help="mull-runner report file(s)")
    p.add_argument("--ledger", default=default_ledger,
                   help="path to mull-ignores.toml")
    p.add_argument("--root", default=".",
                   help="repo root the ledger file paths are relative to")
    p.add_argument("--out", default=None,
                   help="filtered report path (default: report.filtered.txt)")
    p.add_argument("--check-range", default=None, metavar="BASE..HEAD",
                   help="enforcement-hook mode: stale-detect over a commit range")
    p.add_argument("--fail-on-new", default=None, metavar="BASE..HEAD",
                   help="gate: in the default filter mode, also exit non-zero if "
                        "a remaining (non-ignored) survivor lies in a line region "
                        "added/changed by BASE..HEAD. An unresolvable range "
                        "(missing git / unknown ref) downgrades to stale-only "
                        "gating instead of erroring.")
    args = p.parse_args(argv)

    try:
        if args.check_range:
            return run_check_range(args)
        if not args.report:
            p.error("a report file is required unless --check-range is given")
        return run_filter(args)
    except ValueError as exc:
        # hard ledger error (bucket A, missing fields, bad selector count)
        print("error: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
