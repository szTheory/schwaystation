# Maintaining schwaystation

How a release is made, rehearsed, re-run and withdrawn, how the release App and the repository's settings are set
up and checked, how the pinned downloads move, and what follows the first release. Tests and CI are in
[testing.md](testing.md).

## How a release happens

Nobody tags or uploads by hand. `.github/workflows/release.yml` runs on every push to `main` and does each step:

1. A pull request is squash-merged into `main`. Its title becomes the commit subject, and release-please reads
   that Conventional Commit to decide the next version: before 1.0, `feat` and a breaking change bump the minor
   version, `fix` and `perf` bump the patch, and `docs`, `chore`, `ci`, `test`, `build`, `refactor` and `style`
   cut no release. Write `ci:`, not `fix(ci):`, for pipeline work; the `title` check refuses the latter.
2. The `release-please` job mints a token for the release App, then opens or updates the release pull request,
   which edits only `version.txt`, `.release-please-manifest.json` and `CHANGELOG.md`. When it creates that pull
   request, the same job turns on squash auto-merge for it with the App's token.
3. The release pull request runs CI like any other, because the App opened it. GitHub merges it, as the App, once
   `CI required` passes.
4. That merge's push runs `release.yml` again. release-please now tags the merge commit and creates a draft
   release. The manifest starts at `0.0.0`, which counts as no release, so the first release is 0.1.0 whatever the
   commit types.
5. `release-ci` runs the whole of `ci.yml` on the release's commit, the `sha` release-please reports, never the
   commit that started the run. Its permissions are the union of every job's in `ci.yml`, the rehearsal's
   included: widen them when a `ci.yml` job asks for more, or GitHub refuses to start the call.
6. `publish` runs `scripts/publish.sh` on the draft, found by its release ID, because a draft cannot be looked up
   by its tag:
   - `prepare`: exactly the 18 zips, every build leg's recorded commit equal to the release commit, `SHA256SUMS`;
   - `upload`, then `actions/attest` over `SHA256SUMS`;
   - `attested`: each zip's attestation verifies from `refs/heads/main`, and a copy with one byte flipped does not;
   - publish the draft as the latest release, by ID;
   - `verify`: `attested` again, `gh release verify`, 19 non-empty assets, and every zip downloaded from the
     release's `releases/download/<tag>/` URL matches `SHA256SUMS` and verifies as signed by `release.yml` from
     `refs/heads/main`.
7. `install-line-latest` runs the README's install line verbatim on `macos-15` against
   `releases/latest/download/`.

The 19 assets are the 18 zips, `schwaystation-{library,runner,libretro}-{linux,macos,windows}-{x64,arm64}.zip`,
and `SHA256SUMS`. No asset name carries the version.

Only an attestation from `release.yml` on `refs/heads/main` is release provenance. A rehearsal signs from
`ci.yml` on a pull request's merge ref, so a check that a download is a release pins both:

```sh
gh attestation verify schwaystation-runner-linux-x64.zip --repo szTheory/schwaystation \
  --signer-workflow szTheory/schwaystation/.github/workflows/release.yml --source-ref refs/heads/main
```

Release runs queue behind each other and are never cancelled. A version moves to 1.0.0 only through a final
`Release-As: 1.0.0` line in the body of the pull request that should cut it.

## The rehearsal

A same-repository pull request that changes a release input (`release.yml`, `ci.yml`, `scripts/publish.sh`,
`release-please-config.json` or `cmake/packaging.cmake`) runs every publish step inside `ci.yml`, and so inside
`CI required`, before it can merge. It runs again on every push to that pull request.

- `rehearsal` checks that the App is configured (it fails naming `SCHWAYSTATION_APP_CLIENT_ID` and
  `SCHWAYSTATION_APP_PRIVATE_KEY` otherwise), mints the App token, gathers the six build legs' archives from the
  pull request's merge commit, and runs `prepare`, `rehearsal-create rehearsal-<run id>-<attempt>`, `upload`,
  `actions/attest`, `attested`, a publish as a prerelease that is never latest, and `verify`;
- `cleanup` runs whatever the rehearsal did, once it started, and deletes the release and its tag.

Only the trigger, the signer workflow, the source ref and the `latest` flag differ from a real release, and
`policy.publish_ref` and `policy.publish_signer` keep a release from ever accepting a rehearsal's attestation. Each
attempt gets a new tag, because a deleted tag of a published immutable release can never be reused. A pull request
from a fork has no secrets, skips the rehearsal and so fails `CI required` when it changes a release input:
rehearse such a change from a branch in this repository.

After any rehearsal, `gh release list` shows no `rehearsal-*` release and
`git ls-remote --tags origin 'rehearsal-*'` prints nothing.

## Re-running a failed release

- **A real release:** open the failed run and choose "Re-run failed jobs". `publish` is safe to re-run: uploads
  replace the draft's assets, and on a release already published it only runs `verify`, which exits 0 when the
  release has its 19 verified assets. Do not choose "Re-run all jobs": release-please then finds the release
  already tagged and skips `publish`.
- **A failed rehearsal:** choose "Re-run all jobs", which takes a fresh `rehearsal-<run id>-<attempt>` tag, or
  push a new commit. If `cleanup` itself failed, delete the leftover by hand:
  `gh release delete rehearsal-<run id>-<attempt> --cleanup-tag --yes`. `publish.sh` deletes only `rehearsal-*`
  tags.

## Closing a phase

Every phase ends in a release, and the release pull request is never held. Before a phase is called done:

- no pull request labelled `autorelease: pending` is open (`gh pr list --label 'autorelease: pending'` prints
  nothing), so no release is stuck behind a red check;
- the latest release is the expected version with 19 assets:
  `gh release view --json tagName,assets --jq '.tagName, (.assets | length)'`.

## Withdrawing a release

A published release is immutable, so a bad one is withdrawn and fixed forward:

1. Point `latest` back at the last good release: `gh release edit v<good> --latest`.
2. Edit the bad release's notes to "Withdrawn: <reason>; use v<good>".
3. Fix forward with a `fix:` pull request, which cuts the next patch release.
4. Delete a release only when its content must not be distributed. Its tag name is then gone for good.

## Release App and secrets

The GitHub App `schwaystation-release`, installed on this repository only, mints the tokens that open and merge
release pull requests and publish releases. Events caused by the default `GITHUB_TOKEN` start no workflow runs,
so a release pull request opened with it would never get its required check, and its merge would never publish.
An App token does both, is scoped to this one repository and acts as its own bot. Its repository permissions are
Contents, Pull requests, Issues and Workflows read and write, and Metadata read; each workflow narrows its token
further (`release-please` asks for contents, pull requests and issues; `publish`, `rehearsal` and `cleanup` for
contents only).

CI holds one variable and one secret:

- `SCHWAYSTATION_APP_CLIENT_ID`, an Actions variable;
- `SCHWAYSTATION_APP_PRIVATE_KEY`, stored twice: as an Actions secret and as a Dependabot secret, because a
  Dependabot-triggered run sees only Dependabot secrets.

Without them, `release-please` and `rehearsal` fail at their first step and name both.

**Setup.** The App was registered once, before the first phase, with a one-off script that is no longer in the
tree. It was removed in commit `61a5bb7`, and `git show 61a5bb7^:scripts/setup-release-app.sh` prints it. It
creates the App through GitHub's manifest flow, sets the variable and both secrets straight from GitHub's response
(the key is never printed or written to disk), opens the installation page preselected to this repository, and
waits until a token signed with the key reads the installation. Only the owner runs it, and only to replace the
App.

**Rotation.** On the App's settings page generate a new private key, then run
`gh secret set SCHWAYSTATION_APP_PRIVATE_KEY < <key file>` and
`gh secret set SCHWAYSTATION_APP_PRIVATE_KEY --app dependabot < <key file>`, delete the old key on the settings
page, and delete the key file. Only the owner handles the key. To replace the App itself, run the script from
history with `--force`, then uninstall the old App.

## Repository settings

`scripts/apply-repo-settings.sh` holds the repository's settings as code, and `.github/rulesets/main.json` holds
the branch ruleset. The script applies, idempotently:

- squash merging only, with the pull request's title and body as the commit message, auto-merge, branch deletion
  on merge, branch updates, and no wiki or projects. The body, not the list of commits, keeps a CI skip token in a
  branch commit from landing on `main` and suppressing its CI and release runs;
- secret scanning with push protection, private vulnerability reporting, Dependabot alerts and security updates;
- Actions pinned by full commit SHA, a read-only default token, and no pull request approval by Actions;
- immutable releases;
- the eight labels `bug`, `compatibility`, `enhancement`, `docs`, `performance`, `needs-info`, `nightly` and
  `digest`;
- the ruleset, replaced by name: no deletion or force push on the default branch, linear history, pull requests
  with no required approval and squash merging only, and the `CI required` check from GitHub Actions, with no
  bypass.

The owner runs it with admin rights, once after creating the repository and again at each milestone tune-up. The
ruleset blocks every pull request until a run has reported `CI required`, so before CI has ever reported, run it
with `--no-ruleset`, and run it again without that flag once a run has reported the check.

`scripts/apply-repo-settings.sh --check` only reads. It compares every setting above, each label's colour and
description, the App's variable and secrets, and the live ruleset with the committed file. It prints one `drift:`
line per difference and exits 1 if there was any (`no drift on <repository>` otherwise). Run it at each tune-up
and after changing anything on the repository's settings page. Change a setting in the script, not by hand, then
apply it.

## The nightly run

`.github/workflows/nightly.yml` runs the 300,000-frame bench of the test card every night on `ubuntu-26.04`,
`ubuntu-26.04-arm`, `macos-15` and wasm32 under the pinned wasmtime. The numbers are trend data in job summaries
and artifacts, never a gate. A failed night opens or comments on the one open issue labelled `nightly`; a good
night closes it. To see the issue written, run it by hand on any branch with `force_failure` set:
`gh workflow run nightly.yml --ref <branch> -f force_failure=true`.

## After the first release

Two `ci:` pull requests follow the first release, one step on their own before the next phase starts:

- **Arm the instructions-per-frame gate.** Run `ci.yml` by hand twice (`gh workflow run ci.yml --ref main`); each
  `perf` job measures one commit three times and prints `spread=`. When both runs print `spread=0`, set
  `SCHWAYSTATION_PERF_MODE: gate` in `ci.yml`. From then on a pull request fails above +1.00 % per fixture, and an
  accepted rise is a dated line in `tests/perf/allow.txt`.
- **Promote wasm32.** Once `wasm32` has a green record on `main`, add it to `CI required`'s needs and its
  `hashes.txt` to `hash-equality`'s comparison, which then counts eight files.

## Pinned downloads

`pins.txt` lists every pinned download as `name version url sha256`, and `scripts/fetch-pin.sh` checks the
SHA-256 before use. Take a long-term-support patch release when it fixes a security advisory, and move to the
next long-term-support line before the current one's end of life. Each bump is one `ci:` pull request whose
digest comes from the release API (the asset's `digest` field). Packages installed with apt stay unpinned: they
are signed, and drift fails loudly as a build failure or a hash change. Actions are pinned by commit SHA in the
workflows, with the tag in a comment; `policy.action_pins` rejects any other reference.
