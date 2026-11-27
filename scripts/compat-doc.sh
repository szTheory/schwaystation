#!/bin/sh
set -eu
if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    printf '%s\n' 'usage: compat-doc.sh ledger.csv output.md [--check]' >&2
    exit 2
fi
ledger=$1
output=$2
mode=${3:-write}
case "$mode" in
    write|--check) ;;
    *) printf '%s\n' 'compatibility: third argument must be --check' >&2
        exit 2
        ;;
esac
prefix=${TMPDIR:-/tmp}/compat-doc-$$
trap 'rm -f "$prefix".*' EXIT HUP INT TERM
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
features="$root/tests/compat/features.csv"
today=${COMPAT_TODAY:-$(date -u +%F)}
if ! sh "$root/scripts/compat-ratchet.sh" "$ledger" "$ledger" '' \
    "$root/tests/compat/allow.txt" "$today" >/dev/null ||
   ! sh "$root/scripts/compat-ratchet.sh" "$features" "$features" '' \
    "$root/tests/compat/allow.txt" "$today" features >/dev/null; then
    printf '%s\n' 'compatibility: cannot generate docs from an invalid compatibility ledger' >&2
    exit 1
fi
matrix="$root/tests/compat/required-capabilities.csv"
evaluator="$root/tests/cmake/Capabilities.cmake"
results=/dev/null
if [ -n "${COMPAT_CAPABILITIES_BUILD:-}" ]; then
    build=$COMPAT_CAPABILITIES_BUILD
    results="$prefix.results"
    cmake "-DCAPABILITIES_MATRIX=$matrix" "-DCAPABILITIES_SOURCE_ROOT=$root" \
        "-DCAPABILITIES_CONFIG=$build/compat/capabilities-config.csv" "-DCAPABILITIES_JUNIT=$build/junit.xml" \
        "-DCAPABILITIES_OUTPUT=$results" -P "$evaluator"
else
    cmake "-DCAPABILITIES_VALIDATE_MATRIX=$matrix" -P "$evaluator" >/dev/null
fi
awk -F, 'NR > 1 { print $8 "\t" $5 "\t" $6 }' "$ledger" |
    LC_ALL=C sort |
    uniq -c |
    awk '{ printf "%s\t%s\t%s\t%s\n", $2, $3, $4, $1 }' > "$prefix.counts"
awk -F, 'NR > 1 {
    confidence = ($9 ~ /^fixtures\./ || $9 ~ /^built\./) ? "measured fixture" : "source unverified"
    printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n", $4, $1, $8, $7, confidence, $9, $10
}' "$ledger" |
    LC_ALL=C sort -t "$(printf '\t')" -k1,1 -k2,2 > "$prefix.titles"
awk -F, 'NR > 1 { printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", $1, $2, $3, $4, $5, $6, $7, $8 }' "$features" |
    LC_ALL=C sort -t "$(printf '\t')" -k1,1 -k2,2 -k5,5 > "$prefix.features"
{
    printf '%s\n\n' '# Compatibility results' 'Generated from the compatibility ledgers, pinned DAT summary and required capability matrix.'
    printf '%s\n' '## Redump inventory' '' '| Entries | Pass | Fail | Unknown | Missing serial | Duplicate-serial entries |' '| ---: | ---: | ---: | ---: | ---: | ---: |'
    awk -F ': ' '
        { sub(/\r$/, ""); values[$1] = $2 }
        END {
            if (values["Entries"] <= 0 || values["Entries"] != values["Pass"] + values["Fail"] + values["Unknown"] || !values["Source URL"]) exit 1
            printf "| %d | %d | %d | %d | %d | %d |\n", values["Entries"], values["Pass"], values["Fail"], values["Unknown"], values["Missing serial"], values["Duplicate-serial entries"]
            printf "\n[Source at the pinned revision](%s), CC BY-SA 4.0. [Reproduce the inventory](compatibility-inventory.md). Inventory counts describe the source entries.\n", values["Source URL"]
        }' "$root/tests/compat/dat-summary.txt"
    printf '%s\n' '' '## Required capabilities' '' 'Libretro stays experimental while any required result is missing or fails. Provider and profile rows require boot, input-driven control, changing picture and audible output. CHD is required when the core includes it. The checked-in summary records unknowns until it has current build evidence.' '' '| Category | Pass | Fail | Unknown |' '| --- | ---: | ---: | ---: |'
    awk -F, 'FILENAME==ARGV[1] { if (FNR > 1) status[$1] = $6; next }
        FNR > 1 { result = status[$1]; count[$2, result == "pass" || result == "fail" ? result : "unknown"]++; groups[$2] = 1 }
        END { for (group in groups) printf "| %s | %d | %d | %d |\n", group, count[group,"pass"], count[group,"fail"], count[group,"unknown"] }' "$results" "$matrix" | LC_ALL=C sort
    printf '\n'
    printf '%s\n' '## Counts' '' '| Status | Region | Media | Rows |' '| --- | --- | --- | ---: |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s |\n", $1, $2, $3, $4 }' "$prefix.counts"
    printf '%s\n' '' '## Results' '' '| Title | Serial | Status | Predicate | Confidence | Evidence | Date |' '| --- | --- | --- | --- | --- | --- | --- |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s | %s | %s | %s |\n", $1, $2, $3, $4, $5, $6, $7 }' "$prefix.titles"
    printf '%s\n' '' '## Historical feature evidence' '' 'Prior passes cannot regress or disappear without a dated allowance. Each row describes the named predicate and source. Profile selection and protocol checks leave boot-to-play unknown. See [patches](patches.md) and [cheats](cheats.md) for supported subsets.'
    printf '%s\n' '' '| Provider | Profile | Protection | Peripheral | Predicate | Status | Evidence | Date |' '| --- | --- | --- | --- | --- | --- | --- | --- |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s | %s | %s | %s | %s |\n", $1, $2, $3, $4, $5, $6, $7, $8 }' "$prefix.features"
} > "$prefix.generated"
if [ "$mode" = write ]; then
    cp "$prefix.generated" "$output"
elif ! cmp -s "$prefix.generated" "$output"; then
    printf '%s\n' 'compatibility: generated documentation is stale' >&2
    exit 1
fi
