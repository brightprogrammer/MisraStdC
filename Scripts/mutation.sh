#!/usr/bin/env bash

# Mutation testing for MisraStdC using Mull.
#
# For each component, compiles the project with clang + the mull-ir-frontend
# pass plugin scoped (via a generated MULL_CONFIG) to that component's source,
# then runs the component's test suites under mull-runner. The score measures
# how well a component's suites kill mutations in that component's
# implementation. Scoping per component keeps each run tractable (only that
# file is mutated) and gives a meaningful per-component number.
#
# Portable across environments:
#   - Local: `nix develop .#mutation` exports CC / MULL_IR_FRONTEND / MULL_RUNNER.
#   - CI (Ubuntu): clang + mull come from apt; the plugin is found under /usr/lib.
#
# Usage:
#   ./Scripts/mutation.sh [Component ...]
# With no args it runs the default component set below. A component is a test
# prefix (Map, Vec, Str, ...) whose source is Source/.../Container/<C>.c or
# Source/Misra/Std/<C>.c, and whose suites are the meson tests named "<C>.*".
#
# Environment (all optional):
#   MULL_LLVM_VERSION  LLVM major version (default: 19)
#   CC                 clang to compile with (default: clang-<v>)
#   MULL_IR_FRONTEND   path to the mull-ir-frontend plugin
#   MULL_RUNNER        mull runner binary (default: mull-runner-<v>)
#   MIN_SCORE          fail if any suite scores below this percent (default: 0)
#   MUTATION_SANITIZE  meson b_sanitize value (default: none)

set -euo pipefail

cd "$(dirname "$0")/.."

LLVMV=${MULL_LLVM_VERSION:-19}
ROOT=build_mull
CC=${CC:-clang-$LLVMV}
MULL_RUNNER=${MULL_RUNNER:-mull-runner-$LLVMV}
MIN_SCORE=${MIN_SCORE:-0}
SAN=${MUTATION_SANITIZE:-none}

COMPONENTS=("$@")
if [ ${#COMPONENTS[@]} -eq 0 ]; then
  COMPONENTS=(Map Vec Str List BitVec Graph Int Float)
fi

# Locate the pass plugin if the environment did not hand it to us.
if [ -z "${MULL_IR_FRONTEND:-}" ]; then
  for cand in "/usr/lib/mull-ir-frontend-$LLVMV" "/usr/lib/llvm-$LLVMV/lib/mull-ir-frontend-$LLVMV"; do
    [ -f "$cand" ] && MULL_IR_FRONTEND="$cand" && break
  done
fi
if [ -z "${MULL_IR_FRONTEND:-}" ] || [ ! -f "$MULL_IR_FRONTEND" ]; then
  echo "error: mull-ir-frontend-$LLVMV not found. Set MULL_IR_FRONTEND, run inside" >&2
  echo "       'nix develop .#mutation', or apt-install mull-$LLVMV." >&2
  exit 1
fi

echo "==> compiler    : $CC"
echo "==> mull plugin : $MULL_IR_FRONTEND"
echo "==> runner      : $MULL_RUNNER"
echo "==> components  : ${COMPONENTS[*]}"
echo

# Resolve a component name to its implementation source file.
component_source() {
  local c=$1 p
  for p in "Source/Misra/Std/Container/$c.c" "Source/Misra/Std/$c.c"; do
    [ -f "$p" ] && { echo "$p"; return 0; }
  done
  return 1
}

overall_fail=0
for comp in "${COMPONENTS[@]}"; do
  src=$(component_source "$comp") || { echo "::warning::no source for component '$comp', skipping"; continue; }
  bdir="$ROOT/$comp"
  cfg="$bdir/mull.yml"
  mkdir -p "$bdir"

  # Scope mutations to this component's source for both compile and run.
  printf 'includePaths:\n  - %s\nmutators:\n  - cxx_all\n' "$src" > "$cfg"
  export MULL_CONFIG="$PWD/$cfg"

  if [ ! -f "$bdir/build.ninja" ]; then
    CC="$CC" meson setup "$bdir" \
      -Db_sanitize="$SAN" \
      -Db_lundef=false \
      -Dc_args="-g -O0 -grecord-command-line -fno-discard-value-names -fpass-plugin=$MULL_IR_FRONTEND"
  fi

  # Suites for this component: meson tests named "<comp>.*".
  mapfile -t suites < <(meson test -C "$bdir" --list 2>/dev/null | grep -oE "(^|[^A-Za-z0-9_])$comp\.[A-Za-z0-9_]+" | grep -oE "$comp\.[A-Za-z0-9_]+" | sort -u)
  if [ ${#suites[@]} -eq 0 ]; then
    echo "::warning::no suites found for component '$comp', skipping"
    continue
  fi

  echo "==================== $comp  ($src)  [${#suites[@]} suites] ===================="
  for suite in "${suites[@]}"; do
    echo "-------------------- $suite --------------------"
    ninja -C "$bdir" "Tests/$suite"
    bin="$bdir/Tests/$suite"
    report="$bdir/mutation-$suite.txt"

    if ! "$bin" >/dev/null 2>&1; then
      echo "::warning::$suite baseline FAILS unmutated -- skipping mutation run"
      echo "$suite: baseline failed" > "$report"
      continue
    fi

    # Mutators + includePaths come from MULL_CONFIG at COMPILE time; the runner
    # only discovers the embedded mutants. mull-runner exits nonzero when any
    # mutant survives -- expected, so don't let it trip `set -e`.
    set +e
    "$MULL_RUNNER" --workers "$(nproc)" "$bin" 2>&1 | tee "$report"
    set -e

    score=$(grep -oiE 'mutation score: *[0-9]+' "$report" | grep -oE '[0-9]+' | tail -1 || true)
    if [ -n "$score" ] && [ "$score" -lt "$MIN_SCORE" ]; then
      echo "::error::$suite mutation score ${score}% < required ${MIN_SCORE}%"
      overall_fail=1
    fi
    echo
  done
done

exit $overall_fail
