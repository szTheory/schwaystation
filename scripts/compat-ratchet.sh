#!/bin/sh
set -eu

if [ "$#" -ne 5 ]; then
    printf '%s\n' 'usage: compat-ratchet.sh candidate.csv fresh.csv prior.csv allow.txt YYYY-MM-DD' >&2
    exit 2
fi

candidate=$1
fresh=$2
prior=$3
allow=$4
today=$5
root=$(CDPATH= cd "$(dirname "$0")/.." && pwd)
if [ -n "${COMPAT_BASE_DIR:-}" ] || [ -n "${COMPAT_BASE_SHA:-}" ]; then
    if [ -z "${COMPAT_BASE_DIR:-}" ] || [ -z "${COMPAT_BASE_SHA:-}" ]; then
        printf '%s\n' 'compatibility: protected base directory and commit must be supplied together' >&2
        exit 1
    fi
    actual=$(git -C "$COMPAT_BASE_DIR" rev-parse --verify HEAD 2>/dev/null) || {
        printf '%s\n' 'compatibility: protected base checkout is unavailable' >&2
        exit 1
    }
    if [ "$actual" != "$COMPAT_BASE_SHA" ]; then
        printf '%s\n' 'compatibility: protected base checkout does not match its commit' >&2
        exit 1
    fi
    have_ledger=no
    have_judge=no
    [ ! -f "$COMPAT_BASE_DIR/tests/compat/ledger.csv" ] || have_ledger=yes
    [ ! -f "$COMPAT_BASE_DIR/scripts/compat-ratchet.sh" ] || have_judge=yes
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
        cp "$COMPAT_BASE_DIR/tests/compat/ledger.csv" "$tmp/tests/compat/ledger.csv"
        if COMPAT_BASE_DIR= COMPAT_BASE_SHA= sh "$tmp/scripts/compat-ratchet.sh" \
            "$candidate" "$fresh" "$tmp/tests/compat/ledger.csv" "$allow" "$today"; then
            exit 0
        else
            result=$?
            exit "$result"
        fi
    else
        printf '%s\n' 'compatibility: malformed base has only one protected artifact' >&2
        exit 1
    fi
fi
exec cmake "-DLEDGER=$candidate" "-DFRESH=$fresh" "-DBASE=$prior" \
    "-DALLOW=$allow" "-DTODAY=$today" -P "$root/tests/cmake/compat-ratchet.cmake"
