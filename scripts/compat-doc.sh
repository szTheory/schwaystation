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
    printf '%s\n\n' '# Compatibility results' 'Generated from `tests/compat/ledger.csv` and `tests/compat/features.csv`.'
    printf '%s\n' '## Counts' '' '| Status | Region | Media | Rows |' '| --- | --- | --- | ---: |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s |\n", $1, $2, $3, $4 }' "$prefix.counts"
    printf '%s\n' '' '## Results' '' '| Title | Serial | Status | Predicate | Confidence | Evidence | Date |' '| --- | --- | --- | --- | --- | --- | --- |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s | %s | %s | %s |\n", $1, $2, $3, $4, $5, $6, $7 }' "$prefix.titles"
    printf '%s\n' '' '## Feature evidence' '' 'The key is provider, profile, protection, peripheral and predicate. Every field is required; unknown is valid until fresh evidence exists. A pass must match the fresh result status and evidence reference. Prior passes cannot regress or disappear without a matching dated allowance.' 'Phase 10 onward adds provider/profile/protection/peripheral rows as each capability lands. Feature allowances use `<date> <provider> <profile> <protection>!<peripheral>!<predicate> behavior-rev=4 expires=<date> <reason>` and may cover at most 60 days.'
    printf '%s\n' '' '| Provider | Profile | Protection | Peripheral | Predicate | Status | Evidence | Date |' '| --- | --- | --- | --- | --- | --- | --- | --- |'
    awk -F '\t' '{ printf "| %s | %s | %s | %s | %s | %s | %s | %s |\n", $1, $2, $3, $4, $5, $6, $7, $8 }' "$prefix.features"
} > "$prefix.generated"
if [ "$mode" = --check ]; then
    if ! cmp -s "$prefix.generated" "$output"; then
        printf '%s\n' 'compatibility: generated documentation is stale' >&2
        exit 1
    fi
else
    cp "$prefix.generated" "$output"
fi
