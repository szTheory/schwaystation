#!/bin/sh
set -eu
case "$#" in 3) ;; *) echo 'usage: compat-controls.sh CASE SOURCE_ROOT WORK_ROOT' >&2; exit 2 ;; esac
case_name=$1 source_root=$2 work_root=$3
today=2026-11-11
tmp=$(mktemp -d "$work_root/compat-controls.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
title_header=serial,md5,track1_sha1,title,region,media,predicate,status,evidence,date
feature_header=provider,profile,protection,peripheral,predicate,status,evidence,date
control_data=$source_root/tests/compat/compat-control-cases.tsv
fixture() { awk -F'|' -v id="$1" '$1=="row" && $2==id { sub(/^[^|]*[|][^|]*[|]/, ""); print; exit }' "$control_data"; }
row() { case "$1" in none) return ;; *~*) row "${1%%~*}"; printf '\n'; row "${1#*~}" ;; *) fixture "$1" ;; esac; }
ledger() { kind=$3; case "$kind" in features) header=$feature_header ;; *) header=$title_header ;; esac; printf '%s\n' "$header" > "$1"; [ -z "$2" ] || printf '%s\n' "$2" >> "$1"; }
allowance() { awk -F'|' -v id="$1" '$1=="allow" && $2==id { sub(/^[^|]*[|][^|]*[|]/, ""); print }' "$control_data"; }
expect() {
    name=$1 want=$2 pattern=$3; shift 3
    if env COMPAT_BASE_DIR= COMPAT_BASE_SHA= COMPAT_TODAY="$today" "$@" > "$tmp/out" 2>&1; then rc=0; else rc=$?; fi
    if { [ "$want" = TRUE ] && [ "$rc" -ne 0 ]; } || { [ "$want" = FALSE ] && [ "$rc" -eq 0 ]; }; then cat "$tmp/out" >&2; echo "$name: expected $want, got $rc" >&2; exit 1; fi
    if [ "$want" = FALSE ] && [ -n "$pattern" ] && ! grep -Eq "$pattern" "$tmp/out"; then cat "$tmp/out" >&2; echo "$name: failure did not match $pattern" >&2; exit 1; fi
}
judge() {
    name=$1 candidate=$2 fresh=$3 prior=$4 allow=$5 want=$6 kind=$7 pattern=${8:-}
    candidate=$(row "$candidate"); fresh=$(row "$fresh"); prior=$(row "$prior")
    [ -n "$prior" ] || prior=$candidate
    allowance "$allow" > "$tmp/allow"
    ledger "$tmp/prior.csv" "$prior" "$kind"; ledger "$tmp/candidate.csv" "$candidate" "$kind"; ledger "$tmp/fresh.csv" "$fresh" "$kind"
    set -- sh "$source_root/scripts/compat-ratchet.sh" "$tmp/candidate.csv" "$tmp/fresh.csv" "$tmp/prior.csv" "$tmp/allow" "$today"
    [ "$kind" = title ] || set -- "$@" "$kind"
    expect "$name" "$want" "$pattern" "$@"
}
invalid() {
    judge "$1" "$2" "$2" none none "$3" title "$4"
}
preflight() {
    name=$1 rows=$(row "$2") want=$3 pattern=${4:-} max=${5:-}
    ledger "$tmp/$name.csv" "$rows" features
    set -- cmake "-DFEATURES=$tmp/$name.csv" -DMODE=features-preflight
    [ -z "$max" ] || set -- "$@" "-DFEATURE_MAX_ROWS=$max"
    set -- "$@" -P "$source_root/tests/cmake/compat-ratchet.cmake"
    expect "feature preflight $name" "$want" "$pattern" "$@"
}

case "$case_name" in
ratchet)
    for spec in \
        'unchanged_pass|tpass|tpass|tpass|none|TRUE|title' 'pass_to_fail|tfail|tfail|tpass|none|FALSE|title' 'pass_to_unknown|tunknown|tunknown|tpass|none|FALSE|title' \
        'unknown_to_pass|tpass|tpass|tunknown|none|TRUE|title' 'unknown_to_fail|tfail|tfail|tunknown|none|TRUE|title' 'unknown_to_unknown|tunknown|tunknown|tunknown|none|TRUE|title' \
        'fail_to_pass|tpass|tpass|tfail|none|TRUE|title' 'allowance_60_day|tfail|tfail|tpass|title60|TRUE|title' 'allowance_61_day|tfail|tfail|tpass|title61|FALSE|title' \
        'expired_allowance|tfail|tfail|tpass|expired|FALSE|title' 'wrong_revision|tfail|tfail|tpass|titlerev|FALSE|title' 'removed_prior_pass|tother|tother|tpass|none|FALSE|title' \
        'missing_fresh_row|tpass|tother|none|none|FALSE|title' 'extra_fresh_row|tpass|tpass~tother|none|none|FALSE|title' 'fresh_status_disagreement|tpass|tfail|none|none|FALSE|title' \
        'changed_frame_hash|tchanged|tchanged|tpass|none|FALSE|title' 'frame_hash_allowance|tchanged|tchanged|tpass|titlematch|TRUE|title'; do
        old_ifs=$IFS; IFS='|'; set -- $spec; IFS=$old_ifs; judge "$@"
    done
    for spec in 'duplicate_key|tpass~tpass|FALSE|duplicate ledger key' 'comma_field|tcomma|FALSE|row does not have' \
        'quote_field|tquote|FALSE|quoted or carriage-return field' 'newline_field|tpass~bad~field|FALSE|row does not have'; do
        old_ifs=$IFS; IFS='|'; set -- $spec; IFS=$old_ifs; invalid "$@"
    done
    ;;
features)
    for spec in 'unknown_stays_unknown|chd|chd|none|none|TRUE|features' \
        'pass_without_evidence|chdpass|chd|none|none|FALSE|features|fresh result or evidence' \
        'evidence_reference_mismatch|chdpass|chdfresh|none|none|FALSE|features|fresh result or evidence disagrees' 'fresh_context_mismatch|chdpass|chdprotectedpass|none|none|FALSE|features|fresh fixture result' \
        'retained_pass_new_provider|chdpass~pbp|chdpass~pbp|chdpass|none|TRUE|features' 'new_provider_row|chd~pbp|chd~pbp|chd|none|TRUE|features' \
        'pass_to_fail|chdfail|chdfail|chdpass|none|FALSE|features|prior result changed or disappeared' \
        'removed_prior_pass|chdpass|chdpass|chdprotectedpass|none|FALSE|features|prior result changed or disappeared' \
        'expired_allowance|chdfail|chdfail|chdpass|fexpired|FALSE|features|prior result changed or disappeared' \
        'overlong_allowance|chdfail|chdfail|chdpass|foverlong|FALSE|features|allowance window exceeds 60 days' \
        'malformed_allowance|chdfail|chdfail|chdpass|fmalformed|FALSE|features|malformed allowance' \
        'mismatched_allowance|chdfail|chdfail|chdpass|fmismatch|FALSE|features|prior result changed or disappeared' \
        'wrong_revision|chdfail|chdfail|chdpass|frev|FALSE|features|does not name behavior revision' \
        'matching_allowance|chdfail|chdfail|chdpass|fmatch|TRUE|features'; do
        old_ifs=$IFS; IFS='|'; set -- $spec; IFS=$old_ifs; judge "$@"
    done
    for spec in 'zero_rows|none|FALSE|feature row count is outside' 'one_row|chd|TRUE|' 'many_rows|chd~pbp|TRUE|' \
        'duplicate_key|chd~chd|FALSE|duplicate ledger key' 'missing_key_field|fmissing|FALSE|row does not have' 'tuple_prefix|chd~chd2|TRUE|' \
        'tuple_reversed|chd2~chd|FALSE|not sorted by provider' 'count_overflow|chd~pbp|FALSE|feature row count is outside|1' 'invalid_date|fdate|FALSE|date out of range'; do
        old_ifs=$IFS; IFS='|'; set -- $spec; IFS=$old_ifs; preflight "$@"
    done
    ;;
docs)
    printf '%s\n%s\n%s\n' "$title_header" "$(row zulu)" "$(row alpha)" > "$tmp/ledger.csv"
    expect 'compatibility docs generation' TRUE '' sh "$source_root/scripts/compat-doc.sh" "$tmp/ledger.csv" "$tmp/docs.md"
    expect 'compatibility docs freshness' TRUE '' sh "$source_root/scripts/compat-doc.sh" "$tmp/ledger.csv" "$tmp/docs.md" --check
    grep -q '| Alpha |' "$tmp/docs.md" && grep -q '| Zulu |' "$tmp/docs.md" && grep -q 'measured fixture' "$tmp/docs.md" && grep -q 'fixtures.alpha:exit=0' "$tmp/docs.md" && grep -q '2026-11-11' "$tmp/docs.md" && awk '/\| Alpha \|/{a=NR} /\| Zulu \|/{z=NR} END{exit !(a && z && a<z)}' "$tmp/docs.md" || exit 1
    printf '\nStale output\n' >> "$tmp/docs.md"
    expect 'compatibility docs stale output' FALSE '' sh "$source_root/scripts/compat-doc.sh" "$tmp/ledger.csv" "$tmp/docs.md" --check
    for value in 'Bad,Title' 'Bad"Title'; do
        sed "s/Alpha/$value/" "$tmp/ledger.csv" > "$tmp/invalid.csv"
        expect 'compatibility docs malformed field' FALSE '' sh "$source_root/scripts/compat-doc.sh" "$tmp/invalid.csv" "$tmp/docs.md" --check
    done
    ;;
*) echo "unknown compatibility control case '$case_name'" >&2; exit 2 ;;
esac
