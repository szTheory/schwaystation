#!/bin/sh
# Decides whether a change needs CI's heavy jobs and whether it touches the
# release's inputs. Reads the changed paths on standard input, one per line,
# with both the old and the new name of a rename, and takes the pull
# request's changed-file count as its argument.
# Fault prevented: a code change skipping CI as "planning only", and a change
# to the release pipeline merging without a release rehearsal. Prints
# heavy=false only when there is at least one path, every path is under
# .planning/ and the count is at most 3000; anything else prints heavy=true.
# Prints release_inputs=true when a path is release.yml, ci.yml,
# scripts/publish.sh, release-please-config.json or cmake/packaging.cmake, and
# also for empty or unreadable input, a missing count or one over 3000: a
# change that cannot be seen rehearses. The files API lists at most 3000
# files, in path order, so .planning/ paths alone can fill it. It reads all of
# its input, so the writer never sees a closed pipe. Paths are compared as
# bytes.
set -u
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
