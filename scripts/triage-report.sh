#!/bin/sh
# Emit a bounded triage artifact from an issues array and three workflow API responses.
set -eu
if [ "$#" -ne 2 ]; then
  printf '%s\n' 'usage: triage-report.sh ISSUES_JSON RUNS_JSON' >&2
  exit 1
fi
if ! "${TRIAGE_JQ:-jq}" -en --slurpfile issues "$1" --slurpfile runs "$2" '
  def object: type == "object";
  def positive_id: type == "number" and . > 0 and . <= 9007199254740991 and floor == .;
  def timestamp: type == "string" and test(
    "^[0-9]{4}-(0[1-9]|1[0-2])-(0[1-9]|[12][0-9]|3[01])T([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9]Z\\z");
  def choice($values): . as $value | $values | index($value) != null;
  def scrub($limit):
    # Remove ANSI control strings (OSC, DCS, SOS, PM and APC), CSI, then single escapes.
    gsub("(?:\u001b[]PX^_]|[\u0090\u0098\u009d-\u009f])[^\u0007\u001b\u009c]*(?:\u0007|\u001b\\\\|\u009c|\\z)"; "")
    | gsub("(?:\u001b\\[|\u009b)[0-?]*[ -/]*[@-~]|\u001b[ -/]*[@-~]"; "")
    | gsub("[\\p{Cc}\\p{Cf}]"; "")
    | gsub("(?i)[a-z][a-z0-9+.-]*://[^\\s<>]+"; "[redacted]")
    # Redact through the next quote or text boundary so spaces in paths cannot expose a suffix.
    | gsub("(?i)(?<before>^|[^[:alnum:]_])(?:[a-z]:[/\\\\]|\\\\\\\\|/[^\\s])[^\"<>|`]*"; "\(.before)[redacted]")
    | gsub("[^\\s<>@]+@[^\\s<>@]+"; "[redacted]")
    | gsub("(?i)[a-f0-9]{7,}"; "[redacted]")
    | .[:$limit];
  def issue:
    if (.number | positive_id) and (.title | type == "string") and
       (.labels | type == "array" and all(.[]; object and (.name | type == "string"))) and
       (.created_at | timestamp) and (.updated_at | timestamp)
    then {id: .number, title: (.title | scrub(160)), labels: [.labels[:8][] | .name | scrub(48)],
          created_at, updated_at}
    else error("invalid issue") end;
  def workflow($name):
    .[$name] | if object and (.workflow_runs | type == "array") then .workflow_runs
      else error("invalid workflow response") end
    | if length == 0 then {workflow: $name, run_id: null, status: null, conclusion: null, updated_at: null}
      else .[0] | if object and (.id | positive_id) and (.updated_at | timestamp) and
        (.status | choice(["completed", "queued", "in_progress", "requested", "waiting", "pending"])) and
        has("conclusion") and (.conclusion | choice([null, "action_required", "cancelled", "failure",
          "neutral", "skipped", "stale", "startup_failure", "success", "timed_out"]))
      then {workflow: $name, run_id: .id, status, conclusion, updated_at}
      else error("invalid run") end end;
  if ($issues | length == 1 and (.[0] | type == "array" and all(.[]; object))) and
     ($runs | length == 1 and (.[0] | object)) then
    ($issues[0] | map(select(has("pull_request") | not) | issue)) as $rows
    | {schema_version: 1, issues_truncated: ($rows | length > 100), issues: $rows[:100],
       workflows: [$runs[0] | workflow("ci"), workflow("release"), workflow("nightly")]}
  else error("invalid input") end
' 2>/dev/null; then
  printf '%s\n' 'triage-report: invalid issues or workflow metadata' >&2
  exit 1
fi
