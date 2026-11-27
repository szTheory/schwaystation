#!/bin/sh
set -eu
[ "$#" -ge 5 ] && [ "$#" -le 6 ] || { printf '%s\n' 'usage: compat-ratchet.sh candidate.csv fresh.csv prior.csv allow.txt YYYY-MM-DD [title|features]' >&2; exit 2; }

candidate=$1
fresh=$2
prior=$3
allow=$4
today=$5
kind=${6:-title}
case "$kind" in title) ledger_name=ledger.csv ;; features) ledger_name=features.csv ;; *) printf '%s\n' "compatibility: unknown ledger kind '$kind'" >&2; exit 2 ;; esac
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
if [ -n "${COMPAT_BASE_DIR:-}" ] || [ -n "${COMPAT_BASE_SHA:-}" ]; then
    [ -n "${COMPAT_BASE_DIR:-}" ] && [ -n "${COMPAT_BASE_SHA:-}" ] || { printf '%s\n' 'compatibility: protected base directory and commit must be supplied together' >&2; exit 1; }
    actual=$(git -C "$COMPAT_BASE_DIR" rev-parse --verify HEAD 2>/dev/null) || { printf '%s\n' 'compatibility: protected base checkout is unavailable' >&2; exit 1; }
    [ "$actual" = "$COMPAT_BASE_SHA" ] || { printf '%s\n' 'compatibility: protected base checkout does not match its commit' >&2; exit 1; }
    if [ "$kind" = features ]; then
        matrix=tests/compat/required-capabilities.csv
        tracked=$(git -C "$COMPAT_BASE_DIR" ls-tree --name-only HEAD -- "$matrix") || exit 1
        if [ -n "$tracked" ]; then
            missing=$(grep -Fvx -f "$root/$matrix" "$COMPAT_BASE_DIR/$matrix") || [ "$?" -eq 1 ] || exit 1
            [ -z "$missing" ] || { printf '%s\n' "compatibility: protected capability rows changed or disappeared: $missing" >&2; exit 1; }
        fi
    fi
    have_ledger=no
    have_judge=no
    [ ! -f "$COMPAT_BASE_DIR/tests/compat/$ledger_name" ] || have_ledger=yes
    if [ -f "$COMPAT_BASE_DIR/scripts/compat-ratchet.sh" ] && { [ "$kind" = title ] || grep -Fq '[title|features]' "$COMPAT_BASE_DIR/scripts/compat-ratchet.sh"; }; then have_judge=yes; fi
    if [ "$have_ledger" = no ] && [ "$have_judge" = no ]; then
        prior=
    elif [ "$have_ledger" = yes ] && [ "$have_judge" = yes ]; then
        if [ ! -f "$COMPAT_BASE_DIR/tests/cmake/compat-ratchet.cmake" ]; then
            printf '%s\n' 'compatibility: protected base judge dependency is missing' >&2
            exit 1
        fi
        tmp=$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/schway-compat-base.XXXXXX") || exit 1
        trap 'rm -rf "$tmp"' EXIT HUP INT TERM
        mkdir -p "$tmp/scripts" "$tmp/tests/cmake" "$tmp/tests/compat"
        cp "$COMPAT_BASE_DIR/scripts/compat-ratchet.sh" "$tmp/scripts/compat-ratchet.sh"
        cp "$COMPAT_BASE_DIR/tests/cmake/compat-ratchet.cmake" "$tmp/tests/cmake/compat-ratchet.cmake"
        [ ! -f "$COMPAT_BASE_DIR/tests/cmake/date_days.cmake" ] || cp "$COMPAT_BASE_DIR/tests/cmake/date_days.cmake" "$tmp/tests/cmake/date_days.cmake"
        cp "$COMPAT_BASE_DIR/tests/compat/$ledger_name" "$tmp/tests/compat/$ledger_name"
        set -- "$candidate" "$fresh" "$tmp/tests/compat/$ledger_name" "$allow" "$today"
        [ "$kind" = title ] || set -- "$@" "$kind"
        COMPAT_BASE_DIR= COMPAT_BASE_SHA= sh "$tmp/scripts/compat-ratchet.sh" "$@" || exit $?
        exit 0
    else
        printf '%s\n' 'compatibility: malformed base has only one protected artifact' >&2
        exit 1
    fi
fi
exec cmake "-DLEDGER=$candidate" "-DFRESH=$fresh" "-DBASE=$prior" \
    "-DALLOW=$allow" "-DTODAY=$today" "-DKIND=$kind" -P "$root/tests/cmake/compat-ratchet.cmake"
