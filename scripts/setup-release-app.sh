#!/bin/sh
# Creates the release GitHub App through GitHub's manifest flow, stores its
# client ID and private key where CI reads them, and waits until the App is
# installed on the repository. The owner runs this once; it needs a browser.
#
#   scripts/setup-release-app.sh [--force] [owner/repo]
#
# Why an App: events caused by the default GITHUB_TOKEN start no workflow runs,
# so a release pull request opened with it never gets its required check and
# its merge never publishes. An App token does both, is scoped to this one
# repository, never expires and acts as its own bot rather than as the owner.
#
# The private key goes from GitHub's response straight into `gh secret set`
# (once for Actions, once for Dependabot); it is never printed or written to
# disk. Needs: gh (admin on the repository), jq, curl, openssl, python3.
set -eu

force=0
[ "${1:-}" = --force ] && { force=1; shift; }
repo=${1:-$(gh repo view --json nameWithOwner -q .nameWithOwner)}
name=$(basename "$repo")-release
var=SCHWAYSTATION_APP_CLIENT_ID
secret=SCHWAYSTATION_APP_PRIVATE_KEY

has_var=$(gh variable list --repo "$repo" --json name -q "map(select(.name == \"$var\")) | length")
if [ "$has_var" != 0 ] && [ "$force" = 0 ]; then
  echo "$var is already set on $repo; rerun with --force to create a new App"
  exit 0
fi

# One-shot local page: posts the manifest to GitHub, then catches the redirect
# carrying the one-time conversion code (valid for an hour).
echo "Opening the browser: confirm 'Create GitHub App' for $name."
code=$(python3 -I - "$repo" "$name" <<'PY'
import html, http.server, json, secrets, sys, urllib.parse, webbrowser
repo, name = sys.argv[1], sys.argv[2]
state = secrets.token_urlsafe(16)
srv = http.server.HTTPServer(("127.0.0.1", 0), http.server.BaseHTTPRequestHandler)
base = "http://127.0.0.1:%d" % srv.server_port
manifest = json.dumps({
    "name": name, "url": "https://github.com/" + repo, "public": False,
    "hook_attributes": {"url": "https://github.com/" + repo, "active": False},
    "redirect_url": base + "/callback",
    "default_permissions": {"contents": "write", "pull_requests": "write",
                            "issues": "write", "workflows": "write", "metadata": "read"},
    "default_events": []})
found = []
class Page(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def do_GET(self):
        u = urllib.parse.urlparse(self.path)
        q = urllib.parse.parse_qs(u.query)
        if u.path == "/callback" and q.get("state") == [state] and "code" in q:
            found.append(q["code"][0])
            body = "<p>App created. Return to the terminal.</p>"
        elif u.path == "/":
            body = ('<form method="post" action="https://github.com/settings/apps/new?state=%s">'
                    '<input type="hidden" name="manifest" value="%s">'
                    '<button>Create the GitHub App</button></form>'
                    '<script>document.forms[0].submit()</script>'
                    % (state, html.escape(manifest, quote=True)))
        else:
            body = "<p>Unexpected request.</p>"
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(body.encode())
srv.RequestHandlerClass = Page
webbrowser.open(base + "/")
while not found:
    srv.handle_request()
print(found[0])
PY
)

# The conversion response carries the App's ID, slug, client ID and key.
app=$(gh api -X POST "app-manifests/$code/conversions")
slug=$(printf '%s' "$app" | jq -r .slug)
client_id=$(printf '%s' "$app" | jq -r .client_id)
pem=$(printf '%s' "$app" | jq -r .pem)
unset app

gh variable set "$var" --repo "$repo" --body "$client_id"
printf '%s\n' "$pem" | gh secret set "$secret" --repo "$repo"
printf '%s\n' "$pem" | gh secret set "$secret" --repo "$repo" --app dependabot
echo "App $slug created; $var and $secret (Actions and Dependabot) set on $repo."

# Install on this repository only. The link preselects the repository.
owner_id=$(gh api "users/${repo%%/*}" -q .id)
repo_id=$(gh api "repos/$repo" -q .id)
url="https://github.com/apps/$slug/installations/new/permissions?suggested_target_id=$owner_id&repository_ids[]=$repo_id"
echo "Opening the browser: install $slug on $repo only."
python3 -I -m webbrowser -t "$url" >/dev/null 2>&1 || echo "Open: $url"

# Proof: an App JWT signed with the key reads this repository's installation.
# The key is fed to openssl on stdin; only the unsigned JWT text touches disk.
b64url() { openssl base64 -A | tr '+/' '-_' | tr -d '='; }
signing=$(mktemp)
trap 'rm -f "$signing"' EXIT
tries=0
while :; do
  now=$(date +%s)
  head=$(printf '{"alg":"RS256","typ":"JWT"}' | b64url)
  body=$(printf '{"iat":%s,"exp":%s,"iss":"%s"}' $((now - 60)) $((now + 540)) "$client_id" | b64url)
  printf '%s.%s' "$head" "$body" >"$signing"
  sig=$(printf '%s\n' "$pem" | openssl dgst -sha256 -sign /dev/stdin "$signing" | b64url)
  if curl -fsS -o /dev/null -H "Authorization: Bearer $head.$body.$sig" \
      -H 'Accept: application/vnd.github+json' "https://api.github.com/repos/$repo/installation" 2>/dev/null; then
    echo "verified: $slug is installed on $repo and its key signs"
    exit 0
  fi
  tries=$((tries + 1))
  [ "$tries" -lt 100 ] || { echo "error: $slug is not installed on $repo after 5 minutes" >&2; exit 1; }
  sleep 3
done
