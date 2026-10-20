#!/bin/sh
# Repository hygiene for a public repo: no personal data, no disc, executable,
# BIOS or memory-card images, no binary file the fixture manifest does not
# list, no GPL text in code, no chain-mode GSD settings, and no commits on main
# once a remote exists.
#
#   scripts/hygiene.sh --staged             the commit being made       (pre-commit)
#   scripts/hygiene.sh --tree               every file git would track  (CI)
#   scripts/hygiene.sh --history [revs...]  commits; default --all      (pre-push)
#
# Findings name the rule and the file, never the matched text, so the output is
# safe to paste anywhere. Exit status is 1 if anything was found.
set -eu

# The C locale for every command: under a UTF-8 locale grep -I treats bytes that
# are not valid UTF-8 as binary and skips the file. In the C locale only a NUL
# byte makes a file binary.
LC_ALL=C
export LC_ALL

# Path quoting off, so a name with bytes above 0x7F is scanned, not skipped.
git() { command git -c core.quotePath=false "$@"; }

cd "$(git rev-parse --show-toplevel)"

SELF=scripts/hygiene.sh
MANIFEST=tests/fixtures/manifest.txt
fail=0
found() { printf 'hygiene: %s: %s\n' "$1" "$2" >&2; fail=1; }

# A home directory names a person (also in the dashed form agent tools use for
# project folders). Hosted CI runner homes are the exception.
HOME_PATH='(/Users/|/home/)[A-Za-z0-9._-]+|[A-Za-z]:\\Users\\[A-Za-z0-9._-]+|-Users-[A-Za-z0-9._]+-'
HOME_OK='(/Users/|/home/)runner'
# Any address except GitHub noreply, the AI co-author noreply, documentation
# domains and SSH remotes.
EMAIL='[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)*\.[A-Za-z][A-Za-z]+'
EMAIL_OK='@users\.noreply\.github\.com|noreply@github\.com|noreply@anthropic\.com|@example\.(com|org|net)|git@github\.com'
# Disc images and their sheets, PlayStation executables, BIOS dumps, memory
# cards, saves and states.
ROM_NAME='\.(bin|cue|iso|img|chd|pbp|ecm|mcr|mcd|gme|exe|psexe|psx|sbi|m3u|ccd|sub|mds|mdf|sav|state)$'
# GPL or LGPL licence text must never appear in code (PROVENANCE.md).
GPL_TEXT='GNU (Lesser |Library )?General Public License|SPDX-License-Identifier:[[:space:]]*L?GPL'
CODE_PATH='^(src|include|runner|libretro|host|tests|tools|cmake)/|CMakeLists\.txt$'

# Sizes of the images that are recognised by size: a PlayStation BIOS dump is
# 512 KiB and a memory-card image is 128 KiB.
BIOS_SIZE=524288
CARD_SIZE=131072

# stdin is text. True if some line matches $1 and does not match $2.
leaks() { grep -I -E -e "$1" | grep -v -E -e "$2" | grep -q .; }

# True if $1 is the first field of a line in the fixture manifest.
in_manifest() { [ -f "$MANIFEST" ] && awk -v p="$1" '$1 == p { f = 1 } END { exit !f }' "$MANIFEST"; }

# Prints $3 bytes of stdin starting at byte offset $2 as lower-case hex. Reads
# only up to the end of the window, so a large image is not read in full.
hex_at() { head -c $(($1 + $2)) | tail -c "$2" | od -An -tx1 | tr -d ' \n'; }

# $1 is a path; $2 is a command that prints that path's content. A file whose
# content cannot be read is a finding, so the scan fails closed.
check_file() {
  [ "$1" = "$SELF" ] && return 0
  if ! $2 "$1" >/dev/null 2>&1; then
    found 'cannot read file' "$1"
    return 0
  fi
  if printf '%s\n' "$1" | grep -q -i -E -e "$ROM_NAME" && ! in_manifest "$1"; then
    found 'disc, executable, BIOS or memory-card image by name' "$1"
  fi
  # Every signature below is compared as hex so this script never contains
  # the signature bytes themselves.
  if ! in_manifest "$1"; then
    # A PlayStation executable carries an 8-byte signature ("PS-X EXE") at
    # offset 0, whatever the file is called.
    magic=$(printf '50532d5820455845')
    if [ "$($2 "$1" 2>/dev/null | hex_at 0 8)" = "$magic" ]; then
      found 'PlayStation executable by content' "$1"
    fi
    # An ISO 9660 image carries the 5-byte "CD001" identifier one byte into
    # logical sector 16: at 0x8001 in a 2048-byte-sector image and at 0x9319
    # in a 2352-byte raw (Mode 2 Form 1) image.
    magic=$(printf '4344303031')
    for off in 32769 37657; do
      if [ "$($2 "$1" 2>/dev/null | hex_at $off 5)" = "$magic" ]; then
        found 'disc image by content' "$1"
        break
      fi
    done
    size=$($2 "$1" 2>/dev/null | wc -c | tr -d ' ')
    # A memory-card image starts with the 2-byte frame header "MC" and is
    # exactly 128 KiB.
    magic=$(printf '4d43')
    if [ "$size" = "$CARD_SIZE" ] && [ "$($2 "$1" 2>/dev/null | hex_at 0 2)" = "$magic" ]; then
      found 'memory-card image by content' "$1"
    fi
    # Any binary of exactly the BIOS size is treated as a BIOS dump: the first
    # instruction bytes are deliberately not compared, so no BIOS bytes (not
    # even as hex) live in this repository.
    if [ "$size" = "$BIOS_SIZE" ] && ! $2 "$1" 2>/dev/null | grep -I -q ''; then
      found 'BIOS image by size' "$1"
    fi
  fi
  # A file with a NUL byte is binary and must be listed in the manifest. This
  # also stops headerless images, save states and screenshots.
  size=$($2 "$1" 2>/dev/null | head -c 1 | wc -c | tr -d ' ')
  if [ "$size" != 0 ] && ! $2 "$1" 2>/dev/null | grep -I -q '' && ! in_manifest "$1"; then
    found 'binary file not in manifest' "$1"
  fi
  if $2 "$1" 2>/dev/null | leaks "$HOME_PATH" "$HOME_OK"; then found 'home directory path' "$1"; fi
  if $2 "$1" 2>/dev/null | leaks "$EMAIL" "$EMAIL_OK"; then found 'email address' "$1"; fi
  if printf '%s\n' "$1" | grep -q -E -e "$CODE_PATH" && $2 "$1" 2>/dev/null | grep -q -I -E -e "$GPL_TEXT"; then
    found 'GPL licence text in code' "$1"
  fi
}

# $1 is a newline-separated list; $2 as in check_file. A name holding a newline
# splits into parts that name no file, and each part fails closed.
check_files() {
  old_ifs=$IFS
  IFS='
'
  set -f
  for f in $1; do check_file "$f" "$2"; done
  set +f
  IFS=$old_ifs
}

# GSD runs one step per command in this project. The live config governs, so
# the working-tree file is checked. SCHWAYSTATION_ALLOW_CHAIN=1 is a bypass for
# the owner's own shell, never set by an agent (AGENTS rule 9).
check_stop_policy() {
  [ "${SCHWAYSTATION_ALLOW_CHAIN:-0}" = 1 ] && return 0
  [ -f .planning/config.json ] || return 0
  grep -q -E '"mode"[[:space:]]*:[[:space:]]*"interactive"' .planning/config.json ||
    found 'GSD mode is not interactive' .planning/config.json
  if grep -q -E '"(auto_advance|_auto_chain_active)"[[:space:]]*:[[:space:]]*true' .planning/config.json; then
    found 'GSD auto-advance or chain flag is on' .planning/config.json
  fi
}

check_identity() { # $1 is an email; $2 says where it came from
  case $1 in
    *@users.noreply.github.com | noreply@github.com) ;;
    *) found 'commit identity is not a noreply address' "$2" ;;
  esac
}

# Once the repository has a remote, every change reaches main by pull request.
# SCHWAYSTATION_ALLOW_MAIN_COMMIT=1 bypasses this for a deliberate exception.
check_branch() {
  [ "${SCHWAYSTATION_ALLOW_MAIN_COMMIT:-0}" = 1 ] && return 0
  [ -n "$(git remote 2>/dev/null)" ] || return 0
  if [ "$(git symbolic-ref --quiet --short HEAD 2>/dev/null || true)" = main ]; then
    found 'commit on main once a remote exists; branch first' 'git switch -c phase/NN-slug origin/main'
  fi
}

# The two readers are called indirectly, as check_file's $2.
# shellcheck disable=SC2329
staged_show() { git show ":$1"; }
# A tracked file deleted from the working tree is still read from the index.
# shellcheck disable=SC2329
tree_show() { if [ -f "$1" ]; then cat "$1"; else git show ":$1"; fi; }

mode=${1:---tree}
[ $# -gt 0 ] && shift
case $mode in
  --staged)
    check_files "$(git diff --cached --name-only -z --diff-filter=ACMR | tr '\0' '\n')" staged_show
    check_stop_policy
    check_branch
    check_identity "$(git var GIT_AUTHOR_IDENT | sed -E 's/.*<([^>]*)>.*/\1/')" 'author'
    check_identity "$(git var GIT_COMMITTER_IDENT | sed -E 's/.*<([^>]*)>.*/\1/')" 'committer'
    ;;
  --tree)
    check_files "$(git ls-files -z --cached --others --exclude-standard | tr '\0' '\n')" tree_show
    check_stop_policy
    ;;
  --history)
    [ $# -gt 0 ] || set -- --all
    for c in $(git rev-list "$@"); do
      if git grep -I -h -E -e "$HOME_PATH" "$c" -- . ":!$SELF" 2>/dev/null | grep -v -E -e "$HOME_OK" | grep -q .; then
        found 'home directory path' "commit $c"
      fi
      if git grep -I -h -E -e "$EMAIL" "$c" -- . ":!$SELF" 2>/dev/null | grep -v -E -e "$EMAIL_OK" | grep -q .; then
        found 'email address' "commit $c"
      fi
      # A squash merge copies the pull request's title and body into the
      # message, so messages are scanned as well.
      if git log -1 --format=%B "$c" | leaks "$HOME_PATH" "$HOME_OK"; then
        found 'home directory path' "message of $c"
      fi
      if git log -1 --format=%B "$c" | leaks "$EMAIL" "$EMAIL_OK"; then
        found 'email address' "message of $c"
      fi
      # --numstat shows a binary file's line counts as "-".
      for b in $(git diff-tree -r --root --no-commit-id --numstat "$c" |
        awk -F '\t' '$1 == "-" && $2 == "-" { print $3 }'); do
        [ "$b" = "$SELF" ] || in_manifest "$b" || found 'binary file not in manifest' "$b (in history)"
      done
    done
    for e in $(git log "$@" --format='%ae%n%ce' | sort -u); do check_identity "$e" 'history'; done
    for n in $(git log "$@" --name-only --format= | sort -u | grep -i -E -e "$ROM_NAME" || true); do
      in_manifest "$n" || found 'disc, executable, BIOS or memory-card image by name' "$n (in history)"
    done
    ;;
  *)
    printf 'usage: %s --staged | --tree | --history [rev-list args]\n' "$0" >&2
    exit 2
    ;;
esac

exit "$fail"
