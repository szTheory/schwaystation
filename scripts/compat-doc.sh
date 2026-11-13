#!/bin/sh
set -eu

if [ "$#" -lt 2 ] || [ "$#" -gt 3 ]; then
    printf '%s\n' 'usage: compat-doc.sh ledger.csv output.md [--check]' >&2
    exit 2
fi
ledger=$1
output=$2
mode=${3:-write}
if [ "$mode" != write ] && [ "$mode" != --check ]; then
    printf '%s\n' 'compatibility: third argument must be --check' >&2
    exit 2
fi
prefix=${TMPDIR:-/tmp}/compat-doc-$$
trap 'rm -f "$prefix.counts" "$prefix.titles" "$prefix.generated"' EXIT HUP INT TERM
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
today=$(awk -F, 'NR > 1 { date = $10 } END { print date }' "$ledger")
if [ -z "$today" ] || ! sh "$root/scripts/compat-ratchet.sh" \
    "$ledger" "$ledger" '' "$root/tests/compat/allow.txt" "$today" >/dev/null; then
    printf '%s\n' 'compatibility: cannot generate docs from an invalid ledger' >&2
    exit 1
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
{
    printf '%s\n\n' '# Compatibility results' 'Generated from `tests/compat/ledger.csv`.'
    printf '%s\n' '## Counts' '' '| Status | Region | Media | Rows |' '| --- | --- | --- | ---: |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s |\n", $1, $2, $3, $4 }' "$prefix.counts"
    printf '%s\n' '' '## Results' '' '| Title | Serial | Status | Predicate | Confidence | Evidence | Date |' '| --- | --- | --- | --- | --- | --- | --- |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s | %s | %s | %s |\n", $1, $2, $3, $4, $5, $6, $7 }' "$prefix.titles"
} > "$prefix.generated"
if [ "$mode" = --check ]; then
    if ! cmp -s "$prefix.generated" "$output"; then
        printf '%s\n' 'compatibility: generated documentation is stale' >&2
        exit 1
    fi
else
    cp "$prefix.generated" "$output"
fi
