#!/bin/sh
# Checks a pull request's title (argument 1) and body (standard input), which
# become the squash commit on main. Fault prevented: a title that cuts the
# wrong release or none (a plan-number scope, a "Phase N: Name" title, a
# capitalised subject, or pipeline work typed feat, fix or perf), and a skip
# token in the title or body, which would start no CI run on main.
set -u
# Bytes, not characters: a subject starting with a non-ASCII letter is not
# lower case.
LC_ALL=C
export LC_ALL
title=${1-}
body=$(cat) # all of it first, so the writer never meets a closed pipe
status=0
fail() {
  echo "check-title: $1" >&2
  status=1
}
case $title in *'
'*) fail "the title is more than one line" ;; esac
printf '%s\n' "$title" |
  grep -Eq '^(feat|fix|perf|docs|test|build|ci|chore|refactor|style|revert)(\([a-z][a-z0-9._-]*\))?!?: [a-z]' ||
  fail "not a Conventional Commit title, e.g. 'fix(runner): exit 2 on a missing file': $title"
printf '%s\n' "$title" | grep -Eq '^(feat|fix|perf)\((ci|release|build|test|docs)\)' &&
  fail "pipeline work cuts no release: use ci:, build:, test: or docs: instead of $title"
skip='\[(skip ci|ci skip|no ci|skip actions|actions skip)\]|skip-checks: ?true'
printf '%s\n' "$title" | grep -Eiq "$skip" && fail "the title holds a CI skip token"
printf '%s\n' "$body" | grep -Eiq "$skip" && fail "the body holds a CI skip token"
exit "$status"
