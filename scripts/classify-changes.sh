#!/bin/sh
#   scripts/classify-changes.sh COUNT < paths   (changed paths, both names of a rename)
# Fault prevented: a code change skipping CI as "planning only", or a release
# pipeline change merging unrehearsed. heavy=false only for a non-empty list
# wholly under .planning/ and COUNT at most 3000 (the most the files API lists);
# release_inputs=true for a release input and for any list that cannot be
# trusted. Reads all input, so the writer never meets a closed pipe.
set -u
# Paths are compared as bytes.
LC_ALL=C
export LC_ALL
paths=0
heavy=false
release=false
unknown=false
case ${1-} in
  '' | *[!0-9]*) unknown=true ;;
  *) [ "$1" -le 3000 ] || unknown=true ;;
esac
while IFS= read -r path || [ -n "$path" ]; do
  [ -n "$path" ] || continue
  paths=$((paths + 1))
  case "$path" in
    .planning/*) ;;
    *) heavy=true ;;
  esac
  case "$path" in
    .github/workflows/release.yml | .github/workflows/ci.yml | scripts/publish.sh | \
      release-please-config.json | cmake/packaging.cmake) release=true ;;
  esac
done
[ "$paths" -gt 0 ] || unknown=true
if [ "$unknown" = true ]; then
  heavy=true
  release=true
fi
echo "heavy=$heavy"
echo "release_inputs=$release"
