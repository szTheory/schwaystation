---
title: "Lesson exchange"
summary: "Lessons imported from the owner's sibling emulators (hypotheses until reproduced here) and the template for exporting this project's lessons."
read_when: "At every discuss step, at each phase or milestone boundary, when a sibling lesson might apply, or when the owner asks to export lessons."
updated: 2026-10-19
---

# Lesson exchange

Rules that came from these lessons live in `AGENTS.md` and `.planning/preparation/DECISIONS.md`, because agents
read those files; this ledger records where each rule came from and whether it has been reproduced here.
AGENTS.md says to re-read this file at every discuss step: a sibling planning on the same day as an export still
missed one (XFER-059), and a third sibling repeated XFER-005 a week later (XFER-100), so reading at kickoff alone
is not enough.

## Protocol

- **Inbound.** A sibling's lesson is a hypothesis until its mechanism is shown to apply here. Record the source
  project and revision, whether it was implemented or only planned there, how this project differs, and the
  local decision. Supersede entries; do not erase them.
- **Outbound.** When the owner says "export lessons", produce one pasteable block per lesson in the template
  below. Exclude paths, identities, private disc names and hashes, BIOS bytes, credentials and transcripts. Never
  imply that another console shares a PlayStation timing behaviour.
- **IDs.** `SW-<AREA>-<NNN>` for lessons learned here (areas: GSD, CI, REL, TEST, ARCH, PERF, AUDIO, VIDEO, HOST,
  LEGAL, DOCS); `XFER-NNN` for imports. IDs are unique; a test (`policy.lessons_unique`) rejects duplicates,
  because a sibling's ledger reused two IDs (XFER-069).
- **Status vocabulary.** observed → reproduced → adopted → superseded (or rejected, with the reason).
- **How this project differs.** The CPU is a MIPS R3000A with a GTE coprocessor, not an 8-bit core; a BIOS is
  required by the hardware and never enters the repository; fixtures are PS-EXE files or disc images, not
  cartridges; the host surface (GPU, SPU, CD-ROM, MDEC) is several times larger than a sibling's.

## Template

```
Lesson <ID> (<date>, <project>@<revision>)
Symptom and reproduction: …
Root cause / invariant: …
Fix and evidence (command, exit status, run URL): …
Applies when: …
Does not establish: …
Suggested check: …
Sources: …
Status: observed | reproduced | adopted | superseded
```

## Imported lessons, first batch (2026-10-19, preparation; as imported by muhtari on 2026-10-15)

These 33 were imported by muhtari from nesturbator, gabbaboy, glueyneo, playstead, lattice_stripe and exifcleaner,
and re-imported by cogmaster on 2026-10-18. muhtari `.planning/LESSONS.md` at `952fce9701175c` carries the
original evidence columns; cogmaster `.planning/LESSONS.md` at `abd950941e93` carries the dispositions re-judged
here for a PlayStation core.

| ID | Lesson | Source | Local disposition |
|---|---|---|---|
| XFER-001 | Pre-write every artifact GSD would ask for and dry-run the roadmapper on a copy; the kickoff then resumes at roadmap creation. | nesturbator | adopted (this preparation; GSD-HANDOFF) |
| XFER-002 | Phase 1 is a content-free walking skeleton that ships the whole release pipeline; every later phase ends in a release by merging. | nesturbator | adopted (v1.0 phase 1 boots no BIOS and no disc: a built-in test frame) |
| XFER-003 | Requirements written as the command that proves them make verification mechanical. | nesturbator | adopted (REQUIREMENTS) |
| XFER-004 | Verification is automation-first; the owner is asked only for consent or irreducible judgement. | nesturbator; gabbaboy | adopted (AGENTS rule 6) |
| XFER-005 | Implement the whole CPU against exhaustive public vectors before any game; never grow it opcode by opcode from a game trace. | glueyneo | adopted (v1.0 phase 2: the R3000A against SingleStepTests/r3000 at `51e227447d0b`; phase 3: the GTE against the ps1-tests table) |
| XFER-006 | Governance machinery must stay proportional; per-file rights digests, prose validators and threat registers consumed a sibling's milestone. | glueyneo | adopted (AGENTS rule 7) |
| XFER-007 | Review warnings about test code that grow into requirements stalled a milestone; keep real-host e2e to a screenshot compare and a save round trip. | nesturbator | adopted, strengthened (AGENTS rule 7: ≤ 300 lines, tested; see XFER-044) |
| XFER-008 | A release PR opened with the default Actions token needs maintainer approval and blocks; mint a GitHub App token for every bot merge. | gabbaboy | adopted (ENGINEERING; owner task) |
| XFER-009 | Aligned machine-edited version comments broke formatting and silently blocked releases; keep machine-edited lines out of aligned blocks and check the release PR at every phase close. | nesturbator | adopted (`version.txt` only) |
| XFER-010 | Merges after a phase's verification make it stale and force override closeouts; decide the closeout policy up front. | nesturbator | adopted (GSD-HANDOFF) |
| XFER-011 | Tests that skip themselves hid a broken GUI path; RetroArch aborts inside the agent sandbox, so the hosted job is the evidence. | nesturbator | adopted (no self-skip; hosted e2e) |
| XFER-012 | Timeouts guessed instead of measured cancelled green runs; use twice the slowest recent run with its run ID. | nesturbator | adopted (ENGINEERING) |
| XFER-013 | A CTest regex with `--no-tests=error` does not prove each named test ran; keep an exact executed-test inventory and make negative controls assert the specific failure. | gabbaboy | adopted (ENGINEERING) |
| XFER-014 | A globally ignored ROM extension silently dropped new fixtures; exempt the fixture directory and count tracked files. | gabbaboy | adopted (`.bin`, `.cue`, `.iso`, `.exe` ignored globally; `tests/fixtures/` exempt and counted) |
| XFER-015 | Agents read AGENTS.md and the plan, not the lessons file; a lesson becomes a control only as a rule plus a failing check. | gabbaboy | adopted (this file's preamble; AGENTS) |
| XFER-016 | Prove playability with one integrated host session: boot, a control that changes play, moving picture, audible sound. | nesturbator, gabbaboy | adopted (v1.0 phase 3 and 4) |
| XFER-017 | Diagnose a stalled guest at its first divergence with a bounded trace; more cycles prove nothing. | nesturbator | adopted, strengthened (first-fault capture in the runner from phase 1; XFER-055) |
| XFER-018 | Confidence-qualified models: implement the best-supported model, label what is unmeasured, stop gap loops that lack new evidence. | gabbaboy | adopted (DECISIONS) |
| XFER-019 | A repository's licence is not the licence of every byte in a built ROM (SDK runtime, fonts, compiler libraries). | genewhip | adopted (ASSET_POLICY: a PS-EXE built with PSn00bSDK carries MPL-2.0 runtime bytes; fixtures are pure assembly or carry notices) |
| XFER-020 | A cited licence was wrong at the pinned commit; check the LICENSE at the exact pin. | nesturbator | adopted (PROVENANCE) |
| XFER-021 | CI that runs on every branch push and on the pull request doubles cost; push triggers only `main`. | gabbaboy | adopted (ENGINEERING) |
| XFER-022 | A README that doubles as the CI manual grew to 114 KB and buried the install line; split user and maintainer docs with budgets. | nesturbator | adopted (AGENTS rule 5) |
| XFER-023 | GSD's config writer merges the owner's global defaults; write config with an empty `GSD_HOME`. | GSD 1.17.0 | adopted; re-checked on 1.18.0 (tested, XFER-097) |
| XFER-024 | GSD's ship step pushes a CI-skip commit to the PR branch; squash messages must come from the PR title and body. | GSD 1.17.0 | adopted; re-checked on 1.18.0 (tested, XFER-096) |
| XFER-025 | Homebrew whose licence is unclear makes a poor CI fixture; self-authored test programs, designed so the oracle cannot be fooled, were cheaper and safer; a gate that exists but never executes is the real hazard. | playstead | adopted (CONFORMANCE: self-authored PS-EXE fixtures with a RAM mailbox) |
| XFER-026 | A newly queued run cancels the pending one in its concurrency group even with `cancel-in-progress: false`; CI must build release-please's `sha`. | lattice_stripe | adopted (ENGINEERING) |
| XFER-027 | A publish job must be safe to re-run: `GET releases/tags/{tag}` omits drafts; a full re-run skips publish once the release PR is tagged. | lattice_stripe, exifcleaner | adopted, strengthened (XFER-039) |
| XFER-028 | A scheduled reporter that tested a piped `$?` under `bash -e` without `pipefail` stayed green while its report listed failures. | lattice_stripe | adopted (ENGINEERING) |
| XFER-029 | Pipeline fixes titled `fix(release):` cut ten patch releases in one day; a title lint outside the required check let bad scopes reach `main`. | lattice_stripe | adopted (`scripts/check-title.sh` in the required check) |
| XFER-030 | Check each release as a user would: download from `releases/latest/download/`, verify `SHA256SUMS`, pin the attestation to the release workflow. | lattice_stripe, exifcleaner | adopted (ENGINEERING) |
| XFER-031 | A live setting that no check covered made every Dependabot auto-merge run fail; a drift check must cover every setting it applies. | lattice_stripe | adopted (`apply-repo-settings.sh --check`) |
| XFER-032 | GitHub Actions updates are mostly majors, so a Dependabot group that includes majors never qualifies for patch/minor auto-merge. | lattice_stripe | adopted (ENGINEERING) |
| XFER-033 | An executable-only phase-completion gate grew to about 3,400 lines with its verifier and tests. | exifcleaner | rejected: AGENTS rules 6–8 keep the policy without the machinery |

## Imported lessons, second batch (2026-10-19, preparation; muhtari's first week and the older siblings, as imported by cogmaster on 2026-10-18)

| ID | Lesson | Source | Local disposition |
|---|---|---|---|
| XFER-034 | GSD's config writer merges `$GSD_HOME/.gsd/defaults.json`; a global yolo mode and web gates leak into a public `config.json`. Write it with `GSD_HOME` pointing at an empty directory. | muhtari MU-GSD-001 | adopted; still true on 1.18.0 (tested, XFER-097) |
| XFER-035 | `/gsd-ship` titles PRs "Phase N: name" and pushes a `[ci skip]` commit; squash title = PR title, body = PR body; retitle as a Conventional Commit. | muhtari MU-GSD-002 | adopted; still true on 1.18.0 (tested, XFER-096) |
| XFER-036 | `/gsd-verify-work` after a passing execute is redundant and can start gap planning; run it only for `human_needed`. | muhtari MU-GSD-003 | adopted (GSD-HANDOFF) |
| XFER-037 | `branching_strategy: phase` forks only at execute; branch before the first committing step and let a pre-commit hook refuse `main`. | muhtari MU-GSD-006 | adopted (AGENTS rule 10; hygiene) |
| XFER-038 | Dry-run the roadmap step on a copy before the real kickoff. | muhtari MU-GSD-007 | adopted (GSD-HANDOFF) |
| XFER-039 | A fresh draft release is missing from `gh release list` and lookups read only 100 releases; address drafts by release ID with `gh api`, use `--paginate`. | muhtari phase 1 | adopted (ENGINEERING) |
| XFER-040 | release-please 17.6.0: `initial-version` applies only while the manifest says `0.0.0`; `feat` bumps minor before 1.0 only with `bump-patch-for-minor-pre-major: false`; use a `Release-As:` footer. | muhtari MU-REL-001 | adopted (ENGINEERING) |
| XFER-041 | Events caused by `GITHUB_TOKEN` start no workflow runs; Dependabot-triggered runs see only Dependabot secrets; store the App key as a Dependabot secret too. | muhtari MU-REL-002 | adopted (owner task) |
| XFER-042 | A `.planning`-only PR ran the whole matrix; skip heavy jobs with a `changes` job while the aggregator still reports; never a trigger-level `paths` filter. | muhtari MU-CI-002 | adopted (ENGINEERING) |
| XFER-043 | Repository settings drift from their docs; keep settings as a script with a `--check` mode run at each tune-up. | muhtari MU-CI-003 | adopted (`scripts/apply-repo-settings.sh`) |
| XFER-044 | A real-host e2e phase added about 10,250 test lines and changed nothing under `src/`. The ≤ 300-line rule needs a CI check, not a sentence. | nesturbator phase 16 | adopted (AGENTS rule 7; a CTest counts `tests/retroarch/` lines) |
| XFER-045 | Agents invent attempt quotas, "formal slots" and once-only ceremonies to stop loops, and the quotas become the work. | glueyneo METHODOLOGY; nesturbator STATE | adopted (AGENTS rule 8; see XFER-099 for the worst case) |
| XFER-046 | A requirement that needs a *published* release cannot pass before merge and kept a phase open with a "publication gap" plan. | nesturbator EVID-04 | adopted (confirmed at ship instead; XFER-092) |
| XFER-047 | A milestone scoped as evidence plus governance shipped no release in five days; every milestone names a behaviour a player can see. | nesturbator v4 | adopted (PRODUCT-ROADMAP) |
| XFER-048 | Eight phases merged without a release because release was deferred to a milestone-end phase; the owner stopped it by hand. | gabbaboy v0.2 phase 15 | adopted (AGENTS rule 10: every phase releases) |
| XFER-049 | A fuzz *harness* assertion blocked a release; a failing harness is a test bug to triage; fuzz targets are report-only until green for N runs. | gabbaboy `tests/fuzz_core.c` | adopted (AGENTS rule 8; ENGINEERING); reproduced again at gabbaboy's phase 15 pause (XFER-102) |
| XFER-050 | No rights-clear game fixture was ever admitted for a hardware milestone, so 128 plans produced no game evidence; choose or author the fixture before the milestone starts. | gabbaboy D-101 | adopted (CONFORMANCE: PS-EXE fixtures named at kickoff; the hardware needs a BIOS, so the BIOS-free boot path and the BIOS the owner supplies locally are both decided before phase 3) |
| XFER-051 | Perf regressions were found only at milestone end; arm the per-PR instruction-count gate in phase 1–2. | gabbaboy 15-05; nesturbator v2 | adopted (ENGINEERING; DECISIONS); reproduced at gabbaboy 15-05 (XFER-102) |
| XFER-052 | Release qualification that required a first-time fork contributor and an independent approver halted a workstream for a solo maintainer. | glueyneo workstream 1 | adopted (AGENTS rule 8); that workstream is still halted since 2026-10-07 |
| XFER-053 | On OpenGSD 1.15 a stale `gaps_found` report routed back to gap planning without re-verifying; run one fresh verifier before any gaps plan. | glueyneo 2026-10-04 note | observed; 1.18.0 re-runs the verifier on a stale report (tested, XFER-098); keep the rule until the first `gaps_found` here |
| XFER-054 | A permission denial was treated as a verification result; write evidence from the command that produced it, from the repository root, with no `cd` chains. | glueyneo; muhtari 01-01 | adopted (AGENTS rule 11) |
| XFER-055 | Tooling grew to 1.4× the emulator and no game booted; the last fault's address was never captured. Put first-fault capture (PC, address, width, device) in the runner from phase 1. | glueyneo | adopted (ARCHITECTURE; HOSTS) |
| XFER-056 | 8 of 13 phase-1 plans were fixture and toolchain provenance (patched MAME builds, an expired signing key halting Binutils). Pin the assembler tarball by SHA-256; keep signature chains off the critical path. | genewhip phase 1; gabbaboy | adopted (ENGINEERING: the mipsel toolchain and PSn00bSDK pinned by SHA-256); genewhip still has two halted phase-1 plans (XFER-101) |
| XFER-057 | Preparation pinned a sibling commit that existed only on an unpushed branch; six plans re-fetched seeds. Pin only commits the public API answers 200 for; fetch with `gh api`, never from a local checkout. | muhtari phase 1 | adopted (AGENTS rule 12) |
| XFER-058 | Plan acceptance text hard-coded tool output that changed (CTest 4 summary line, CMake 4.4 ConfigVersion text). Accept on exit status and tolerant regexes. | muhtari phase 1 | adopted (AGENTS "GSD summaries"; ENGINEERING) |
| XFER-059 | A sibling planned to grow its CPU by instruction family from ROM demands two weeks after XFER-005 was exported; lessons were not re-read at discuss. | genewhip 02-CONTEXT D-32 | adopted (AGENTS preamble: re-read at every discuss) |
| XFER-060 | CMake 4 leaves `CMAKE_OSX_SYSROOT` empty so Homebrew clang-tidy misses `string.h`; Apple clang lowers `memset` to `_bzero`, breaking a freestanding allowlist. | muhtari 01-11, 01-01 | adopted (ENGINEERING) |
| XFER-061 | `file(WRITE)` writes CRLF on Windows so hash files differed with equal values; use `file(CONFIGURE … NEWLINE_STYLE UNIX)`. | muhtari 01-14 | adopted (ENGINEERING) |
| XFER-062 | A script that exits on first match under `pipefail` kills the upstream `gh api` with SIGPIPE; read all input, then decide. | muhtari 01-14, 01-15 | adopted (ENGINEERING) |
| XFER-063 | `[skip ci]` in a PR *title* becomes the squash subject and suppresses CI on `main`; the title check scans title and body. | muhtari 01-15 | adopted (`scripts/check-title.sh`) |
| XFER-064 | A code-review fix round after verification changed 25 files including the release workflow; the rehearsal predated the fixes. Review before verification; rehearsal re-runs on release-input changes and joins `CI required`. | muhtari 01-VERIFICATION | adopted (AGENTS rule 7; ENGINEERING); cogmaster moved the rehearsal into `ci.yml` |
| XFER-065 | Phase-1 leaves a tail of CI follow-ups (arming the perf gate needs post-merge evidence; promoting a report-only lane). Budget one `ci:` PR right after the first merge. | muhtari todos | adopted, strengthened (XFER-104: the tail PR is a numbered step before phase 2 discuss) |
| XFER-066 | A CPU phase planned as 16 strictly sequential waves; opcode groups and devices that do not share an inventory can run in parallel waves. | muhtari phase 2 plans | adopted, refined (XFER-083: plan 1 lands the whole CMake skeleton, then CPU opcode classes, COP0 and GTE run in parallel with per-directory inventories) |
| XFER-067 | Native failures came from the shell and the host, not the emulator (dash noclobber exit codes, timing bounds on ASan hosts); no stateful shell harness around RetroArch, and CI shell scripts run under dash. | nesturbator 16-22 | adopted (ENGINEERING; HOSTS) |
| XFER-068 | Owner attention is the scarce resource across five parallel projects: a sibling sat 14 days at a phase boundary. Keep phases short and self-contained, each a release. | genewhip log | adopted (six short phases in v1.0) |
| XFER-069 | A lesson ledger reused IDs; keep a uniqueness check. | gabbaboy LESSONS | adopted (this file's protocol) |
| XFER-070 | Instance-owned machine on an integer master timeline with device deadlines; bounded stepping with stop reasons; RTC/time injected by the host and a CTest proving no host clock; every claim scoped to build, model and corpus; suites declare their exclusion count; deferred registers fail closed. | gabbaboy D-009…D-073 | adopted (ARCHITECTURE: the PS1 has no RTC, but the CD-ROM's seek and read timing is a device deadline, never a host clock) |
| XFER-071 | Under PIC a file-scope `const` table of function pointers lands in `.data.rel.ro` and fails a no-mutable-globals check; dispatch with `switch`. | muhtari MU-ARCH-001 | adopted (DECISIONS: MIPS opcode and GTE dispatch by `switch`) |
| XFER-072 | A permissive licence today does not clear a repository's history or its embedded firmware; audit licence history and per-file editors at the pin. | muhtari MU-LEGAL-001 | adopted (PROVENANCE: DuckStation's relicence and any repository shipping a BIOS are facts-only) |
| XFER-073 | The `theft` property-testing library needs `fork`; a ~60-line seeded splitmix64 harness covers the need on every platform. | muhtari MU-TEST-001 | adopted (ENGINEERING) |
| XFER-074 | GSD's secret-read guard blocks agents from `.env.local` and key files; secret setup is an owner command. | muhtari MU-GSD-008 | adopted (GSD-HANDOFF) |
| XFER-075 | GitHub's App manifest flow makes the release App one owner command; a helper pipes the key into `gh secret set` for Actions and Dependabot. | muhtari MU-REL-004; cogmaster `scripts/setup-release-app.sh` | adopted (GSD-HANDOFF) |
| XFER-076 | A hygiene rule that scans for a header signature must not contain the signature itself as text, or every copy of the script matches. | muhtari hygiene | adopted (`scripts/hygiene.sh` compares the PS-EXE magic and the Sony licence string as hex) |
| XFER-077 | Hosted ASan outgrew job and per-test timeouts; scale sanitizer timeouts in one pass after registration, per CMake directory. | gabbaboy GB-CI-004, GB-CI-005 | adopted (ENGINEERING: `policy.timeout_scaled`; a PS1 frame is heavier, so measure early); seed gate (SEED-007): not phase-1 work unless a requirement names it |
| XFER-078 | CMake embeds the git revision at configure time; reconfigure after committing or the build reports a stale revision. | gabbaboy GB-GSD-020 | adopted (FRAME-05: `build.revision_matches_head`) |
| XFER-079 | A failing negative control leaks under LSan, which replaces its exit code; run controls with leak detection off and free before failing. | gabbaboy GB-CGB-002 | adopted (ENGINEERING: `expect_fail.cmake`) |

## Imported lessons, third batch (2026-10-19, preparation; new since 2026-10-15)

Read this session: muhtari at `ecbe7f52aa37` (0.2.0 published by `ce8a6174ec03` on 2026-10-19), cogmaster at
`abd950941e93` (phase 1 plans 01-08 of 12 done on 2026-10-19), nesturbator `41bb1f170eb6`, gabbaboy
`954ca111c7e6`, glueyneo `a18cc171e831`, genewhip `f0230d095d8b`, playstead `f7a5c9ba293c`, and the installed
GSD 1.18.0 package (`VERSION` reads `1.18.0`; the 1.17.0 package is not installed, so rows XFER-096 to 098
record what 1.18.0 does, not a diff).

| ID | Lesson | Source | Local disposition |
|---|---|---|---|
| XFER-080 | CPU vector harness: a committed per-opcode sample (one chunk per opcode) runs on every PR lane; the full public set is fetched nightly at a pin with a SHA; the harness exits 1 on a zero-test run and refuses a chunk with count 0 naming the offset. | muhtari 02-01…02-03, 02-14 @`ce8a6174ec03` | adopted (phase 2: a sample per MIPS opcode class on every lane; the full set or the own-EXE suite nightly) |
| XFER-081 | Vectors localise faults: 223 of 256 opcodes passed before decimal mode and the 33 failures were exactly the decimal forms; after the fix no other opcode needed a change, and the full 2,560,000 tests passed on every leg. | muhtari 02-02 | adopted (expect the same for load-delay, branch-delay and exception classes: pin each as a labelled vector class) |
| XFER-082 | GitHub sends nothing for 75–80 s while packing a vector repository's blobs; `--filter=blob:none` makes the checkout the slow step, so `GIT_HTTP_LOW_SPEED_*` is set on fetch and checkout; measured fetch TIMEOUT 220 s, job 8 min. | muhtari 02-14, 02-16 | adopted (ENGINEERING: nightly fetch rule) |
| XFER-083 | Phase 2 (16 plans) ran sequentially with `parallelization: false`; the research graph showed the CPU and the header/probe plans independent, with `CMakeLists.txt`, `internal.h` and the inventories as merge hotspots. cogmaster kept phase 1 sequential because `tests/inventory/ci.txt` is touched by nearly every plan. | muhtari 02-RESEARCH; cogmaster 01-RESEARCH | adopted (plan 1 lands the whole CMake skeleton and per-directory inventories; later waves run in parallel only where no inventory is shared) |
| XFER-084 | Measuring timeouts at phase close cost a 33-minute plan: per-test times came from job logs (`gh api …/logs --allow-escape-sequences`) because CI uploaded no JUnit, and dispatched runs on one commit share a concurrency group, so samples came one at a time. | muhtari 02-16 | adopted (every lane uploads its JUnit as an artifact; a `-P` script computes twice-the-slowest from them); seed gate (SEED-007): not phase-1 work unless a requirement names it |
| XFER-085 | Phase 2 took three code reviews and two fix rounds in one day; the rule "at most two fix rounds" moved the leftovers (WR-05, IN-04, IN-05) into a todo bound to the next phase's first caller, and the verifier re-ran after each fix round (fix 20:14Z, verify 20:24Z). | muhtari 02-REVIEW-DISPOSITION, 02-VERIFICATION | adopted (AGENTS rule 7 cap; one todo per deferred finding, named by the plan that will close it) |
| XFER-086 | A gate-code budget written as "a tenth of `src/`" was exceeded 2.1× to 7.4× by phase 3 depending on what counts, because gates are front-loaded while `src/` is small; no phase's plans could meet it. | muhtari 03-RESEARCH "Gate-Code Budget" | adopted, changed (AGENTS rule 7 here: an absolute line budget per phase plus a ratchet, measured at ship) |
| XFER-087 | A contract on an internal entry point with no production caller (a bus request returning 1 or 0) over-promised and drew a third review; settle such contracts in the phase that adds the first caller. | muhtari 02 WR-05 | adopted (internal APIs are documented when their first caller lands) |
| XFER-088 | MSVC rejects `(uint8_t)~CONST` as C4310; x64 references `__ImageBase` for a switch jump table once the TU joins an archive; ARM64 exports a string literal as an external COMDAT, so core string constants are `static const` byte arrays; clang-format 21 reflows a one-line header. | muhtari 02-05, 02-06, 02-09, 02-08 | adopted (ENGINEERING `abi.undefined` allowlist; hygiene format pin 21) |
| XFER-089 | `file(DOWNLOAD file://… RANGE_START n)` is the one CMake call that copies a byte range containing NULs; a scan makes its temporary directory outside every archive's directory. | muhtari 02-15 | adopted (the 2,048-byte PS-EXE header is stripped the same way in `-P` scripts) |
| XFER-090 | Discuss ran four advisor agents in parallel at calibration tier `minimal_decisive` from the owner's fan-out brief, reconciled them, and asked the owner only two questions (a public API addition and a requirement that could not pass as written). | muhtari 02-CONTEXT, 03-CONTEXT | adopted (GSD-HANDOFF discuss protocol) |
| XFER-091 | The public API grows consumer-minimal per phase: API version N adds only names with a phase consumer, `tests/api/vN.c` touches every new name, and the header comment reads the macro rather than a literal so it cannot drift. | muhtari 03-CONTEXT D-01, D-02; cogmaster 01-06 | adopted (ARCHITECTURE API policy) |
| XFER-092 | Ship mechanics that held twice: a probe PR is closed unmerged before `/gsd-ship`; `bump-minor-pre-major: true` plus a `feat:` squash cuts 0.N.0; the "merge publishes 0.N.0" criterion is recorded `pending-ship` and set to pass after the release run. | muhtari 01-21, 02-16, `1a4a8faa303b` | adopted (REQUIREMENTS wording; GSD-HANDOFF ship step) |
| XFER-093 | cogmaster 01-08 took 45 min against 4–13 min for its siblings because the format gate arrived in plan 8 and reformatted five files from earlier plans; `check-title.sh` also matched a non-ASCII letter in the caller's locale. | cogmaster 01-08 @`c54238292af3` | adopted (hygiene preset and format gate in plan 1; scripts set `LC_ALL=C`) |
| XFER-094 | cogmaster phase 1 is a port of muhtari at its 0.1.0 pin: 126 files fetched with `gh api`, 8 plans in one day at 4–13 min each; STATE records runtime and model per GSD step. | cogmaster 01-RESEARCH, STATE @`abd950941e93` | adopted (phase 1 ports cogmaster at its phase-1 release pin; STATE keeps the runtime log) |
| XFER-095 | A preparation pack that lists gates no requirement names invites them all into phase 1 and repeats XFER-006 and 044; each such gate is a seed unless a requirement names it. | cogmaster 01-RESEARCH pitfalls | adopted (ENGINEERING marks seed gates; REQUIREMENTS names the phase-1 gates) |
| XFER-096 | GSD 1.18.0 ship: the PR is still titled `Phase N: name` (`workflows/ship.md:376`), the ship-note commit still carries `[ci skip]` (`:494`), and when that leaves the PR BLOCKED with zero checks the step pushes an empty `chore: trigger CI` commit (`:520–524`); two extra commits land on the branch. | GSD 1.18.0 (tested) | adopted (squash from PR title and body, XFER-035, now matters twice) |
| XFER-097 | GSD 1.18.0 config: the writer still merges `$GSD_HOME/.gsd/defaults.json` (`bin/lib/config.cjs:295, 312`); the loader now warns on stderr when a project config shadows the global file (`bin/lib/config-loader.cjs:933–941`, #3532) but changes no precedence; `--auto` is still a substring match (`workflows/execute-phase.md:193`); the instruction-file generator still keys on `<!-- GSD:…-start -->` markers and defaults to `.claude/CLAUDE.md` (`bin/lib/profile-output.cjs:245–247, 964`); an interrupted kickoff resumes at the first missing artifact (`workflows/new-project.md:99`, #4040). | GSD 1.18.0 (tested) | adopted (XFER-001, 023, 034 and MU-GSD-004/005 hold unchanged) |
| XFER-098 | GSD 1.18.0 execute-phase re-runs the verifier when the report is missing or stale (`workflows/execute-phase.md:394–397`, #2868, #4682), and verify-work marks gaps resolved when a gap plan with a SUMMARY names them (`workflows/verify-work.md:507`, #1921). | GSD 1.18.0 (tested) | observed (XFER-053's stale route looks addressed; confirm at the first `gaps_found`) |
| XFER-099 | nesturbator plan 16-22 accepted 0 of 3 tasks in four days: it metered itself with a 3,600,000 ms monotonic "allowance", "formal slots" and a boot-identity `sysctl` the sandbox denies; a reboot made the clock incomparable; fourteen checkpoint and amendment documents followed and no source changed since 2026-10-16. | nesturbator STATE @`41bb1f170eb6` | adopted (AGENTS rule 8 extended: no self-metering clocks or privileged OS queries in plans; a plan whose evidence depends on a value the sandbox cannot read is re-planned, not amended) |
| XFER-100 | glueyneo 05-02 and 05-03: 212 commits tagged 05-02 and 05-03 since 2026-10-14 adding 68000 forms one addressing mode at a time from the guest's demands (MOVE.B indirect, CMPA immediate, CLR.L memory), estimate confidence `low`; the third sibling to repeat XFER-005 after it was exported. | glueyneo @`a18cc171e831` | adopted (the R3000A and the GTE are finished against vectors before any EXE; the discuss step quotes XFER-005 and 100) |
| XFER-101 | genewhip: 30 plans over two phases at 35 min average and up to 103 min per plan, two phase-1 plans halted on toolchain provenance, no remote, no tag, no release, and a "non-gating" smoke test. | genewhip STATE @`f0230d095d8b` | adopted (a remote and a release before phase 2; a plan past 30 min is split; no non-gating tests) |
| XFER-102 | gabbaboy v0.2 phase 15 was paused by the owner at 4 of 10 plans after a fuzz-harness assertion blocked the campaign and a 23.96 % slowdown against v0.1.0 surfaced at milestone-end qualification; the retrospective adds: budget a freshness refresh when shared files change late, give the roadmap a `## vX.Y` heading, record model mix per session. | gabbaboy STATE, RETROSPECTIVE @`954ca111c7e6` | adopted (perf gate armed in phase 1–2; roadmap heading; runtime log) |
| XFER-103 | playstead's main-push CI was suppressed by historical squash markers until repository squash defaults used the PR title and body: the third project to reproduce XFER-024/035. | playstead STATE 2026-10-15 | adopted (`apply-repo-settings.sh` sets squash defaults at kickoff) |
| XFER-104 | muhtari's phase-1 `ci:` tail (arm the perf gate, promote wasm32, rehearsal filter, re-review findings) was still pending when phase 2 started, and phase 2 added two more todos. | muhtari todos @`ecbe7f52aa37` | adopted, strengthened (GSD-HANDOFF: the `ci:` tail PR is a numbered step between the phase-1 release and phase 2 discuss) |
| XFER-105 | Phase 1 split the core into OBJECT libraries at a cost of 0.31 % Ir per emulated cycle (under the 5 % bar), so the object under test is the object that ships on MSVC, wasm-ld and the nofp lane. | muhtari 02-01, 02-07 | adopted (ARCHITECTURE: OBJECT libraries per device; one Cachegrind check at the split) |

## Lessons learned here

None yet. The first export is offered at the v1.0 phase 1 close.
