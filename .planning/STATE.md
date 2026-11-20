---
gsd_state_version: "1.0"
milestone: v2.0
milestone_name: The whole library
current_phase: 09
current_phase_name: CHD disc images
current_plan: 13
status: executing
stopped_at: Completed Plan 09-13 Task 1 ON package checks
last_updated: "2026-11-20T18:08:00Z"
last_activity: 2026-11-20
last_activity_desc: Phase 09 Plan 09-13 Task 1 passed and was committed; ON package evidence recorded before OFF validation
state_head: d275d99
progress:
  total_phases: 10
  completed_phases: 2
  total_plans: 34
  completed_plans: 30
  percent: 20
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-11-19)

**Core value:** PlayStation games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.
**Current focus:** Phase 09 — CHD disc images

## Current Position

Phase: 09 (CHD disc images) — EXECUTING
Current Plan: 13 (ON/OFF archive validation)
Total Plans in Phase: 14
Status: Executing
Last activity: 2026-11-20 — Plan 09-13 ON archive checks passed; OFF package validation is next

### Phase 9 plan verification (2026-11-19)

Runtime/model: Codex typed `gsd-plan-checker`, gpt-6-luna xhigh. The final independent review passed all fourteen
plans with no requirement, decision, edge-case, dependency, file-scope, or command-direction gaps. Its prior
blocker for OFF archive validation was fixed by explicitly packaging and checking both CHD ON and OFF; three
newly surfaced file-scope warnings were resolved by splitting the package and final guest-evidence slices without
changing acceptance scope. The fixture-generator license gate remains a prerequisite before any tool use. The
command-path probe had 30 `not_applicable` rows because no Phase 9 verification artifact exists yet; the
failure-direction probe checked all 30 commands with no findings. No test suite or CI ran during planning.

### Phase 9 Plan 09-01 execution (2026-11-19)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID unavailable. The configured worktree opt-out kept this
plan sequential and inline. The owned CUE's production callback preflight passed 1/1 after configure and a
targeted build; it was rerun at the committed head in under one second with no build work. Disc, fuzz and
compatibility inventories and both diff checks passed. No guest boot, full suite, hosted CI or `act` run was
needed for the skeleton. The first configure noted that `SCHWAYSTATION_CHD=OFF` is unused until the later option
plan; configuration still passed. The two task commits are `b6ae475` and `80be717`; Plan 09-01 summary is
committed separately. MEDIA-01 and MEDIA-02 remain pending because this shared preflight does not complete their
phase-wide provider, bounds and fuzz requirements. Plan 09-02 is next.

### Phase 9 Plan 09-02 execution (2026-11-20)

Runtime/model: Codex with OpenGSD 1.20.0; primary GPT-6 model ID is not exposed. The configured worktree opt-out kept this plan sequential and inline. The exact libchdr commit and archive digest were verified, and a fresh enabled build compiled the static C closure without enabling C++. Explicit CMake cache settings keep system codecs, micro-flac, upstream tests and LZMA assembly disabled. ON/OFF malformed-header selectors passed 1/1, the default-OFF graph fetched no dependency, and a core-only CHD-on configure also fetched none. The cache-policy warning and off-only `-Werror` helper warning were corrected before the passing runs. No hosted CI, `act`, or full suite was needed for this plan. Task commits are `d2b0f31` and `e63727f`; summary `db9134e`. The private TOC and sector functions remain fail-closed until Plan 09-05 projects validated metadata and storage padding.

### Phase 9 Plan 09-03 execution (2026-11-20)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID is not exposed. The wall interval from the previous
saved state at 04:28:46Z to completion at 05:12:09Z was 43 minutes; active time was not measured. The pinned
`libchdman-rs` archive and full reachable C/C++/codec closure cleared the permissive-license gate. The native
fixture tool is opt-in and excluded from normal build/install paths. Its configure preflight checks archive and
source identity, licenses, source count and submodule metadata before target creation. The ON configure/build
passed; a fresh OFF configure completed in 0.3 seconds with no C++ compiler or generator archive. A deliberate
mutation of the extracted shim was removed on reconfigure, confirming source reset from the verified archive.
The incremental native build had no work; no fixture generation, full suite, hosted CI or `act` run applies yet.
The useful bottleneck discoveries were the missing OSD include and macOS `std::filesystem` deployment floor;
both were corrected in the target-only wrapper. A cached-source weakness was also closed before accepting the
preflight. Task commits are `a8a37c0` and `153f90b`; Plan 09-04 is next.

### Phase 9 Plan 09-04 execution (2026-11-20)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID is not exposed. The wall interval from the previous
saved state at 05:12:09Z to completion at 06:03:28Z was 51 minutes; active time was not measured. The owned
two-track source totals 522,144 bytes (64 data sectors and 158 audio sectors). The pinned native generator
produced the same 5,552-byte CHD twice, SHA-256
`4952c3a05ff2e2e596deed7ed2d37358a646790b890d5ed38484feec5b797569`, and wrote source/tool/output identity to a
build-directory manifest. Sorted inventory, generated-media and exact CUE/track-size preflights passed before
compression. Enabled and disabled structural selectors each passed 2/2 (0.07 seconds and 0.02 seconds). The
controls reject bad hashes, missing inputs, wrong counts, over-cap output and malformed files before guest work.
No guest or full suite ran because the plan explicitly stops before parity; no hosted workflow changed, so CI
and `act` were not useful. The plan correction `b3b38c0` keeps the fixture build-local; task commits are
`54bae5a` and `472c371`. Plan 09-05 is next.

### Phase 9 Plan 09-05 execution (2026-11-20)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID is not exposed. The wall interval from the preceding
STATE checkpoint at 06:03:28Z through completion at 09:22:03Z was 3h19m; active time was not measured. The
bounded CHTR/CHT2 parser projects the owned CHD into the same logical TOC as its CUE, including the stored
150-sector pregap without counting four-frame storage padding. Cheap sorted-inventory/diff checks passed first.
The CHD-on generated-fixture selectors passed 4/4 in 0.06 seconds; CHD-off selectors passed 3/3 in 0.03
seconds. No workflow changed, so no full suite, `act` or hosted CI run applied. The implementation and docs task
commits are `725c464` and `6a4d468`; plan-scope corrections are `ba579e5`, `7af3a5f` and `04d3558`; summary is
committed separately. MEDIA-01, MEDIA-02 and CHD-01 remain phase-level pending. Plan 09-06 is next.

### Phase 9 Plan 09-06 execution (2026-11-20)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID is not exposed. The 32-minute wall interval from
the preceding STATE checkpoint at 09:22:03Z to plan closeout included a targeted diagnosis of a byte-order
mismatch at the first data-track sector. The owned fixture showed decoded CHD CDDA words are pair-swapped from
the CUE/BIN payload; normalizing each audio word restored exact output. The enabled and disabled CHD/media
slices each passed 8/8 (0.06 seconds and 0.01 seconds); parity covered all 222 sectors in forward and reverse
order and returned the same content identity. Sorted inventory and diff preflights passed before the full
slice. No workflow changed, so `act`, hosted CI and a full suite added no relevant coverage. Task commits are
`9758617` and `277b592`; the summary is committed separately. MEDIA-01 and CHD-01 remain phase-level pending.
Plan 09-07 is next.

### Phase 9 Plan 09-07 execution (2026-11-20)

Runtime/model: Codex/OpenGSD CLI 1.20.0; primary model ID unavailable; typed executor gpt-6-luna xhigh.
The wall interval from the preceding STATE checkpoint at 09:54:00Z through closeout at 12:46:09Z was 2h52m;
active time was not measured. Shared checked arithmetic and the existing 1 GiB bound now guard both CUE and CHD
parsing. CHD preflight rejects oversized hunk/map/metadata allocations, invalid extents, parent references and
unsafe offset narrowing before libchdr decode. The production-parser replay validates stable TOCs, repeated
successful reads, bounded leadout and failure-atomic rejected opens in a build-owned directory.

Cheap `git diff --check`, preset JSON parsing, sorted fuzz inventory and the generated fixture size/hash checks
passed first. The enabled CTest replay passed 2/2, including `disc.media.preflight`; the disabled replay passed
1/1. The CHD-on `disc.image.*` selector slice passed 16/16 in 0.03s after rebuilding the ON-mode preflight
binary and fixture. The sanitizer campaign ran in native ARM64 Ubuntu 26.04 Docker because Homebrew's macOS
ASan runtime stalled before `main`. The image's CMake 4.2.3 was below this repository's 4.3/schema-11 preset
minimum, so the fuzz tree was configured directly with the preset's ASan/UBSan flags and CHD settings; the user
preset file was left untouched. A one-iteration smoke on a copy of the 13-input corpus passed, then the bounded
campaign exited 0 after 31s and 47,207 runs (701 edges, 1,304 features, about 383 MB peak RSS). The run-local
corpus and crash artifact paths stay under `build/fuzz`.

The first campaign exposed a harness false positive: a one-byte payload mutation can open with a stable TOC
while lazy decode of a compressed hunk fails consistently. The harness now allows stable read failure, compares
repeated data when a read succeeds, validates the TOC, and rejects a leadout read; `unreadable_hunk` derives
from the generated fixture, so no disc bytes are committed. The CTest selector command now builds the generated
fixture and mode-specific media preflight first, avoiding stale ON/OFF binaries after configuration changes.
No workflow changed, so no `act` or hosted CI run was needed. Task commits are `b6205e7` and `5e5915b`; the
bounded-campaign and selector-prerequisite evidence commits are `57d4579` and `a6271c2`. Plan 09-08 is next.

### Phase 9 Plan 09-08 execution (2026-11-20)

Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna xhigh and reviewer gpt-6-luna xhigh. The wall
interval from the saved STATE checkpoint at 12:51:00Z through final focused verification at 15:37:33Z was
2h46m33s; active time was not measured. Review found and fixed a feature-identity gap: the protected ratchet
now binds provider, profile, protection, peripheral and predicate. The dated allowance and negative controls
cover changes to prior passing combinations and candidate/fresh-result disagreement. Review also caught
minified gate branches and an outdated three-field allowance fixture; readable branches and the five-field
fixture are restored. The final focused selector passed 6/6 in 0.91s, helper budget passed at 3900/3901,
sorted inventory and shell syntax checks passed, and code review is clean. No full suite, `act` or hosted CI run
applied because this plan changed no workflow. Task and follow-up commits are `aa18277`, `d40e18b`, `45b7867`,
`8d66514` and `249e753`; Plan 09-10 is next by dependency order, then Plan 09-09.

### Phase 9 Plan 09-10 execution (2026-11-20)

Runtime/model: Codex runtime, executor model gpt-6-luna. The wall interval from the saved STATE checkpoint at
15:44:44Z through the summary at 16:07:53Z was 23m09s; active time was not measured. Main CI and Release both
passed on source SHA `830bf4c`; Main used 139.13 runner-minutes and Release's duplicate CI jobs used 117.53.
Every lane's commit file matched that SHA, but all eighteen Main/Release product ZIP pairs differed by digest;
Release ZIPs matched the published assets. Waiting for Main's slower required path before Release publication
projects about 24m25s more wall time than the observed release run. Separate gates and all lanes remain required.
The measured evidence and disposition are in `09-CI-REUSE.md`; todo 050 now carries the decision and the Plan 11
workflow-invariant check. No build, test suite, `act`, or hosted run was needed. Task commits are `3847aa3` and
`f12fbe7`. Plan 09-09 then completed; Plan 09-13 is next because Plan 09-11 depends on its package checks.

### Phase 9 Plan 09-09 execution (2026-11-20)

Runtime/model: Codex session; primary model ID unavailable; independent gsd-code-reviewer and two read-only planning/CI audits used gpt-6-luna xhigh. The inherited checkpoint did not preserve the full plan start time. The measured review-to-Task-2-commit interval was 6m15s; the wider active duration is unknown. ON/OFF metadata selectors passed 2/2 each, notice selectors 2/2, and compatibility setup/protected-base plus perf allowance selectors passed 3/3 in 0.92s. The counted gate is 3900/3901. Review found a malformed CHD suffix regex; it was corrected and confirmed before the selectors. The first notice run caught an Applies-to parser treating the backticked option as a path; CTest listing also showed the compatibility/perf selectors belong to the CI preset, not hygiene. Both issues were corrected before closeout. No build, broad suite, `act` or hosted run applied because no workflow changed. CI efficiency rules are already discoverable in `AGENTS.md` and `.planning/AUTONOMOUS.md`; Plan 09-11 retains the routing dry run and producer-path measurement. The owner chose to defer the generic source-wide SPDX scanner to the first tracked SPDX-bearing caller; todo 005 now targets Phase 10 planning. Task commits are `d336d33` and `1470276`; review resolution is in `09-REVIEW.md`. Plan 09-13 is next because 09-11 requires both package configurations.

### Phase 9 Plan 09-13 ON package checkpoint (2026-11-20)

Runtime/model: Codex GPT-6 session; exact model ID unavailable. Plan Task 1 is committed as `d275d99`.
The corrected ON preflight and package command passed: configured CHD ON; metadata claims passed 2/2,
policy notices passed 2/2, the build completed 229 Ninja actions, and the release archive/metadata/notices
selectors passed 4/4 in about 0.13 seconds. CPack produced the library, libretro and runner macOS arm64
archives. The release inventory is sorted and the gate remains 3900/3901 lines. Exact build duration was not
measured. This records ON evidence before Task 2 replaces these build-tree archives; no hosted URL applies.
The plan selectors were corrected because `policy.notices` is registered by the hygiene preset, not CI.
Next: configure CHD OFF in the same build directory, then rebuild, repackage and run the same release checks.

### Phase 9 planning (2026-11-19)

Runtime/model: Codex typed gsd-planner, gpt-6.1-sol xhigh. Fourteen short plans cover the optional pinned reader,
complete fixture-tool closure audit, owned fixture producer, exact metadata/sector/subcode parity, shared limits,
fuzzing, feature ledger, notices, measured CI reuse, all-lane workflow wiring and both shipped hosts. The generator
is still not cleared; Plan 03 blocks its use until the complete compiled closure is permissive. Independent ledger
and CI measurement plans can proceed after the skeleton. The source audit maps all four requirements, nine
decisions and thirteen edge-probe candidates. Source review remains before final verification; no tests or hosted
runs were launched during planning. Estimate calibration had zero samples, factor 1 and confidence low.

The scope-sanity revision removed redundant notice work from Plan 02, moved ON/OFF archive validation and
todo 005 closeout to Plan 13, and moved owned-guest parity/feature evidence/docs from Plan 12 to final Plan 14.
Every plan now declares at most ten modified paths; no requirement, acceptance case, edge resolution, lane,
fuzz case, notice, resource bound or review-before-verification rule was deferred. Plan 11 waits for both
package configurations before distributed enablement; source review follows the final Plan 14 handoff.

### Phase 9 research (2026-11-19)

Runtime/model: Codex; `gsd-phase-researcher` gpt-6-luna xhigh. The first research handoff exceeded its expected
window without an artifact and was stopped; a narrow synthesis pass reused the three completed advisor reports
and verified project references, then committed `09-RESEARCH.md`. It records the approved libchdr pin and host
boundary, parity/preflight/package sequence, and required Validation Architecture. The full fixture-generator
license/build closure remains explicitly unresolved and must be cleared before use. Plan-pre checks found no
assumption delta, skipped codebase drift because no structure map exists, and found no context drift. The
deterministic edge probe surfaced 13 applicable edge candidates across the four requirements; the planner must
resolve or explicitly flag every one. No tests or CI ran during research.

### Phase 9 discussion (2026-11-19)

Runtime/model: Codex, primary GPT-6 model ID unavailable; three `gsd-advisor-researcher` agents on
gpt-6-luna xhigh. The discussion captured parity-first CHD projection, a run-local fixture generated only from
a license-cleared pinned tool closure, structural/boundary preflights before guest execution, CHD-enabled release
artifacts after parity, and measured exact-SHA CI reuse as the only basis for changing Main/Release wiring.
Folded todos 005 and 050; todo 006 remains with Phase 10. All three focused advisor reviews completed and the
adversarial pass found no decision that changes the approved scope. Context and discussion log were committed in
`09-CONTEXT.md` and `09-DISCUSSION-LOG.md`; no implementation test or CI run applies yet. CI/CD efficiency
guidance is already discoverable in `AGENTS.md` and `.planning/AUTONOMOUS.md` and will be applied at each run.

### Phase 8 Plan 08-10 execution (2026-11-19)

Runtime/model: Codex/OpenGSD session; GPT-6, exact deployed model and CLI minor version unavailable. The plan
spanned at least 224 minutes from the 18:47 STATE checkpoint through the second exact-head CI roll-up; active
time was not measured. The local integrated workflow passed 621/621 CTests and 5/5 release checks before the
final one-line workflow-test consolidation. Exact-head CI run 44604545904 then passed on the claim-false head
`f52ef90b7653c1f172f3141a85080751e6a306c5`; its required RetroArch card step verified two 120-frame launches
and counters `01/02` then `02/03` for two 131,072-byte card files. After enabling the claim, the focused local
selectors passed 4/4 and run 44618451958 passed the full required matrix on
`d4463e798bcb23b3b059a6844908616e526a9b20`, including the same fresh-process proof, hash equality and required
roll-up. BE32 took about 76 minutes in the first run and 65m39s in the second. The true-claim self-test was
updated to derive its baseline from generated metadata after its first run exposed the old hardcoded-false
assumption. Both full run links and artifacts are recorded in `08-10-SUMMARY.md`; the phase review and
verification are complete.

### Phase 8 verification and transition (2026-11-19)

Runtime/model: Codex/OpenGSD 1.20.0; GPT-6 session, exact deployed model ID unavailable. The wall interval from
the 22:31:22Z checkpoint to phase transition at 23:09:35Z was about 38 minutes; active inline verification was
about 12 minutes, not precisely measured. The typed verifier was interrupted before it wrote a report, so its
existing evidence was recovered and checked inline rather than repeating the full suite. A fresh CMake CI
configure and the consolidated Phase 8 selector passed 24/24 tests in 5.13 seconds; all ten plans' artifact
checks passed 40/40; canonical verification status and the five-check automated UAT predicate both passed.
The exact-head hosted run 44618451958 passed the required matrix and two-process RetroArch card continuation;
BE32 remained the critical path at 65m39s. Its run and artifact links are in 08-VERIFICATION.md.

CI efficiency review: the stable-state-size preflight ran in about nine seconds before the 4m03s Linux
build/test job. The local full CMake suite and exact-head hosted run already covered the useful local and hosted
paths, so act would have repeated equivalent checks. Phase 8 has no supported additional coverage-preserving
CI change. Its PR CI and automatic release remain required; do not begin Phase 9 until main CI is green and the
release is published. The only open todo is 050, explicitly bound to Phase 9.

### Phase 8 Plan 08-02 execution (2026-11-19)

Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna. Completed three tasks in 33 minutes with six atomic task commits. The focused card-file selectors passed 3/3, 4/4, and 1/1. Wrapper fields were checked against the pinned public PSX-SPX source before implementation. SAVE-02 is complete; SAVE-01, SAVE-03, and SAVE-04 were already complete. No hosted run applies. Plan 08-03 is next.

### Phase 7 execution (2026-11-18–19)

Runtime/model: Codex/OpenGSD CLI 1.20.0; inherited GPT-6 session (exact deployed ID unavailable). Plan 07-02
used the same inherited model at xhigh effort. Wall elapsed was about 55 minutes from the planning commit through
the completed report; active time was not measured.
Plan 07-01 added five native CI policy cases. Its historical gate count was 3,133/3,200 lines and 300/300
RetroArch lines; the reviewed 3,300-line ceiling is recorded in the plan and will be applied by Plan 03 before
more counted gate code is added.
Plan 07-02 captured 40 workflow runs, all 40 attempt job pages and 573 jobs. Exact-head filtering yielded 25
usable completed runs across 20 workload groups. The report includes the Release reusable-workflow path,
failure-log classifications, timeout formulas and explicit unknowns; its one-shot validator passed, including
missing-section and wrong-arithmetic controls. The actual release-App PR comparison and timeout application
remain with later plans.
Plan 07-03 completed on 2026-11-18. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna at xhigh
effort. The repository-check budget is now 3,300 lines with 3,133 counted; the independent RetroArch budget
remains 300/300. Cachegrind and serialized-state baselines are recorded, while WASI module size remains unmeasured
until its focused Ubuntu producer check. The settings audit found no drift, and the evidence-based runner/pin
review is recorded in the plan artifacts.

Plan 07-04 completed on 2026-11-18. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna at xhigh
effort. All seven quality dimensions were scored before selecting one change. Documentation/README tied with
upgrade path at 2; the review selected the false README BIOS/disc status claim for its direct user impact and
left Dependabot, CI routing, pins and runner labels unchanged. The README now describes the documented PS-X EXE
and BIOS/CUE/ISO/M3U paths and limits compatibility evidence to fixtures. Its obsolete-claim check and diff
check passed, as did the workflow pin check (57 active uses). Existing CI/performance run evidence is linked in
the review; no new hosted run or full suite was started. The phase-level TUNE-04 and TUNE-06 requirements remain
pending because their GSD readiness check is blocked by remaining phase work.

Plan 07-05 completed on 2026-11-18. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna. The bounded
collector, hostile-input controls and read-only weekly/manual workflow are complete. The focused `ci.triage`
test passed, along with the pin, timeout-comment and gate-budget checks. The scoped `act --dryrun` passed its
wiring check. No hosted artifact run was made; the phase-level TUNE-01 requirement stays pending until ship
confirms that the merged workflow uploads its single seven-day artifact. This is deferred evidence, not an owner
verification task. Plan 07-06 is next.

Plan 07-06 completed on 2026-11-18. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna. The exact release App metadata PR route now requires a matching event and fresh REST bot identity plus stable, complete pagination; focused classifier and production route checks passed. Sampled job timeouts follow the linked Plan 02 runs and retain setup floors; unsampled jobs remain provisional. The repository-check gate is 3,299/3,300. The scoped `act --dryrun` parsed the CI changes route without executing actions. No hosted CI was started; TUNE-02/03 remain Pending until ship compares the next automatic release PR with run 44253419074. Plan duration was 40 minutes; RED, GREEN, and timeout commits are recorded in `07-06-SUMMARY.md`. Plan 07-07 followed.

### Phase 7 Plan 07-07 preflight and recovery (2026-11-18)

Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna xhigh; read-only budget review gpt-6-luna xhigh. Before source edits, `policy.gate_budget` confirmed 3,299 counted lines against 3,300. Plans 05–06 had used 166 lines since Plan 01, 31 above the earlier estimate; review estimated 370–500 more for Plans 07–08. Corrected the temporary absolute ceiling to 3,900 (3,299 current + 500 upper estimate + 100 headroom), retained all counted globs and assertions, and confirmed `policy.gate_budget` passes at the new value. Plans 09–10 add no counted files; Plan 10 still ratchets to the measured ship total.

The local machine has no clang-21/WASI sysroot or configured `build/wasm32` graph. Read-only inspection of prior Ubuntu 26 run 44253419074 found only the `hashes-wasm32` artifact, not the linked module, so no real Wasm size could be recovered. The size bound remains withheld until an actual focused Release WASI build reports it. An initial `act` preflight hit its image-selection prompt; a noninteractive dry run parsed only the `changes` dependency and did not execute the `wasm32` job. No source files or hosted runs were changed or started. `.planning/config.json` remains a pre-existing user edit and is excluded from commits.

Plan 07-07 completed on 2026-11-18. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna xhigh. The single `quality.json` registry now sends real Cachegrind measurements, linked WASI module bytes, and actual stable-state bytes through the shared comparator while retaining the original relative performance gate. Task 2 followed RED/GREEN commits; focused CTests passed, and the actual state output measured 3,755,020 bytes. The repository-check budget is 3,509/3,900. TUNE-05 remains Pending until the Ubuntu 26 WASI Release lane measures the module and a bound is added at ship. The historical artifact contained only hashes; two isolated toolchain setup attempts failed before CMake/build (package resolution and container tar ENOSYS), so local emulation was stopped. No hosted CI, guest test or whole suite was run. Plan duration was 43 minutes; task commits are recorded in `07-07-SUMMARY.md`. Plan 07-08 is next.

Plan 07-08 completed on 2026-11-19. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna xhigh. The exact-attempt timing helper and `CI required` production path are complete. The focused CI selectors passed 2/2; hygiene, pin, timeout-comment, and gate-budget selectors passed 3/3; the duplicate-ID control passed in a 24.30-second focused rerun. The repository-check budget is 3,890/3,900 lines. Historical replay exited 0 across 40 run rows and 573 job records. The test-first failure was observed while the helper was absent, but no durable RED report or exit-status record was retained; no classifier result is claimed. Two Plan 07-08 commits are `931e055` and `577ef3d`; `bf84ae2` is the prior Plan 07-07 gate-budget closeout, and the full execution-start range contains three commits. No hosted Phase PR measurement was made; both the matching critical-path measurement and actual WASI module size/bound remain for ship. `.planning/config.json` was preserved. Plan duration was approximately 58 minutes; Plan 07-09 is next.

Plan 07-09 completed on 2026-11-19. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna xhigh. Maintainer and testing docs, engineering, product roadmap, prompt coverage, and the upgrade audit now reflect current workflow sources and measured baselines; README installation remains unchanged and is backed by existing release run 44262491280. `git diff --check HEAD`, the docs byte budget, the repository gate budget (3,890/3,900), and the RetroArch budget (300/300) passed. ARCHITECTURE §10 now records the owned `disc_boot` DMA3 → RAM → DMA0 → MDEC → DMA1 → RAM → DMA2 → native 24-bit display path, with Phase 6 Plan 06-16 and DISC-03 citations; it explicitly leaves full decoded output and external frame/movie hardware conformance unproven. Device MIPS remain assumptions and `gpu_motion` exceeds each provisional low-power budget. This plan's TUNE-04 and TUNE-06 deliverables are complete; phase-level readiness remains pending on the remaining work. TUNE-01 still needs a post-merge triage artifact run; TUNE-02/03 need the release-App comparison; TUNE-05 still needs the matching release-PR timing and actual Ubuntu 26 WASI module size before setting that bound. No new hosted workflow was started. Plan range `98d0b9c..11a1251` contains two commits: `b9a4ecd` (bounded plan revision) and `11a1251` (documentation/evidence). Plan duration was about 27 minutes wall elapsed; exact start and active time were not captured across handoff. Plan 07-10 is next.

Plan 07-10 completed on 2026-11-19. Runtime/model: Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna xhigh. The focused serialized CI selector passed 5/5 at the 60-second quality timeout (39.17 seconds total; 38.61 seconds for `policy.quality_metrics`); after the final 80-second cap was registered, the isolated `policy.quality_metrics` selector passed 1/1 in 28.26 seconds total (28.25 seconds for the test). The repository-check budget passes at 3,890/3,890 and the separate RetroArch budget at 300/300; focused hygiene checks passed 5/5. The repository-root producer command `cmake -DROOT="$PWD" -DBIN="$PWD/build/ci/policy/quality" -P tests/cmake/quality_controls.cmake` exited 0 in about 25 seconds. The proposed release title and release-please strategy checks passed. The timeout changes are measured headroom, not a speedup; test cases and assertions were preserved. Plan range `501a1a1..ff767a2` contains three commits: `ed5a86b` (30→60 evidence-based plan revision), `4b8c14b` (60→80 revision after the 38.61-second run), and `ff767a2` (test and evidence changes). No phase review, final verification, hosted workflow, PR, merge, release, or triage dispatch was run. The matched release-App comparison, Ubuntu 26 WASI size, and post-merge artifact remain pending for ship. Next: independent phase-level code review, then final automated verification; Phase 7 remains in progress.

### Phase 7 review fixes and verification (2026-11-19)

Runtime/model: Codex/OpenGSD CLI 1.20.0; gsd-code-reviewer, gsd-code-fixer, and gsd-verifier gpt-6-luna xhigh; primary session model ID unavailable. Independent review found one critical PR timing SHA defect and four fingerprint/registry warnings. Fixes added focused regression controls and preserved existing production-gate cases. A second review prevented applying the historical push/main timing interval to the PR route without matching hosted evidence; the stored push interval remains tied to its original fingerprint and the current PR route is explicitly unbaselined. Final code review is clean in `07-REVIEW.md`.

The review fixes raised the counted repository checks from 3,890 to 3,900; the registered limit then matched 3,900. The separate RetroArch count remains 300/300. The post-review `policy.quality_metrics` selector passed 1/1 in 37.22 seconds. After `cmake --preset hygiene` refreshed the stale generated test registration, `ctest --preset hygiene --no-tests=error -R '^(policy[.](action_pins|e2e_budget|gate_budget)|hygiene[.]timeout_comments)$' --output-on-failure --parallel 1` passed 4/4 in 0.03 seconds. The verifier also passed locally automatable CI behavior controls 4/4, no-skip controls 2/2, and all 10 plan artifact queries. Under the infrastructure-phase verifier rule, `07-VERIFICATION.md` records `passed` (5/5 pre-ship implementation and data-flow truths; 0 behavior-unverified) with no owner UAT. The first ship run later measured the WASI module, and its Windows timing failures are recorded below. A report-path preflight caught and corrected an extension typo before transition; all 51 covered files now exist and the refreshed fingerprint passes. Phase completion emitted only non-blocking warnings for command examples misread as missing files; no implementation artifacts are missing.

### Phase 7 ship preflight follow-up (2026-11-19)

Runtime/model: Codex/OpenGSD CLI 1.20.0; primary GPT-6 session (exact deployed model ID unavailable); read-only
code review gpt-6-luna xhigh. PR #28's first hosted attempt on `8a3a502ab4dd3a47fe906e7976a33680a71b8584`
measured the linked Ubuntu 26.04 WASI module at 595,820 bytes. Both Windows jobs exposed the same CRLF parsing
failure in `policy.quality_metrics`; 617/618 tests passed on each, and the required jobs were not green. The PR
Nightly fuzz job also hit the five-minute timeout; a matching successful historical fuzz job took 7m03, so the
workflow now allows 15 minutes (twice that run, rounded up). The initial PR run was canceled after the known
Windows failures and WASI measurement were captured. Read-only review supports the corrective diff, including a
595,820-byte exact WASI bound and a one-line CRLF normalization; the repository-check cap is now 3,901 lines.
`jq`/CMake quality-schema validation, the 3,901/3,901 repository budget, 300/300 RetroArch budget, workflow timeout
comments and selector inventory all passed as cheap preflights. The serialized `policy.quality_metrics` test then
passed 1/1 in 30.27 seconds; the four focused hygiene tests passed 4/4 in 0.03 seconds. The final diff check passed.
A new exact-head hosted CI/Nightly run and the release are still required. Phase 8 planning remains stopped until
Phase 7 is shipped, merged, green on main, and released.

The next exact-head CI attempt used commit `4d389aef0a58ceadc8e010a15559852bbbe237f4` (run 44464772297). It
reproduced a second Windows-only path issue in `policy.quality_metrics`: CMake passed a native `D:/.../quality.json`
path, but the timing helper recognized only POSIX absolute paths and prefixed the Windows drive path with the
repository root. The arm64 Windows lane completed successfully; x64 failed at this registry read and the run was
canceled once the known required failure was confirmed. This is new platform evidence beyond the CRLF failure.
Independent follow-up review confirmed the one-line case pattern now recognizes drive-rooted and POSIX absolute
paths while still rooting relative paths. After review, local preflights passed (`sh -n`, `git diff --check`, CI
and hygiene configure); `policy.quality_metrics` passed 1/1 in 35.45 seconds, and gate, E2E, action-pin and
timeout-comment controls passed 4/4 in 0.03 seconds. At that checkpoint, the branch held the one-line correction;
commit 57501d4 and its hosted run follow below. Successful CI, PR timing and release evidence remained pending.
Runtime/model: Codex/OpenGSD CLI
1.20.0; primary GPT-6 session (deployed ID unavailable); follow-up read-only reviewer gpt-6-luna xhigh.

Exact-head run 44466369143 on commit `57501d46bf5b102e448e0436e7f28fc888cbe136` then passed `policy.quality_metrics`
on Windows x64 in 38.51 seconds but Windows ARM64 timed out at 80.02 seconds; the other 617/618 tests in its
matrix job passed. The run was canceled once this single known failure was captured; its failed aggregate is not
a phase pass. Matching Nightly run 44466369199 completed successfully on that head, including fuzz. The
assertion-heavy quality control launches many CMake checks, so a read-only review approved a provisional 180s
CTest allowance without removing coverage. After `cmake --preset ci` reconfigured the selector, the local
serialized test passed 1/1 in 29.36 seconds. The next exact-head Windows ARM64 lane must establish its actual
completion time; the 180s allowance is not yet validated there. Runtime/model: Codex/OpenGSD CLI 1.20.0; primary
GPT-6 session (deployed ID unavailable); follow-up read-only reviewer gpt-6-luna xhigh.

### Phase 7 ship closeout (2026-11-19)

Runtime/model: Codex/OpenGSD CLI 1.20.0; primary GPT-6 model ID unavailable. Phase PR #28 exact-head CI run
44468099352 and Nightly run 44468099361 passed. Windows ARM64 completed the quality control in 116.03 seconds
under the 180-second cap. Release PR #29 merged as `af64484f388eddf67c615673b98edd6ccb5dae4c`; main CI passed
after a targeted retry of an anomalously slow macOS x64 path. Release run 44477155670 published v0.7.0 with 19
assets and passed the latest-install check. Triage run 44486437761 uploaded a valid artifact with zero open
issues and no truncation. Release-App baseline SHA limitations and same-commit duplicated runner cost are
documented in `07-DELIVERY-EVIDENCE.md` and `07-CI-EFFICIENCY.md`; the safe-reuse investigation is bound to a
Phase 9 workflow-editing plan in todo 050. Phase 7 ship evidence is complete; no owner UAT is needed.

### Phase 7 discussion and planning (2026-11-18)

Runtime/model: Codex, OpenGSD 1.21.0; primary GPT-6 session (exact deployed model ID not exposed). Phase researcher
and initial CI/security/quality advisors inherited the Codex model (exact ID not exposed); planner used
gpt-6.2-sol high and independent plan checker used gpt-6.2-sol xhigh.

Discussion used the existing 40-run CI evidence. BE32 remained the critical path (64m52s on run 44322945842 and
72m18s on run 44335613581); release PR #23 spent 60m21s there for generated metadata alone. The safe candidate
skips heavy PR tests only for a verified release App PR with the exact metadata file list and event head. All
main and actual release checks remain. Plans cover TUNE-01 through TUNE-06 in ten plans/eight waves. Independent
plan review passed after correcting one shell-command quote. No tests or workflows ran during planning. Plan
07-03 is next after completed Plans 07-01 and 07-02.


## Performance Metrics

**Velocity:**
- Total plans completed: 215
- Average duration: Not measured
- Total execution time: Not measured (active time is not tracked consistently)

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 01 | 15 | - | - |
| 02 | 27 | - | - |
| 03 | 26 | - | - |
| 04 | 58 | - | - |
| 05 | 24 | - | - |
| 06 | 45 | - | - |
| 7 | 10 | - | - |
| 8 | 10 | - | - |

**Recent Trend:**
- Last 5 plans: -
- Trend: -

*Updated after each plan completion*
**Per-Plan Metrics:**

| Plan | Duration | Tasks | Files |
|------|----------|-------|-------|
| Phase 01 P01 | 13 min | 3 tasks | 55 files |
| Phase 01 P02 | 3min | 3 tasks | 7 files |
| Phase 01 P03 | 9 min | 3 tasks | 8 files |
| Phase 01 P04 | 6min | 3 tasks | 11 files |
| Phase 01 P05 | 8min | 3 tasks | 14 files |
| Phase 01 P06 | 8 min | 3 tasks | 10 files |
| Phase 01 P07 | 6min | 3 tasks | 9 files |
| Phase 01 P08 | 6 min | 2 tasks | 6 files |
| Phase 01 P09 | 11h30m wall (checkpoint interruption) | 3 tasks | 16 files |
| Phase 01 P10 | 12min | 2 tasks | 7 files |
| Phase 01 P11 | 4min | 2 tasks | 6 files |
| Phase 01 P12 | 6min | 3 tasks | 11 files |
| Phase 01 P13 | 5min | 3 tasks | 6 files |
| Phase 01 P14 | 17min | 3 tasks | 0 files |
| Phase 01 P15 | 50min | 3 tasks | 27 files |
| Phase 05 P15 | ~2h30 wall across restart | 2 tasks | 19 files |
| Phase 05 P24 | ~30min wall | 1 task | 4 files |
| Phase 02 P01 | 2 min | 2 tasks | 15 files |
| Phase 02 P02 | 4 min | 3 tasks | 16 files |
| Phase 02 P03 | 6 min | 3 tasks | 8 files |
| Phase 02 P04 | 5 min | 2 tasks | 6 files |
| Phase 02 P05 | 11 min | 3 tasks | 11 files |
| Phase 02 P06 | 13 min | 3 tasks | 10 files |
| Phase 02 P07 | 14 min | 3 tasks | 12 files |
| Phase 02 P08 | 25 min | 2 tasks | 16 files |
| Phase 02 P09 | 36 min | 3 tasks | 6 files |
| Phase 02 P10 | 13 min | 3 tasks | 11 files |
| Phase 02 P11 | 10 min | 3 tasks | 14 files |
| Phase 02 P12 | 14 min | 3 tasks | 12 files |
| Phase 02 P13 | 13 min | 3 tasks | 13 files |
| Phase 02 P14 | 11 min | 3 tasks | 10 files |
| Phase 02 P15 | 15 min | 3 tasks | 9 files |
| Phase 02 P16 | 5 min | 3 tasks | 12 files |
| Phase 02 P17 | 12 min | 3 tasks | 15 files |
| Phase 02 P18 | 3 min | 3 tasks | 15 files |
| Phase 02 P19 | 20 min | 3 tasks | 13 files |
| Phase 02 P20 | 7 min | 3 tasks | 10 files |
| Phase 02 P21 | 15 min | 3 tasks | 15 files |
| Phase 02 P23 | 6 min | 3 tasks | 14 files |
| Phase 02 P24 | 12min | 3 tasks | 13 files |
| Phase 02 P25 | 25min | 3 tasks | 8 files |
| Phase 02 P22 | 55min | 3 tasks | 13 files |
| Phase 02 P26 | 9 min | 3 tasks | 6 files |
| Phase 02 P27 | 37 min | 3 tasks | 7 files |
| Phase 03 P01 | 25 min | 3 tasks | 30 files |
| Phase 03 P02 | 6 min | 3 tasks | 13 files |
| Phase 03 P03 | 11 min | 3 tasks | 12 files |
| Phase 03 P04 | 3 min | 3 tasks | 13 files |
| Phase 03 P05 | 7 min | 2 tasks | 9 files |
| Phase 03 P06 | 9 min | 3 tasks | 12 files |
| Phase 03 P26 | 5 min | 2 tasks | 6 files |
| Phase 03 P08 | 15 min | 3 tasks | 6 files |
| Phase 03 P09 | 19 min | 3 tasks | 9 files |
| Phase 03 P07 | 9min | 3 tasks | 5 files |
| Phase 03 P10 | 7 min | 3 tasks | 6 files |
| Phase 03 P11 | 10 min | 3 tasks | 9 files |
| Phase 03 P12 | 10 min | 3 tasks | 5 files |
| Phase 03 P13 | 12 min | 3 tasks | 6 files |
| Phase 03 P14 | 13 min | 3 tasks | 5 files |
| Phase 03 P15 | 16 min | 2 tasks | 15 files |
| Phase 03 P16 | 5 min | 2 tasks | 5 files |
| Phase 03 P24 | 6 min | 2 tasks | 3 files |
| Phase 03 P17 | 4 min | 2 tasks | 5 files |
| Phase 3 P18 | 22 min | 2 tasks | 2 files |
| Phase 03 P19 | 19 min | 2 tasks | 6 files |
| Phase 03 P20 | 10 min | 2 tasks | 3 files |
| Phase 03 P21 | 5 min | 2 tasks | 14 files |
| Phase 03 P22 | 4min | 2 tasks | 10 files |
| Phase 03 P23 | 6 min | 2 tasks | 7 files |
| Phase 03 P25 | 7 min | 2 tasks | 32 files |
| Phase 04 P01 | 16 min | 3 tasks | 14 files |
| Phase 04 P02 | 32 min | 3 tasks | 13 files |
| Phase 04 P03 | 47min | 2 tasks | 20 files |
| Phase 04 P04 | 34min | 2 tasks | 6 files |
| Phase 04 P05 | 39min | 2 tasks | 9 files |
| Phase 04 P06 | 20min | 2 tasks | 8 files |
| Phase 04 P07 | 24min | 2 tasks | 7 files |
| Phase 04 P08 | 38min 51s+ (lower bound; includes restart gap) | 2 tasks | 13 files |
| Phase 04 P09 | 59 min | 2 tasks | 23 files |
| Phase 04 P10 | 16min | 2 tasks | 24 files |
| Phase 04 P14 | 47min | 3 tasks | 17 files |
| Phase 04 P11 | 100min | 2 tasks | 17 files |
| Phase 04 P15 | 58min | 2 tasks | 11 files |
| Phase 04 P12 | 74min+ | 2 tasks | 10 files |
| Phase 04 P16 | 44 min | 2 tasks | 18 files |
| Phase 04 P13 | 1h 11m | 2 tasks | 16 files |
| Phase 04 P33 | 35min | 2 tasks | 9 files |
| Phase 04 P34 | 28 min | 2 tasks | 8 files |
| Phase 04 P17 | 23min | 2 tasks | 7 files |
| Phase 04 P35 | 29min | 2 tasks | 6 files |
| Phase 04 P18 | 27 min | 2 tasks | 7 files |
| Phase 04 P36 | 41 min | 2 tasks | 7 files |
| Phase 04 P19 | 29 min | 2 tasks | 5 files |
| Phase 04 P20 | 40min | 2 tasks | 21 files |
| Phase 04 P37 | 49min | 2 tasks | 6 files |
| Phase 04 P38 | 51min | 2 tasks | 14 files |
| Phase 04 P21 | 44min | 2 tasks | 9 files |
| Phase 04 P22 | 106min | 2 tasks | 20 files |
| Phase 04 P39 | 1h 15m | 2 tasks | 7 files |
| Phase 04 P23 | 24 min | 2 tasks | 5 files |
| Phase 04 P50 | 17 min | 2 tasks | 5 files |
| Phase 04 P51 | 22min | 2 tasks | 10 files |
| Phase 04 P52 | 18m 38s | 2 tasks | 9 files |
| Phase 04 P53 | 45min | 2 tasks | 8 files |
| Phase 04 P40 | 4h 28m | 2 tasks | 7 files |
| Phase 04 P27 | at least 20 min (start unavailable after compaction) | 2 tasks | 8 files |
| Phase 04 P24 | 35 min | 2 tasks | 11 files |
| Phase 04 P41 | 23min | 2 tasks | 7 files |
| Phase 04 P43 | 31min | 2 tasks | 5 files |
| Phase 04 P25 | 37min | 2 tasks | 8 files |
| Phase 04 P49 | 38min | 2 tasks | 7 files |
| Phase 04 P42 | 40min | 2 tasks | 16 files |
| Phase 04 P26 | 86min | 2 tasks | 11 files |
| Phase 04 P28 | 92 min | 2 tasks | 5 files |
| Phase 04 P44 | 50min | 2 tasks | 5 files |
| Phase 04 P45 | 45min | 2 tasks | 11 files |
| Phase 04 P54 | 40 min | 2 tasks | 7 files |
| Phase 04 P55 | 17min | 2 tasks | 2 files |
| Phase 04 P56 | 16 min | 2 tasks | 6 files |
| Phase 04 P30 | at least 19 min; executor start unavailable after recovery | 2 tasks | 5 files |
| Phase 04 P46 | 96 min | 2 tasks | 20 files |
| Phase 04 P57 | 47min | 2 tasks | 6 files |
| Phase 04 P58 | 86 min | 2 tasks | 22 files |
| Phase 04 P47 | 125 min | 2 tasks | 13 files |
| Phase 04 P48 | not measured (checkpointed execution) | 2 tasks | 16 files |
| Phase 04 P31 | not measured (checkpointed execution) | 2 tasks | 9 files |
| Phase 04 P29 | not measured (checkpointed execution) | 2 tasks | 26 files |
| Phase 04 P32 | not measured (checkpointed execution) | 2 tasks | 7 files |
| Phase 05 P01 | 14 min | 2 tasks | 7 files |
| Phase 05 P02 | 19 min | 2 tasks | 8 files |
| Phase 05 P04 | under 40 min (exact start unavailable) | 2 tasks | 7 files |
| Phase 05 P16 | 26 min | 2 tasks | 6 files |
| Phase 05 P03 | 40min | 1 tasks | 8 files |
| Phase 05 P20 | 20 min | 1 tasks | 6 files |
| Phase 05 P05 | 44 min | 1 tasks | 13 files |
| Phase 05 P06 | 43min | 1 tasks | 7 files |
| Phase 05 P21 | 46 min | 1 tasks | 7 files |
| Phase 05 P07 | 29min | 2 tasks | 9 files |
| Phase 05 P23 | 12 min | 2 tasks | 7 files |
| Phase 05 P22 | 13m | 1 tasks | 2 files |
| Phase 05 P08 | 28min | 2 tasks | 9 files |
| Phase 05 P11 | 27min | 2 tasks | 5 files |
| Phase 05 P09 | at least 22 min | 2 tasks | 9 files |
| Phase 05 P13 | at least 13 min | 2 tasks | 11 files |
| Phase 05 P17 | 32min | 2 tasks | 5 files |
| Phase 05 P10 | 31 min | 2 tasks | 5 files |
| Phase 05 P18 | at least 36 min | 2 tasks | 5 files |
| Phase 05 P12 | about 34 min wall | 2 tasks | 4 files |
| Phase 05 P19 | about 30m wall across restart; exact active time unavailable | 2 tasks | 9 files |
| Phase 06 P01 | 12m | 2 tasks | 8 files |
| Phase 06 P02 | 26min | 2 tasks | 7 files |
| Phase 06 P03 | 18min | 2 tasks | 8 files |
| Phase 06 P07 | not measured | 3 tasks | 10 files |
| Phase 06 P25 | 14min | 2 tasks | 3 files |
| Phase 06 P08 | 35m | 2 tasks | 7 files |
| Phase 06 P26 | 20min | 2 tasks | 7 files |
| Phase 06 P04 | 107min | 2 tasks | 20 files |
| Phase 06 P05 | 58m56s elapsed wall; active time unavailable | 1 task | 13 files |
| Phase 06 P27 | 41m 08s | 1 tasks | 11 files |
| Phase 06 P28 | 6m | 1 tasks | 3 files |
| Phase 6 P29 | at least 1h29m57s wall lower bound | 2 tasks | 14 files |
| Phase 6 P30 | 37m+ | 2 tasks | 10 files |
| Phase 06 P06 | >=2h35m20s wall lower bound | 2 tasks | 11 files |
| Phase 6 P09 | 32m | 2 tasks | 13 files |
| Phase 06 P11 | 39m 33s | 2 tasks | 7 files |
| Phase 6 P31 | at least 31m | 2 tasks | 9 files |
| Phase 6 P32 | at least 7m34s | 1 tasks | 4 files |
| Phase 6 P10 | at least 49m47s wall lower bound | 1 tasks | 21 files |
| Phase 6 P33 | at least 1h22m00s wall lower bound | 1 tasks | 20 files |
| Phase 06 P34 | at least 10m48s wall lower bound | 1 tasks | 1 files |
| Phase 06 P12 | at least 1h24m18s wall; active time unavailable | 2 tasks | 12 files |
| Phase 6 P35 | at least 30m55s wall | 2 tasks | 24 files |
| Phase 6 P36 | at least 57m55s wall | 1 tasks | 20 files |
| Phase 6 P38 | at least 1h00m19s wall; active work capped at 30m | 1 partial task | 20 files |
| Phase 6 P14 | elapsed wall time spanned a 22-hour pause/resume; active time unavailable | 2 tasks | 28 files |
| Phase 06 P17 | not measured (continued after 22-hour pause) | 2 tasks | 12 files |
| Phase 06 P18 | at least 1h09m wall from prior state checkpoint; active time unavailable | 2 tasks | 14 files |
| Phase 06 P20 | about 32m wall from 04:35 UTC checkpoint; active time unavailable | 3 tasks | 9 files |
| Phase 6 P40 | 19m27s | 2 tasks | 7 files |
| Phase 6 P41 | 36min | 2 tasks | 5 files |
| Phase 07 P01 | 11min | 3 tasks | 9 files |
| Phase 07 P03 | 44min | 3 tasks | 6 files |
| Phase 07 P04 | 8min | 2 tasks | 2 files |
| Phase 07 P05 | 35min | 2 tasks | 3 files |
| Phase 07 P06 | 40min | 2 tasks | 6 files |
| Phase 07 P07 | 43min | 2 tasks | 14 files |
| Phase 07 P08 | 58min | 2 tasks | 4 files |
| Phase 07 P09 | 27 min | 2 tasks | 12 files |
| Phase 07 P10 | not captured across handoffs | 2 tasks | 9 files |
| Phase 8 P1 | 74m | 3 tasks | 6 files |
| Phase 08 P02 | 33m | 3 tasks | 3 files |
| Phase 08 P04 | 37 min | 3 tasks | 10 files |
| Phase 08 P03 | 39min | 2 tasks | 6 files |
| Phase 8 P7 | at least 318 min; active time unknown | 2 tasks | 6 files |
| Phase 8 P08 | at least 27min; active time unavailable | 2 tasks | 10 files |
| Phase 8 P09 | at least 20min; active time unavailable | 2 tasks | 2 files |
| Phase 09 P01 | 9m | 2 tasks | 4 files |
| Phase 09 P08 | 2h 05m | 2 tasks | 10 files |
| Phase 09 P10 | 23m 09s | 2 tasks | 2 files |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table (full list: `.planning/preparation/DECISIONS.md`).
Recent decisions affecting current work:

- [Roadmap]: Six phases, one per requirement category, kept despite the `coarse` granularity because each category is one branch, one pull request and one release.
- [Roadmap]: Phase 1 is the content-free walking skeleton ported from cogmaster and muhtari; every later phase ends in an automated release.
- [Phase 01]: schway_video is 80 + 2P bytes with trailing reserved1; schway_audio 16 + 2P; schway_config has reserved0 so the allocator sits at offset 8 on every target
- [Phase 01]: State header is 20 bytes (magic, format, total size, body CRC-32); the profile lock is set by the first run_frame and survives reset and load
- [Phase 01]: PROVENANCE PV.11 keeps the old cogmaster pin as a historical citation; seed pins moved to v0.1.0 f09ac97096ac
- [Phase 01]: The state header size is SCHWAY__STATE_HEADER_SIZE in src/state.h, shared by state.c and the state tests
- [Phase 01]: Plan 01-01's core passed every 01-03 contract case unchanged; mutants of src/ are caught by the core and state cases
- [Phase 01]: hashes.txt holds five LF lines, region first (ntsc-u frame 5, ntsc-u frame 60, ntsc-j frame 60, pal frame 60, ntsc-u png 5); the hex is field 5
- [Phase 01]: The runner takes one --png N:FILE; an unwritable path exits 1 and plan 01-09 owns its test; the PNG digest is pinned only after the pixel-by-pixel decode passed
- [Phase 01]: Darwin abi.undefined allows memset_pattern16 (Apple clang lowers the test card's repeating fill to it); Windows allows __ImageBase for MSVC switch tables
- [Phase 01]: core.no_host_clock is the whole host-clock proof: the PS1 has no RTC, so no injected-RTC form
- [Phase 01]: abi.header.c17/cxx17 and abi.api_v1 are compile/run checks without self-tests; the five gates with negative controls are abi.symbols, abi.globals, abi.undefined, abi.api_version and core.no_host_clock
- [Phase 01]: line_budget.cmake counts newline bytes only, as wc -l does, so its total equals the wc -l the gate ceilings use
- [Phase 01]: policy.publish_set.self_test fails both a 17-archive and a 19-archive set, covering both sides of exactly 18
- [Phase 01]: ci.* hygiene tests carry label hygiene only; policy.* tests carry hygiene and policy
- [Phase 01]: 01-07: perf_compare.cmake reads its mode only from SCHWAYSTATION_PERF_MODE; an unset mode is an input error
- [Phase 01]: 01-07: budgets.cmake resolves ROOT with file(REAL_PATH) so a relative ROOT still globs docs/*.md
- [Phase 01]: embed.subdirectory accepts exactly the library and its three object libraries, and fails on any install rule in the embedded tree
- [Phase 01]: 01-09: --bench shares the --frames limit 1..1000000; the revision is read by git rev-parse at configure into a runner-only generated header and build.revision_matches_head compares it with HEAD
- [Phase 01]: libretro adapter recreates the instance on a region option change (profile locks after the first frame) and sends SET_SYSTEM_AV_INFO only on a changed update
- [Phase 01]: libretro .info claims core_options true (tested in-process); savestate stays false although retro_serialize is wired and tested
- [Phase 01]: 01-11: the README install line keeps the seed form (curl zip and SHA256SUMS, shasum -c --ignore-missing, tar -xf cores info), so a damaged download installs nothing
- [Phase 01]: 01-11: release.floor is not ported; only the two archive-contents tests carry the release label
- [Phase 01]: 01-12: the tests/retroarch files carry no PS1 value; FRAMES=5, --frames 5 --png 5:runner.png and the 320x240 regex live in ci.yml, so the e2e code stays 293 lines
- [Phase 01]: 01-12: every ci.yml timeout keeps the seed bound marked provisional with no run ID until plan 01-15 measures them
- [Phase 01]: 01-12: CI required omits the rehearsal and cleanup clauses until plan 01-13 adds those jobs
- [Phase 01]: 01-12: tests/cmake/workflows.cmake MODE=timeouts|pins replaces the seed's two workflow scans
- [Phase 01]: 01-13: the one-off actionlint command also ignores the concurrency queue key, which actionlint 1.7.12 does not know; the seed release.yml ran with queue: max
- [Phase 01]: 01-13: the rehearsal not-configured message points to docs/maintaining.md, which cites the removed App script as git show 61a5bb7^:scripts/setup-release-app.sh
- [Phase 01]: 01-13: docs/testing.md CI jobs table does not list rehearsal and cleanup yet; the next plan that edits docs/testing.md adds the row
- [Phase 01]: 01-14: the first hosted CI run (39455724329) was green on every job; no hosted fix, no RetroArch frame pin
- [Phase 01]: 01-14: perf on the probe reports NO_BASE (Ir/frame 1620415) because main has no perf preset
- [Phase 01]: 01-15: be32 (36 s) and wasm32 (22 s) stay per-PR lanes; libgcc and wasmtime-count questions wait for the GTE in phase 3
- [Phase 01]: 01-15: workflow.test_gate_timeout is 4 s, twice the measured 1.62 s fresh local ci workflow
- [Phase 01]: 01-15: policy.gate_budget re-pinned from 1500 to the measured gate-line total (1392 at d8a0ab4) at phase 1 ship, after code review
- [Phase 01]: 01-15: the one-off windows-x64 install.consumer.dll_crt failure is a test bug to triage (todo 022), not a gate
- [Phase 01]: Verification: run 40929433476 passes on reviewed revision 384ee9d, with six build legs, all eight hashes equal, and exact RetroArch frame-5 equality. Attempt 1's Windows ARM64 job timeout recovered on a targeted rerun; todo 024 carries the timing sample to the CI tail.
- [Phase 01]: Verification: 117 UAT entries pass, 5/5 pre-merge truths verified; no owner verification remains. Real publication and latest-download installation are confirmed at ship.

- [Phase 01 CI tail]: Two controls on v0.1.0 each measured 1620424 Ir/frame three times with zero spread (runs 40939706087 and 40939709809); performance now gates above +1.00 percent, wasm32 is required, and hash-equality compares eight files. Implementation run 40940434499 is green.
- [Phase 02]: Test selection files stay byte-sorted; SCHWAYSTATION_FIXTURES option declared in tests/tests.cmake before the fixtures include
- [Phase 02]: 02-02: schway__irq_raise takes an interrupt number 0-10; I_STAT and I_MASK are bounded to bits 0-10 in the state; the profile flags survive a power reset
- [Phase 02]: PASS_REGEX tests run through expect_fail.cmake with CODE 0; the self test nests through a configure-time driver script
- [Phase 02]: docs.budgets globs docs/*/*.md in the same file(GLOB) as docs/*.md, so policy.gate_budget stays at 1392 lines against the 1520 limit
- [Phase 02]: 02-04: sst-sample.bin is a u16 count, then per slice a 16-byte name, a u32 length and the upstream slice with its count patched to 100 (2,395,390 bytes); records carry no opcode block after the cycles
- [Phase 02]: 02-05: fixtures name their image IMAGE (OUTPUT collides with an add_custom_command keyword); header.inc uses .create IMAGE, 0 plus .headersize 0x80010000 - 0x800; the armips target is armips-bin
- [Phase 02]: 02-05: a digest mismatch is fatal inside file(DOWNLOAD EXPECTED_HASH), so Armips.cmake prints an armips: ... SHA-256 line before downloading and the pin self test matches that
- [Phase 02]: 02-06: the vectors' next-PC model is pc plus the branch latch (next pc = target while take is set, else pc + 4) and every non-branch instruction clears the latch to zeros; the load latch is the pending load landing as the instruction ends, held as register 32 for none because a load into r0 is a real pending state
- [Phase 02]: 02-06: schwaystation-test-vectors exits 1 for a failure, a malformed file or fewer cases than --expect and 2 for a bad command line; an allow line for a file that ran and matched no failing case is an error
- [Phase 02]: 02-06: the cpu layout assertion pins offsetof(log) = 288 instead of sizeof, since the non-state log pointer differs by target; cpu_read and cpu_write are static inline until loads and stores land
- [Phase 02]: 02-07: the CPU hands the bus the virtual address (cpu_physical removed from cpu_exec.inc) and the bus decodes the segment; the scratchpad is a KSEG1 gap and KSEG2 answers only FFFE0130h
- [Phase 02]: 02-07: the delay-register cost table uses the psx-spx formula as published (COM0, COM2, COM3; COM1 adds nothing), is derived, not saved, and is rebuilt on a delay or COM_DELAY write and on a state load; non-port I/O registers charge one approximate cycle
- [Phase 02]: 02-07: EXP1 and EXP3 writes are dropped without a fault (reads are bus errors); the EXP2 debug ports act only for byte writes with the debug-port flag set and never fault; bus.bios is NULL until plan 02-16 so BIOS accesses are gaps
- [Phase 02]: Exe status split: ERR_EXE_INVALID is a malformed file, ERR_EXE_RANGE is addresses or size outside RAM; launch block at physical 0200h
- [Phase 02]: 02-09: armips builds unchanged on all eight hosted lanes; the probe found three phase-2 test-file failures only the hosted legs show (core.audio.pairs timeout tier 30 s, a drive-letter file URL, MSVC C4702), fixed in place; per-leg armips cost is in the 02-09 summary for plan 02-27 — run 42689688852 and 42691517611 green with CI required and hash-equality; draft probe pull request 7 stays open until it is closed unmerged before ship
- [Phase 02]: 02-10: Exception entry: schway__cop0_exception is the one entry; cpu_step takes a trapped flag so the program counter stays at the vector, and writes the branch record and the issued load once at the end
- [Phase 02]: 02-10: Layout assertion: offsetof(exc_repeat) == 296 pins the cpu state part, since the log pointer after a 300-byte state aligns differently on 32 and 64 bit
- [Phase 02]: 02-10: COP0 behaviour not covered by vectors: MTC0 ignores TAR, BadVaddr, EPC and PRID; MFC0 into r0 is a discarded pending load; COP0 in user mode with CU0 clear is CpU with CE 0 and no first fault; BCondZ links only for rt 10h and 11h
- [Phase 02]: 02-11: schway_run never crosses a frame boundary, and a budget ending exactly on it reports FRAME; max_cycles 0 changes nothing
- [Phase 02]: 02-11: instance.frame_end joins the state, validated as ticks <= frame_end <= ticks + the region's longest frame, so a run from a hostile state stays bounded
- [Phase 02]: 02-11: gate total after docs.api_names is 1457 lines against the 1520 limit; todo 008 is closed, plan 02-27 moves the file
- [Phase 02]: Rule (a) of the vector harness asserts the exception for a misaligned access and compares every other final field with CAUSE CE masked; the upstream interpreter copies opcode bits 27-26 into CE
- [Phase 02]: LWL and LWR read the aligned word once, SWL and SWR write the covered bytes as aligned chunks, and rule (b) coalesces upstream chunks to the covered byte set; no bus-hook field was needed
- [Phase 02]: A multiply or divide issued at counter t is ready at t plus 6, 9, 13 or 36; an early MFHI/MFLO waits to the stamp, capped at 36 cycles; MTHI/MTLO clear the stamp
- [Phase 02]: A user-mode access outside KUSEG raises AdEL or AdES and records no first fault; a misaligned access records misaligned_read or misaligned_write
- [Phase 02]: LWC1, SWC1, LWC3 and SWC3 follow the COP1/COP3 CpU rule; LWC0 and SWC0 stay reserved-instruction exceptions
- [Phase 02]: 02-13: rcheevos_md5 is MD5 over the first t_size + 0800h bytes of the file (rcheevos dfdc64ea035c, rc_hash_disc.c rc_hash_psx line 965); a bare file has no boot-file name, so none is prepended
- [Phase 02]: 02-13: --probe is valid only alone with one file argument not starting with a dash; every refused, empty, unreadable or missing file exits 1 with one stderr line and no report
- [Phase 02]: 02-14: the write queue is a count with no retirement by time; it empties by a fifth store, a KSEG1 access or a power reset, and the 4-cycle drain is an estimate — Every store reaches the bus at once, so only the cost of waiting is modelled; a time-based rule would be an unmeasured timing behaviour no test may pin
- [Phase 02]: 02-14: the I-cache refills from the missed word to the line end on any miss, IBLKSZ is ignored, isolation sends KUSEG, KSEG0 and KSEG1 data accesses to the cache arrays and leaves the KSEG2 cache-control word on the bus — One fill mechanism; FlushCache needs cache control reachable while Isc is set; the scratchpad alternative for TAG clear is not modelled and the bus page says so
- [Phase 02]: Plan 02-15: the nightly lane's tests.cmake defines its own schwaystation-test-vectors target and the prefix check compares every sample byte after the count field with the upstream file
- [Phase 02]: 02-16: the stub's RAM vector is a four-word lui, ori, jr, nop jump to a handler body in ROM (a KSEG0 J cannot reach KSEG1), the handler uses only k0 and k1, the bytes between the reset code and BFC00180h are FFh so a runaway records an exception, and the array is regenerated by cmake -DCASE=array -P tests/fixtures/tests.cmake — The image is 524 bytes (sha256 c567e5b4...); power-on maps it when bus.bios is NULL and a mapped image survives a reset
- [Phase 02]: 02-17: schway__cpu_run returns the stop reason and the cycles it ran are the advance of inst->ticks, added once per block; a stop sits on the first instruction boundary at or after the limit or frame end and carries no overshoot, so N then M equals N + M where N is a boundary of the one-shot run
- [Phase 02]: 02-17: a failed instruction fetch is IBE (no BadVaddr), and one at BFC00180h or 80000080h is a double fault that stops the run with FAULT whatever the flags; the exit-port report consumes exit_pending and keeps exit_code
- [Phase 02]: 02-17: mailbox layout follows the committed stub (CRC 04h, status byte 08h, version 09h, count 0Ah, id 0Bh, handler word 0Ch, records 10h+8n), not CONFORMANCE section 4; plan 02-23's runner reads status at 08h and the count at 0Ah
- [Phase 02]: 02-18: the stub's A and B kernel entries are four-word RAM slots that jump to ROM putchar routines using only k0 and k1, and the reset code copies a 48-byte slot table; the stub is 620 bytes — A putchar that is real MIPS reaches the debug port through the bus like any guest store, so no host hook watches a kernel address
- [Phase 02]: 02-18: schway_get_tty drains the 4 KiB ring oldest first and reports and clears the bytes dropped since the last drain, also for capacity 0; the ring stays outside the state, every hash and a load — The same pull shape as schway_get_audio; a host that drains every frame sees every byte, and a hash never depends on whether a host drained
- [Phase 02]: 02-19: an interrupt is armed by a retirement stamp (irq_stamp, in the state) on the false-to-true change of the line, 2 retirements for a device edge, I_MASK and SR writes and 1 for CAUSE and RFE, taken at the last only if the line is still true — The latencies are an inference unmeasured on silicon; an interrupt due on an instruction beats SYSCALL, BREAK, RI, CpU and fetch errors and yields to data errors and overflow, per the documented order
- [Phase 02]: 02-19: the interrupt entry calls schway__cop0_irq_entry_cop2 with the word at EPC, read without charge or first fault; it only reports a COP2 command and the GTE plan executes it — The double execution belongs to phase 3 with the GTE; the hook is this plan's whole deliverable for that bug
- [Phase 02]: 02-20: the exit port acts only with the debug-port flag, so a fixture that reports through it runs with --tty; without --tty the write is ignored and the run goes the full frame count
- [Phase 02]: 02-20: --exe shares the probe's file reader and refusal line; a line over 4096 bytes is printed in pieces; reaching the 600-frame cap with the program still running exits 0 until plan 02-23 lands the hang report
- [Phase 02]: the trace ring is a member of struct schway (131,096 bytes: 4,096 32-byte records plus bookkeeping), outside the state and every hash, and is cleared only by a power reset so a per-frame runner keeps one rolling tail
- [Phase 02]: schway_trace is 64 bytes with no reserved field; a record packs event, width, device and a 16-bit note into one word and keeps the frame's low 32 bits, widened by schway_get_trace
- [Phase 02]: line and dot of a trace record are derived at read time from the cycles since its frame began at 11 to 7 GPU clocks over 3,413 (NTSC) or 3,406 (PAL) clocks per line until the GPU keeps a beam
- [Phase 02]: 02-23: --stop-on mailbox reads the fixture mailbox through schway_peek once per frame; done with a matching CRC-32 exits 0, a fault status or a CRC mismatch exits 6, and no verdict by the frame cap prints hang frames=N pc=0x... and exits 8, only under --stop-on, so a plain --exe run at its cap still exits 0 — A fault status skips the CRC check because the stub kernel writes FFh over the status word without rewriting the CRC; the exit-table check compares --help with docs/runner.md both ways and the gate total is 1494 lines against 1520 (plan 02-26 decides any deferral)
- [Phase 02]: 02-24: an --exe run goes through schway_run per frame with stop-on-fault on by default and exit 7; the unbuilt line's n is the last device's count, printed once at the end of any --exe run; --trace and --no-stop-on-fault need --exe
- [Phase 02]: 02-25: the mailbox judge pairs a fixture's CSV rows with its records by order (row n is record id n) and lives in the fixtures script case; under --no-stop-on-fault the runner prints the first fault the program made at the end of the run and keeps the run's status — ExcCode persists until the next exception, so the masked-line and CAUSE-after-I_STAT doc rows read 1Ch, not 0; those two rows and the interrupt EPC (MTC0 plus 8) pin models, not measurements, and change only through tests/diag/allow.txt
- [Phase 02]: 02-26: the gate-line total is 1495 of 1520 after the last gate of the phase, so no check is deferred; the cap is never raised and ship ratchets it to the measured total — The stub regeneration commands live once, in docs/maintaining.md, with the armips and vectors procedures; the vectors one reproduced the committed sample byte for byte
- [Phase 02]: 02-27: build timeout 11 min (windows-x64 311 s, run 42725764966), hygiene 2 min, rehearsal 13 min; floors kept for asan, be32, wasm32, nofp, vectors-full, fuzz; each cites a run of this phase
- [Phase 03]: 03-01: SCHWAY__DMA_BUSY_MAX is 2,691,328 cycles (65,536 words at the CD's 40 per word, 17 per 16 words on the RAM side, 256 per-unit overhead); CHCR mask 71770703h, DICR mask 7FFF803Fh, the timers' base tick is never walked and a load sets it to the loaded ticks
- [Phase 03]: 03-01: the state field caps in test_state.c and test_run.c are 128, the embed test lists the new object libraries, and core.state.device_fields pins the DMA stored-bit and started-channel refusals that the max+1 sweep cannot reach
- [Phase 03]: 03-02: gte_cases.inc holds 26 records of 66 words (case number, opcode, 64 inputs): one per command chosen by output FLAG, plus register-only cases 1 to 4
- [Phase 03]: 03-02: policy.gate_budget limit is 1513, the measured total with policy.notices (30 lines) in; the vector file is a count plus 516-byte records (593,404 bytes, sha256 e8a0e61a...d9c2; the records alone hash to ed630d2f...58d2)
- [Phase 03]: 03-03: a deadline still due after its event limits the next block to one instruction so the run loop always advances; the no-program path steps ticks to the next deadline and the test card moved 121 Ir/frame (1,620,545 against 1,620,424, run 42760163838)
- [Phase 03]: 03-03: schway__sched_sync restarts the block clock after a power reset, a state load, a no-program run and the one-instruction entry, so fault.ticks needed no phase-2 assertion change
- [Phase 03]: 03-04: the stall hook is called once per bus_access for reads and fetches on RAM, I/O, EXP1-3 and BIOS (not stores, scratchpad, gaps or the cache-control word); an I_STAT read syncs the timers and a write recomputes the deadline; core.bus_unbuilt reads owned=672 unbuilt=3228 gap=8388
- [Phase 03]: 03-04: the interrupt entry's EPC read and the I-cache refill words reach the DMA hook through throwaway cycle sinks; plan 03-08 decides whether the EPC read may set a stall
- [Phase 03]: 03-05: the GTE register file is seeded from ares interpreter-gte.cpp at 9408cb43d4948fc3ea6e152a307a34348df3fe04 with an SPDX header, a notices section and a PROVENANCE seed row; 16-bit registers are stored as patterns and extended on read, FLAG bit 31 is recomputed on write, on read and at the end of every command, and the 50 register-only hardware cases pass
- [Phase 03]: 03-06: the GTE interrupt hook returns 1 only when it executed a command, and does nothing with SR.CU2 clear or a non-interrupt ExcCode
- [Phase 03]: 03-06: the entry's command stamps gte.ready from the entry cycle without waiting; the second run after the return waits through cpu_cop2
- [Phase 03]: GTE numerics clamp IR to the low 32 bits of the shifted MAC as the seed does; the 1,150 cases do not separate that from a full-width clamp, so the reference differential decides
- [Phase 03]: A GTE reciprocal table literal is pinned by a formula test, because the hardware cases pass with one entry corrupted
- [Phase 03]: 03-08: Unit enum: a phase per channel, 0 idle and 1 unit running; DMA_UNIT_CHANNELS (OTC only) is the mask the engine passes to schway__dma_arbitrate, so channels 0-5 keep registers and wait until plan 03-10
- [Phase 03]: 03-08: DICR bit 31 is bit 15 OR (bit 23 AND any flag): the per-channel enables gate only the setting of a flag, as the nugget reading 84800000h requires; OTC CHCR stores bits 24, 28 and 30 only, reads bit 1 as 1, clears bit 28 at the start and bit 24 at the end
- [Phase 03]: 03-08: A DMA register read finishes a running unit as of its end tick (the bus hook has frozen the CPU until then); a CHCR write to the running channel keeps bit 24 set; OTC costs ceil(words*17/16), 4,352 cycles for 4,096 words
- [Phase 03]: 03-09: the write hold is two counter cycles after the write's own cycle (a write at W shows at W+1 and W+2, counts from W+3); a reset at the target shows 0 for two cycles counting the landing, so the loop is target + 2; a landing is a count or a reset, never a software write
- [Phase 03]: 03-09: a counter whose I_STAT bit is set gives no deadline, an I_STAT write catches the counters up first, and an advance runs a deadline that falls on its end tick, so a saved state never holds an event behind its tick; the saved timers view is as of the instance's tick and the span function is shared by catch-up, save and the bisection that finds the next raise
- [Phase 03]: GTE: a command number outside the 22 documented ones changes no register (the seed's mirrors of 00h and 1Ah are not copied); documented as an engine choice, not a hardware claim
- [Phase 03]: 03-10: a sync 1 block takes its channel's request bit with it and schway__dma_dreq raises it again (a raise with the engine idle starts the channel); the linked-list and reserved modes are not pending until plans 03-14 and 03-15
- [Phase 03]: 03-10: a unit costs n * rate + ceil(n / 16) with rates 1 (MDEC, GPU, OTC), 24 (CD-ROM, BIOS default; 40 is the phase-6 device end's choice), 4 (SPU), 20 (PIO); a sync 1 block adds a derived 11; the stub sink's word count and CRC-32 sit in dma.words and dma.crc, and remain is bounded at 10000h in the state walk
- [Phase 03]: Plan 03-11: beam blank levels are stored state (timers.blank); a blank push of the held level is no edge; a blank reset has no hold; counters on dot or hblank clocks give no scheduler deadline
- [Phase 03]: gte_vectors: register moves are .word encodings in macros (armips takes no macro argument as a coprocessor register); the loop variant uses -definelabel LOOP 1
- [Phase 03]: 03-13: the independent GTE model takes SZ3 and the IR3 flag from the whole 44-bit sum shifted by 12 and saturates IR from the 32-bit MAC; it agreed with the engine on all 22,000 seeded cases at seed 0x6774652D70726F70, cycles included, so no engine fix was needed
- [Phase 03]: 03-14: a list node costs 20 cycles of header fetch and setup plus its words with a 6-cycle CPU window between nodes, fitted to the chain-looping log (1.625 against 1.598, band 1.4 to 1.8); the first pair locked in phase with the test loop and showed no slowdown
- [Phase 03]: 03-14: the 1000h-node guard is a gap that reaches past the step's tick, nodes_done wraps and is bounded only by the u32 range, and the CHCR word 0 stops a channel with no flag and no interrupt (the chain-looping test procedure); chopping windows cost ceil(17 * dw / 16) + 6 with a CPU window of min(2^m, 32 * dw), mean signed error 1.0 percent and largest row error 10.4 percent against the 64 hardware rows
- [Phase 03]: 03-15: sync mode 3 on a non-OTC channel with the start bit does not start and latches SCHWAY_FAULT_DMA (10, API version 3); start bits dropped, other CHCR bits kept
- [Phase 03]: 03-16: dma_chain records a list outcome as one value (bit 0 finished and bit 1 the DICR flag), the OTC time of 4,096 words reads 4,362 clocks through Timer 2 against the doc range 4,340..4,400, and no citation carries a comma or a semicolon because the judge splits rows with a CMake list
- [Phase 03]: 03-24: the split case compares run_to targets, not run lengths, because a run with a program stops on the first instruction boundary at or after its budget; the program-loaded runs are made in NTSC-U and PAL and the multi-million-cycle ones in NTSC-U alone (2.2 s native)
- [Phase 03]: 03-17: timers_lines installs its handler at the exception vector and snapshots counters against Timer 1 reset by the vertical blank — The stub path writes status FFh and costs about 700 cycles an entry, and a blank can land behind a timer handler, so the delay between blank and read must cancel
- [Phase 3]: Seed variants come from a row's place among rows of the same code kind, not a struct field; the armips unity build leaves all 24 fixture digests unchanged
- [Phase 03]: 03-19: --hash-state hashes the saved state after its 20-byte header; hashes.txt carries the frame and state of hello, gte_vectors, dma_chain and timers_lines, and the gate total stays at 1513 — dma_chain prints an unbuilt line first, so hashes.cmake takes the last two lines with a capture group; the unbuilt count is in the state hash
- [Phase 03]: 03-21: a source comment cites docs/hardware/<page>.md and a heading that exists on that page, checked by script; a hardware page cites psx-spx and says the core's own model for a cost, so no citation points back to a preparation section
- [Phase 03]: 03-22: test and fixture comments cite docs/hardware pages and headings; the stub keeps PSX-SYSTEM sections 7 and 9
- [Phase 03]: 03-23: the preparation sections that docs/hardware pages replace keep their headings, Sources rows and the C3 to C6 and 5.A choice paragraphs, with the corrections phase 3 found stated in each
- [Phase 03]: 03-25: todos 016 and 017 bind to SEED-007 (no later first caller), 014 to the phase 4 System.dat plan; every phase-2 review finding is fixed (WR-06, IN-01) or deferred with one todo (025-029, 032-037)
- [Phase 04]: GPU smoke cases reuse the existing core test executable; a GPU-specific executable belongs to its first behavior implementation plan.
- [Phase 04]: SIO, card, compatibility, and built-input registration seams start with empty inventories until real behavior tests exist.
- [Phase 04]: User-mode kernel fetches reuse the data-access permission check to raise guest AdEL before bus access.
- [Phase 04]: Saved Cause.IP2 is validated against serialized I_STAT and I_MASK before state application.
- [Phase 04]: An RFE return to a taken branch target past EPC resets the non-interrupt exception-loop count; same-EPC and interrupt paths retain it.
- [Phase 04]: The bounded GPU word-entry path serves CPU MMIO and future DMA callers.
- [Phase 04]: API v4 adds a read-only native VRAM span while preserving API v1-v3 callers.
- [Phase 04]: GPU save-state validation checks candidate parser fields before applying state.
- [Phase 04]: Copies process source and destination coordinates in row-major order, making overlap reads observe earlier writes in the same packet.
- [Phase 04]: Texture and CLUT cache data is instance-owned and serialized; GP0(01h) clears texture entries and GP1(01h) invalidates the CLUT row.
- [Phase 04]: Cache behavior is a deterministic model contract for rendering and is not described as hardware-measured.
- [Phase 04]: Reserved texture depth three follows the direct fifteen-bit model and remains a documented model rule.
- [Phase 04]: Primitive callers decide dither eligibility; rectangles and raw textured semi-transparent polygons do not acquire dither.
- [Phase 04]: Blend width and truncation remain doc/derived until a hardware capture distinguishes the internal arithmetic.
- [Phase 04]: Use 64-bit integer area and edge setup, normalize winding, and assign exact-edge samples with a top-left rule.
- [Phase 04]: Reject offset-applied spans above 1023 by 511 before clipping; retain inclusive E3/E4 edges.
- [Phase 04]: Split quads into triangles (1,2,3) then (2,3,4) and write each covered pixel immediately through the shared helper.
- [Phase 04]: Store Gouraud and UV attributes as signed 32-bit 16.16 steps, using wider setup and wide evaluation when a step is unrepresentable.
- [Phase 04]: Treat 16.16 rounding as the deterministic model; hardware precision awaits plan 30 pinned capture comparison.
- [Phase 04]: Advance attributes from original geometry when clipping polygon spans.
- [Phase 04]: Plan 04-08: GPU v0 uses additive profile flag value 4; default remains v2 and frozen API v1-v3 callers remain unchanged.
- [Phase 04]: Plan 04-08: Odd-UV/odd-width rectangle sampling remains uncharacterized pending plan 30 pinned capture comparison.
- [Phase 04]: GPU beam catch-up uses instance-owned rational carry and deadline scheduling; dot IRQs do not poll each dot or recurse into the scheduler.
- [Phase 04]: Loaded-content frame completion is driven by beam wrap, while the reset-resistant public-call ceiling and no-content test-card path remain independent.
- [Phase 04]: Keep no-content test-card scanout and pinned output separate from guest-programmed GPU scanout.
- [Phase 04]: Use one bounded native video validator for runner hashing, PNG output, and host conversion.
- [Phase 04]: Leave libretro dynamic geometry notifications and public runner documentation to plans 04-27 and 04-32.
- [Phase 04]: Keep byte completion, pad ACK assertion, and ACK release at separate scheduler ticks; keep card timing distinct.
- [Phase 04]: Store SIO and pad state per instance and serialize it through the existing visitor without changing API or state format revisions.
- [Phase 04]: Leave GAME-03 partial until DualShock, card, host-input, and integrated gameplay plans complete their behavior.
- [Phase 04]: Keep state format 4 and scalar nodes_done shape; store linked-list counts in each channel's remain slot.
- [Phase 04]: Treat busy DMA sync-mode-3 rewrites as a derived fail-closed policy because console behavior is unmeasured.
- [Phase 04]: Accept valid DMA phases 0 through 5 and reject only out-of-range phases and invalid cross-field links.
- [Phase 04]: Keep the approved API at version 4 and preserve frozen v1-v3 callers while appending host input and result fields.
- [Phase 04]: Keep pending host input outside save states; serialize frame-latched controller input and in-flight protocol state.
- [Phase 04]: 04-12: Keep dot totals derived from documented beam rates and line lengths; correct CPU sampling cadence using the PAL /4 endpoint trace.
- [Phase 04]: 04-12: Put GPU-motion frame and continuation-state hashes into the portable lane artifacts, including be32.
- [Phase 04]: API v4 memory and card names remain additive under the unreleased v4 policy; frozen v1-v3 callers remain unchanged.
- [Phase 04]: Card bytes and generations stay outside ordinary states; only in-flight card protocol state is serialized.
- [Phase 04]: Card ACK and Read pause timing are deterministic model values, not new physical hardware measurements.
- [Phase 04]: Measure gpu_motion against the same fixture at an explicit control commit; preserve the separate test-card ceiling.
- [Phase 04]: Use frames 10 through 60 for gpu_motion after the original hosted Cachegrind window exceeded its job budget.
- [Phase 04]: Keep all required CI lanes and retain a provisional five-minute perf-job timeout based on measured successful runs.
- [Phase 04]: SIO card writes stage payload per slot and commit only at terminal 47h; failures and interruption preserve committed bytes and generation.
- [Phase 04]: Keep the unreleased API, state format, and behavior revision at 4; preserve frozen v1-v3 callers.
- [Phase 04]: Accept raw imports up to the fixed image size, pad the suffix with FFh, and reject all invalid or late imports before mutation.
- [Phase 04]: Keep persistent card bytes and generations out of ordinary states while validating pending transport before state application.
- [Phase 04]: Use exclusive sibling temp names and fail without mutation if all 64 candidates are occupied.
- [Phase 04]: Card files remain host-owned; the core does not read files or resolve save paths.
- [Phase 04]: Content basename is the default card save-name source; validated serial naming is optional.
- [Phase 04]: Keep the runner's input-file I/O outside the emulator core and validate the entire script before execution.
- [Phase 04]: Use one-based frame records with explicit digital or DualShock devices, documented button symbols, and left-X/left-Y/right-X/right-Y axes.
- [Phase 04]: Keep API revision 4 and preserve the no-content frame hash; the saved state hash proves latched input reached the core.
- [Phase 04]: Use the guest SIO0 path and GPUREAD mailbox records to prove that scheduled input changes actual counter pixels.
- [Phase 04]: Poll every other VBlank so runner lag totals remain observable while mailbox peeks run each frame.
- [Phase 04]: Default card basenames come from the executable content name, or schwaystation for a content-free run; serial naming requires the caller to provide a validated serial.
- [Phase 04]: The runner saves a slot only after a successful card Write advances its generation; signal handlers only request a stop.
- [Phase 04]: Save states store BIOS loaded state and SHA-256 identity, not BIOS bytes, and reject a different image before mutation.
- [Phase 04]: Public SHA-256 image identity remains separate from the DAT-based retail recognition planned in 04-38.
- [Phase 04]: Normalize formatted blank-card sector value A0h to visible slot counters 1 and 2, then increment each saved slot independently.
- [Phase 04]: Wait for 117 guest VBlanks so the mailbox and selected frame-120 capture complete under the runner's 120-frame cap.
- [Phase 04]: BIOS recognition matches the pinned DAT MD5 rows; SHA-1 remains metadata, public SHA-256 is independent, and unknown images stay loadable.
- [Phase 04]: Keep firmware-size and content-marker checks inside the existing release firmware scanner gate.
- [Phase 04]: Generate synthetic digest keys and content-marker samples only in build-directory fixtures.
- [Phase 04]: Use the upstream-supported OPENBIOS_SHELL=shell target with pinned nugget source and Linux GCC; no custom standard-library overlay is needed.
- [Phase 04]: Raise the be32 job timeout to seven minutes based on the expanded 433-test suite; the exact-head lane completed in 6m09s with all tests retained.
- [Phase 04]: Keep OpenBIOS transfer in a one-day current-run artifact; consumers verify expected size and SHA-256 before use, with no cross-run cache or package/publication path.
- [Phase 04]: Preserve distinct empty-input, repeated-held, and three-edge guest proofs at frame 300; reuse the existing one-edge pin instead of repeating a redundant guest run.
- [Phase 04]: Runner BIOS recognition follows the image digest, never the filename; exact-size unknown images remain loadable.
- [Phase 04]: The runner rejects simultaneous --bios and --exe selectors and preserves the BIOS-free PS-EXE stub-kernel path.
- [Phase 04]: Synthetic BIOS fixtures are generated only under the build directory; actual OpenBIOS shell observation is deferred to plan 04-40.
- [Phase 04]: Accept only source-identified width-one writes to physical 1F802041h as ignored POST status, preserving EXP2 gap faults and existing debug-port behavior.
- [Phase 04]: Keep API, state and behavior revision 4; model only exact halfword ATTR readback as saved instance state.
- [Phase 04]: Derive DMA4 RAM-to-device readiness from saved ATTR bits 5-4 equal to mode 2; keep manual DREQ credits independent.
- [Phase 04]: Keep the deterministic sink, existing unit cost and serialized DMA fields; no new state layout or revision is needed.
- [Phase 04]: Plan 04-53: save one GPU field-status phase because bit 13 retains its value after IRS is disabled; expose v2 bit 13 and mask v0.
- [Phase 04]: Plan 04-53: preserve the fixed 263/314-line beam model and document unmodeled physical-field timing instead of claiming exact timeline parity.
- [Phase 04]: Pin NTSC-U frame 240 because it is the earliest independently inspected frame with a clearly recognizable cube shell.
- [Phase 04]: Make the shell test a CTest fixture setup for runner.hashes_file so tuple provenance is fresh in each run.
- [Phase 04]: Require OpenBIOS only at consumer execution time and reuse the existing comparison helper for portable refusal controls.
- [Phase 04]: Plan 04-27: default no-content loads keep the test card; selected BIOS images apply only to no-content boot, while PS-EXE content always uses the stub.
- [Phase 04]: Plan 04-27: validate EXE structure before refusing valid late loads to preserve frozen API error statuses.
- [Phase 04]: Compatibility rows use (serial, MD5, predicate) keys; fresh fixture rows and statuses must match in both directions.
- [Phase 04]: Ratchet accepts an optional prior ledger; protected PR-base capture and artifact plumbing remain in plan 04-41.
- [Phase 04]: Compatibility allowances identify behavior revision 4 and expire within an inclusive maximum 60-day window.
- [Phase 04]: Resumed plan task-order deviation: remaining Task 1 judge/docs and Task 2 controls landed together after the preserved RED and partial GREEN commits; no second RED or history rewrite.
- [Phase 04]: Protected compatibility baseline is selected from the exact pull-request base SHA; only both artifacts absent selects introduction.
- [Phase 04]: Preserve the exact sparse base checkout in RUNNER_TEMP before head checkout so workspace cleanup cannot remove the protected judge.
- [Phase 04]: Native libretro video uses the shared bounded converter for RGB555 and packed RGB24 before host callbacks.
- [Phase 04]: SET_GEOMETRY is sent only when dimensions change, and SET_SYSTEM_AV_INFO only when the rate changes.
- [Phase 04]: Plan 04-25 probes direct raw ISO9660 metadata only; CUE and track modes remain with 04-49.
- [Phase 04]: Plan 04-25 reports boot and play as unavailable and never starts the core or submits sectors.
- [Phase 04]: The synthetic track SHA-1 is computed independently by CMake; production identity reuses the existing MD5 implementation.
- [Phase 04]: Keep CUE parsing in the runner's host-only probe; it does not execute a disc or add CD-ROM behavior.
- [Phase 04]: Cap each CUE sheet at 64 KiB, tracks and total indexes at 99, relative FILE names at 255 bytes, and companion image bytes together at 1 GiB.
- [Phase 04]: Preserve boot_status and play_status as unavailable because the probe only reports identity.
- [Phase 04]: Expose a disc serial, boot MD5, physical track-1 SHA-1 and DAT title only for an exact passing metadata.identity ledger row.
- [Phase 04]: Keep boot_status and play_status unknown; the probe does not execute a disc.
- [Phase 04]: Use the pinned libretro-database PlayStation DAT only as an attributed title source; do not vendor it or add a package.
- [Phase 04]: Keep test metadata arguments in a dedicated non-installed test runner and require an isolated root and marker file.
- [Phase 04]: The local library and isolated tests share the production runner metadata lookup path.
- [Phase 04]: Unknown and unlisted discs remain private, and boot/play remain unavailable until measured by a disc backend.
- [Phase 04]: The approved Phase 4 repository-check ceiling is 2700 lines; plan 04-26 measures 2448 lines with all checks retained.
- [Phase 04]: Keep libretro controller-family selection separate from the DualShock digital/analog wire protocol.
- [Phase 04]: Use independent serialized-state references per controller family and prove full analog packet/axis behavior with a separate real guest.
- [Phase 04]: Deliver both ports’ strong and weak rumble reports once per frame with exact scaling and lifecycle clearing.
- [Phase 04]: The frontend owns card-one .srm writes; libretro exposes the stable SAVE_RAM span and does not write the file.
- [Phase 04]: Card two uses GET_SAVE_DIRECTORY and the canonical content basename, with atomic persistence gated by a changed successful-write generation.
- [Phase 04]: Memory descriptors map existing RAM and scratchpad spans with explicit aliases and are cleared with adapter lifetime.
- [Phase 04]: Keep disk-control, cheats, save states, and subsystem claims false until their behavior exists.
- [Phase 04]: Advertise PSEXE only after the shared loader passes a case-varied PSEXE fixture.
- [Phase 04]: Back the input-descriptor claim with assertions for all mappings and their callback lifetime.
- [Phase 04]: Preserve repository CI defaults and use an ignored four-job preset for local bounded verification.
- [Phase 04]: Keep GAME-03, GAME-06 and GAME-07 acceptance pending until their full hosted/runtime evidence is available.
- [Phase 04]: Validate the local OpenBIOS handoff, then place it at the test integration's expected binaryDir path before configuring a temporary workflow preset.
- [Phase 04]: Use independently captured reference extents to poison movie active pixels; invalid extents retain full-buffer fallback.
- [Phase 04]: Treat the byte reduction as measured work only; runtime improvement and timeout causality require hosted timing.
- [Phase 04]: Use one file-local, NULL-initialized fopen_s/fopen helper at all five host disc-probe binary opens while preserving error behavior.
- [Phase 04]: Keep fixture production available to selected ASan/fuzz libretro consumers while the selected inventory controls standalone test registration.
- [Phase 04]: Hash all 1024x512 VRAM words after clearing bit 15 and serializing each word low byte first.
- [Phase 04]: Keep the PNG reference decoder synthetic-only; extend it for indexed 4-bit and dynamic-DEFLATE captures before 04-46 runs its captured clipping oracle.
- [Phase 04]: The selected capture adapter uses host Python standard-library features only and emits build-tree RGB555LE pixels for the ten pinned PNGs.
- [Phase 04]: The clipping judge accepts a FRAME or a no-fault progressing BUDGET stop and compares the fixed whole-VRAM golden.
- [Phase 04]: The test BIOS stub supports SYS(01h)/SYS(02h) with SWMB precedence and keeps reset-installed entry bytes stable.
- [Phase 04]: An ignored local CMake user preset caps CTest at four jobs while tracked and hosted defaults remain unchanged.
- [Phase 04]: 04-58: Keep the overlap image comparison at eight advances; the separate caller-progress witness uses a source-derived 24-advance allowance after eight advances completed 10,633 of 27,657 uploads.
- [Phase 04]: 04-58: IRQ callback, acknowledgement, return and VSync progress do not establish pixel agreement; the 34,390 overlap image differences remain with checkpointed 04-47.
- [Phase 04]: Exact GPU allowance variants are tied to the reviewed executable SHA-256 as well as their exact pixel rules and use counts.
- [Phase 04]: Overlapping VRAM copies use bounded 16-word read batches as a capture-backed model; the selected batch size is not claimed as physical hardware width.
- [Phase 04]: The rectangle and overlap comparisons use fixed 32- and 24-advance bounds supported by source work and caller-progress evidence.
- [Phase 04]: Keep all ten GPU captures and three real logs in a required exact-head hosted lane.
- [Phase 04]: Use project-owned GPU contracts and vetted hardware references; exclude the forbidden GPL helper.
- [Phase 04]: Extend the existing replay infrastructure with independent hostile-input state and memory assertions.
- [Phase 04]: Use the existing hosted RetroArch driver to compare two distinct BIOS-free frames against runner references.
- [Phase 04]: Correct stale plan metadata to the actual tests/retroarch/verify.py comparator.
- [Phase 04]: Exercise every included API v4 addition with a strict registered caller and retain frozen v1-v3 callers.
- [Phase 04]: Document retail BIOS supply, the MIT OpenBIOS CI stand-in and distinct runner/libretro card naming behavior.
- [Phase 05]: SPU sample production belongs to the scheduler and uses the existing public PCM ring.
- [Phase 05]: SPU mutable state is fixed-size and embedded in each emulator instance.
- [Phase 05]: Use the public build-158 transfer executable as a fetch-only build asset, with archive and member digests checked before use.
- [Phase 05]: Treat the seven exact transfer log lines as transfer, status, and timing evidence only; they do not provide a PCM oracle.
- [Phase 05]: Accept only ordered one-to-one pass cases with exit code zero and the completion marker; allow CRLF and TTY prefixes.
- [Phase 05]: Pass saved scheduler ticks into the private SPU visitor to validate sample phase before applying decoded state.
- [Phase 05]: Advance behavior revision and save-state format to 5, reject format-4 states, and preserve the public API and ABI.
- [Phase 05]: Guest SPU register writes latch at the next scheduled sample tick; byte and word access order is explicit.
- [Phase 05]: Manual FIFO, capture, and IRQA behavior is isolated in plan 05-20, and its consumers remain blocked until completion.
- [Phase 05]: SPU FIFO accesses update the FIFO at bus-access time, while other register writes apply on the next sample tick.
- [Phase 05]: ManualWrite transfers one halfword per sample with RAM_CTRL replication; DMA direction status reflects FIFO availability without adding DMA transfers.
- [Phase 05]: Capture writes the planned zero inputs only while RAM_CTRL capture bits are active, and IRQ9 requires both enable bits.
- [Phase 05]: DMA4 readiness observes the latest queued ATTR value while public status remains scheduled.
- [Phase 05]: The own stub implements only literal and %s output for the pinned transfer guest A(3Fh) path.
- [Phase 05]: Manual FIFO data drains at the scheduled transfer tick so the bounded guest transfer cases complete.
- [Phase 05]: Move 24-voice ordering, loop/ENDX, PMON and stereo mixing to plan 05-22 so plan 05-06 remains one focused BRR interpolation tracer.
- [Phase 05]: DMA4 sync-mode-0 starts from CHCR bit 24 alone, matching the pinned guest register writes; other DMA channels retain their bit-28 trigger requirement.
- [Phase 05]: Use the documented integer ADSR and sweep baseline; treat the ares table as conformance data, not a hardware waveform.
- [Phase 05]: Keep high-rate and phase-negative sweep behavior unresolved without a direct hardware capture; do not add a profile switch.
- [Phase 05]: SPU voice expansion reuses existing serialized fields; no save-state visitor or size change was needed.
- [Phase 05]: Keep the authored BRR fixture integer-generated and commit source plus manifest digest, not assembled bytes.
- [Phase 05]: Measure actual voice levels from a sealed guest mailbox at frame 600.
- [Phase 05]: Preserve a unity 22.05 kHz feedback seam in plan 05-09; plan 05-10 supplies the register-driven feedback network.
- [Phase 05]: Describe the measured 38-sample FIR delay and signal path without claiming an exact silicon preset waveform.
- [Phase 05]: Kept production SPU code unchanged because existing scheduled-sample behavior already met the new deterministic and bounded assertions.
- [Phase 05]: Added the SPU fuzzer to the explicit fuzz preset target list and wired the existing hosted perf job to CASE=spu_idle.
- [Phase 05]: Keep the hash at the native 44.1 kHz stereo PCM boundary before host processing.
- [Phase 05]: Preserve SOUND-01 as pending until final Phase 5 verification.
- [Phase 05]: Keep per-stage rounding and saturation placement unresolved; the implementation uses a deterministic integer baseline.
- [Phase 05]: Treat the ten preset impulse digests as local regression values, not hardware waveform claims.
- [Phase 05]: Leave SOUND-01 and SOUND-02 pending until final Phase 5 verification.
- [Phase 05]: Store complete frame-600 PCM digests and lengths as self-authored regression baselines, not hardware waveform claims.
- [Phase 05]: Require the sealed mailbox CRC, completion status and complete ordered records before judging frame-600 ENVX.
- [Phase 05]: Keep SOUND-01 and SOUND-02 pending until final Phase 5 verification.
- [Phase 05]: Treat docs/hardware/spu.md as the current Phase-5 SPU contract, preserving source pins, open precision questions and separate evidence claims. — The implementation is now the maintained contract; concise preparation pointers and evidence limits prevent stale behavior descriptions or unsupported PCM accuracy claims.
- [Phase 06]: Top-level builds expose CD-ROM and MDEC object targets; embedded builds compile their sources directly into the library to preserve the add_subdirectory target contract.
- [Phase 06]: Subsystem inventories remain empty until their behavior callers are implemented and registered.
- [Phase 06]: Phase 6 API v5 installs the first disc once with schway_open_disc; replacements require an open lid and schway_swap_disc.
- [Phase 06]: Host disc callbacks and checked TOC metadata are copied into non-serialized instance state; invalid metadata and closed-lid swaps validate before apply.
- [Phase 06]: CD-ROM and MDEC state uses fixed per-instance buffers sized for the accepted algorithms.
- [Phase 06]: Host disc callbacks and TOC metadata remain outside device state visitors.
- [Phase 06]: Scheduler event ties follow timers, GPU, DMA, CD-ROM, SPU, SIO, MDEC.
- [Phase 06]: Keep ISO/cue parsing and all host file handles in a host-only provider; the emulated core receives sectors only through callbacks.
- [Phase 06]: Model INDEX 00 as physical file content and PREGAP as synthetic silence, preserving their distinct sector offsets.
- [Phase 06]: Reject unsupported image types with named planned-v2.0 results instead of partial format sniffing.
- [Phase 06]: Keep the API v5 consumer public-header-only; core.disc_api owns internal attachment-state assertions.
- [Phase 06]: Playlist index selection prepares stable descriptors; the host alone chooses when to swap the active core disc.
- [Phase 06]: Shared CNF boot-path components omit ISO version suffixes before directory lookup.
- [Phase 06]: Append CD-ROM and MDEC visitors after SPU and advance the state/behavior revision to 6.
- [Phase 06]: Keep host callbacks and the external audio drain ring outside serialized machine state.
- [Phase 06]: Use SIO1 for remaining unbuilt-device diagnostics after CD-ROM and MDEC become owned bus devices.
- [Phase 06-discs-and-fmv]: Use cdrom.* selector and inventory names for the CD behavior lane.
- [Phase 06-discs-and-fmv]: Keep CD sector acquisition and INT1 delivery in Plan 06-05.
- [Phase 06-discs-and-fmv]: Serialize CD unlock, lock and licensed-profile state with the device.
- [Phase 6]: Serialize BFRD, ReadS selection, retry state, and deferred INT1; bump state format and behavior revision to 9.
- [Phase 6]: Parse and CRC-check supplied interleaved Q; synthesize absent or invalid Q from the TOC while retaining the prior location on bad_q.
- [Phase 6]: Select XA audio before guest data delivery; defer XA decoding and audio output to the later XA/SPU plans.
- [Phase 6]: Keep DMA3 sector-cursor and 24/40-cycle transfer behavior in sequential Plan 06-27.
- [Phase 06]: DMA3 readiness requires CHCR trigger, BFRD and buffered sector data; CD-ROM uses no DREQ line.
- [Phase 06]: Split chopped-window readiness into Plan 06-28 before the pinned Getloc caller.
- [Phase 06]: Recheck CD BFRD and buffered-sector readiness before every chopped DMA3 window.
- [Phase 6]: BUSYSTS tracks command acceptance independently from active CD drive status.
- [Phase 6]: Pinned raw Q/GetlocP seek rows move to dependent Plan 06-30 because their offsets differ.
- [Phase 6]: GetlocP returns the latest CRC-valid host Q record; synthesize a TOC record only before a valid record exists.
- [Phase 6]: Preserve the last valid Q record across absent or invalid host subcode.
- [Phase 6]: Pinned seek rows are separate fixtures; no general target offset applies.
- [Phase 06]: Keep fixed 451,584/225,792-cycle schedule assertions separate from the pinned drive timing bands.
- [Phase 06]: Treat positive same-speed, read-active intervals between consecutive LBA callbacks as timing samples; exclude nonconsecutive LBA gaps and reject buffer overflow.
- [Phase 06]: Keep DISC-01 pending until the remaining Phase 6 media, fuzz, and integration acceptance is complete.
- [Phase 6]: Keep XA predictor history separate from resampler history so low-rate duplication does not alter ADPCM chronology.
- [Phase 6]: Use the cited Q15 tap table with deterministic integer truncation; this does not claim silicon waveform proof.
- [Phase 6]: Reject XA emphasis and reserved coding/header values until their effects are defined.
- [Phase 06]: MDEC single-block availability uses a selected 1,000-cycle baseline; the unstamped reference log does not measure cycles.
- [Phase 06]: Plan 06-31 completes the mono stream, FIFO backpressure, raw layout and documentation controls before Plan 06-12.
- [Phase 6]: Keep the completed MDEC stream/backpressure scope closed and move missing pixel-order, clipping, and FE00-padding vectors to dependent Plan 06-32; DISC-03 remains partial.
- [Phase 6]: Add nonuniform AC, signed endpoint clipping, and FE00 pre-DC vectors before the pinned MDEC guest caller.
- [Phase 6]: Keep CDDA storage bounded to 1,176 stereo pairs and drop newest pairs on overrun.
- [Phase 6]: Apply staged ATV coefficients atomically and serialize pending and active matrices.
- [Phase 6]: Split the unstarted XA mixing task into dependent Plan 06-33 at the 30-minute bound.
- [Phase 6]: Keep XA in a 9,409-pair serialized ring (9,408 frames plus one tied-event reserve) separate from the 1,176-pair CDDA queue.
- [Phase 6]: Keep voice/reverb mixture and mode-enabled file/channel rejection in dependent Plan 06-34; Plan 06-12 MDEC work remains independent.
- [Phase 06]: The existing XA filter and native mixer paths satisfy the mode-enabled mismatch and XA/CDDA/voice/reverb controls; retain this slice as test-only.
- [Phase 06]: Keep the pinned MDEC guest selectors gating and assign the production DMA0 input/decode mismatch to dependent Plan 06-35.
- [Phase 6]: Split Plan 06-36 after its 30-minute bound: it closes the DMA1 destination harness and exact RED vectors; Plan 06-37 owns the source-backed 4/8-bit arithmetic and ordered status repair.
- [Phase 6]: Keep the v1.0 MDEC acceptance claim limited to deterministic decoding with available verified evidence; preserve the original hardware arrays, arithmetic, cycle cap and owned disc_boot path.
- [Phase 6]: Keep DISC-03 and 06-23/24 acceptance pending until successor registration and final local/hosted acceptance in 06-43 through 06-45.
- [Phase 6]: Preserve the pinned 8-bit hardware log and bytes as evidence; compare deterministic model/reference regression output to its separately named frozen baseline.
- [Phase 6]: Do not run compressed pinned reference guests until Plan 06-42 installs the in-process source guard; Plan 06-43 owns selector registration.
- [Phase 07]: Redact path text through its next quote or boundary to protect path components containing spaces.
- [Phase 07]: Compare decimal digit strings exactly so rounding cannot admit a value just above a large integer bound.
- [Phase 07]: Correct the temporary repository-check budget to 3900 after measured Plan 05–06 growth and the remaining Plan 07–08 control estimates; retain the separate RetroArch budget at 300 and ratchet to the actual total at ship.
- [Phase 07]: Withhold the WASI module size bound until the focused Release target measurement runs with the Ubuntu 26 WASI toolchain.
- [Phase 07]: Keep current settings, action pins, and runner labels where evidence did not justify changes; review Wasmtime 48.0.9 in the normal pin-update step using new upstream asset digests.
- [Phase 07]: Documentation and upgrade path tied at 2; selected the README correction because it resolves a false current capability claim with immediate user impact.
- [Phase 07]: Keep compatibility statements limited to the four passing fixture rows; do not imply commercial-game coverage.
- [Phase 07]: Bound open-issue collection to one 100-result page and set issues_truncated when that cap is reached.
- [Phase 07]: Grant the triage job only actions, contents, and issues read scopes.
- [Phase 07]: Require fresh release App identity and stable paginated metadata before the generated-only PR fast route.
- [Phase 07]: Keep unsampled workflow timeouts provisional and TUNE-02/03 pending until the hosted release PR comparison at ship.
- [Phase 07]: Keep size.wasm measured but unbounded until the focused Ubuntu 26 Release build produces the actual module size.
- [Phase 07]: Feed measured Cachegrind instruction counts to the shared bound comparator while preserving the existing base and relative gate.
- [Phase 07]: CI timing compares completed current-attempt dependency paths only when the workload fingerprint matches; changed fingerprints remain explicitly unbaselined.
- [Phase 07]: Keep the in-progress CI required job out of elapsed-path arithmetic; final matching PR measurement remains ship evidence.
- [Phase 07]: The README install command remains unchanged because it still matches the automated install control and attributable release evidence.
- [Phase 07]: The actual WASI runner module remains unmeasured; size.wasm stays unbounded until the Release module is directly measured.
- [Phase 07]: Device MIPS remain assumptions; the measured gpu_motion workload exceeds provisional low-power budgets and stays a named SEED-005 follow-up.
- [Phase 07]: Use an 80-second policy.quality_metrics timeout from the 38.61-second serialized measurement; this adds headroom and is not a speedup.
- [Phase 07]: Keep matched release-App timing, triage artifact, actual WASI module size, release, and hosted verification explicitly pending until ship.
- [Phase 8]: Preserve caller-supplied serial spelling for literal runner card filenames.
- [Phase 8]: Map a true libretro_saves claim to both card persistence and controls tests while keeping the template false.
- [Phase 8]: Use a standard card ID guest as the reusable transport base for later PocketStation transfers.
- [Phase 08]: Directory entry size occupies bytes 4–7 in the pinned source, but its byte order is unspecified; preserve a focused probe for the first 08-06 storage caller.
- [Phase 08]: Keep existing DEC.60 ACK scheduling where PSX-SPX gives no command-specific delay; fixed BCD and unsupported storage commands are deterministic bounds, not RTC or PocketStation CPU emulation.
- [Phase 08]: Resolve one save identity from a complete PS-EXE stem or the framed full-image digest; explicit serial overrides remain literal.
- [Phase 08]: Keep runner auto-identity opt-in and preserve libretro frontend-based card filenames.
- [Phase 8]: Format 18's new card protocol latches require only the two changed state pins; frame hashes and host-owned card bytes and generations stay unchanged.
- [Phase 09]: Feature ledger keys use provider, profile, and predicate; protection and peripheral are mandatory descriptive fields.
- [Phase 09]: Production compatibility allowance checks use the current UTC date; tests inject fixed dates.
- [Phase 09]: Protected delegation preserves the legacy five-argument title judge and appends kind only for feature-capable judges.
- [Phase 09]: Retain separate Main and Release CI gates: measured reuse has no wall-clock benefit, and same-source product archive digests differ across all eighteen outputs.

### Pending Todos
- [2026-10-20] [general] "Port the CI required roll-up check (policy.ci_required)" — [todo file](.planning/todos/pending/001-ci-required-gate.md)
- [2026-10-20] [general] "Port the release floor check (release.floor)" — [todo file](.planning/todos/pending/002-release-floor-gate.md)
- [2026-10-20] [general] "Port the pins file check (policy.pins)" — [todo file](.planning/todos/pending/003-pins-gate.md)
- [2026-10-20] [general] "Port the single version source check (release.single_version_source)" — [todo file](.planning/todos/pending/004-single-version-source-gate.md)
- [2026-10-20] [general] "Port the notice checks (policy.notices and release.notices)" — [todo file](.planning/todos/pending/005-notices-gates.md)
- [2026-10-20] [general] "Port the vendored-file digest check (libretro.vendored)" — [todo file](.planning/todos/pending/006-vendored-digest-gate.md)
- [2026-10-20] [general] "Add the Dependabot configuration" — [todo file](.planning/todos/pending/010-dependabot-config.md)
- [2026-10-20] [general] "Add the Dependabot auto-merge workflow" — [todo file](.planning/todos/pending/011-dependabot-automerge-workflow.md)
- [2026-10-20] [general] "Port the Dependabot auto-merge decision check" — [todo file](.planning/todos/pending/012-dependabot-gate.md)
- [2026-10-20] [general] "Port the test-only key check" — [todo file](.planning/todos/pending/014-no-test-entries-gate.md)
- [2026-10-20] [general] "Add the weekly digest workflow" — [todo file](.planning/todos/pending/015-digest-workflow.md)
- [2026-10-20] [general] "Add the formatting check (hygiene.format)" — [todo file](.planning/todos/pending/016-hygiene-format-gate.md)
- [2026-10-20] [general] "Add the static analyzer check (hygiene.analyzer)" — [todo file](.planning/todos/pending/017-hygiene-analyzer-gate.md)
- [2026-10-20] [general] "Teach a local actionlint run the runner labels and queue key the workflows use" — [todo file](.planning/todos/pending/021-actionlint-local-config.md)
- [2026-10-20] [general] "Triage the one-off install.consumer.dll_crt failure on windows-x64" — [todo file](.planning/todos/pending/022-windows-dll-crt-consumer-flake.md)
- [2026-11-10] [general] "Raise an address error for a user-mode instruction fetch outside KUSEG" — [todo file](.planning/todos/pending/025-user-mode-fetch-address-error.md)
- [2026-11-10] [general] "Validate r0 and the COP0 registers when a state loads" — [todo file](.planning/todos/pending/026-state-load-cop0-and-r0-validation.md)
- [2026-11-10] [general] "Keep a valid cached word when a later refill word bus-errors" — [todo file](.planning/todos/pending/027-icache-refill-bus-error-word.md)
- [2026-11-10] [general] "Stop the exception-loop heuristic reporting a guest that makes progress" — [todo file](.planning/todos/pending/028-exception-loop-progress-heuristic.md)
- [2026-11-10] [general] "Zero the vector harness case record and reject doubly covered store bytes" — [todo file](.planning/todos/pending/033-vector-harness-uninitialised-record.md)
- [2026-11-10] [general] "Reject a long file stem in the SST slice tool" — [todo file](.planning/todos/pending/035-sst-slice-name-field.md)
- [2026-11-10] [general] "Refuse schway_load_exe on an instance that has already run" — [todo file](.planning/todos/pending/036-load-exe-after-run.md)
- [2026-11-10] [general] "Decide the bus contract for SWL and SWR and pin it with a test" — [todo file](.planning/todos/pending/037-swl-swr-single-bus-write.md)
- [2026-11-10] [general] "Model the two-instruction write-to-use latency of MTC2, CTC2 and LWC2" — [todo file](.planning/todos/pending/038-gte-write-to-use-latency.md)
- [2026-11-10] [general] "The DMA fault record uses addr and opcode like every other fault kind" — [todo file](.planning/todos/pending/042-dma-fault-record-fields.md)
- [2026-11-10] [general] "Share the register-lane helpers" — [todo file](.planning/todos/pending/044-lane-helpers-shared.md)
- [2026-11-10] [general] "Put the ISC permission text in the src/gte.c header" — [todo file](.planning/todos/pending/046-gte-file-header-notice.md)
- [2026-11-10] [general] "Select the 40-clock CD-ROM DMA cost from the CD-ROM delay setting" — [todo file](.planning/todos/pending/047-cd-dma-40-clock-cost.md)
- [2026-11-10] [general] "Split long GPU pixel test cases" — [todo file](.planning/todos/pending/048-split-gpu-pixel-test-cases.md)
- [2026-11-11] [general] "Review runner and checkpoint repeats against existing frame and state output" — [todo file](.planning/todos/pending/049-review-runner-checkpoint-repeats.md)
- [Phase 07]: Keep historical push timing bounds tied to their workload fingerprint; the PR route remains unbaselined until exact-head hosted runs prove comparability.
- [Phase 07]: Keep `size.wasm` unbounded until the linked Ubuntu 26 Release module is measured; set the bound only from its actual byte count.
- [Phase 07]: Infrastructure verification with zero behavior-unverified invariants has no owner UAT; hosted delivery checks are recorded as ship evidence.
### Blockers/Concerns

- [Phase 4]: Research flag: GPU blend width, size rejection, stepper precision, GPU clocks per line, and the adapter's stub-kernel path for PS-EXE content in RetroArch.
- [Phase 6]: Research flag: the CD seek model and INT1 period, and whether OpenBIOS boots the own mkpsxiso disc without licence data.
- [Phase 7]: Closed; hosted verification, main CI, release v0.7.0 and triage artifact passed. Follow-up efficiency investigation is in todo 050, bound to Phase 9.

### Quick Tasks Completed

| # | Description | Date | Commit | Directory |
|---|-------------|------|--------|-----------|
| 261118-bvm | Skip heavyweight main-push CI for planning-only changes | 2026-11-18 | 11c9789 | [261118-bvm-skip-heavyweight-main-push-ci-for-planni](./quick/261118-bvm-skip-heavyweight-main-push-ci-for-planni/) |

## Deferred Items

Items acknowledged and deferred at milestone close, most recent first. Acknowledgment does not resolve an item; its original pending or dormant status remains in its source ledger. Changed items can resurface at a later audit.

| Category | Item | Status | Deferred At | Milestone |
|----------|------|--------|-------------|-----------|
| seeds | SEED-001 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-002 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-003 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-004 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-005 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-006 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-007 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-008 | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-261116-mrh | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-261116-osn | dormant | 2026-11-18 | v1.0 |
| seeds | SEED-261118-8b0 | dormant | 2026-11-18 | v1.0 |
| todos | 001-ci-required-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 002-release-floor-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 003-pins-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 004-single-version-source-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 005-notices-gates.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 006-vendored-digest-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 010-dependabot-config.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 011-dependabot-automerge-workflow.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 012-dependabot-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 014-no-test-entries-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 015-digest-workflow.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 016-hygiene-format-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 017-hygiene-analyzer-gate.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 021-actionlint-local-config.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 022-windows-dll-crt-consumer-flake.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 025-user-mode-fetch-address-error.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 026-state-load-cop0-and-r0-validation.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 027-icache-refill-bus-error-word.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 028-exception-loop-progress-heuristic.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 033-vector-harness-uninitialised-record.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 035-sst-slice-name-field.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 036-load-exe-after-run.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 037-swl-swr-single-bus-write.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 038-gte-write-to-use-latency.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 044-lane-helpers-shared.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 046-gte-file-header-notice.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 047-cd-dma-40-clock-cost.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 048-split-gpu-pixel-test-cases.md | (presence-only) | 2026-11-18 | v1.0 |
| todos | 049-review-runner-checkpoint-repeats.md | (presence-only) | 2026-11-18 | v1.0 |
| deferred_items | 01/deferred-items.md: AppleClang Debug abi.undefined for memcpy_chk and memset_chk; ci Release passes; fix belongs with a future core-options or ABI-gate change | acknowledged | 2026-11-18 | v1.0 |
| deferred_items | 04/deferred-items.md: local actionlint does not recognize existing ubuntu-26.04 runner labels; hosted workflow passes with only those warnings ignored | acknowledged | 2026-11-18 | v1.0 |

## Runtime Log

Going forward, use Codex with the adaptive OpenGSD profile until the owner switches
runtime again. Heavy / Opus-tier work uses `gpt-6.1-sol` at `high` effort;
standard / Sonnet-tier and light / Haiku-tier work use `gpt-6-luna` at `xhigh`
effort. Resolve both model and effort for each agent dispatch. Project instructions
come from `AGENTS.md`.

| Date | Step | Runtime | Model |
|------|------|---------|-------|
| 2026-11-18 | v1.0 audit, completion and cleanup | Codex/OpenGSD 1.20.0; reused exact-tree hosted CI; no local long suite rerun | Primary GPT-6 session (exact model ID unavailable); integration checker gpt-6-luna xhigh |
| 2026-11-15 | /gsd-discuss-phase 6 (via /gsd-autonomous) | Codex, OpenGSD 1.20.0; three read-only advisors and adversarial synthesis | GPT-6 session (exact model ID not exposed); advisors gpt-6-luna xhigh |
| 2026-11-15 | /gsd-plan-phase 6 (via /gsd-autonomous) | Codex, OpenGSD 1.20.0; 26 plans, 19 waves; schema/structure, command/failure probes, decision and requirement coverage; post-planning gaps clear | GPT-6 session (exact model ID not exposed); planner gpt-6.1-sol high; researcher and checker gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-01 | Codex, OpenGSD 1.20.0; sequential shared checkout; subsystem skeleton and inventories; tracer 19/19 and policy 4/4; 12 min | gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-02 | Codex, OpenGSD 1.20.0; sequential shared checkout; disc callback API and API v1–v4 compatibility checks; 26 min | gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-03 | Codex, OpenGSD 1.20.0; sequential shared checkout; bounded CD/MDEC state, reset/visitor contracts, MMIO and fixed-order scheduler integration; focused lane 8/8 after each task; 18 min | gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-07 | Codex, OpenGSD 1.20.0; sequential shared checkout; bounded ISO/cue provider, focused image tests 8/8, selected disc/policy checks 12/12; duration not measured | gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-25 | Codex, OpenGSD 1.20.0; sequential shared checkout; versioned API v5 consumer and compatibility checks; 14 min | gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-08 | Codex, OpenGSD 1.20.0; sequential shared checkout; bounded M3U and SYSTEM.CNF parsing; disc suite 10/10, probe, inventory and hygiene checks; 35 min | gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-26 | Codex, OpenGSD 1.20.0; sequential shared checkout; CD-ROM/MDEC state serialization and fixture migration; state 8/8 and bus/unbuilt/gap checks 3/3; 20 min | gpt-6-luna xhigh |
| 2026-11-15 | /gsd-execute-plan 06-04 | Codex, OpenGSD 1.20.0; sequential shared checkout; CD commands, response timing, region/licence/lid behavior; focused checks 13/13; 107 min | gpt-6-luna xhigh |
| 2026-10-19 | /gsd-discuss-phase 1 | Claude Code, OpenGSD 1.18.0 | claude-opus-5-5 |
| 2026-10-19 | /gsd-plan-phase 1 | Claude Code, OpenGSD 1.18.0 | claude-opus-5-5 |
| 2026-10-19 | /gsd-execute-phase 1 | Claude Code, OpenGSD 1.18.0 | claude-opus-5-5 |
| 2026-10-20 | /gsd-execute-phase 1 (continued) | Claude Code, OpenGSD 1.18.0 | claude-opus-5-5 |
| 2026-10-20 | /gsd-code-review 1 | Codex, inline standard review | GPT-6 (session; exact model ID not exposed) |
| 2026-10-20 | /gsd-code-review 1 --fix | Codex, inline fix of CR-01; WR-01 remains a tooling seed | GPT-6 (session; exact model ID not exposed) |
| 2026-10-20 | /gsd-pause-work | Codex, handoff before phase verification | GPT-6 (session; exact model ID not exposed) |
| 2026-10-29 | /gsd-ship 1 | Codex, OpenGSD 1.19.0; inline shipping and hosted CI | GPT-6 (session; exact model ID not exposed) |
| 2026-10-29 | /gsd-verify-work 1 | Codex, OpenGSD 1.19.0; inline verification and hosted CI | GPT-6 (session; exact model ID not exposed) |
| 2026-10-29 | /gsd-quick phase-1 CI tail | Codex, OpenGSD 1.19.0; inline quick fallback | GPT-6 (session; exact model ID not exposed) |
| 2026-11-09 | /gsd-discuss-phase 2 | Claude Code, OpenGSD; seven advisor agents fanned out | claude-sonnet-5-5 |
| 2026-11-09 | /gsd-plan-phase 2 | Claude Code, OpenGSD; orchestrator claude-sonnet-5-5, planner claude-opus-5-5 (4 rounds), checker claude-haiku-5-5 (4 passes); research skipped | claude-sonnet-5-5 |
| 2026-11-09 | /gsd-execute-phase 2 | Claude Code, OpenGSD; orchestrator claude-sonnet-5-5, executors claude-sonnet-5-5 (sequential, no worktrees) | claude-sonnet-5-5 |
| 2026-11-09 | /gsd-ship 2 | Claude Code, OpenGSD; inline shipping, PR #8 | claude-sonnet-5-5 |
| 2026-11-09 | /gsd-discuss-phase 3 (via /gsd-autonomous) | Claude Code, OpenGSD; six advisor agents fanned out | claude-sonnet-5-5 |
| 2026-11-10 | /gsd-plan-phase 3 (via /gsd-autonomous) | Claude Code, OpenGSD; orchestrator claude-sonnet-5-5, planner claude-opus-5-5 (1 plan + 2 revision rounds), checker claude-haiku-5-5 (3 passes); research skipped; the last blocker (sweep vs sync-3 fault) and warnings patched by the orchestrator after pass 3 | claude-sonnet-5-5 |
| 2026-11-10 | /gsd-execute-phase 3 (via /gsd-autonomous) | Claude Code, OpenGSD; orchestrator claude-sonnet-5-5, 26 sequential executors claude-sonnet-5-5 (no worktrees) | claude-sonnet-5-5 |
| 2026-11-10 | /gsd-code-review 3 and fix round 1 | Claude Code, OpenGSD; deep review claude-sonnet-5-5, fixer claude-sonnet-5-5; CR-01 and WR-01 fixed, 7 info findings to todos 040-046 | claude-sonnet-5-5 |
| 2026-11-10 | /gsd-verify-work 3 (verify step) | Claude Code, OpenGSD; verifier claude-sonnet-5-5, status passed, no human items | claude-sonnet-5-5 |

| 2026-11-10 | /gsd-ship 3; renewed /gsd-autonomous for remaining milestone | Codex, OpenGSD 1.20.0; inline shipping and hosted CI; next autonomous phase stops before ship | GPT-6 (session; exact model ID not exposed) |

| 2026-11-10 | /gsd-discuss-phase 4 via /gsd-autonomous | Codex, OpenGSD 1.20.0; three parallel advisors and combined adversarial pass | GPT-6 (session; exact model ID not exposed); advisors gpt-6-luna at xhigh |

| 2026-11-10 | /gsd-plan-phase 4 research | Codex, OpenGSD 1.20.0; independent researcher handoff recovered after a stalled write; pins rechecked by orchestrator | gpt-6-luna at xhigh; orchestrator GPT-6 (session) |

| 2026-11-10 | /gsd-plan-phase 4 complete | Codex, OpenGSD 1.20.0; 32 plans, focused revision, independent check/recheck passed; literal command repair verified | planner gpt-6.1-sol high; checkers gpt-6-luna xhigh; orchestrator GPT-6 (session) |

| 2026-11-10 | /gsd-execute-phase 4 via /gsd-autonomous | Codex, OpenGSD 1.20.0; sequential typed executors in approved shared checkout | gpt-6-luna xhigh; orchestrator GPT-6 (session) |
| 2026-11-11 | /gsd-execute-plan 04-13 | Codex, OpenGSD 1.20.0; sequential executor with hosted workflow dispatch, fixes, and plan bookkeeping | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-02 | Codex, OpenGSD 1.20.0; sequential executor | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-03 | Codex, OpenGSD 1.20.0; sequential executor in shared checkout | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-04 | Codex, OpenGSD 1.20.0; sequential executor in shared checkout | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-05 | Codex, OpenGSD 1.20.0; sequential executor, review follow-up, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-06 | Codex, OpenGSD 1.20.0; sequential executor, flat polygon implementation, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-07 | Codex, OpenGSD 1.20.0; sequential executor, polygon interpolation tests, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-08 (recovered after daemon restart) | Codex, OpenGSD 1.20.0; sequential executor, line/polyline and rectangle spans, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-09 | Codex, OpenGSD 1.20.0; sequential executor, beam/timer integration, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-10 | Codex, OpenGSD 1.20.0; sequential executor, native GPU scanout and RGB24 runner conversion, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-14 | Codex, OpenGSD 1.20.0; sequential executor, scheduled SIO0 digital polling, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-11 | Codex, OpenGSD 1.20.0; sequential executor, GPU DMA2 integration, DMA validation, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-10 | /gsd-execute-phase 4 plan 04-15 | Codex, OpenGSD 1.20.0; sequential executor, input/rumble tracer CI, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-12 | Codex, OpenGSD 1.20.0; sequential executor, GPU-motion and beam-timing fixtures, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | Phase 04 execution-evidence replan at safe boundary | Codex, OpenGSD 1.20.0; planning-only sizing/dependency/source-coverage check, sequential/no worktrees | gpt-6.1-sol high |

| 2026-11-11 | Phase 04 focused sizing-warning revision | Codex, OpenGSD 1.20.0; focused verification commands, preserved required oracle/hosted evidence | gpt-6.1-sol high |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-16 | Codex, OpenGSD 1.20.0; sequential executor in shared checkout; full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-33 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-34 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; card import/state validation, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-17 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; raw card host helper, focused and full CI, hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-35 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; legacy card wrappers, canonical save names, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-37 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; guest card persistence, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-21 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; firmware scanner and archive controls, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-22 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; pinned OpenBIOS output 524288 bytes/SHA-256 verified, digest corruption refused; CI run 43069874480 passed, including WASI and 433 be32 tests; local CI/hygiene passed | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-39 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; same-run OpenBIOS handoff with digest-checked consumers, corruption/missing refusal, local CI/hygiene, and exact-head hosted CI | gpt-6-luna xhigh |

| 2026-11-11 | /gsd-execute-phase 4 plan 04-23 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; synthetic runner BIOS reset proof, CLI edge tests, local CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | Phase 04 source-owned POST boot-blocker re-slice | Codex, OpenGSD 1.20.0; pinned source/bus-width review, planning-only dependency and coverage update | gpt-6.1-sol high |

| 2026-11-11 | /gsd-execute-plan 04-50 | Codex, OpenGSD 1.20.0; sequential executor, source-specific POST bus fix, RED/GREEN, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | Phase 04 source-owned ATTR first-caller planning | Codex, OpenGSD 1.20.0; minimal bus/reset/state ownership and primary-source review; planning-only ordering/coverage update | gpt-6.1-sol high |

| 2026-11-11 | /gsd-execute-plan 04-51 | Codex, OpenGSD 1.20.0; sequential executor, exact ATTR readback with RED/GREEN, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | Phase 04 DMA4 first-caller planning | Codex, OpenGSD 1.20.0; actual DMA credit/ATTR/state/deadline review, planning-only source slice and ordering | gpt-6.1-sol high |

| 2026-11-11 | /gsd-execute-plan 04-52 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; ATTR-derived DMA4 request path, RED/GREEN controls, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-11 | Phase 04 GPUSTAT first-caller planning | Codex, OpenGSD 1.20.0; current primary status evidence, approved minimal saved phase, planning-only ordering/coverage update | gpt-6.1-sol high |

| 2026-11-11 | /gsd-execute-plan 04-53 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; GPUSTAT saved-phase latch, RED/GREEN, full CI and hygiene | gpt-6-luna xhigh |

| 2026-11-12 | /gsd-execute-plan 04-40 | Codex, OpenGSD 1.20.0; sequential executor in shared phase checkout; inspected shell frame 240, portable selector/refusal controls, full CI/hygiene and exact-head hosted CI | gpt-6-luna xhigh |

| 2026-11-12 | /gsd-execute-plan 04-24 (incomplete checkpoint) | Codex, OpenGSD 1.20.0; sequential executor; measured ledger/schema GREEN subset, scratch ratchet/docs sizing and refusal controls; plan remains incomplete | GPT-6 (session; exact model ID not exposed) |
| 2026-11-12 | /gsd-execute-plan 04-27 | Codex, OpenGSD 1.20.0; sequential executor; PS-EXE libretro loading, no-content BIOS selection, full CI and hygiene | gpt-6-luna xhigh |
| 2026-11-12 | Phase 04 approved gate-budget implementation | Codex, OpenGSD 1.20.0; concrete 2300-line ceiling and approval record, no plan advance | GPT-6 (session; exact model ID not exposed) |
| 2026-11-12 | /gsd-execute-plan 04-24 (resumed after approval) | Codex, OpenGSD 1.20.0; sequential shared-checkout execution; compatibility ratchet/docs, full CI and hygiene; preserved task-order deviation recorded | gpt-6-luna xhigh |
| 2026-11-12 | /gsd-execute-plan 04-41 | Codex, OpenGSD 1.20.0; sequential executor; protected base judge and controls, full CI and hygiene | gpt-6-luna xhigh |
| 2026-11-12 | /gsd-execute-plan 04-43 | Codex, OpenGSD 1.20.1; sequential executor in shared phase checkout; native RGB555/RGB24 adapter conversion, geometry/rate edge tests, full CI and hygiene | gpt-6-luna xhigh |
| 2026-11-12 | /gsd-execute-plan 04-26 incomplete resource checkpoint | Codex/OpenGSD; complete library candidate and ordinary-mode/BIN proof preserved off-tree; no plan advance | executor gpt-6-luna xhigh; coordinator GPT-6 session |
| 2026-11-12 | /gsd-execute-plan 04-26 resumed after approved budget update | Codex, OpenGSD 1.20.0; sequential shared checkout; applied candidate, focused library cases, full CI, hygiene and plan closeout | gpt-6-luna xhigh |
| 2026-11-12 | Phase 04 complete-code budget approval implementation | Codex/OpenGSD; concrete 2700-line ceiling, preserved proposal and renewed efficiency authority; no plan advance | GPT-6 session |
| 2026-11-12 | /gsd-execute-plan 04-28 | Codex, OpenGSD 1.20.1; sequential shared-checkout execution; per-port controller mapping, family-specific guest movies, analog DualShock and both-port rumble proof, full CI/release checks and hygiene | gpt-6-luna xhigh |
| 2026-11-12 | /gsd-execute-plan 04-44 | Codex, OpenGSD 1.20.1; sequential shared-checkout execution; libretro card persistence and memory descriptors, full CI/release checks and hygiene | gpt-6-luna xhigh |
| 2026-11-12 | /gsd-execute-plan 04-45 | Codex, OpenGSD 1.20.1; sequential shared-checkout execution; metadata truth guard, PSEXE host proof, input-descriptor assertions, bounded four-job CI/release checks, and hygiene | gpt-6-luna xhigh |

| 2026-11-12 | Phase 04 source-owned hosted portability prerequisite planning | Codex, OpenGSD 1.20.0; 29 checkpoint preserved, two focused prerequisite plans, no execution advance | gpt-6.1-sol high |

| 2026-11-12 | /gsd-execute-plan 04-54 | Codex/OpenGSD; sequential shared checkout; strict compiler portability, inventory-owned compatibility registration and emulator argument forwarding; 509/509 local CI tests, package/release 5/5; hygiene CTest log 23 passed with outer exit unrecorded; hosted acceptance pending | executor gpt-6-luna xhigh; planner gpt-6.1-sol high; checker gpt-6-luna xhigh |

| 2026-11-12 | /gsd-execute-plan 04-55 | Codex, OpenGSD 1.20.0; sequential shared checkout; bounded movie active-pixel poison canaries, measured byte work, 509 CI tests, package/release 5/5, hygiene 23/23; hosted timing pending combined run | gpt-6-luna xhigh |

| 2026-11-12 | /gsd-execute-plan 04-54 post-closeout follow-up | Codex/OpenGSD; sequential shared checkout; canonicalized drive-rooted CMake paths, retained `.exe` runner names and refusal controls; focused `compat.library` tests 2/2 passed; native Windows CI pending | gpt-6-luna xhigh |

| 2026-11-12 | Phase 04 second hosted-run source prerequisite planning | Codex, OpenGSD 1.20.0; focused 04-56 and downstream DAG/checkpoint bookkeeping only; no execution advance or source/test run | gpt-6.1-sol high |

| 2026-11-14 | /gsd-execute-plan 05-06 | Codex, OpenGSD 1.20.0; sequential shared checkout; BRR tracer TDD GREEN and focused save-state continuation checks; full 24-voice/mixer task moved to 05-22 | GPT-6 session (exact model ID not exposed) |
| 2026-11-14 | /gsd-execute-phase 5 plan 05-12 | Codex, OpenGSD 1.21.0; sequential shared checkout; test-only PCM capture, six region/callback comparisons, reset/reload/cadence/AV checks | GPT-6 session (exact model ID not exposed) |
| 2026-11-14 | /gsd-execute-phase 5 plan 05-14 | Codex, OpenGSD 1.21.0; sequential shared checkout; active PCM save/load continuation, malformed queued SPU address rejection, atomic ring refusal, state/DMA/reverb/API checks | GPT-6 session (exact model ID not exposed) |

## Session Continuity

Last session: 2026-11-20T16:09:19.598Z
Stopped at: Completed 09-10-PLAN.md
Resume file: None

### v1.0 session history

The archived Phase 6 resume point was `milestones/v1.0-phases/06-discs-and-fmv/06-42-PLAN.md`.

Verification: .planning/phases/05-the-sound-matches-the-console/05-VERIFICATION.md

Phase 4 shipped as [PR #14](https://github.com/szTheory/schwaystation/pull/14); release [PR #15](https://github.com/szTheory/schwaystation/pull/15) merged with green required CI. PR #17's post-release CI follow-up is merged and green.
Phase 3 ship PR: https://github.com/szTheory/schwaystation/pull/12
Phase 2 ship PR: https://github.com/szTheory/schwaystation/pull/8
Phase 1 ship PR: https://github.com/szTheory/schwaystation/pull/4
CI tail PR: https://github.com/szTheory/schwaystation/pull/6

Phase 3 merged as PR #12; release PR #13 is green and merged. Phase 4 release v0.4.0 is latest.
Final download/install checks: https://github.com/szTheory/schwaystation/actions/runs/42841474140
The v1.0 audit records the limits of the available frame and movie evidence. The owner capture remains unavailable. See `.planning/AUTONOMOUS.md` for standing preferences.

### Independent local work during immutable CI (2026-11-12)

Codex coordinator / inherited parent model: plan29 explicitly acknowledged monitor-only ownership of run43321926969 at ea3ec542, with no active local command or source/shared metadata work. Plan30's edge to29 was sequencing only; its existing VRAM/runner/PNG interfaces and other prerequisites are complete, and its GPU inventory/runner-main/doc source paths do not overlap the monitor. Remove only that edge and run30 as the single local writer. Plan29 metadata closeout waits for the safe writer handoff; this old-head run does not verify30 or replace final current-source verification. Actual progress remains49/56 summaries until another terminal summary exists.

### Hardware-reference writer handoff (2026-11-12)

Codex coordinator / inherited parent model: plan30 returned terminal complete at3bdad2b; actual progress50/56 summaries, next ordinal51. Plan29 explicitly retains monitor-only ownership of the Windows diagnostic and /tmp findings with no local source/shared metadata writes. Plan46 is the sole local writer using the evidence-backed host-Python input-adapter/C per-word-comparison seam. Old dispatched CI/diagnostic heads do not verify46 or replace final current-source verification.

### Remaining GPU capture writer handoff (2026-11-13)

Codex coordinator / inherited parent model: plan46 returned terminal complete at eb93663; actual progress51/56 summaries, next ordinal52. Plan29 explicitly retains monitor-only ownership of targeted Windows run43345311739 at f502338 and has no active local command/source/shared metadata writes. Plan47 is the sole local writer using46's independently checked adapter and C word-comparison path, with per-guest source-supported fixed bounds. Old dispatched runs do not verify47 or replace final current-source verification.

### Focused registered IRQ prerequisite application (2026-11-13)

The source-owned first caller is the exact build-158 overlap program: IRQ0 interrupts upload before completion because its B0(19) hook is not dispatched. Independently checked plans57/58 split the measured45–60-minute implementation/validation scope into two25–30-minute forecasts. Context remains guest RAM; valid SWMB, unregistered exceptions, SYS1/2 and original reset-installed slots remain intact. Chain46→57→58→47→48→31→32 preserves all other dependencies and leaves47/29 incomplete. Actual physical summary count stays51; only denominator changes56→58. Stable production review is staged at8c3bdcb with a file fingerprint; its reported findings require disposition before final phase verification.

### Daemon recovery after the registered IRQ tracer (2026-11-13)

The previous agents ended at restart; current source commits and the interrupted57 summary were preserved. Native summary checks and source digest confirm the recorded7-case tracer result still belongs to this source. Native counters advanced once to52/58, next ordinal53; no completed source work was repeated. Root coordinates directly from here. Plan58 is the next sole writer, followed by47/48/31/32 and29 acceptance. Source review precedes final verification; stop before ship4.

Both retained stable-review defects have real failing caller proofs and isolated native repair candidates. Card candidate: /private/tmp/phase4-card-fix/READY.md (two files, registered card regression, state/API compatibility checks). Region candidate: /private/tmp/phase4-region-fix (adapter, host regression and embedding prose; native lifecycle RED/GREEN and NTSC/PAL host checks passed). Fixed instance profiles survive reset, so the authorized region repair defers selection until next content load and says Reload content to apply; live input options remain live. No new API, allocation layer or content cache. Neither candidate is applied yet; review/check base fingerprints at the safe source handoff.

Staged stable review was corrected: BIOS identity binding already exists; the initial claim read phase-start bus.c. Retained transient error04 and runtime region-loss claims have independent runtime RED. Fresh08 metadata, card bytes and generation stay receiver-owned. Final changed-source review remains required. The earlier local RetroArch LaunchServices denial remains a safety stop; no bypass or foreground test is authorized by recovery.

### Independent hostile-input integration after58

Codex primary / GPT-6 session integrated the prepared31 test-only candidate after58's terminal handoff; worker gpt-6-luna xhigh. The registered GPU/SIO/card plus unchanged loader cases pass with the full project warning flags. At that checkpoint 53/58 summaries remained and plan31 acceptance awaited plan48 and shared hosted evidence; that status was superseded by the closure below. See04-31-CHECKPOINT.md for the focused commands.

### Phase 4 remaining-plan closure (2026-11-13)

Plans48,31,29 and32 summaries are now recorded; phase progress is58/58 and the native state is ready for phase verification. Exact-head hosted run [43404686338](https://github.com/szTheory/schwaystation/actions/runs/43404686338) succeeded on `2f839fd29819d3d32a7ade8f0348db8fc6a1b1c5`, including the required GPU reference lane, Windows/native matrix, WASM, BE32 (492/492), ASan, RetroArch frames59/60 with an empty system directory, and the aggregate required check. Runtime/model for this closure: Codex/OpenGSD1.20.0, gpt-6-luna xhigh. Exact active duration across the checkpointed plans was not recoverable; summaries record that rather than estimate. The verifier owns the remaining goal-backward report. Stop after phase verification before phase4 ship or phase5.

### Final Phase 4 verification boundary (2026-11-13)

Runtime/model: Codex/OpenGSD 1.20.0; executor and verifier gpt-6-luna xhigh; primary GPT-6 session (exact model ID not exposed). All 58 summaries are present; native verification.status exits 0 with passed, no human items or gaps. Hosted run https://github.com/szTheory/schwaystation/actions/runs/43404686338 is completed/success on tested source 2f839fd; later changes are planning metadata only. Native phase.complete exits 0; the automatic Phase 5 pointer was restored to the required before-ship Phase 4 boundary.

The phase-completion summary scanner emitted advisory missing-path warnings for historical temporary RED reports, upstream-only paths, moved completed todos, literal command strings and the historical compare.c name. These are not missing delivered behavior: the verifier checked every current plan artifact and the real target/caller wiring. Preserve the command evidence; do not add files, repeat broad suites or rewrite historical evidence to satisfy that heuristic.

Efficiency corrections are durable in AUTONOMOUS.md: bring real oracles into the first runnable slices, batch independent prepared consumers before final CI, reuse reviewed source fingerprints, configure the focused GPU lane without the CPU corpus, and stop an obsolete known-failed CI tail after collecting useful results. All required coverage remains.

### Phase 5 verification boundary (2026-11-14)

Plans 05-15 and 05-24 are complete; all 24 plan summaries are present. Goal-backward verification passed 3/3 roadmap truths with no gaps or owner checks. Local CI passed 547/547 tests plus 5/5 release package checks; the explicit audio suite passed 10/10, libretro PCM and idle-SPU checks passed 2/2, and hygiene passed 24/24. The repository-check total is exactly 2688 lines. Code review found no issues. Runtime/model: Codex/OpenGSD 1.21.0, GPT-6 session (exact model ID not exposed), gpt-6-luna xhigh. The release PR, exact-head hosted CI/nightly/RetroArch run and green release PR remain for `$gsd-ship 5`; stop here, before Phase 6.

### Phase 6 plan 06-01 execution (2026-11-15)

Runtime/model: Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh. Plan 06-01 completed in 12 minutes with two task commits. Focused tracer checks passed 19/19 after preserving the embedding target contract; policy checks passed 4/4, and archive/link inspection confirmed host filesystem support stays outside the core archive. Phase 6 is at plan 2 of 26. DISC-01 through DISC-03 remain pending until their behavior plans complete; next is 06-02.

### Phase 6 plan 06-02 execution (2026-11-15)

Runtime/model: Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh. Plan 06-02 completed in 26 minutes with two task commits. API v5 host disc attachment, copied TOC validation, and closed-lid swap coverage landed; the focused disc API test and API v1-v4 compatibility checks passed. Phase 6 is at plan 3 of 26.

### Phase 6 plan 06-03 execution (2026-11-15)

Runtime/model: Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh. Plan 06-03 completed in 18 minutes with two task commits. Bounded instance-owned CD/MDEC state, POWER reset and field visitors landed; bus dispatch and scheduler deadlines were connected. The required configure/build plus no-busy-loop/scheduler command passed 8/8 after each task. No hosted URL applies to these local checks. The plan's task-level TDD tags had no new test files or direct behavior tests in scope; first CD/MDEC behavior callers and state serialization remain assigned to Plans 06-04/06-11/06-26. Phase 6 is at plan 4 of 26.

### Phase 6 plan 06-08 execution (2026-11-15)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Plan 06-08 completed in 35 minutes with three task commits. Bounded M3U selection and shared SYSTEM.CNF parsing landed with playlist/CNF tests and host-facing format documentation. The local disc suite passed 10/10, the ISO probe edge case passed, and inventory/hygiene checks passed; no hosted URL applies. The stale `runner.probe` selector matched no test, so the actual `compat.probe.iso_edges` test was run separately. Plan 06-26 is next in wave 4; 06-04 remains the sequential next plan.

### Phase 6 plan 06-26 execution (2026-11-15)

Runtime/model: Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh. Plan 06-26 completed in 20 minutes with four task commits. CD-ROM and MDEC visitors now participate in state format 6; the state suite passed 8/8 and the bus/unbuilt/gap fixture lane passed 3/3. The initial TDD seed used an invalid MDEC deadline and was corrected to the supported 900-cycle bound. No hosted URL applies. This out-of-order wave plan is complete; 06-04 remains the sequential next plan. Global DISC-01 and DISC-03 acceptance remain pending the later CD-ROM and MDEC behavior plans.

### Phase 6 plan 06-05 execution (2026-11-15)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall time from the earliest durable execution marker to summary creation was 58m56s; exact active duration is unavailable because executor start and idle intervals were not captured. Sector acquisition, filtering, Q handling, RDDATA delivery and state coverage are committed; code review is clean. The final Plan 05 configure/build/selected CTest command passed (8 selected tests), and hygiene, inventory sort and `git diff --check` passed. The TDD RED command failed the new sector caller as expected (exit 8), and its corrected report classified `RED_EVIDENCE_OK`. No hosted run applies. DISC-01 remains pending aggregate Phase 6 acceptance; XA decoding/audio, DMA3 cursor/rates and disc-boot caller acceptance remain with Plans 06-09/06-10, 06-27 and their downstream callers. Plan 06-06 is next; Plan 06-27 remains sequential with DMA consumers.

### Phase 6 plan 06-29 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall time is at least 1h31m23s from the earliest durable execution marker at 00:43:26Z through final gate closeout at 02:14:49Z; total runtime is longer, and active time was not measured. The post-review native command passed 2/2 tests and the pinned reference command passed 2/2, including the unchanged Getloc caller; focused code review is clean. ReadN BUSYSTS now clears on command acceptance, and full DMA3/status/GetlocL controls pass. DISC-01 remains partial: Plan 06-30 owns raw Q/GetlocP rows, then Plan 06-06 runs the full timing/Getloc gate. No hosted URL applies.

### Phase 6 plan 06-30 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall time is at least 37m36s from the 02:17:11Z start marker through the 02:54:47Z closeout preparation; final metadata commit follows. Source review added last-good Q preservation and validation of serialized candidate bytes. Native CD-ROM/state checks passed 10/10 and the pinned reference guest/controls passed 2/2. The three pinned rows, malformed-Q handling, TOC track range and state continuation are covered. DISC-01 remains partial pending Plan 06-06's full timing/Getloc gate; Plan 06-06 is next.

### Phase 6 plan 06-06 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. The execution chain lasted at least 2h35m20s from Plan 06-29's 00:43:26Z durable marker through Plan 06 closeout preparation; the original Plan 06 start and active time were not captured. The final CD-only configure/build/reference gate passed 3/3. Focused review cleared the comparator and pin/fetch logic before the final gate. Timing comparisons retain all eligible consecutive-LBA samples, reject overflow, and disclose that adjacent-LBA restarts are included. DISC-01 remains pending aggregate Phase 6 acceptance; Wave 8 Plan 06-09 is next. No hosted run URL applies.

### Phase 6 plan 06-09 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall time from the 03:29:33Z start through closeout preparation at 04:01:32Z is 31m59s; the exact Plan 09 configure/build/decoder gate completed at 03:57:33Z (28 minutes after start). The post-review exact gate passed `disc.xa` 1/1; the focused decoder and state controls passed 6/6. XA predictor and resampler histories are instance-owned and serialized; state format/behavior revision advanced to 11. No hosted URL applies. DISC-01 remains pending aggregate acceptance; DISC-02 remains pending the later XA/SPU caller integration. Plan 06-11 is next.

### Phase 6 plan 06-33 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Wall lower bound is at least 1h22m from the first durable RED report at 06:29:28Z through summary preparation at 07:51:28Z; the plan started earlier and exact active time is unavailable. The accepted XA sector route, separate 9,409-pair serialized ring, native cadence and equal-deadline reserve passed the focused review and 12/12 configure/build/state/audio/SPU gate. State format and behavior revision are 14. DISC-02 remains partial: XA/CDDA/voice/reverb mixing and the mode-enabled file/channel mismatch control continue in 06-34. The MDEC-only 06-12 plan remains independent; 06-34 is next.

### Phase 6 plan 06-12 oracle/control slice (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. The original execution marker is 08:10:43Z; split metadata was recorded at 09:28:28Z, giving a wall lower bound of at least 1h18m; exact active time is unavailable. The pinned 4/8-bit guest and status logs are resolved at public commit `f727802fead11f1daa7549285548392ef87749cb`; reference executables remain in build storage. The harness has independent byte/status comparators and mutation controls. The 4-bit guest reached the caller but the core consumed DMA0 command `0x20000020` from MADR `0x12000`, BCR `0x00010020`, CHCR `0x01000201`, then became idle with no output while the guest polled `0x80040000`; DMA1 remained unarmed. Keep DISC-03 partial. Plan 06-35 owns the bounded production repair and full passing reference gate; the exact Plan 06-12 gate resumes afterward.

### Phase 6 plan 06-13 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID not exposed). Exact active time was not tracked; bounded local checks and follow-up closed at 18:57:07Z. Both MDEC target builds passed. Seven targeted color/display/FIFO/bounds/vector/4-bit checks passed; full `mdec.*` was 10/11 because `mdec.guest_input` still reports 12 exact 8-bit word differences. Reference asset and mutation controls passed 2/2; all four frame/movie callers failed fast on exhausted input, and all six reference cases took 0.64s. The frame stream has 296 macroblocks for 300 requested; movie frame one has 162 for 240 requested. Public hardware records cover compact color points and 19 of 20 frame strips but do not provide a complete frame/movie pair. Code review retained two warnings: incomplete image references and no raw guest-output hash. The latter is deferred in seed SEED-261116-mrh for Plan 06-16's first complete owned STR caller. DISC-03 remains pending; no hosted URL applies. No long suite or emulator process is running.

### Phase 6 plan 06-14 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable), review gpt-6-luna xhigh. Execution resumed after a 22-hour pause; active time was not attributable. Pinned source-cache invalidation, Ninja Windows output path, MSVC floating-point/link options, and fixture media downloads/emulator invocation in all relevant CI lanes were corrected. The local disc target and pinned producer script built successfully; focused pin, media-corruption, probe and budget checks passed 5/5; hygiene passed 24/24 in 38 seconds. The gate total is 2698/2700 lines. DISC-02 runtime acceptance remains open until downstream boot/playback and hosted matrix checks. No owner hardware run or hosted URL applies. Plan 06-21 is next in Wave 15.

### Phase 6 plan 06-21 execution (2026-11-16)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable). Active duration was not tracked after the resumed-session pause. API v6 binds save states to a host-supplied disc identity; different or unavailable media is rejected before apply, legacy descriptor prefixes remain valid for emulation, and callback-free save/load behavior is covered. The no-busy-loop and focused state/API/CD suite passed 16/16; the synthetic perf lane passed 19/19. Workflow wiring checks now require both SPU and CD Cachegrind jobs; direct workflow check passed. Repository-check code is 2699/2700 lines. Public SCPH-5501 seek and MDEC capture data are recorded in the hardware pages; they do not settle XA interpolation rounding. Local Valgrind was unavailable, so real idle-CD instruction measurement awaits hosted perf CI. DISC-01/03/04 remain partial pending their downstream callers; no owner hardware run is scheduled. Next is Plan 06-15, whose fixture must supply the host-owned identity. No hosted URL applies yet.

### Phase 6 plan 06-16 execution (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); inline code review. The owner reported over 22 hours elapsed during the autonomous run; active runtime was not measured reliably. The cheap media-geometry preflight passed before final tests. The primary two-track fixture build enforces the 512 KiB cap at 522,144 bytes; reducing XA to three complete sectors shifts the authored CD-DA and STR extents, and the guest seeks their resulting LBA 214 and 54 positions. The post-review `ctest --test-dir build/ci -R '^disc\\.fixture' --no-tests=error --output-on-failure` passed 4/4 in 22.47 seconds. The pre-commit hygiene scan caught symbol-based pin labels; hyphenated labels passed the affected audio/FMV tests 2/2. The one-off record command passed and produced PCM/frame pins; no physical PlayStation capture or hosted URL is claimed. DISC-02/03 remain partial pending runner, libretro, oracle, fuzz and hosted matrix callers. Plan 06-17 is next.

### Phase 6 plan 06-17 execution (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); standard gsd-code-reviewer. Active time was not tracked reliably across the owner's reported 22-hour pause. Cheap diff/inventory checks and CMake configure passed before builds. The runner now boots the owned CUE and M3U images through OpenBIOS, attaches ISO, rejects direct and nested unsupported media, and applies frame-indexed pad/lid/swap controls. Standard review caught and fixed a disc `--hash-state` usage mismatch, a CUE probe extension bypass, and a frame-300 control test that had stopped at frame 274; the final review is clean. `cmake --preset ci`, the runner target build, and the focused runner CTest selector passed; final selector result was 16/16 in 2.46 seconds. No physical PlayStation capture or hosted run URL applies. Plan 06-18 is next.

### Phase 6 plan 06-18 execution (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); standard gsd-code-reviewer. Active time was not measured reliably; the wall interval from the prior state checkpoint through final verification was about 1h09m and includes review and test waits. Libretro now loads CUE/ISO/M3U with the system BIOS, exposes extended disk control, removes media through an empty TOC while the tray is open, and proves the no-media GetTN error with the guest. Runner and libretro PCM match byte-for-byte for the combined XA/CD-DA fixture in batch and single callbacks for NTSC-U and PAL. The review's no-media blocker was fixed and re-reviewed clean. Cheap checks caught startup cadence assumptions, stale extension assertions, a 2-line repository-gate overage, and a brittle negative-control matcher before closeout; all were corrected without broad reruns. The final libretro host selector passed 4/4 in 39.95 seconds; core disc API and info-claim controls passed 2/2 each. Repository-check code is exactly 2700/2700 lines. No physical capture or hosted URL applies. Plan 06-19 is next.

### Phase 6 plan 06-19 execution (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); standard gsd-code-reviewer gpt-6-luna xhigh. Active time was not reliably measured after the owner-reported 22-hour pause. The pinned MIT rcheevos PlayStation hash source is tested against production runner identity on provider-owned disc bytes; executable mutation changes both results. The vendored digest negative control rejects changed bytes. Review findings about license wording and a dead shim were fixed; the post-review disc/probe/vendor filter passed 4/4. Cheap selectors, sorted inventory, digest and 2699/2700-line budget checks preceded the focused build. No physical capture or hosted URL applies. Plan 06-22 is next.

### Phase 6 plan 06-22 execution (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable). The optional SDL2 example compiles from the installed library package and reuses existing host disc/pixel helpers without adding SDL2 to product targets. The first package configure found the build-tree config shadowing the installed one; the corrected command supplies the installed `schwaystation_DIR`, and the final example build passed. CI builds it only on Linux x64, using Ubuntu's SDL2 compatibility development package if the hosted image lacks `sdl2.pc`. Actionlint passed when ignoring its outdated Ubuntu 26.04 label list. The app was not launched and hosted verification remains for ship. The 2699-line repository gate is unchanged; no owner hardware capture or hosted URL applies. Plan 06-20 is next.

### Phase 6 plan 06-23 independent local integration (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); standard gsd-code-reviewer gpt-6-luna xhigh. Active duration was not measured. The earlier Getloc pass came from a stale binary; fresh builds reproduced the failure. The actual test fixture used 150-based TOC values against the documented zero-based-LBA contract. Production track selection and synthetic Q generation now follow the contract, including INDEX 00 pregap countdown; the review's WR-08 was fixed and re-reviewed. Focused CI CD/controller/XA/runner/fuzz checks passed 19/19 in 12.28s, nightly timing/Getloc/control references passed 3/3 in 2.29s, and runner hash/libretro integration plus fixture producers passed 4/4 (libretro host 39.12s). No full media suite was repeated. DISC-03 remains open: the pinned 8-bit selector differs at byte 6 and the status guest exceeds its preserved cycle bound. Public searches found no replacement full-output oracle; the owner says capture is not available anytime soon, so none is planned or requested. Hosted all-leg and background RetroArch acceptance remain outstanding; no hosted URL applies. No phase verification or ship claim.

### Phase 6 plans 06-23/06-24 final verification (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); final source review by gsd-code-reviewer gpt-6-luna xhigh. Active duration was not measured. The current diff review found no open source findings after correcting API v6 compatibility and updating all runner/libretro/example disc-flag call sites. The reviewed-source `cmake --workflow --preset ci` configured and built successfully, then exited 8 with 609/610 tests passing; only `mdec.guest_input` failed. Direct `ctest -L disc` passed 25/25 in 11.48s; `ctest -L mdec` was 10/11 and confirms 12 pinned output-word mismatches; `cmake --workflow --preset hygiene` passed 24/24. The exact 8-bit mismatch remains byte 6 (expected 04, actual 05), while the status guest's 7.68M requested delay cycles exceed its unchanged 4.9152M cap. No expected data, MDEC contract, selector or cap changed. Hosted CI/nightly/fuzz/RetroArch checks remain pending without a URL. Phase 6 verification is `gaps_found`; no ship, PR or milestone completion is claimed. The owner capture is unavailable and will not be requested. Next action is new primary MDEC evidence or an evidence-backed production correction; no next phase or v2.0 is authorized by this run.

### Phase 6 MDEC completed-command status follow-up (2026-11-17)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); independent MDEC math, hardware-source and cycle-budget reviews. Active duration was not measured. The public SCPH-5501 status capture shows `8004FFFFh` after full table loads and `2204FFFFh` after all decode input is consumed while output remains unread. `mdec_status` previously left the low counter at zero; it now reports the documented minus-one wrap for nonempty MDEC(1/2/3) commands. The focused status test checks MDEC(2) and MDEC(3) through exact `8004FFFFh`, and decode input exhaustion to `FFFFh` while a block is pending. `cmake --build --preset ci --target schwaystation-test-mdec` and `ctest --test-dir build/ci -R '^mdec\\.status$' --no-tests=error --output-on-failure` passed. The full MDEC label passed 10/11; only the exact 8-bit vector remains red. The post-fix `cmake --workflow --preset ci` exited 8 with 609/610 tests passing in 66.57 seconds, only `mdec.guest_input` failed. Rebuilt nightly MDEC reference tests passed 4-bit and controls; 8-bit still differs at byte 6 (expected 04, actual 05). Two independent reviews confirmed identical pinned 4/8-bit input/tables and that the permitted ares arithmetic matches the current implementation; no source-backed alternative removed the differences. The full status guest still requests 7.68M delay cycles against the unchanged 4.9152M cap, and this status-register fix does not change that. The final `cmake --workflow --preset hygiene` passed 24/24 in 39.77 seconds. DISC-03 remains incomplete; no phase ship, hosted URL, owner capture, next phase or v2.0 work is claimed.

### Phase 6 plan 06-45 generated-playlist hash repair (2026-11-18)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); independent gsd-code-reviewer. Exact-head Nightly run 44210060292 passed; CI run 44210060285 passed all lanes including be32 but failed only the final hash-equality job. Artifact comparison isolated the difference to `disc_boot_runner.m3u`: Windows `file(WRITE)` emitted CRLF and Linux emitted LF. Changed its CMake producer to `file(CONFIGURE ... NEWLINE_STYLE UNIX)`. The reviewer found no issue. `cmake --preset ci` passed; the regenerated playlist hash matches the be32 artifact (`722ce2c1c07b9c90102dd872faea02e306617c5604cdf331ad7a7dc2df3509b8`), and `runner.disc.api` passed 1/1. Current source changes still need exact-head hosted CI and Nightly; no verification or ship claim is made.

## v1.0 stale-verification refreshes

### Phase 01 — verification refresh (2026-11-18)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID unavailable; gsd-verifier gpt-6-luna xhigh; gsd-code-reviewer gpt-6-luna xhigh. Refreshed `01-VERIFICATION.md` through the execute-phase stale-report route; all five roadmap criteria and FRAME-01–09 passed. No local tests were rerun. Reused `cmake --workflow --preset ci` (exit 0) and `cmake --workflow --preset be32` (exit 0) from [release-PR CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074), whose tree matches current `main`; the [main CI run 44253375502](https://github.com/szTheory/schwaystation/actions/runs/44253375502) also passed. Release artifact validation and install-line checks passed in [release run 44262491280](https://github.com/szTheory/schwaystation/actions/runs/44262491280). `phase.complete` warnings were shell commands embedded in historical summaries misread as filenames; no source files were missing. The Phase 1 review also found a latent filename-separator bypass in the test-only firmware scanner; the current tracked tree contains no semicolon/newline paths, the release scan passed, and the deferred tooling work is captured in seed `SEED-261118-8b0` per project policy.

### Phase 02 — verification refresh (2026-11-18)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID unavailable; gsd-verifier gpt-6-luna xhigh; gsd-code-reviewer gpt-6-luna xhigh. Refreshed `02-VERIFICATION.md`; all five roadmap criteria and CPU-01–05 passed. No local tests were rerun. Reused the all-platform [release-PR CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074), whose source tree matches current `main`, plus successful `vectors-full` and `fuzz` jobs from [nightly run 44233700921](https://github.com/szTheory/schwaystation/actions/runs/44233700921); the CPU/vector/fuzz source files exercised are unchanged. Code review found no new source/test delta or findings. The exception-loop false-positive remains a non-blocking historical advisory. `phase.complete` warnings were command examples embedded in historical summaries misread as missing files; no source files were missing.

### Phase 03 — verification refresh (2026-11-18)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID unavailable; gsd-verifier gpt-6-luna xhigh; gsd-code-reviewer gpt-6-luna xhigh. Refreshed `03-VERIFICATION.md`; all three roadmap criteria and GTE-01–03 passed. No local tests were rerun. Reused the all-platform [release-PR CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) on the exact current source tree; the GTE/DMA/timer code and tests are unchanged. The existing Phase 03 review/disposition remain valid; fixed findings have regression tests and remaining findings are tracked as todos. The 40-clock CD-ROM cost deferral remains documented. `phase.complete` warnings were historical shell commands and path expressions misread as missing files; no source files were missing.

### Phase 04 — verification refresh (2026-11-18)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID unavailable; gsd-verifier gpt-6-luna xhigh; gsd-code-reviewer gpt-6-luna xhigh. Refreshed `04-VERIFICATION.md`; all seven roadmap criteria and GAME-01–07 passed (199/199 must-haves). No local tests were rerun. Reused the all-platform [release-PR CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) on the exact current source tree: six platform legs, OpenBIOS, hash equality, RetroArch pad-control pixel comparison, and all 13 GPU reference cases passed. The Phase 6 nightly [run 44233700921](https://github.com/szTheory/schwaystation/actions/runs/44233700921) did not run GPU reference cases; that placement is recorded accurately in the report. Existing review findings remain disposed; this branch adds no source/test changes. `phase.complete` warnings were historical commands, scratch paths and removed intermediate files misread as missing source; no shipped source was missing.

### Phase 05 — verification refresh (2026-11-18)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID unavailable; gsd-verifier gpt-6-luna xhigh; gsd-code-reviewer gpt-6-luna xhigh. Refreshed `05-VERIFICATION.md`; all three roadmap criteria, SOUND-01/02 and 74/74 must-haves passed. No local tests were rerun. Reused the all-platform [release-PR CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) on the exact current source tree, including audio hashes, platform lanes and libretro/RetroArch checks. [Nightly run 44233700921](https://github.com/szTheory/schwaystation/actions/runs/44233700921) passed 30/30 tests and its log explicitly records `spu.reference.memory_transfer` passing; the relevant SPU, comparator, fixture, pin and inventory files are unchanged since that run. The existing Phase 05 review remains valid; the only stale input was later CI workflow wiring, which retains the SPU gates. `phase.complete` flagged one test command embedded in a historical summary as a missing file; no source files were missing.

## Operator Next Steps

- Continue Phase 9 in dependency order: 09-13 package checks, 09-11 workflow/fuzz wiring, 09-12 runner/libretro integration and 09-14 owned-guest evidence; then review all changed source before final verification and ship. Keep todo 005 open for the first tracked SPDX-bearing source. Preserve required CI and release gates; use the planned `act --dryrun` routing check and measure the CHD producer critical path before any expensive hosted run. No owner UAT is needed.

### v2.0 roadmap creation (2026-11-18)

Runtime/model: Codex, OpenGSD 1.20.0, gsd-roadmapper gpt-6.2-sol high. The approved milestone has 10 phases and 34 requirements. Phase 9 establishes the shared media controls and feature-ledger checks with the first provider. Later feature phases record their own evidence. Phase 16 reconciles the pinned DAT inventory and gates libretro metadata on the required-capability matrix. The v1.0 MDEC evidence boundary and the standing 300-line real-host limit carry forward. No implementation or phase verification is claimed here.

### Phase 7 verification refresh (2026-11-20)

Runtime/model: Codex/OpenGSD; primary GPT-6 model ID unavailable; gsd-verifier gpt-6-luna xhigh. The Phase 7 review scope contained only release-version metadata, so no new source review was needed. Refreshed `07-VERIFICATION.md`; 5/5 implementation truths and 27/27 plan artifacts passed, and `verification.status` returned `passed`. The baseline note now identifies the initial 595,820-byte WASI and 3,755,020-byte state measurements as superseded by Phase 8 producer evidence: 608,540 and 3,755,328 bytes. `git diff --check` passed. The 24 Phase 8 regression selectors were present; full suite execution was not repeated because exact-head Main CI [44645037841](https://github.com/szTheory/schwaystation/actions/runs/44645037841) and Release [44645037923](https://github.com/szTheory/schwaystation/actions/runs/44645037923) passed on `830bf4c`. BE32 took 4,694 seconds on Main CI and 3,402 seconds in the concurrent Release matrix; todo 050 remains bound to Phase 9 for measured reuse analysis. Phase 7 is complete and its existing v0.7.0 release remains valid.


### v1.0 milestone closeout (2026-11-18)

All six phase verification reports passed; milestone audit found 30/30 requirements, 30/30 integration paths and 7/7 flows, with accepted MDEC conformance limits and non-blocking follow-ups recorded in `.planning/milestones/v1.0-MILESTONE-AUDIT.md`. The closeout scanner found 29 pending todos, 11 dormant seeds and two existing deferred notes. All 42 were acknowledged as deferred, not marked resolved; the details are listed in the Deferred Items table above. The phase trees and one completed quick task are archived under `.planning/milestones/v1.0-phases/` and `.planning/milestones/v1.0-quick/`.

Hosted evidence reused: [release-PR CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074), [nightly full-vector job 134564625732](https://github.com/szTheory/schwaystation/actions/runs/44233700921/jobs/134564625732), and [v0.6.0 release run 44262491280](https://github.com/szTheory/schwaystation/actions/runs/44262491280). Planning-only edits received `git diff --check`; no implementation tests were added or rerun.
