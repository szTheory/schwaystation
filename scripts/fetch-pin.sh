#!/bin/sh
#   scripts/fetch-pin.sh NAME DEST   (NAME is a pins.txt row "name version url sha256"; $PINS overrides)
# Fault prevented: an unpinned or tampered download reaching a build. DEST is
# touched only once the SHA-256 matches; prints DEST; exit 1 on failure, 2 on misuse.
set -eu
set -f
if [ $# -ne 2 ]; then
  echo "usage: fetch-pin.sh NAME DEST" >&2
  exit 2
fi
name=$1
dest=$2
pins=${PINS:-$(dirname "$0")/../pins.txt}
row=$(awk -v n="$name" '$1 == n { print; exit }' "$pins")
if [ -z "$row" ]; then
  echo "fetch-pin: no row $name in $pins" >&2
  exit 1
fi
# shellcheck disable=SC2086 # split the row into its four fields
set -- $row
url=$3
want=$4
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
file=$tmp/${url##*/}
curl -fsSL --retry 3 -o "$file" "$url"
if command -v sha256sum >/dev/null 2>&1; then
  got=$(sha256sum "$file")
else
  got=$(shasum -a 256 "$file")
fi
got=${got%% *}
if [ "$got" != "$want" ]; then
  echo "fetch-pin: $name: sha256 mismatch: pins.txt has $want, the download is $got" >&2
  exit 1
fi
mkdir -p "$dest"
case $file in
  *.tar.xz | *.tar.gz) tar -xf "$file" -C "$dest" --strip-components=1 ;;
  *) cp "$file" "$dest/" ;;
esac
echo "$dest"
