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
3. The release pull request runs CI. The `changes` job skips the heavy matrix only when fresh REST metadata
   confirms the release App identity and refs, the complete paginated file list is stable, and its exact three
   modified paths are the generated release metadata. Any mismatch or uncertainty takes the heavy path. The
   `CI required` roll-up still runs; see [the CI route and roll-up](testing.md#ci-jobs). GitHub merges the PR as
   the App once `CI required` passes.
4. That merge's push runs `release.yml` again. release-please now tags the merge commit and creates a draft
   release. The manifest starts at `0.0.0`, which counts as no release, so the first release is 0.1.0 whatever the
   commit types.
5. `release-ci` runs the whole of `ci.yml` on the release's commit, the `sha` release-please reports, never the
   commit that started the run. Its permissions are the union of every job's in `ci.yml`, including `actions: read`
   for the exact-attempt CI timing check and the rehearsal permissions: widen them when a `ci.yml` job asks for
   more, or GitHub refuses to start the call.
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

## Fast preflight and CI cost review

The `changes` job runs the cheap source, workflow-route, sorted-inventory and gate-budget checks immediately
after checkout, before the build, guest and release jobs that depend on it. Keep input counts, selectors,
artifact wiring and budget totals in these existing checks; add a permanent check only when it prevents a
recurring fault. For a local route check, use `act pull_request --dryrun -W .github/workflows/ci.yml` and
`act pull_request --dryrun -W .github/workflows/nightly.yml`, mapping each hosted runner label to an available
local image when needed. `act --dryrun` validates the workflow route without running steps. In particular, it
cannot execute the `changes` classifier or populate its outputs, so it does not prove that dependent matrix jobs
will run. A real local `act` job is useful only when it exercises behavior beyond direct CMake checks; it cannot
stand in for hosted macOS, Windows, ARM, release App permissions, attestations or GitHub artifact handoffs.

For every hosted run, inspect it while it runs and after it finishes. Record the exact head SHA, run URL, job
durations, total runner-minutes and the critical path; then look for safe reductions in duplicate work and serial
waits without removing platforms, cases, assertions, archives or required checks. Main CI and Release CI remain
separate gates: the measured runs produced different bytes for all 18 product zips, and waiting for Main before
Release added 24 minutes 25 seconds to the observed release duration. The evidence and disposition are tracked in
the completed [artifact-reuse review](../.planning/todos/completed/050-release-ci-artifact-reuse.md). Do not
repeat a green full suite for bookkeeping when exact-head evidence already covers the unchanged checks.

When Release Please pushes generated metadata to `main`, Main CI skips its heavy matrix only for the App actor and
an exact diff containing modifications to `.release-please-manifest.json`, `CHANGELOG.md` and `version.txt`.
The lightweight checks and required roll-up still run. Release CI keeps the full build, test and artifact matrix on
that same commit; any other actor or changed path keeps Main CI on its full route.

Before pushing a changed head, run the relevant local configure/build/test paths and the cheapest consistency
checks for selectors, inventories, workflow/artifact wiring and budget totals. Use `act` when it exercises a
workflow job beyond the local suite; use `act --dryrun` for routing only. Local checks cannot guarantee hosted
runner, permission, artifact or integration behavior, so keep the required hosted gates. If Docker is needed,
name task-created images, containers, networks and volumes with a `schwaystation-<phase>-<task>` prefix and
remove only those named resources after the check. Avoid global prune commands that can remove unrelated data.
When a change can affect a `quality.json` metric, run its producer locally and compare the result with the active
bound before broad hosted CI when its pinned host and toolchain are available. Otherwise record the hosted-only
producer and use its exact result to review the bound; never guess or widen a quality limit.

## The rehearsal

A same-repository pull request that changes a release input (`release.yml`, `ci.yml`, `scripts/publish.sh`,
`release-please-config.json` or `cmake/packaging.cmake`) runs every publish step inside `ci.yml`, and so inside
`CI required`, before it can merge. It runs again on every push to that pull request.

- `rehearsal` checks that the App is configured (it fails naming `SCHWAYSTATION_APP_CLIENT_ID` and
  `SCHWAYSTATION_APP_PRIVATE_KEY` otherwise), mints the App token, gathers the six build legs' archives from the
  pull request's merge commit, and runs `prepare`, `rehearsal-create rehearsal-<run id>-<attempt>`, `upload`,
  `actions/attest`, `attested`, a publish as a prerelease that is never latest, and `verify`;
- `cleanup` runs whatever the rehearsal did, once it started, and deletes the release and its tag.

After deletion, `publish.sh` retries release and tag lookups for up to ten attempts and accepts only HTTP 404 as
proof that each is gone. A persistent resource or repeated API error still fails cleanup.

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

## Compatibility evidence

Main CI compares historical feature rows and required capabilities with the protected PR base. Package staging reads the completed CTest report for the current build and keeps the libretro core experimental while any required result is unknown.
To render a build-local copy of the compatibility report, set `COMPAT_CAPABILITIES_BUILD` to the build directory
when invoking `scripts/compat-doc.sh`. Leave it unset when regenerating the checked-in snapshot.

A feature allowance uses `<date> <provider> <profile> <protection>!<peripheral>!<predicate> behavior-rev=4 expires=<date> <reason>`
and covers at most 60 days.

## The nightly run

`.github/workflows/nightly.yml` runs the 300,000-frame bench of the test card every night on `ubuntu-26.04`,
`ubuntu-26.04-arm`, `macos-15` and wasm32 under the pinned wasmtime. The numbers are trend data in job summaries
and artifacts, never a gate. A failed night opens or comments on the one open issue labelled `nightly`; a good
night closes it. To see the issue written, run it by hand on any branch with `force_failure` set:
`gh workflow run nightly.yml --ref <branch> -f force_failure=true`.

Pull requests first run a read-only `changes` job over the complete paginated file list. A complete list of only
the four PBP provider/parser/test paths runs fuzz only; a list entirely under `docs/` or `.planning/` skips
Nightly. Any mixed, unknown, renamed, incomplete or unclassified list runs the full workload. API errors or a
failed/timed-out classifier job also fall back to full. Schedule and manual runs always keep every job. The local route controls exercise
these decisions; `act --dryrun` checks job wiring but cannot call GitHub's PR files API or prove its outputs.

Two more jobs in the same workflow report through the same issue and are not part of `CI required`: `vectors-full`
replays all 55,000 public CPU vectors at their pin, and `fuzz` replays the loader and nine media seed sets before
running the loader and media campaigns for 60 seconds each. See [the fuzz target notes](testing.md#fuzz-targets);
a red one is a finding to fix, not a block on a merge.

## Phase-1 CI tail (complete)

The first-release follow-up has already armed the instructions-per-frame gate and made wasm32 required in the
eight-file hash comparison. There is no remaining setup command. The control runs on the v0.1.0 commit were
[40939706087](https://github.com/szTheory/schwaystation/actions/runs/40939706087) and
[40939709809](https://github.com/szTheory/schwaystation/actions/runs/40939709809); each measures the same commit
three times with zero spread. wasm32's successful `main` run is
[40935782754](https://github.com/szTheory/schwaystation/actions/runs/40935782754).

## Weekly triage artifact

`.github/workflows/triage.yml` runs Saturdays at 05:17 UTC and supports manual dispatch. Its read-only collector
filters pull requests from the issues response and publishes only bounded, sanitized issue metadata plus the
latest CI, Release and Nightly status. It does not include issue bodies, authors, logs, branches or commit SHAs,
and it does not post messages. The `weekly-triage` artifact contains `triage.json` and expires after seven days.

List recent runs and download one artifact with:

```sh
gh run list --workflow triage.yml --limit 10
gh run download <run-id> -n weekly-triage
```

To start a run manually, use `gh workflow run triage.yml --ref main`; the artifact appears after that run completes.

## Pinned downloads

`pins.txt` lists every pinned download as `name version url sha256`, and `scripts/fetch-pin.sh` checks the
SHA-256 before use. Take a long-term-support patch release when it fixes a security advisory, and move to the
next long-term-support line before the current one's end of life. Each bump is one `ci:` pull request whose
digest comes from the release API (the asset's `digest` field). Packages installed with apt stay unpinned: they
are signed, and drift fails loudly as a build failure or a hash change. Actions are pinned by commit SHA in the
workflows, with the tag in a comment; `policy.action_pins` rejects any other reference.

## Bumping armips

armips assembles the fixtures and the stub kernel. A bump takes a new commit of the assembler, never a moving
branch, and ends with the stub kernel checked against its source.

1. Find the commit and its 12-character short form: `gh api repos/Kingcom/armips/commits/master --jq .sha`.
2. Hash the archive of that full commit: `curl -fsSL https://github.com/Kingcom/armips/archive/<sha>.tar.gz | shasum -a 256`.
3. Check what the archive vendors: `gh api 'repos/Kingcom/armips/contents/ext?ref=<sha>' --jq '.[].name'`. A new
   submodule or a new vendored directory needs its row in `PROVENANCE.md` and a look at whether the build fetches it.
4. Replace the `armips` row of `pins.txt` with the short commit, the archive URL and the digest, and the commit and
   digest in the Kingcom/armips row of `PROVENANCE.md`.
5. Build from nothing: `rm -rf build/ci && cmake --workflow --preset ci`. Configure fails with the expected and the
   found digest if step 2 was wrong.
6. If `fixtures.stub_drift` fails, the new assembler produced other bytes: follow
   [Regenerating the stub kernel](#regenerating-the-stub-kernel) from its second step.
7. Open one `ci:` pull request; `hash-equality` shows the eight lanes agree.

## Re-pinning the CPU vectors

The vectors are the SingleStepTests `r3000` files. `pins.txt` names each by its git blob SHA-1, and
`tests/vectors/r3000/sst-sample.bin` holds the first 100 cases of each. Moving the pin replaces both.

1. Find the commit: `gh api repos/SingleStepTests/r3000/commits/main --jq .sha`; its first 12 characters are the pin.
2. List the files at it, as `pins.txt` rows:
   `gh api 'repos/SingleStepTests/r3000/contents/v1?ref=<sha>' --jq '.[] | select(.name|endswith(".json.bin")) | "r3000-\(.name|rtrimstr(".json.bin")) <pin> \(.sha) \(.size)"'`.
   Replace the `r3000-` rows of `pins.txt` with the output, and the pin in the comment above them.
3. Fetch and check each file, never committing one:
   `rm -rf build/sst && mkdir -p build/sst`, then
   `grep '^r3000-' pins.txt | while read -r name _ sha _; do f=build/sst/${name#r3000-}.json.bin; gh api "repos/SingleStepTests/r3000/git/blobs/$sha" -H 'Accept: application/vnd.github.raw' > "$f"; test "$(git hash-object "$f")" = "$sha" || exit 1; done`.
4. Slice the sample: `python3 -I tools/sst_slice.py build/sst tests/vectors/r3000/sst-sample.bin`, then
   `shasum -a 256 tests/vectors/r3000/sst-sample.bin`.
5. Put the new pin and that digest in the sample's row of `tests/fixtures/manifest.txt`. Put the pin wherever the old one
   is named: `git grep -n <old pin> -- ':!pins.txt'` lists `ASSET_POLICY.md`, `PROVENANCE.md` and
   `THIRD-PARTY-NOTICES.md`, the last with the full commit.
6. If the number of files changed, change the case counts, 5500 for the sample in `tests/vectors/tests.cmake` and
   55000 for the full set in `tests/nightly/tests.cmake`.
7. Run both lanes: `cmake --workflow --preset ci`, then `cmake --workflow --preset nightly`. A case that now fails is
   either a core bug to fix or an upstream error: the second is a dated line in `tests/vectors/r3000/allow.txt` with the
   upstream issue's URL, and nothing else.

## Regenerating the GTE vector file

`tests/vectors/gte/ps1tests-1150.bin` holds the 1,150 hardware-captured GTE cases of JaCzekanski/ps1-tests, and
`tests/fixtures/src/gte_cases.inc` the 26 cases the `gte_vectors` fixture replays. `tools/gte2bin.py` makes both from the
upstream `gte/test-all/tests.c`, pinned by the `ps1tests-gte-test-all` row of `pins.txt`. CI never runs the tool; the
files change only when the pin or the layout does.

1. Fetch the pinned blob, never committing it:
   `mkdir -p build/ps1tests && gh api repos/JaCzekanski/ps1-tests/git/blobs/<sha> -H 'Accept: application/vnd.github.raw' > build/ps1tests/tests.c`,
   with `<sha>` the row's third column.
2. Check it: `git hash-object build/ps1tests/tests.c` prints that SHA. The tool refuses any other input.
3. Transcode: `python3 -I tools/gte2bin.py build/ps1tests/tests.c tests/vectors/gte/ps1tests-1150.bin --inc tests/fixtures/src/gte_cases.inc`.
4. Prove it repeats: run it again into `build/ps1tests/again.bin` and `build/ps1tests/again.inc`, then `cmp` each with the
   committed file. Both must match, byte for byte.
5. If the layout or the selection changed, put the new SHA-256 of each file in its row of `tests/fixtures/manifest.txt`
   and the `.bin` digest in the "ps1-tests GTE cases" section of `THIRD-PARTY-NOTICES.md`. `policy.notices` fails if either
   file stops being named there.

## Regenerating the stub kernel

`src/stub_kernel.c` is the assembled `tests/fixtures/src/stub/stub.asm` as a byte array. Never edit it by hand;
`fixtures.stub_drift` fails while it differs from the source.

1. Edit `tests/fixtures/src/stub/stub.asm`.
2. Assemble it: `cmake --preset ci && cmake --build --preset ci`.
3. Write the array:
   `cmake -DCASE=array -DFILE=build/ci/fixtures/stub.bin -DOUT=src/stub_kernel.c -P tests/fixtures/tests.cmake`.
4. Measure both files: `shasum -a 256 build/ci/fixtures/stub.bin src/stub_kernel.c` and
   `wc -c < build/ci/fixtures/stub.bin`.
5. Put the digest of `src/stub_kernel.c` in its row of `tests/fixtures/manifest.txt`, and the image's size and digest in
   the Stub kernel row of `PROVENANCE.md`.
6. Run everything again: `cmake --workflow --preset ci`. The hashes of any frame the stub ran change with it, in
   `build/ci/hashes.txt`, and all eight lanes must still agree.
