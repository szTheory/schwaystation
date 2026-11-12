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
exec cmake "-DLEDGER=$candidate" "-DFRESH=$fresh" "-DBASE=$prior" \
    "-DALLOW=$allow" "-DTODAY=$today" -P "$root/tests/cmake/compat-ratchet.cmake"
