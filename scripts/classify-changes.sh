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
fast=false
metadata=true
manifest=false
changelog=false
version=false
nightly_full=true
pbp_only=false
all_pbp=true
all_irrelevant=true
case ${1-} in
  '' | *[!0-9]*) unknown=true ;;
  *) [ "$1" -le 3000 ] || unknown=true ;;
esac
case ${2-} in
  '') ;;
  release-metadata) fast=true ;;
  *) unknown=true ;;
esac
while IFS= read -r path || [ -n "$path" ]; do
  [ -n "$path" ] || continue
  paths=$((paths + 1))
  case "$path" in
    .release-please-manifest.json | CHANGELOG.md | version.txt)
      if [ "$fast" = true ]; then
        case "$path" in
          .release-please-manifest.json) [ "$manifest" = false ] || metadata=false; manifest=true ;;
          CHANGELOG.md) [ "$changelog" = false ] || metadata=false; changelog=true ;;
          version.txt) [ "$version" = false ] || metadata=false; version=true ;;
        esac
      else heavy=true
      fi ;;
    .planning/*) metadata=false ;;
    *) heavy=true; metadata=false ;;
  esac
  case "$path" in
    host/disc_pbp.c | host/disc_pbp.h | tests/disc/pbp.c | tests/fuzz/pbp.c) all_irrelevant=false ;;
    .planning/* | docs/* | examples/* | quality.json) all_pbp=false ;;
    *) all_pbp=false; all_irrelevant=false ;;
  esac
  case "$path" in
    .github/workflows/release.yml | .github/workflows/ci.yml | scripts/publish.sh | \
      release-please-config.json | cmake/packaging.cmake) release=true ;;
  esac
done
[ "$paths" -gt 0 ] || unknown=true
if [ "$unknown" = false ] && [ "$paths" -eq "$1" ]; then
  if [ "$all_pbp" = true ]; then nightly_full=false; pbp_only=true
  elif [ "$all_irrelevant" = true ]; then nightly_full=false; fi
fi
if [ "$unknown" = true ]; then
  heavy=true
  release=true
elif [ "$fast" = true ]; then
  if [ "$metadata" = true ] && [ "$paths" -eq 3 ] && [ "$1" -eq 3 ] && \
    [ "$manifest" = true ] && [ "$changelog" = true ] && [ "$version" = true ]; then
    heavy=false
    release=false
  else
    heavy=true
  fi
fi
echo "heavy=$heavy"
echo "release_inputs=$release"
echo "nightly_full=$nightly_full"
echo "pbp_only=$pbp_only"
