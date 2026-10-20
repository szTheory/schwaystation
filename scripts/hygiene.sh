#!/bin/sh
#   scripts/hygiene.sh --staged | --tree | --history [revs...]   (pre-commit, CI, pre-push)
# Fault prevented: personal data, a disc, executable, BIOS or memory-card image,
# an unlisted binary, GPL text in code, chain-mode GSD settings or a commit on
# main reaching this public repository. Findings name the rule and the file,
# never the matched text. Exit status 1 if anything was found.
set -eu
# The C locale: under UTF-8, grep -I skips a file with invalid UTF-8 as binary.
LC_ALL=C
export LC_ALL
# Path quoting off, so a name with bytes above 0x7F is scanned, not skipped.
git() { command git -c core.quotePath=false "$@"; }
cd "$(git rev-parse --show-toplevel)"
SELF=scripts/hygiene.sh
MANIFEST=tests/fixtures/manifest.txt
fail=0
found() { printf 'hygiene: %s: %s\n' "$1" "$2" >&2; fail=1; }
# A home directory names a person, also in agent tools' dashed form; CI runner homes do not.
HOME_PATH='(/Users/|/home/)[A-Za-z0-9._-]+|[A-Za-z]:\\Users\\[A-Za-z0-9._-]+|-Users-[A-Za-z0-9._]+-'
HOME_OK='(/Users/|/home/)runner'
EMAIL='[A-Za-z0-9._%+-]+@[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)*\.[A-Za-z][A-Za-z]+'
EMAIL_OK='@users\.noreply\.github\.com|noreply@github\.com|noreply@anthropic\.com|@example\.(com|org|net)|git@github\.com'
ROM_NAME='\.(bin|cue|iso|img|chd|pbp|ecm|mcr|mcd|gme|exe|psexe|psx|sbi|m3u|ccd|sub|mds|mdf|sav|state)$'
GPL_TEXT='GNU (Lesser |Library )?General Public License|SPDX-License-Identifier:[[:space:]]*L?GPL'
CODE_PATH='^(src|include|runner|libretro|host|tests|tools|cmake)/|CMakeLists\.txt$'
# A PlayStation BIOS dump is 512 KiB and a memory-card image 128 KiB.
BIOS_SIZE=524288
CARD_SIZE=131072
# True if some line of stdin matches $1 and not $2.
leaks() { grep -I -E -e "$1" | grep -v -E -e "$2" | grep -q .; }
in_manifest() { [ -f "$MANIFEST" ] && awk -v p="$1" '$1 == p { f = 1 } END { exit !f }' "$MANIFEST"; }
# Prints $2 bytes of stdin from offset $1 as hex, reading no further than that.
hex_at() { head -c $(($1 + $2)) | tail -c "$2" | od -An -tx1 | tr -d ' \n'; }
# $1 is a path; $2 prints its content. An unreadable file is a finding (fail closed).
check_file() {
  [ "$1" = "$SELF" ] && return 0
  if ! $2 "$1" >/dev/null 2>&1; then
    found 'cannot read file' "$1"
    return 0
  fi
  if printf '%s\n' "$1" | grep -q -i -E -e "$ROM_NAME" && ! in_manifest "$1"; then
    found 'disc, executable, BIOS or memory-card image by name' "$1"
  fi
  # Signatures are compared as hex, so this script never holds their bytes.
  if ! in_manifest "$1"; then
    # A PlayStation executable's 8-byte signature at offset 0, whatever the name.
    magic=$(printf '50532d5820455845')
    if [ "$($2 "$1" 2>/dev/null | hex_at 0 8)" = "$magic" ]; then
      found 'PlayStation executable by content' "$1"
    fi
    # ISO 9660's identifier, one byte into sector 16 of a 2048- or 2352-byte-sector image.
    magic=$(printf '4344303031')
    for off in 32769 37657; do
      if [ "$($2 "$1" 2>/dev/null | hex_at $off 5)" = "$magic" ]; then
        found 'disc image by content' "$1"
        break
      fi
    done
    size=$($2 "$1" 2>/dev/null | wc -c | tr -d ' ')
    # A memory-card image: a 2-byte frame header, exactly 128 KiB.
    magic=$(printf '4d43')
    if [ "$size" = "$CARD_SIZE" ] && [ "$($2 "$1" 2>/dev/null | hex_at 0 2)" = "$magic" ]; then
      found 'memory-card image by content' "$1"
    fi
    # Any binary of the BIOS size is a BIOS dump, so no BIOS bytes, even as hex, live here.
    if [ "$size" = "$BIOS_SIZE" ] && ! $2 "$1" 2>/dev/null | grep -I -q ''; then
      found 'BIOS image by size' "$1"
    fi
  fi
  # A file with a NUL byte is binary and must be in the manifest.
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
# $1 lists paths, one per line; a name holding a newline splits and fails closed.
check_files() {
  old_ifs=$IFS
  IFS='
'
  set -f
  for f in $1; do check_file "$f" "$2"; done
  set +f
  IFS=$old_ifs
}
# One GSD step per command; SCHWAYSTATION_ALLOW_CHAIN=1 is the owner's bypass, never an agent's.
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
# Once a remote exists, every change reaches main by pull request.
check_branch() {
  [ "${SCHWAYSTATION_ALLOW_MAIN_COMMIT:-0}" = 1 ] && return 0
  [ -n "$(git remote 2>/dev/null)" ] || return 0
  if [ "$(git symbolic-ref --quiet --short HEAD 2>/dev/null || true)" = main ]; then
    found 'commit on main once a remote exists; branch first' 'git switch -c phase/NN-slug origin/main'
  fi
}
# shellcheck disable=SC2329
staged_show() { git show ":$1"; }
# A tracked file deleted from the working tree is read from the index.
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
      # A squash merge copies the pull request's title and body into the message.
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
