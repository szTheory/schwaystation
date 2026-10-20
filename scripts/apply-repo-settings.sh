#!/bin/sh
# Applies the GitHub repository settings this project relies on. Idempotent;
# run once after the repository exists and again at each milestone tune-up.
#
#   scripts/apply-repo-settings.sh [--check] [--no-ruleset] [owner/repo]
#
# --check only reads: it prints one drift line for every setting, label or
# ruleset part that differs from this file and the committed ruleset, then
# exits 1 if any did.
# Needs: gh, authenticated with admin rights on the repository.
#
# Why squash title = PR title and squash body = PR body: GSD's ship step pushes
# a commit whose message carries a CI-skip token. If the squash message listed
# the branch's commits, that token would land on main and suppress the CI and
# release runs that main depends on.
set -eu

check=0
ruleset_on=1
while [ $# -gt 0 ]; do
  case $1 in
    --check) check=1; shift ;;
    --no-ruleset) ruleset_on=0; shift ;;   # before CI has ever reported its check
    *) break ;;
  esac
done
repo=${1:-$(gh repo view --json nameWithOwner -q .nameWithOwner)}

# Merge settings: name=expected value, as the repository API reports them.
MERGE_SETTINGS='allow_squash_merge=true
allow_merge_commit=false
allow_rebase_merge=false
squash_merge_commit_title=PR_TITLE
squash_merge_commit_message=PR_BODY
allow_auto_merge=true
delete_branch_on_merge=true
allow_update_branch=true
has_wiki=false
has_projects=false'

LABELS='bug:d73a4a:Something is not working
compatibility:e99695:A game or homebrew title misbehaves
enhancement:a2eeef:New capability or improvement
docs:0075ca:Documentation
performance:fbca04:Speed, latency or memory
needs-info:d876e3:Waiting on the reporter
nightly:5319e7:Rolling nightly failure report
digest:c5def5:Weekly triage digest'

# The branch ruleset (required check, squash only, no force push), applied by
# name: an existing ruleset with the same name is replaced.
ruleset_file=.github/rulesets/main.json
ruleset=$(dirname "$0")/../$ruleset_file
ruleset_name=$(jq -r .name "$ruleset")

if [ "$check" = 1 ]; then
  drift=0
  # want <setting> <have> <want>: prints one line per difference and goes on,
  # so a single run reports all drift.
  want() {
    [ "$2" = "$3" ] || { printf 'drift: %s is %s, want %s\n' "$1" "$2" "$3"; drift=1; }
  }
  # get <path> <jq filter>: a setting as the API reports it, or "unreadable".
  get() { gh api "repos/$repo$1" -q "$2" 2>/dev/null || echo unreadable; }

  current=$(gh api "repos/$repo")
  while IFS='=' read -r key value; do
    want "$key" "$(printf '%s' "$current" | jq -r ".$key")" "$value"
  done <<EOF
$MERGE_SETTINGS
EOF
  for path in secret_scanning secret_scanning_push_protection; do
    want "$path" "$(printf '%s' "$current" | jq -r ".security_and_analysis.$path.status")" enabled
  done
  want default_workflow_permissions "$(get /actions/permissions/workflow .default_workflow_permissions)" read
  want can_approve_pull_request_reviews "$(get /actions/permissions/workflow .can_approve_pull_request_reviews)" false
  # GitHub then refuses any action not pinned by full commit SHA.
  want sha_pinning_required "$(get /actions/permissions .sha_pinning_required)" true
  want immutable-releases "$(get /immutable-releases .enabled)" true
  want private-vulnerability-reporting "$(get /private-vulnerability-reporting .enabled)" true
  want automated-security-fixes "$(get /automated-security-fixes .enabled)" true
  # Dependabot alerts answer 204 when enabled and 404 when not.
  if gh api "repos/$repo/vulnerability-alerts" --silent 2>/dev/null; then have=true; else have=false; fi
  want vulnerability-alerts "$have" true

  labels=$(gh label list --repo "$repo" --limit 500 --json name,color,description 2>/dev/null) || labels='[]'
  while IFS=: read -r name colour description; do
    have=$(printf '%s' "$labels" | jq -r --arg n "$name" '.[] | select(.name == $n) | "\(.color):\(.description)"')
    want "label $name" "${have:-missing}" "$colour:$description"
  done <<EOF
$LABELS
EOF

  # The release App's client ID and key, set by the owner as docs/maintaining.md describes.
  # Without them the release pull request never gets CI and nothing publishes.
  names() { gh "$@" --repo "$repo" --json name -q '.[].name' 2>/dev/null || echo unreadable; }
  have=$(names variable list | grep -cx SCHWAYSTATION_APP_CLIENT_ID || true)
  want "variable SCHWAYSTATION_APP_CLIENT_ID" "$([ "$have" = 1 ] && echo set || echo missing)" set
  for app in actions dependabot; do
    have=$(names secret list --app "$app" | grep -cx SCHWAYSTATION_APP_PRIVATE_KEY || true)
    want "$app secret SCHWAYSTATION_APP_PRIVATE_KEY" "$([ "$have" = 1 ] && echo set || echo missing)" set
  done

  # The live ruleset with the file's name must match the file. Server-only
  # fields (id, source, timestamps, links) and rule parameters the file does
  # not set are left out; the line names the parts that differ.
  id=$(get /rulesets ".[] | select(.name == \"$ruleset_name\") | .id")
  case $id in
    '' | unreadable) want "ruleset $ruleset_name" missing "$ruleset_file" ;;
    *)
      parts=$(gh api "repos/$repo/rulesets/$id" | jq -r --slurpfile f "$ruleset" '
        def shape: {target, enforcement, conditions, bypass_actors, rules: (.rules | sort_by(.type))};
        def keep($p): if $p == null then del(.parameters)
          else .parameters = ((.parameters // {}) | with_entries(select(.key as $k | $p | has($k)))) end;
        ($f[0] | shape) as $want
        | (.rules |= map(. as $r | keep([$f[0].rules[] | select(.type == $r.type) | .parameters][0]))
           | shape) as $have
        | [$want | keys[] | select($want[.] != $have[.])] | join(",")')
      want "ruleset $ruleset_name" "${parts:+different in }${parts:-as committed}" "as committed" ;;
  esac

  [ "$drift" = 0 ] && echo "no drift on $repo"
  exit "$drift"
fi

set --
for setting in $MERGE_SETTINGS; do
  case $setting in
    *=true | *=false) set -- "$@" -F "$setting" ;;
    *) set -- "$@" -f "$setting" ;;
  esac
done
gh api -X PATCH "repos/$repo" --silent "$@"

# Secret scanning with push protection (free for public repositories).
gh api -X PATCH "repos/$repo" --silent --input - <<'JSON'
{"security_and_analysis": {"secret_scanning": {"status": "enabled"},
                           "secret_scanning_push_protection": {"status": "enabled"}}}
JSON

# Private vulnerability reporting, Dependabot alerts and security updates.
gh api -X PUT "repos/$repo/private-vulnerability-reporting" --silent
gh api -X PUT "repos/$repo/vulnerability-alerts" --silent
gh api -X PUT "repos/$repo/automated-security-fixes" --silent

# Actions: SHA-pinned actions only; read-only default token; Actions cannot approve PRs.
gh api -X PUT "repos/$repo/actions/permissions" --silent -F enabled=true -f allowed_actions=all -F sha_pinning_required=true
gh api -X PUT "repos/$repo/actions/permissions/workflow" --silent \
  -f default_workflow_permissions=read -F can_approve_pull_request_reviews=false

# Immutable releases: tags and assets are locked once a release is published.
gh api -X PUT "repos/$repo/immutable-releases" --silent

# Labels used by the issue forms, the nightly report and the weekly digest.
printf '%s\n' "$LABELS" | while IFS=: read -r name colour description; do
  gh label create "$name" --repo "$repo" --color "$colour" --description "$description" --force >/dev/null
done

if [ "$ruleset_on" = 1 ]; then
  # The ruleset is applied once a CI run has reported its required check;
  # before that it would block the pull request that adds CI.
  id=$(gh api "repos/$repo/rulesets" -q ".[] | select(.name == \"$ruleset_name\") | .id")
  if [ -n "$id" ]; then
    gh api -X PUT "repos/$repo/rulesets/$id" --silent --input "$ruleset"
  else
    gh api -X POST "repos/$repo/rulesets" --silent --input "$ruleset"
  fi
  echo "ruleset '$ruleset_name' applied"
fi

echo "repository settings applied to $repo"
