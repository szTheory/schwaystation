---
title: "GSD hand-off: kickoff, per-phase routine, settings and owner tasks"
summary: "How to start and run schwaystation with OpenGSD one step at a time: the kickoff text, the settings and why, the per-phase and milestone routines including the phase-1 ci: tail step, the owner's tasks, and what was checked before hand-off against GSD 1.18.0."
read_when: "Before the kickoff, before each GSD step, and whenever a GSD step behaves unexpectedly."
updated: 2026-10-19
---

# GSD hand-off

| Question | Section |
|---|---|
| What do I type to start? | 2 |
| What do I do each phase? | 3 |
| What happens at a milestone boundary? | 4 |
| Why is each setting what it is? | 5 |
| What stops GSD from running on by itself? | 6 |
| What only the owner can do | 7 |
| What was verified before hand-off | 8 |

## 1. Why this file exists

schwaystation is built with OpenGSD (`@opengsd/gsd-core`). Preparation wrote the files GSD would otherwise
collect by asking questions (`PROJECT.md`, `REQUIREMENTS.md`, `config.json`, `research/SUMMARY.md` and
`STACK.md`), so the kickoff resumes at roadmap creation [GH.01]. GSD's own researcher still runs before every
phase (`workflow.research: true`), which is where it does its own research and sanity checks from here on. The
siblings' hand-off files are the template [GH.02][GH.03][GH.04]; what changed for a PlayStation core is the
phase-1 `ci:` tail step (section 3 step 9), the absolute gate budget (AGENTS rule 7) and the GSD 1.18.0 notes.

## 2. Kickoff

**Before it** (about five minutes):
- The kickoff can run before the repository exists: it commits on the local branch `gsd/roadmap`, which is
  pushed and merged by pull request once the owner creates the repository (section 7, "Before phase 1").
  In any fresh clone run `git config core.hooksPath .githooks` first.
- `cat ~/.claude/gsd-core/VERSION` prints `1.18.0`, the version these files were checked against (section 8).
  If a stale cache reinstalled an older version, `npx -y @opengsd/gsd-core@1.18.0 --claude --global` restores
  it; 1.10.0 has no resume path and errors "project already initialized" [GH.02]. To use Codex, install the
  same version for Codex first and repeat the dry run (unverified there). Under Codex GSD writes its generated
  guide into `AGENTS.md` rather than `.claude/CLAUDE.md`; keep AGENTS.md marker-free [GH.01].
- Do the owner tasks marked "kickoff" in section 7 (release App), so phase 1 never blocks halfway.
- Skim `.planning/PROJECT.md` and `.planning/REQUIREMENTS.md`; change anything that is off now, because the
  roadmap is built from them.

**Then**, in a new Claude Code session at the repository root, paste this as one message:

```
/gsd-new-project Resume the pre-seeded bootstrap. PROJECT.md, REQUIREMENTS.md, config.json, research/SUMMARY.md and research/STACK.md were written during preparation and are final for initialization: keep them as they are and continue from roadmap creation. Do not run questioning, workflow preferences, project research or requirements scoping. Structure mode is Horizontal Layers (standard template, no Mode lines). Use the "## v1.0 PlayStation games play with sound in RetroArch" milestone heading from research/SUMMARY.md, create the six phases it suggests, copy each suggested phase's "Canonical refs" line into ROADMAP.md, and add "UI hint: no" to every phase. Stop after the roadmap is approved and committed.
```

Following `AGENTS.md`, the agent first creates the branch `gsd/roadmap` off `origin/main`. Expect two prompts:
**Structure** → "Horizontal Layers"; **Roadmap** → approve if the checklist holds, else "Adjust phases" and say
what to change. GSD then commits `ROADMAP.md`, `STATE.md` and the updated `REQUIREMENTS.md` and stops; the agent
pushes the branch and merges it by pull request (before phase 1 adds CI, `gh pr merge --squash` merges at once).

**Roadmap checklist:** heading `## v1.0 PlayStation games play with sound in RetroArch`; six phases, each ending
in an automated release (test frame and pipeline; CPU against the vectors; GTE, DMA and timers; games play in
RetroArch; sound plays in RetroArch; discs and FMV, DECISIONS DEC.06); phase 1 contains shipped C code, CI and the first release; no phase whose goal is
a study, decision or experiment; every phase has "Canonical refs" and "UI hint: no" and no `**Mode:**` line;
every v1 requirement (30: FRAME-01 to 09, CPU-01 to 05, GTE-01 to 03, GAME-01 to 07, SOUND-01 to 02,
DISC-01 to 04) is mapped exactly once.

**If GSD asks its full set of questions instead:** "What do you want to build?" → "Use .planning/PROJECT.md as
written"; saved defaults found → "Configure fresh" (never "Use as-is": the owner's global defaults are yolo mode
with web checks); mode/granularity/execution/git → Interactive, Coarse, Sequential, Yes; research/plan check/
verifier → Yes; compact content, AI models, PR body sections → No, Inherit, none; research the domain first →
Skip; feature scoping → "Use the existing REQUIREMENTS.md unchanged"; structure → Horizontal Layers. GSD's config
writer keeps an existing `config.json`, so these answers matter only if that file is missing; an interrupted
kickoff resumes at the first missing artifact [GH.01].

**After the kickoff**, these print the values shown:
```
node ~/.claude/gsd-core/bin/gsd-tools.cjs query config-get mode --raw                    # interactive
node ~/.claude/gsd-core/bin/gsd-tools.cjs query config-get workflow.auto_advance --raw   # false
node ~/.claude/gsd-core/bin/gsd-tools.cjs query config-get workflow.test_command --raw   # cmake --workflow --preset ci
```

## 3. Each phase: one session per step, so the model can change between steps

1. The agent branches `phase/NN-slug` off `origin/main` (AGENTS rule 10; the pre-commit hook refuses commits on
   `main` once a remote exists) and records the runtime and model in STATE.md.
2. `/gsd-discuss-phase N`. The agent first re-reads `.planning/LESSONS.md` (AGENTS preamble) and quotes XFER-005
   and XFER-100 before any CPU work. The owner's global GSD profile turns on advisor mode, so discuss spawns
   parallel advisor researchers; each gets the owner's fan-out brief (`.planning/preparation/BRIEF.md`,
   "Fan-out method"), ending in one coherent recommendation per point that the step then follows without asking
   again; the owner is asked only what the preparation pack cannot settle (LESSONS XFER-090; DEC.106).
3. `/gsd-plan-phase N`. Plan 1 lands the CMake skeleton and the per-directory inventories; plans that share no
   inventory or header go in parallel waves, the rest sequentially (LESSONS XFER-083; AGENTS rule 14). A plan
   past 30 minutes is split (XFER-101).
4. `/gsd-execute-phase N`. Code review, the test gate (`workflow.test_command`), the verifier and the phase close
   all run inside this step; code review runs before verification (AGENTS rule 7). GSD 1.18.0 re-runs the
   verifier by itself when the report is missing or covered source changed after it ran
   (`workflows/execute-phase.md:394–397`) [GH.01].
5. Read `.planning/phases/NN-*/NN-VERIFICATION.md`:
   - `passed` → step 6;
   - `human_needed` → `/gsd-verify-work N`, which closes every item with code, configuration, a test, CI, hosted
     evidence or the agent's own reading, viewing and running, and hands the owner only what AGENTS rule 6 leaves
     to them;
   - `gaps_found` → run one fresh verifier first (LESSONS XFER-053; 1.18.0 looks to have fixed the stale route,
     XFER-098, confirm at the first occurrence), then one gap round (`/gsd-plan-phase N` for the gaps, then
     `/gsd-execute-phase N` in a new session); a second only with new evidence. On resume, verify-work marks a
     gap resolved when a gap plan with a SUMMARY names it (`workflows/verify-work.md:507`) [GH.01]. Deferred
     review findings become one todo each, named by the plan that closes them (XFER-085).
6. `/gsd-ship N` opens the pull request titled "Phase N: name" (`workflows/ship.md:376`) and pushes a ship-note
   commit carrying `[ci skip]` (`:494`); when that leaves the PR BLOCKED with zero checks, 1.18.0 pushes an empty
   `chore: trigger CI` commit by itself (`:520–524`), so two extra commits land on the branch [GH.01]. The agent
   retitles the PR as a Conventional Commit (`gh pr edit --title "feat: …"`; `fix:`, or `ci:` for CI-only work so
   no release is cut), runs `gh pr merge --auto --squash` in a shell (never inside GSD command text, where GSD
   would read the option as its own auto flag), re-runs the title check if it ran before the retitle
   (`gh run rerun --failed`), and confirms that a CI run exists on the head commit. The repository's squash
   settings take title and body from the PR, which keeps both extra commits' text off `main` (XFER-096, 103).
7. CI goes green → the pull request merges → release-please opens its release pull request with the release App's
   token → it merges when CI passes → the release publishes itself with attested archives.
8. The agent checks that the release pull request is green (a red release PR silently stops all releases).
9. **After phase 1 only, a numbered step of its own:** on `phase/01-ci-tail`, one budgeted `ci:` PR carries the
   post-merge tail (arming the Ir/frame gate from two zero-spread control runs, promoting the wasm32 lane from
   report-only, the rehearsal filter, re-review leftovers), merged and green before
   `/gsd-discuss-phase 2` starts (LESSONS XFER-065, XFER-104; DEC.99).
10. Optional, about two minutes, outside the agent's sandbox: run the README's install line and load the core in
    RetroArch. This is never recorded as a verification item.

**Verification default (settled 2026-10-19; never re-proposed):** automation first. Deterministic behaviour goes
in local tests; integration, end-to-end, smoke and seam checks go in CI when they catch recurring regressions; a
one-time hosted outcome is collected by a read-only evidence command. Passing results are recorded as command,
exit status and run URL. The owner is asked only for identity-bound consent or a run on their own hardware, with
the reason stated. Judging docs, images and RetroArch's menus is the agent's job, and a truth only a merge can
show is confirmed at ship (step 8), never in UAT.

**Closeout policy:** a fix merged after a phase's verification makes that verification "stale". Accept an
override closeout at milestone end when CI passes on the merged head and the milestone audit covers the phase;
do not spend sessions refreshing digests.

**Cross-project operating lessons** (LESSONS XFER-016, 017, 055): prove a playability claim in one integrated host
session; diagnose a stalled guest at its first fault with the runner's trace; close verification gaps in one
current-revision batch; keep acceptance blockers separate from engineering dependencies.

## 4. Milestone boundaries

On a `milestone/vX.Y-slug` branch, one session each: `/gsd-audit-milestone` → `/gsd-complete-milestone` →
`/gsd-new-milestone`. Within those steps the agent also:
- refreshes `.planning/PRODUCT-ROADMAP.md` (reconcile shipped work, keep unresolved debt, re-rank
  Immediate/Near/Mid/Long, record why) and snapshots it as `milestones/vX.Y-PRODUCT-ROADMAP.md`;
- refreshes `.planning/preparation/PROMPT-COVERAGE.md` (every owner ask still has a home and a status);
- surfaces the seeds whose triggers fired; the tune-up seed (SEED-007) is the first phase of every new milestone;
- offers a lesson export ("export lessons", `.planning/LESSONS.md`).

## 5. Settings and why

`config.json` is written by GSD's own writer (`query config-new-project`) with `GSD_HOME` pointing at an empty
directory, so none of the owner's global defaults leak in (the writer still merges `$GSD_HOME/.gsd/defaults.json`
on 1.18.0, `bin/lib/config.cjs:295, 312`), and read back with `config-get` (section 8) [GH.01].

| Setting | Value | Why |
|---|---|---|
| `mode` | interactive | With yolo, the end of one step can start the next [GH.01] |
| `workflow.auto_advance`, `workflow._auto_chain_active` | false | Stops discuss, plan and execute from chaining |
| `granularity` | coarse | Six phases in v1.0 |
| `parallelization`, `workflow.use_worktrees` | false | A small, coupled codebase; no worktree paths in tracked files |
| `model_profile`, `resolve_model_ids` | inherit, omit | Every GSD subagent uses the session's model; no model IDs in a public file |
| `workflow.research`, `plan_check`, `verifier`, `code_review` | true | GSD's own research and checks before and after every phase |
| `workflow.ui_phase`, `ui_safety_gate`, `ui_review`, `ai_integration_phase`, `api_coverage_gate`, `schema_push_detection`, `schema_drift_gate` | false | Written for web projects; they react to words like "screen" |
| `workflow.security_enforcement` | false | A web threat-model checklist; fuzz targets and sanitizer builds are requirements instead |
| `workflow.nyquist_validation`, `pattern_mapper`, `tdd_mode` | false | GSD's own choice at coarse granularity; no code to map before phase 1 |
| `workflow.test_command` | `cmake --workflow --preset ci` | Without it the test gate runs `true`. It fails until phase 1 creates the preset |
| `workflow.test_gate_timeout` | 900 | Phase 1 replaces it with twice the measured gate time |
| `git.branching_strategy`, `git.create_tag` | none, false | The agent branches before discuss, so planning and code share one pull request; release-please owns tags |
| `git.base_branch`, `git.protected_branches` | main, [main] | Ship targets `main`; execute and ship warn if run on it |
| `hooks.community` | true | Turns on GSD's Conventional Commit check |

Subagent reasoning effort comes from the owner's global GSD agent install, not from this repository.

## 6. What keeps GSD from running on by itself

- GSD commands get only a phase number. An argument containing `--auto` or `--chain` makes one step start the
  next, and GSD tests for the substring (`workflows/execute-phase.md:193`) [GH.01].
- `/gsd-next`, `/gsd-autonomous`, `/gsd-manager` and `/gsd-progress --next` or `--do` start the following step.
- In interactive mode the phase transition only moves STATE's pointer; it does not start the next phase [GH.01].
- `scripts/hygiene.sh` refuses a commit if `.planning/config.json` leaves interactive mode or turns a chain flag
  on. `SCHWAYSTATION_ALLOW_CHAIN=1` and `SCHWAYSTATION_ALLOW_MAIN_COMMIT=1` are bypasses for the owner's own
  shell, never set by an agent (AGENTS rule 9).

## 7. Owner tasks

| When | Task |
|---|---|
| **Before phase 1** | Create the public repository and push the local history (the preparation session's attempt was refused by the agent's permission guard, so this is an owner command): `gh repo create szTheory/schwaystation --public --source . --remote origin --push`, then `git push -u origin gsd/roadmap`, `scripts/apply-repo-settings.sh --no-ruleset` (merge, security, Actions and label settings), and merge the `gsd/roadmap` pull request (`gh pr create --fill --base main --head gsd/roadmap && gh pr merge --squash`). Verify with `gh repo view szTheory/schwaystation`. The required-check ruleset is applied in phase 1 once CI reports (`scripts/apply-repo-settings.sh` without the flag) |
| **Kickoff** | GitHub App `schwaystation-release` (private, webhook off; repository permissions: Contents, Pull requests, Issues and Workflows read/write, Metadata read), installed on this repository only. `scripts/setup-release-app.sh` registers it in one command (manifest flow, then waits for the install and proves the key signs); `scripts/apply-repo-settings.sh --check` reports the variable or either secret as drift while missing and pipes the private key straight into `gh secret set SCHWAYSTATION_APP_PRIVATE_KEY`, once for Actions and once with `--app dependabot`; the key is never shown or saved. `SCHWAYSTATION_APP_CLIENT_ID` is an Actions variable. Agents cannot read key files (GSD's secret-read guard), so these commands are yours. Details: ENGINEERING 13 |
| If the key must change | On the App's settings page generate a new private key, run `gh secret set SCHWAYSTATION_APP_PRIVATE_KEY < <key file>` and again with `--app dependabot`, then delete the old key there and the file |
| Done (present) | RetroArch 1.22.2 is installed in `/Applications`; the `retroarch-metal` cask is the Apple Silicon build the hosted job pins (HOSTS 3). The App Store build cannot load outside cores |
| Any time | Optional `.env.local` (copy `.env.example`): `SCHWAYSTATION_BIOS_DIR` for your own BIOS dumps (recognised by digest, ASSET_POLICY 5) and `SCHWAYSTATION_DISC_DIR` for your own disc dumps in the local-only `library` preset. Results stay on your machine; CI never sees either directory (ASSET_POLICY 4) |
| Each phase close | Optional two-minute smoke (section 3 step 10) |
| v3.0 or sooner | In Playstead: the Mac client's per-system extension rule (it accepts only `.psx` today; PS1 needs `cue`, `m3u`, later `chd`), the `biosRequired` flag and BIOS directory hand-off, CHD import on the server, and which digest a cue set binds to; all Playstead-side decisions (HOSTS 4, 8) |
| Never in this repository | `/gsd-profile-user`: it writes a developer profile into the tracked `.claude/CLAUDE.md` |
| Before going public with docs | Read `PROVENANCE.md`, `ASSET_POLICY.md` and the README once; ad-hoc code signing only (a Developer ID certificate puts a legal name in artifacts) |

## 8. What was checked

Against `@opengsd/gsd-core` 1.18.0 (`VERSION` read 2026-10-19). Every row below was run by the preparation session on 2026-10-19; the results follow the table.

| Check | Status | What it covers |
|---|---|---|
| Config written with empty `GSD_HOME` | done 2026-10-19 | `query config-new-project` with `GSD_HOME` at an empty directory; read back with `config-get`: mode interactive, coarse, inherit/omit, both chain flags false, the test command, branching none, create_tag false, community hooks on; no `preferences`, `effort` or model IDs in the file; diff against cogmaster's config |
| Resume state query | done 2026-10-19 | `query init.new-project` on a copy: `project_exists` true, `init_incomplete` true, `is_brownfield` false, `needs_codebase_map` false |
| Roadmapper dry run 1 | done 2026-10-19 (four-phase layout) | GSD's `gsd-roadmapper` with the kickoff prompt on a copy: six phases named as in SUMMARY; every v1 requirement mapped once, 0 orphaned; the `## v1.0 …` heading recognised by `roadmap analyze` and `init progress`; `validate consistency` clean; `list-seeds` reads the seeds; no `**Mode:**` line; "UI hint: no" and "Canonical refs" on every phase |
| Roadmapper dry run 2 | done 2026-10-19 | Repeated after the fresh-eyes fixes; same assertions |
| Hygiene self-test | done 2026-10-19 | `scripts/hygiene.sh --tree` passes on this tree; `sh -n` and a dash parse are clean. In a scratch repository the planted cases were judged as expected: a file with the `PS-X EXE` signature at offset 0 (rejected by content); a `CD001` identifier at 0x8001 and, in a second file, at 0x9319 (both rejected by content); a 524,288-byte binary (rejected as a BIOS by size, no BIOS byte compared); a 131,072-byte file with the `MC` header (rejected as a memory card); a home-directory path (rejected); an e-mail address (rejected; `@example.com` passed); a `.cue` by name (rejected); the same file listed in `tests/fixtures/manifest.txt` (passed, the manifest exemption); a clean control passed |
| Public pins answer 200 | done 2026-10-19 | Every seed, reference and fixture pin in PROVENANCE 2 answered 200 from `gh api repos/<o>/<r>/commits/<sha>`: ares `9408cb43d494`, pcsx-redux/nugget `9db6d88d23c4`, MAME `96df6a708bdd`, ps1-tests `f727802fead1`, SingleStepTests/r3000 `51e227447d0b`, libchdr `47d794151b2b`, rcheevos `dfdc64ea035c`, libretro-common `a3194c18ba65`, libretro-database `12c21cf79f1e`, psx-spx `6d7d1bc106a7`, psxavenc `bdc24e897ed0`, armips `62adab4ef30d`, PSn00bSDK `5d9aa2d3dfc7`, DuckStation `124a4b16feda`, cogmaster `abd950941e93`, muhtari `d698364b180d` (v0.2.0; the local `ecbe7f52aa37` returned 422 and is never a pin); mkpsxiso `b6f9a25c8a96` is the v2.30 tag object whose commit is `54fb1644ed87`; the armips and mkpsxiso tarball digests were recomputed (PSXDEV-EXTRACTS 4) |
| Fact-check by a fresh agent | done 2026-10-19 | Load-bearing claims re-fetched from primary sources: clocks and rationals, line and frame counts, DMA and timer rules, BIOS digests, GTE cycle table, CD timings, SPU constants, every licence at its pin, rcheevos IDs, libretro BIOS names, RetroArch DMG digest, action SHAs and runner images, GSD behaviours, sibling-lesson attributions; corrections applied in place and listed here |
| Fresh-eyes review of the files GSD reads | done 2026-10-19 | PROJECT, REQUIREMENTS, SUMMARY, AGENTS: nothing needs a published release or the owner's hardware; no requirement reads as a study; the gate budget is absolute; no sentence can be read as chaining; findings applied and listed here |
| Commits | done 2026-10-19 | No remote existed during preparation, so the scaffold and the preparation pack are two local commits on `main` (the pre-commit hook allows `main` only while no remote exists) and the kickoff is on the local branch `gsd/roadmap`; from the first push on, every change goes through a pull request (DEC.105) |
| Not checked | n/a | A complete `/gsd-new-project` session (only its init query and its roadmapper, on copies); Codex; anything on real hardware |

**Results.**
- **Config.** GSD 1.18.0's writer (`query config-new-project`) with `GSD_HOME` at an empty directory, given cogmaster's full value set, produced a file byte-identical to cogmaster's `config.json`; `config-get mode` reads `interactive`, the test command `cmake --workflow --preset ci`; the empty `GSD_HOME` stayed empty. Fed only the workflow's own answer set, the writer's hard-coded defaults differ in about ten keys, so always pass the full set.
- **Resume state.** `query init.new-project` on a git copy: `project_exists` true, `init_incomplete` true, `is_brownfield` false, `needs_codebase_map` false, `requirements_exists` true.
- **Dry run 1** (four phases, 28 requirements): 28/28 mapped; `validate consistency` passed with no warning; it reported phase 2 and phase 4 as each holding too much, which the fresh-eyes review confirmed.
- **Dry run 2** (six phases, 30 requirements): 30/30 mapped once (9/5/3/7/2/4), 0 orphaned; `validate consistency` passed with no error or warning; `roadmap analyze` recognised `v1.0 PlayStation games play with sound in RetroArch` and six phases with no mode; `list-seeds` read eight seeds; "UI hint: no" and "Canonical refs" on all six phases; no `**Mode:**` line. It noted that the roadmapper's default for coarse is two to four phases; six is kept because each category is one release.
- **Fact-check.** 54 claims re-fetched: 47 held, 5 corrected in place (pcsx-redux is GPL-2 at `91285bf736af` while its nugget submodule is MIT; the sector-4 licence text is documented by psx-spx; three Redump counts), 2 not reproduced (a demo-disc count, marked unverified; a sibling commit count, replaced by the measured 212). Earlier in preparation, research agents found Avocado and Rustation are GPL, not MIT, and DuckStation is CC BY-NC-ND 4.0 at its pin; BRIEF and PROVENANCE carry the corrections.
- **Fresh-eyes review.** 24 findings. Applied: the v1.0 layout went from four phases to six (the GTE, DMA and timers left the CPU phase; sound and discs became separate phases); armips moved from phase 1 to phase 2; the gate budget became an absolute 1,500 lines in phase 1; seed gates are marked as seeds; `wasm32` alone is report-only until the `ci:` tail; nightly lanes are dispatchable on the phase branch; the rcheevos CTest moved to the discs phase; the audio-capture question was settled (DEC.108: hosted sessions prove video, the in-process host proves audio), so phase 5 is named for what it proves; AGENTS rule 2 allows one untested example host; rule 7's quota became an evidence rule; three PROMPT-COVERAGE rows became Partial. Noted, not changed: the root `CLAUDE.md` is a symlink to `AGENTS.md`, as in cogmaster.
- **Reconciliation.** Fifteen cross-file conflicts (CHD in v1, line length and reported rates, samples per frame, cards inside states, 24-bit encoding, exit statuses, mailbox, symbol prefixes, stop reasons, fuzz names, manifest path) were settled in ARCHITECTURE 13 and applied to every file.

## Sources

| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| GH.01 | T1 | `@opengsd/gsd-core` installed package: `workflows/new-project.md` (interrupted-bootstrap resume at L99), `workflows/ship.md` (PR title L376, `[ci skip]` ship note L494, empty `chore: trigger CI` recovery L520–524), `workflows/execute-phase.md` (`--auto` substring L193, verifier re-run on a missing or stale report L394–397), `workflows/verify-work.md` (gap reconciliation L507), `bin/lib/config.cjs` (`GSD_HOME` defaults merge L295, 312), `bin/lib/config-loader.cjs` (shadow warning only, L933–941), `bin/lib/profile-output.cjs` (marker keys L245–247, default `.claude/CLAUDE.md` L964), `workflows/transition.md` | 1.18.0, read 2026-10-19 (tested; LESSONS XFER-096 to 098) | Resume path, chaining, branching, config, ship behaviour, verifier re-run, instruction file |
| GH.02 | T1 | nesturbator `.planning/preparation/GSD-HANDOFF.md` (version-pin warning, ship-step detail, signing note) | `9f9386c661f6` (200, 2026-10-19) | Routine details |
| GH.03 | T1 | muhtari `.planning/preparation/GSD-HANDOFF.md`, `.planning/LESSONS.md`, todos | `952fce970117`; `d698364b180d` (both 200, 2026-10-19) | Kickoff text, settings table, owner tasks, the `ci:` tail |
| GH.04 | T1 | cogmaster `.planning/preparation/GSD-HANDOFF.md`, `scripts/setup-release-app.sh`, `scripts/apply-repo-settings.sh` | `abd950941e93` (200, 2026-10-19) | Section structure, release App flow, checklist of section 8 |
