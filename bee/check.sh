#!/bin/sh
# The project's quick check of ONE unit, for who writes it: the judge's own scripts and drivers, on cases drawn now.
#   sh bee/check.sh <unit>          INSIDE THE SANDBOX ONLY (AGENTS.md says how): this builds and runs project code
# It copies the reference, builds the generator, then the unit's driver three times (three nonces); for each build it
# draws the manifest's count of cases anew and compares the unit's answers with the reference's, file against file.
# What the judge does that this does not: it draws its own cases when it judges; it runs each build twice, on the
# cases in two orders of its own; it bounds each build and each run in time and memory (bee/reference/project.json).
set -eu
unit="${1:?usage: sh bee/check.sh <unit>   (a unit of bee/reference/project.json: units/UNITS.md names them)}"
[ -f "units/$unit.c" ] || { echo "check: no unit units/$unit.c" >&2; exit 2; }
for tool in cc clang-18 wasm-ld-18 wasm-objdump wasm-validate wasm2c; do
    command -v "$tool" >/dev/null || { echo "check: no $tool inside this sandbox: use the sandbox's image (AGENTS.md)" >&2; exit 3; }
done
count=$(awk -v unit="\"$unit\":" '$1 == unit { found = 1 } found && $1 == "\"cases\":" { gsub(/[^0-9]/, "", $2); print $2; exit }' bee/reference/project.json)
[ -n "$count" ] || { echo "check: the manifest names no unit $unit" >&2; exit 2; }
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
mkdir "$work/reference"
BEE_SRC="$PWD" BEE_OUT="$work/reference" sh bee/reference/build-reference
for build in 1 2 3; do
    seed=$(od -An -N32 -tx1 /dev/urandom | tr -d ' \n')
    printf '%s\n' "$seed" | timeout 60 "$work/reference/generate" "$unit" "$count" "$work/cases$build"
    timeout 60 "$work/reference/reference" "$unit" "$work/cases$build" "$work/expected$build"
    mkdir "$work/unit$build"
    od -An -N32 -tx1 /dev/urandom | tr -d ' \n' | BEE_SRC="$PWD" BEE_OUT="$work/unit$build" sh bee/reference/build-unit "$unit"
    timeout 60 "$work/unit$build/driver" "$work/cases$build" "$work/answers$build" || { echo "check: the driver of $unit failed (build $build, seed $seed)" >&2; exit 1; }
    cmp "$work/expected$build" "$work/answers$build" || { echo "check: $unit does not answer as the reference (build $build, seed $seed)" >&2; exit 1; }
done
echo "check: $unit answers as the reference on $count cases, drawn anew for each of three builds"
