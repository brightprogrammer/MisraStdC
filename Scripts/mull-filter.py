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

True survivors (the gate's unit of accounting)
-----------------------------------------------
A mutant is *killed* if **any** suite kills it. ``Scripts/mutation.sh`` mutates
one source file per component and runs only that component's suites against it,
so a given source file's mutants appear ONLY in that component's reports. The
true survivor set is therefore the per-source-file **intersection** across the
reports that mutate that file, deduped:

  * A mutant is keyed by ``(file, line, col, mutator)``.
  * Each report contributes its set of survived mutant-keys AND the set of
    source files it mutates ("mutated-file-set").
  * A mutant ``m`` in file ``F`` is a TRUE survivor iff ``m`` is in the survived
    set of EVERY report whose mutated-file-set contains ``F``. If any such report
    does not list ``m`` as survived, some suite killed it.
  * Each true survivor is counted ONCE regardless of how many reports list it.

A per-suite **union over-reports** (it double-counts cross-suite duplicates and
re-lists mutants other suites already killed) and is NEVER used as the gate.

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
                   REPORT [REPORT ...]
    mull-filter.py --gate [--ledger PATH] [--root DIR] REPORT [REPORT ...]
    mull-filter.py --check-range <base>..<head> [--ledger PATH] [--root DIR]

In the default mode it reads one or more report files, computes the deduped,
per-file-intersected TRUE survivor set (see above), suppresses the ledgered
ones, writes the filtered report to ``--out`` (default ``report.filtered.txt``
next to the first input), and prints a summary block (total true survivors /
ignored by id / remaining / stale).

``--check-range`` is the enforcement-hook mode: it runs only the
stale-detection (no report needed) over the entries whose files changed in
``<base>..<head>`` and exits non-zero if any covered region moved without its
ledger stanza being updated.

CI / pre-push gate (``--gate``)
-------------------------------
``--gate`` is the convergence gate from MULL-DISCOVERY-CONVENTIONS. It does
everything the default mode does, then EXITS NON-ZERO if ANY true survivor
remains after ledger suppression, OR any ledger entry is stale, OR any
bucket-A / malformed ledger entry exists. On a clean fully-converged report
(``remaining == 0``, no stale, no bad entry) it exits 0. The remaining true
survivors are printed grouped by ``file:line:col [mutator]`` so a human can
triage them. The absolute mutation score is NEVER a gate.

Exit codes: 0 = clean; 1 = one or more stale entries or (with ``--gate``) any
remaining true survivor; 2 = a hard ledger error (bucket A / missing fields).
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

    @property
    def key(self):
        """Identity of the mutant: (file, line, col, mutator). Two reports that
        mutate the same source line+col with the same mutator describe the *same*
        mutant, so this is the unit deduped + intersected across reports."""
        return (self.file, self.line, self.col, self.mutator)


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


def compute_true_survivors(parsed_reports):
    """Reduce the per-report parse results to the deduped, per-file-intersected
    TRUE survivor set.

    ``parsed_reports`` is a list of ``parse_report`` outputs (one per report
    file). For each report we derive:

      * ``survived``   -- the set of mutant keys it lists as Survived;
      * ``mutated``    -- the set of source files it mutates (the files those
                          survived keys belong to).

    A mutant ``m`` in file ``F`` is a true survivor iff it is in the ``survived``
    set of EVERY report whose ``mutated`` set contains ``F`` (if any such report
    omits it, that suite killed it). One representative ``Survivor`` object is
    kept per true-survivor key so the filtered report can echo it exactly once.

    Returns ``(true_survivors, reports_meta)`` where ``true_survivors`` is a list
    of ``Survivor`` (deduped, one per key, in first-seen order) and
    ``reports_meta`` is the list of per-report ``(survived_keys, mutated_files)``
    used (exposed for testing)."""
    reports_meta = []
    first_seen = {}   # key -> Survivor (representative, first occurrence)
    order = []        # keys in first-seen order
    for parsed in parsed_reports:
        survived = set()
        mutated = set()
        for sv, _ in parsed:
            if sv is None:
                continue
            survived.add(sv.key)
            mutated.add(sv.file)
            if sv.key not in first_seen:
                first_seen[sv.key] = sv
                order.append(sv.key)
        reports_meta.append((survived, mutated))

    true_keys = []
    for key in order:
        f = key[0]
        relevant = [survived for survived, mutated in reports_meta if f in mutated]
        # `relevant` is non-empty by construction (the report this key came from
        # mutates f). True survivor iff every relevant report still lists it.
        if relevant and all(key in survived for survived in relevant):
            true_keys.append(key)

    true_survivors = [first_seen[k] for k in true_keys]
    return true_survivors, reports_meta


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
    """Default + ``--gate`` mode.

    Computes the deduped, per-file-intersected TRUE survivor set across the
    reports, suppresses ledgered survivors, writes the filtered report, and
    prints the summary. Returns an exit code:

      * 2 is raised earlier (hard ledger error) by ``load_ledger``;
      * 1 if any ledger entry is stale, or -- under ``--gate`` -- if any true
        survivor remains after suppression;
      * 0 otherwise.

    Under ``--gate`` the remaining true survivors are also printed grouped by
    ``file:line:col [mutator]`` for triage."""
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

    parsed_reports = [parse_report(text) for _, text in inputs]

    # The gate's unit of accounting: the deduped, per-source-file intersection
    # across the reports. A union over the per-suite warning lines over-reports
    # (it double-counts cross-suite duplicates and re-lists mutants other suites
    # killed), so it must NOT drive the summary or the gate.
    true_survivors, _ = compute_true_survivors(parsed_reports)

    total_survivors = len(true_survivors)
    ignored_by_id = {}
    remaining_survivors = []  # true survivors not covered by a LIVE ledger entry

    for sv in true_survivors:
        hit = next((e for e in live if e.matches(sv)), None)
        if hit is not None:
            ignored_by_id[hit.id] = ignored_by_id.get(hit.id, 0) + 1
        else:
            remaining_survivors.append(sv)

    # The filtered report lists each REMAINING true survivor exactly once.
    kept_lines = []
    for sv in remaining_survivors:
        kept_lines.extend(sv.block)

    out_path = args.out or os.path.join(
        os.path.dirname(os.path.abspath(inputs[0][0])) if inputs else ".",
        "report.filtered.txt",
    )
    with open(out_path, "w", encoding="utf-8") as f:
        f.writelines(kept_lines)

    ignored_total = sum(ignored_by_id.values())
    remaining = len(remaining_survivors)

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

    if args.gate:
        print("gate            : converge-to-zero (remaining must be 0)")
        if remaining_survivors:
            print("remaining true survivors (triage each):")
            for sv in sorted(remaining_survivors,
                             key=lambda s: (s.file, s.line, s.col,
                                            s.mutator or "")):
                print("    %s:%d:%d [%s]"
                      % (sv.file, sv.line, sv.col, sv.mutator or "?"))

    rc = 0
    if stale:
        print(
            "error: %d stale ledger entr%s -- suppressed nothing; update the "
            "ledger stanza(s) above (recompute region_hash) and re-run."
            % (len(stale), "y" if len(stale) == 1 else "ies"),
            file=sys.stderr,
        )
        rc = 1
    if args.gate and remaining_survivors:
        print(
            "error: %d true survivor(s) remain after ledger filtering -- the "
            "mutation report has NOT converged to zero. Each must be killed by a "
            "contract test (bucket A) or signed off into the ledger (bucket "
            "B/C). See MULL-DISCOVERY-CONVENTIONS.md (the pre-push invariant)."
            % remaining,
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
    p.add_argument("--gate", action="store_true",
                   help="CI / pre-push convergence gate: suppress ledgered "
                        "survivors, then exit non-zero if ANY true survivor "
                        "remains, any ledger entry is stale, or any bucket-A / "
                        "malformed entry exists. remaining==0 with no stale and "
                        "no bad entry exits 0.")
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
