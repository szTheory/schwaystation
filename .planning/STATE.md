---
gsd_state_version: "1.0"
milestone: v2.0
milestone_name: The whole library
current_phase: 16
current_plan: Not started
status: completed
stopped_at: Phase 16 complete — all phases complete
last_updated: "2026-11-26T18:48:57.404Z"
last_activity: 2026-11-26
last_activity_desc: Phase 16 complete
state_head: a81d14c0d5457c85f2c33efbe5de21e2a9ab0fe3
progress:
  total_phases: 10
  completed_phases: 10
  total_plans: 125
  completed_plans: 125
  percent: 100
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-11-19)

**Core value:** PlayStation games behave as they do on the console, identically on every platform, from a small MIT-licensed C library that any host can embed.
**Current focus:** Phase 16 — Patches, cheats and compatibility evidence

## Current Position

Phase: 16
Current Plan: Not started
Total Plans in Phase: 18
Status: All phases complete
Last activity: 2026-11-26 — Phase 16 complete
Progress: [██████████] 100% (9/10 v2.0 phases released; Phase 16 has all 18 plans complete and awaits ship. Phase 7–14 verification fingerprints need refresh at the milestone audit after shared-file changes. Retain TODO051 as unverified PBP-only routing for that audit.)

### Phase 16 planning (2026-11-25)

Runtime/model: Codex with OpenGSD 1.23.0; gsd-planner gpt-6.1-sol high. Eighteen plans cover the four Phase 16 requirements and eight locked decisions across eleven file-disjoint waves. Plan 01 registers the CMake skeleton and exact test inventories; later owners implement checked RAM writes, bounded GameShark text/state, PPF/BPS source-bound overlays, per-occurrence DAT accounting and the protected fresh-result capability matrix. Plan schemas/structures are validated; code review and final phase verification remain later steps. No application/build/behavior tests ran during planning. The source audit records coverage, concrete bounds and the experimental metadata evidence boundary.

### Phase 16 plan-check revision (2026-11-25)

Runtime/model: Codex with OpenGSD 1.23.0; gsd-planner gpt-6.1-sol high. The independent check found that Plan 01's 15-file scope reached its blocker threshold and the research retained three open questions. Plan 01 now uses the existing api_v10 baseline and modifies 12 files while preserving the CMake skeleton and all six inventory checks; the plan and audit state the AGENTS.md rule 14 first-plan scope exception. A follow-up scope warning moves shared host-cheat wiring/test ownership from Plan 02 into the existing parser Plan 03. Plan 02 now has eight files in two API-only tasks; Plan 03 has five files in three tasks. Plan 02 creates/registers api_v11, adds the public write and updates only older callers' exact-current-version assertions. Every task remains at or below five files, all wave/dependency order is preserved, and the research records resolved source identity, raw-layout and BPS-staging choices consistent with the plans and current context. No application, build or behavior tests ran in this revision; independent recheck precedes execution.

### Phase 16 wave 2 execution (2026-11-26)

Runtime/model: Codex orchestrator; typed `gsd-executor` agents ran on gpt-6-luna xhigh. Plan 16-02 took 16m41s; Plan 16-07 took 34m. Their isolated worktrees started from `1e5a01c`, completed with clean status, passed summary and commit-scope checks, and merged as two disjoint commits. Plan 16-02 added checked API v11 physical-RAM writes while preserving released v9/v10 callers; its four ABI/API selectors passed. Plan 16-07 added identity-bound PPF1 loading and a sparse overlay for one exact raw CUE track mapping; its three patch selectors, runner/libretro consumer builds, and `disc.image.iso_sector` passed. Its Task 2 regression was already green after Task 1; source inspection confirmed the implementation already met the behavior, so the test and documentation were retained and the ordering deviation recorded without redundant production changes.

After merge, `git diff --check 1e5a01c..HEAD`, both affected sorted test inventories, `cmake --preset ci`, and a unique-selector preflight passed. The combined API, PPF1, and disc-image targets plus `schwaystation-run` and `schwaystation_libretro` built successfully; the eight focused selectors passed 8/8. The broad workflow remains for final phase verification after source review. No hosted CI, Act, or Docker work was needed at this wave boundary. Current plan is 3; wave 3 plans 16-03 and 16-08 are ready.

### Phase 16 wave 3 execution (2026-11-26)

Runtime/model: Codex orchestrator; typed `gsd-executor` agents ran on gpt-6-luna xhigh. Plan 16-03 took 31 minutes; Plan 16-08 took 20m23s. Plan 16-03 added the bounded shared `30`/`80` cheat parser and checked RAM consumer with a fixed 16,656-byte canonical state format. Plan 16-08 added PPF2 source-size/block checks and PPF3 BIN, block, 64-bit-record, undo, and bounded FileID validation. The PPF malformed-input tests were already green from Task 1; the executor retained the regression coverage and documented the overlap without redundant production changes. Plan 16-03's measured 31-minute closeout exceeded the 30-minute planning threshold by one minute; its implementation and focused checks had completed before summary and hook closeout extended elapsed time.

Both isolated worktrees started at `542580e`, ended clean, and had resolved commit-scope checks. `verify-summary` produced heuristic false positives: it treated Plan 16-03's documented test failure as a failed self-check and Plan 16-08's inventory-check command as a created file. Both summaries explicitly report `Self-Check: PASSED`; direct inspection confirmed the declared files exist, their task commits and checks match the records, and no changes were needed to the artifacts.

After merge, `git diff --check 542580e..HEAD`, the ABI/runner/disc inventory sorts, `cmake --preset ci`, and the 11-selector preflight passed. The cheat, API, PPF and documentation-budget targets plus runner and libretro consumers built successfully; all 11 focused selectors passed. The broad workflow remains for final phase verification after code review. No hosted CI, Act, or Docker work was needed at this wave boundary. Current plan is 4; wave 4 plans 16-04, 16-06 and 16-09 are ready.

### Phase 16 wave 4 execution (2026-11-26)

Runtime/model: Codex orchestrator; typed `gsd-executor` agents ran on gpt-6-luna xhigh. Plan 16-04 took 31 minutes, Plan 16-06 took 40 minutes, and Plan 16-09 took 63m51s. Plan 16-04 connected libretro cheat callbacks to frame execution and preserved state across `retro_reset`; its two focused selectors and docs budget passed. Plan 16-06 added repeatable runner `--cheat` options and normalized-configuration hash contribution while preserving no-cheat hash pins; the exact six-selector run passed with a pinned OpenBIOS fixture built through the repository's named container producer. Its hash implementation landed with Task 1; the later Task 2 tests passed against it, and the actual TDD ordering was documented without rewriting the passing commit. All generated BIOS/toolchain/source files and task-owned container resources were removed. Plan 16-09 added bounded BPS transformation and complete-target staging (memory through 1 MiB, private temporary file above that); its regression set includes malformed cursors/varints/CRCs, late failures, large targets and injected source-I/O failure. It took 63m51s, including large-target coverage and compaction needed to keep `tests/disc/patch.c` below its existing 1,200-line cap; the final file has 1,199 lines. Its Task 2 tests were already satisfied by Task 1, so the executor retained the regressions and documented the overlap without redundant production changes. These measured durations exceed the plan's 30-minute target; use them when estimating future work and split materially larger plans before execution.

All three worktrees started at `445d699`, ended clean, and had resolved commit-scope checks. `git diff --check 445d699..HEAD`, runner/disc/libretro/ABI inventory sorts, `cmake --preset ci`, unique-selector preflight, and the combined builds passed. The 17 targeted post-merge API, cheat, libretro, PPF/BPS, disc-image and docs-budget selectors passed 17/17. The hygiene preset's `policy.gate_budget` passed 1/1; the measured repository checks remain 3,901/3,901. The exact `runner.hashes_file` case passed earlier in Plan 16-06 with its pinned OpenBIOS fixture; it is not in the post-merge 17-selector set because that generated fixture was cleaned after the plan. The full workflow remains for final phase verification after source review. No hosted CI or Act was run; the plan-owned OpenBIOS container and outputs were cleaned. Current plan is 5; wave 5 plans 16-05, 16-10 and 16-11 are ready.

### Phase 16 wave 5 execution (2026-11-26)

Runtime/model: Codex/OpenGSD 1.23.0 coordinator (primary model ID unavailable); typed `gsd-executor` agents ran on gpt-6-luna xhigh. Plan 16-05 took 33m19s, Plan 16-10 took 44m, and Plan 16-11 took 31m21s. Plan 16-05 added atomic libretro cheat configuration state; its five required selectors passed. Its optional content-profile probe could not run because the generated OpenBIOS fixture was absent after cleanup. Plan 16-10 added runner `--patch` transactional handling and passed eight required selectors plus `runner.hash_state` and repository/documentation budget checks. Its Task 2 tests were already green after Task 1; the test-harness argument mismatch was corrected, actual ordering recorded, and no artificial RED was introduced. Plan 16-11 added patch/cheat replay seeds and nightly fuzz routing; replay passed 2/2, exact seed and workflow checks passed, and `act --dryrun --container-architecture linux/arm64 -W .github/workflows/nightly.yml -j fuzz` exited 0. It added 14 PPF and 10 cheat seeds; local sanitizer execution was unavailable because clang-21 was not installed. Temporary seed/cache data was removed and no task-owned Docker resources remained.

The isolated worktrees completed clean with resolved scope checks and merged. `verify-summary` flagged heuristic false positives for Plans 16-05 and 16-11; both summaries explicitly report `Self-Check: PASSED`, and their created files, commits and checks were verified. After merge, `git diff --check 3d19212..HEAD`, sorted test inventories, `cmake --preset ci`, the combined affected target builds and all 19 focused API/cheat/libretro/patch/fuzz/runner/disc/documentation selectors passed. The hygiene selectors `hygiene.timeout_comments`, `policy.action_pins` and `policy.gate_budget` passed 3/3. No hosted CI ran in this wave. Current plan is 6; wave 6 Plan 16-12 is ready.

### Phase 16 wave 6 execution (2026-11-26)

Runtime/model: Codex/OpenGSD 1.23.0 coordinator (primary model ID unavailable); typed `gsd-executor` gpt-6-luna xhigh. Plan 16-12 took 46m36s, exceeding its 20–28 minute target. It added a bounded host parser and non-emulating runner mode that preserves every DAT occurrence and publishes aggregate counts only; optional per-occurrence identity/evidence stays private. Parser tests first exposed ambiguous duplicate serial fields and synthetic pin mislabeling. A real parse of the pinned D-06 Redump source then exposed valid UTF-8 serial bytes; the encoded-serial regression and digest-based duplicate accounting fixed that without storing or printing serials. The verified source digest was `52978215854d307bfb5353052c9ce3c34995310a970d586f0100bb67f7de01a8`; the runner counted 13,407 entries, all unknown, with 177 missing serials and 988 entries in duplicate-serial groups. The raw DAT was held in a task-specific temporary path and removed after the run. An output-check wrapper's first assertion returned 1 despite correct runner output; a corrected field check returned 0. No owner BIOS/disc paths, Docker, hosted CI, or broad workflow were used.

After merge, `git diff --check be22dcb..HEAD`, compatibility inventory sort, exact selector preflight, `cmake --preset ci`, and builds for `schwaystation-run`, `schwaystation-run-test` and `schwaystation-test-library` passed. The four plan selectors plus `compat.features.preflight` passed 5/5. The first cleanup-wave attempt rejected the abbreviated manifest base; ancestry inspection confirmed the worktree was exactly based on the current phase head, so the manifest was corrected to its full SHA and GSD merged/removed the worktree cleanly. Current plan is 7; wave 7 Plan 16-13 is ready.

### Phase 16 wave 7 execution (2026-11-26)

Runtime/model: Codex/OpenGSD 1.23.0 coordinator (primary model ID unavailable); typed `gsd-executor` gpt-6-luna xhigh. Plan 16-13 took 34 minutes, six minutes over its 20–28 minute target. `gh api` verified the exact public Redump commit, DAT blob/header and CC BY-SA 4.0 license. The pinned DAT digest matched D-06, and the runner reconciled all 13,407 occurrences as unknown without measured evidence, including 177 missing-serial and 988 duplicate-group entries. The library route now verifies cached and newly fetched DATs before parsing and writes aggregate-only reports; the raw source was removed. The cached-wrong-digest control produced accepted RED evidence. Focused runs exposed test-only probe-log contamination and a snapshot-baseline mismatch; those controls were corrected within the declared tests. The final repository-check measure was 3,900/3,901 lines.

After merge, `git diff --check cdac692..HEAD`, the compatibility inventory sort, shell syntax, source/temp-file checks and exact selector preflight passed. `cmake --preset ci`, the runner/library targets and the four plan selectors plus setup fixture and `docs.budgets` passed; `policy.gate_budget` passed 1/1. The DAT, manifest and worktree were removed. Every bounded command session returned; the sandbox denied a separate OS process-table query, so no OS-wide listing is claimed. No owner BIOS/disc paths, Docker, Act, hosted CI, or broad workflow were used. Current plan is 8; wave 8 Plan 16-14 is ready.

### Phase 16 Plan 16-14 budget proposal (2026-11-26)

Runtime/model: Codex/OpenGSD coordinator (primary model ID unavailable); GSD executor gpt-6-luna xhigh. The complete uncommitted candidate measures **4,694 counted gate lines**, including the 347-line synthetic-control file under the existing `tests/cmake/*.cmake` inventory; the baseline was 3,900/3,901, so the candidate is 793 lines over the unchanged ceiling. The independent reuse audit found no safe duplicate logic to remove. The committed matrix draft has 51 required data rows; integrated provider/profile boot-to-play selectors remain absent, so the generated metadata stays experimental. The exact focused capability/info/cheat selector set passed 7/7 after relocating controls; targeted libretro builds passed; direct `cmake --install build/ci --component libretro` staging and the staged metadata claims check exited 0. The candidate budget check passed at 4,694; `git diff --check` passed. The proposal remains uncommitted and the 3,901 ceiling unchanged pending owner review. No broad workflow, hosted CI, Act job, Docker resource or fuzz campaign ran. The temporary install prefix was removed by its exit trap; all launched command sessions exited, while a separate process-table query was sandbox-denied, so no OS-wide process listing is claimed. CI/CD efficiency, local Act use, full local evidence, and task-named Docker cleanup guidance are already discoverable in `AGENTS.md`, `.planning/AUTONOMOUS.md`, and `docs/maintaining.md`. Next command after context clear: `$gsd-resume-work`.

### Phase 16 wave 8 completion (2026-11-26)

Runtime/model: Codex/OpenGSD 1.23.0 coordinator (primary model ID unavailable); resumed the GPT-6 Luna xhigh executor's existing draft. The owner approved the 4,694-line ceiling with all checks counted. Focused capability, metadata, cheat, package staging and budget checks passed. A new truncated-JUnit control failed before the importer fix and passed afterward. The measured total is 4,693 lines. Direct file and commit checks confirmed the summary despite the helper's documented false positives on command strings and test history. GSD manifest-scoped cleanup exited 0 and integrated the completed worktree. The staged libretro metadata remains experimental while provider and profile play evidence is missing. See the [plan summary](phases/16-patches-cheats-and-compatibility-evidence/16-14-SUMMARY.md).

### Phase 16 execution, review and verification (2026-11-26)

Runtime/model: Codex and OpenGSD 1.23.0, primary model ID unavailable. The coordinator completed Plans 16-15 through 16-18. The GPT-6 Luna xhigh source review identified WASI staging, Nightly target and JUnit result attribution defects. Focused native, pinned WASI, and sanitizer replay checks passed after the fixes. The first complete workflow attempt surfaced a CUE probe control that still expected raw Mode 1 to be refused. The corrected control verifies synthetic identity for that mode and retains the unsupported-mode assertion.

`cmake --workflow --preset ci` exited 0 on the reviewed source and test correction, including package staging and release checks. The repository-check inventory measured 4,692 of the approved 4,694 lines. The linked WASI runner measured 1,014,609 bytes, and stable serialized state measured 3,757,754 bytes. The package's current capability report records 16 passing rows and 35 provider/profile rows as unknown. Libretro remains experimental. Hosted gates and the release remain for ship. See the [verification report](phases/16-patches-cheats-and-compatibility-evidence/16-VERIFICATION.md).

### Phase 15 discussion (2026-11-24)

Runtime/model: Codex/OpenGSD 1.23.0; primary session model ID unavailable. Three read-only role reports covered
domain behavior, GPU/SIO/API architecture, libretro integration, input robustness, and CI/devops. The adversarial
pass resolved their topology difference: Phase 15 promises direct-port guns only; multitap gun and shared
multi-Justifier behavior remain unclaimed. The additive API v10 recommendation preserves the 228-byte v9 prefix
and 24-byte nested controller stride. LIGHTGUN is the complete host path; POINTER is a documented subset; missing
capability is invalid input and cannot produce a hit. The beam event samples the aimed pixel at integer GPU time.
The brightness threshold is an explicit emulator model because the cited T2 source has no calibrated number.
No code changed and no tests ran during discussion.

Context and discussion log were preflighted with `git diff --check` (exit 0) and committed as `63a37af`.
Before expensive phase checks, verify ABI offsets, sorted inventories, selectors and budgets; run focused API,
SIO, beam and libretro tests before the required local and hosted gates. Use `act` only if workflow changes add
coverage, review every hosted run's critical path and runner use, and keep todo 052 deferred until the controller
plan is stable. No Docker resources were created. No owner hardware capture is planned.

The emulator has synthetic fixture and host integration evidence, but no current evidence establishes a retail
game as playable. Do not infer retail compatibility from this gun protocol work.

### Phase 15 planning research (2026-11-24)

Runtime/model: Codex/OpenGSD 1.23.0; `gsd-phase-researcher` gpt-6-luna xhigh; primary runtime model ID unavailable.
The three-role discussion fan-out was already complete, so the researcher converted that synthesis into
`15-RESEARCH.md` and checked only the current implementation seams. Timed waits for its return totaled at least
14 minutes; active work was not measured. No tests or source changes were made. The planning inputs
passed `git diff --check`, and GSD reports both `has_context` and `has_research` true. Research commit: `073013a`.

The research identified two plan risks to resolve against code: whether the aimed-pixel sample must read live
RGB555 VRAM or reconstructed RGB24 output, and how to synchronize Timer 0/1 at the exact Justifier beam time
before IRQ10. Keep the threshold numeric decision to the implementation after those representations are fixed.

### Phase 15 planning and plan review (2026-11-24)

Runtime/model: Codex/OpenGSD 1.23.0; `gsd-planner` gpt-6.1-sol high; `gsd-plan-checker` gpt-6-luna xhigh; primary session model ID unavailable. Planning produced ten plans across seven dependency waves for GUN-01, GUN-02 and SIO-05. The independent checker found one portability blocker: machine-specific GSD home paths in the plan execution contexts would fail repository hygiene. All ten references were changed to the existing home-relative `@~/.codex/...` convention; plan structure, shell/failure-direction probes, `git diff --check` and `sh scripts/hygiene.sh --tree` then passed, and the checker passed with no remaining findings. Planning artifacts are ready for the required `docs(15): create phase plan` commit; source work and implementation tests have not started. Wall time from the preceding STATE checkpoint was about 27 minutes; active time was not isolated. No hosted CI, Act or Docker resources were used.

### Phase 15 Plan 15-03 execution (2026-11-24)

Runtime/model: Codex/OpenGSD 1.23.0; `gsd-executor` gpt-6-luna xhigh; primary session model ID unavailable. Plan execution ran from 2026-11-25T04:28:06Z through 04:41:31Z (13m25s wall). Both tasks followed RED/GREEN: the pad-state probe and GPU-storage probe each produced classifier-accepted target assertion failures before their implementation commits. One strict-build `-Wshadow` failure was corrected by renaming the direct input local; the targeted rerun passed. `cmake --preset ci`, the focused SIO pad/API target build, the four-selector API/pad set (4/4), the final `sio.gun.latch` selector (1/1), the fixture budget check (1/1), ABI-offset checks and sorted SIO inventory preflight passed. No broad workflow, hosted CI, Act or Docker work was needed at this plan boundary. Task commits: `3e9af4a`, `fd627ab`, `f275820`, `c0af4f1`; summary: `.planning/phases/15-guncon-and-justifier/15-03-SUMMARY.md`.

### Phase 15 Plan 15-08 execution (2026-11-25)

Runtime/model: Codex/OpenGSD 1.23.0; `gsd-executor` gpt-6-luna xhigh; primary session model ID unavailable. The recorded wall window ran from the Plan 15-07 closeout at 2026-11-25T13:49:09Z through the final task commit at 14:18:35Z (29m26s). Task 1 used an accepted failing POINTER integration assertion before implementing capability-gated fallback. Task 2 found frame latching and tap restrictions already implemented by prior plans; regression tests plus documentation passed without a production fix.

`cmake --preset ci`, required-selector enumeration, ABI v9, documentation budget and fixture budget checks passed. The Task 1 three-selector set passed 3/3, the Task 2 five-selector set passed 5/5, targeted builds and `git diff --check` passed. The first Task 2 run exposed an incomplete test-host LIGHTGUN trigger setup after pointer input state was separated; the mock was corrected before the final green run. A full-tree hygiene scan was stopped after about 25 seconds (exit 130) as redundant; the configured staged hygiene hook passed on both task commits. No broad workflow, hosted CI, Act or Docker or fuzz campaign was run. Task commits: `3390f94`, `f8e2a4e`, `059cc0f`, `a6f6b09`; summary: `.planning/phases/15-guncon-and-justifier/15-08-SUMMARY.md`.

### Phase 15 Plan 15-09 execution (2026-11-25)

Runtime/model: Codex/OpenGSD 1.23.0; `gsd-executor` gpt-6-luna xhigh; primary session model ID unavailable. The plan ran from 2026-11-25T14:26:43Z through task closeout at about 15:05Z (38 minutes wall time). Both sequential tasks had accepted RED evidence before implementation. Focused SIO/GPU continuation selectors passed 4/4, then the aggregate state/version/no-allocation selectors passed 8/8, with the final GPU reset/disabled-display cases passing 1/1. The exact local stable-state producer reported 3,757,754 bytes and the updated exact quality bound comparison passed. An intermediate visitor mismatch led to matching signed fixed-width serialization to visitor bytes; the test-only field inventory was expanded to cover the new fields. `cmake --preset ci`, ABI v9, selector preflight and `git diff --check` passed. No broad workflow, hosted CI, Act, Docker or fuzz campaign was run. Task commits: `f879ecc`, `e9fe132`, `00f3677`, `00f030b`; summary: `.planning/phases/15-guncon-and-justifier/15-09-SUMMARY.md`.

### Phase 15 Plan 15-10 execution (2026-11-25)

Runtime/model: Codex/OpenGSD 1.23.0; `gsd-executor` gpt-6-luna xhigh. Plan work took about 60 minutes after the Plan 15-09 closeout; exact start time was not separately recorded. Both tasks completed: five synthetic gun feature rows and generated compatibility wording were added, and state/WASI/Cachegrind quality values were remeasured against the phase merge base. Cheap inventory, selector, fixture, repo-check budget and docs checks preceded the producers. The 12 focused gun selectors passed; the quality policy group passed 5/5. The linked WASI artifact measured 956,794 bytes; state measured 3,757,754 bytes. Cachegrind was within the existing 100-basis-point limit for both affected workloads. The focused compatibility CTest command returned exit 8 because its `built.openbios.shell` fixture setup produced a different hash on the Phase 15 head than on the merge base; the seven selected compatibility/documentation tests themselves passed. This earlier-phase regression is recorded for phase review. The Ubuntu producer container and image and temporary source trees were removed after measurement; no fuzzing or broad workflow ran. Task commits: `d7ee26e`, `25943c2`; summary: `.planning/phases/15-guncon-and-justifier/15-10-SUMMARY.md`.

### Phase 14 final hosted verification and release (2026-11-24)

Runtime/model: Codex; primary session model ID unavailable. The corrected exact-head Main CI run
[45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076) passed on `75beb849f4b9e8a9a3bd023cc5c80632e4c74d56`
in 3,943 seconds (65m43s); its BE32 job passed in 64m09s, including the exact 947,969-byte WASI gate, portable tests,
all six build legs, ASan, RetroArch, and the cross-platform hash comparison. Nightly
[45442543657](https://github.com/szTheory/schwaystation/actions/runs/45442543657) passed in 814 seconds (13m34s); its
fuzz job took 802 seconds and the 11 bounded campaigns took 672 seconds. Both timing endpoints reported zero billable
milliseconds, so no dollar savings are inferred.

The final-hosted-run review found the BE32 CTest lane was the Main critical path. It ran 657 tests in 3,709.53 seconds;
`libretro.input.movies` was explicitly `RUN_SERIAL` and took 1,106.63 seconds after the other 656 tests completed. A
dedicated movie-test shard could save about 18 minutes while retaining every case, but needs isolated fixture setup,
disjoint shard inventories, and an exact JUnit-union check. Phase 14 research excludes workflow wiring, so this is
recorded as todo 052 bound to Phase 15. The Nightly fuzz job is dominated by its 11 required 60-second campaigns;
parallelizing them might reduce wall time, but the evidence does not show preserved per-target throughput or a cost
reduction. Existing cheap preflight and CTest auto-parallelism remain; no quality or coverage was reduced.

PR #42 merged as `1256732fe7d6d75166a54bbdefa07ab78e667774`. Post-merge Main CI
[45454749260](https://github.com/szTheory/schwaystation/actions/runs/45454749260) and Release
[45454749282](https://github.com/szTheory/schwaystation/actions/runs/45454749282) passed. Release PR #43 merged as
`f08fc08d40a1ebcb42708e0a31be3111e3eed711`; its Main CI
[45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354) and Release
[45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400) passed, and v0.14.0 was published.
No Docker resources were created during this final verification attempt.

### Phase 14 verification closeout (2026-11-24)

Runtime/model: Codex/OpenGSD 1.23.0; primary session model ID unavailable. Verification elapsed time was not separately isolated.
Goal-backward verification passed all four roadmap criteria and all 45 plan truths. The 46 declared plan artifacts passed
existence/substance checks. The path-only link heuristic found 1/30 links; manual C/CMake/data-flow inspection traced the
cross-file function calls, generated inventory/ledger links and documentation links, with the corresponding full-suite tests
passing. SIO-01–04 are satisfied. The GSD phase-completion transition updated the requirement statuses, roadmap and state.

`cmake --workflow --preset ci` exited 0 on the initial Phase 14 source head `d7d3cbf`: 705/705 CTest cases passed, all three macOS arm64 packages
were produced, and the separate CI release checks passed 8/8. The CTest critical path was 115.10s; the release checks took
3.12s. The slowest cases were `policy.quality_metrics` (71.35s), `libretro.host` (51.50s), `libretro.input.movies` (33.12s),
`runner.disc.legacy` (30.44s), and `runner.hashes_file` (23.28s). The local quality gate is serial and instrumentation-heavy;
the prior 44.35s measurement and existing seed `.planning/seeds/SEED-261122-vlf-reduce-quality-gate-test-runtime-without-losing-coverage.md`
show that a safe speed-up needs isolated harness refactoring. No assertion reduction or broad rerun is justified here.
No workflow changed, the local workflow already exercises its controls, and Act would add no job-step coverage; no Act or Docker
resources were used. CI/CD efficiency guidance is already discoverable in tracked `AGENTS.md`, `.planning/AUTONOMOUS.md` and
`docs/maintaining.md`. Hosted WASI module size and Cachegrind instructions/frame remain with the exact named producers; no
quality bound was guessed or widened. This was a local workflow, so there is no run URL; hosted Main and Release checks remain
pending Phase 14 ship. No retail-game compatibility claim is inferred from peripheral protocol fixtures.

### Phase 14 hosted CI correction (2026-11-24)

Runtime/model: Codex; primary session model ID unavailable. A focused read-only source review confirmed the corrected SIO slot indexing.
PR #42 (`phase/14-multitap-and-special-controllers`) had auto-merge enabled at head `39548c5`. Its first Main CI run
[45416515458](https://github.com/szTheory/schwaystation/actions/runs/45416515458) exposed GCC object-bounds UB in the SIO state
serializer: passing `&tap_slot_id[0][0]` with length eight crosses the first inner four-byte array. The same compile failure
caused Linux x64/arm64, no-FP, GPU reference, BE32, WASI and perf jobs to fail. CI checks that finished before cancellation
included hygiene, OpenBIOS and macOS arm64. The initial Nightly benchmark jobs passed; its fuzz job was still running.

After confirming the compiler diagnostic in the linux-x64 log, obsolete Main CI and Nightly runs
[45416515458](https://github.com/szTheory/schwaystation/actions/runs/45416515458) and
[45416515484](https://github.com/szTheory/schwaystation/actions/runs/45416515484) were canceled before a repaired-head attempt.
Their elapsed wall times were 426 seconds and 395 seconds. GitHub's timing endpoint returned zero per-job milliseconds for
these canceled runs, so no raw runner-minute or billing savings are claimed.

Commit `440022c` flattens the eight-byte ID storage and uses `port * 4 + socket` for every read, write and per-port reset,
preserving the old serialized byte order. A review caught and prevented an initial use of the different pad-endpoint mapping;
the final index was corrected before commit. `git diff --check` and the SIO inventory sort check exited 0. The existing local
SIO/core test targets rebuilt successfully, and `ctest --test-dir build/ci --output-on-failure -R
'^(sio[.]tap[.](poll_methods|history_edges)|core[.]state[.](round_trip|rejects))$'` passed all four selectors in 2.07 seconds.
An Ubuntu 26.04 arm64 container compiled `schwaystation_sio` with GCC 15.2 and warnings-as-errors (exit 0). The first attempt
to use `cmake --preset ci` exited 1 because the image's CMake 4.2.3 cannot parse this repository's preset schema 11; direct
CMake configure/build was used instead. The task-named `--rm` container cleaned itself up; no task-created image, volume or
network remains. Workflow wiring was unchanged, and `act` would not add coverage beyond the direct GCC compile and local tests.

That reviewed SIO correction and its evidence were pushed to PR #42; the following hosted attempt and additional findings
are recorded below. Phase 15 remains behind Phase 14's passing final-head CI and release boundary.

### Phase 14 second hosted run and exact quality measurements (2026-11-24)

Main CI run [45420382438](https://github.com/szTheory/schwaystation/actions/runs/45420382438), on head `1dcc24c`, found the
separate C17 GCC issue at `lr_card_sockets_load(card_paths)`: a mutable 2D character array cannot be implicitly converted
to a pointer to arrays with nested `const` before C23 under `-Wpedantic`. Linux x64, no-FP x64/arm64, BE32 and other build
lanes reported the same call-site diagnostic. The fix in `libretro/libretro.c` now builds a bounded eight-element view of
`const char *` row pointers and passes it to the synchronous loader, which copies path data without retaining the view.
Read-only review found no type, bounds, lifetime or behavior issue.

That same run yielded the exact Phase 14 measurements before its quality gate stopped the WASI and perf jobs: linked Release
WASI size was 947,976 bytes; Cachegrind reported testcard 1,620,101 and GPU motion 138,319,540 Ir/frame. The relative perf
comparisons passed, but all three old exact pins were below the new measured values. `quality.json` now tracks these exact
producer results, and `docs/testing.md` shows the current values. Evidence: [WASI job 138538583783](https://github.com/szTheory/schwaystation/actions/runs/45420382438/job/138538583783)
and [perf job 138538117721](https://github.com/szTheory/schwaystation/actions/runs/45420382438/job/138538117721). The run was canceled after
210 seconds wall time once the deterministic compiler and bound failures were confirmed. GitHub returned zero per-job timing
after cancellation, so raw runner-time and billing savings are unavailable.

Local follow-up on the corrected source: `cmake --build build/ci --target schwaystation-test-libretro-host` exited 0;
`ctest --test-dir build/ci --output-on-failure -R '^(libretro[.]cards[.](multitap|persistence|controls)|libretro[.]input[.]multitap)$'`
passed 4/4 in 5.16 seconds. The GCC 15.2 warnings-as-errors CMake build of `schwaystation_libretro` under the no-FP
configuration exited 0; a read-only review confirmed C17 pointer compatibility. The exact WASI/perf measurement checks,
`jq -e . quality.json`, inventory sorting, and `git diff --check` all exited 0. The final local
`cmake --workflow --preset ci` on the corrected source and updated pins exited 0: 705/705 CTest tests passed in 69.07
seconds, all three macOS arm64 packages were produced, and the CI release checks passed 8/8 in 1.47 seconds.

The earlier Nightly run [45420383435](https://github.com/szTheory/schwaystation/actions/runs/45420383435) completed successfully
on `1dcc24c`. The three native bench jobs took 45–61 seconds each, the WASI benchmark 142 seconds, and full vectors 70
seconds; each ran alongside fuzz. The fuzz step itself took 674 seconds and executed eleven bounded 60-second campaigns
sequentially, after fixture and sanitizer setup. This was the run's critical path (13m50 total). Inference from the step
commands and durations: splitting the campaigns into runners repeats builds/setup, while running them together on one
runner risks cutting their executions per campaign. No safe, cost-neutral reduction was evident without changing the
campaign budget. Main CI route checks passed in about 8 seconds and required lanes ran in parallel; deterministic compiler
and exact-pin failures were canceled at 210 seconds. GitHub exposed no per-job billing times for that canceled run. No
workflow wiring changed; direct GCC builds and focused CTests already exercised the fixes, so `act` would not add job-step
coverage. The repaired final head still needs Main CI and Nightly before Phase 14 ship.

### Phase 14 final GCC ABI gate correction (2026-11-24)

Runtime/model: Codex with GSD code-reviewer and gsd-verifier `gpt-6-luna` xhigh; primary runtime model ID unavailable.
The final-head retry on PR #42 initially pointed to commit `de5e368`. Both no-FP Linux jobs failed the same early
`abi.globals` test: GCC's ELF archive classified the static `const char *` watchdog-name table as local writable
relocation data (`d name_watchdogs`). The table contains immutable names, but its pointer representation made the ABI
gate reject it. `src/pad.c` now stores the same eight NUL-terminated names in fixed-width constant character rows,
preserving their endpoint indexing and serialized state keys. A focused GCC 13 Linux C17 compile with
`-mgeneral-regs-only` and GNU `nm -P` exited 0 and reported `name_watchdogs r` (read-only data). Independent code review
found no issue.

The obsolete Main CI run [45425051636](https://github.com/szTheory/schwaystation/actions/runs/45425051636) was canceled
after 219 seconds wall time once both no-FP failures were confirmed. Nightly [45425051737](https://github.com/szTheory/schwaystation/actions/runs/45425051737)
was canceled after 189 seconds; its fuzz job reached the campaign step, which was stopped 21 seconds after starting.
GitHub's timing endpoint returned zero per-job milliseconds for both canceled runs, so no billed runner-minute savings
are claimed. The quick route and source preflights passed; workflow wiring was unchanged and local GNU nm plus GCC
directly reproduced the failing ABI condition, so `act` offered no additional coverage for this source-only correction.

After the correction, `cmake --workflow --preset ci` exited 0: 705/705 CTest tests passed in 73.01 seconds, all three
macOS arm64 packages were produced, and the CI release checks passed 8/8 in 1.47 seconds. The Phase 14 code review is
clean, all 49 goal-backward truths and four roadmap criteria remain verified, and the Phase 14 verification digest is
refreshed at `2ab13f4`.

### Phase 14 final WASI measurement and hosted runner fault (2026-11-24)

Runtime/model: Codex; primary runtime model ID unavailable. The follow-up local module build took about one minute,
including the first dependency install. On source commit `2ab13f4`, the task-named, auto-removed Ubuntu 26.04 arm64
container used Clang 21.1.8, LLD 21.1.8, `wasi-libc` 0.0~git20250726.3f7eb4c-4, the pinned CHD-enabled Release runner
configuration, and the `schwaystation-run` target. Inside that container,
`cmake -S /work -B /tmp/schwaystation-wasi-build -G Ninja -DCMAKE_TOOLCHAIN_FILE=/work/cmake/toolchains/wasm32-wasi.cmake
-DCMAKE_BUILD_TYPE=Release -DCMAKE_COMPILE_WARNING_AS_ERROR=ON -DSCHWAYSTATION_BUILD_TESTS=OFF
-DSCHWAYSTATION_BUILD_LIBRETRO=OFF -DSCHWAYSTATION_BUILD_RUNNER=ON -DSCHWAYSTATION_CHD=ON
-DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` and `cmake --build /tmp/schwaystation-wasi-build --target schwaystation-run`
exited 0. `stat -c 'WASI_MODULE_BYTES=%s' /tmp/schwaystation-wasi-build/schwaystation-run` reported 947899. The exact
`tests/cmake/quality.cmake` artifact check exited 1 because the current floor was 947,976. Its WASM magic check passed,
and its measured byte count is the producer result. The old measurement was from the pre-table source; the final module
is 77 bytes smaller. After updating `quality.json`, `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DMEASURED=947899
-P tests/cmake/quality.cmake` exited 0; `ctest --test-dir build/ci --output-on-failure -R
'^policy[.]quality_metrics$' --no-tests=error` passed 1/1 in 31.17 seconds. `jq -e . quality.json`, the SIO inventory
sort check and `git diff --check` also exited 0. The first container attempt failed because the minimal image lacked
`ca-certificates`; the next named `--rm` run included it and completed the build. Docker left no task-created container,
image, volume or network.

Main CI run [45428490941](https://github.com/szTheory/schwaystation/actions/runs/45428490941) on PR head `4bcb76e`
failed before its WASI build: `apt-get update` saw Microsoft's feed but no Ubuntu package index, so `wasi-libc` and
`libclang-rt-21-dev-wasm32` had no installation candidate. The same-head Nightly run
[45428491372](https://github.com/szTheory/schwaystation/actions/runs/45428491372) installed those exact packages and
passed its WASI benchmark, vectors, benchmarks and fuzz suite. Main CI passed no-FP, perf, hygiene, OpenBIOS, GPU
reference and the six-platform builds that completed before cancellation. It was canceled at 1,159 seconds wall time
after the local producer exposed the stale exact pin; Nightly completed in 796 seconds with its fuzz job taking 782
seconds. GitHub's timing endpoint returned zero billable milliseconds for both runs, so no runner-minute savings are
claimed. Main CI plus its WASI-dependent hash comparison must pass on the corrected exact bound before Phase 14 ships.

### Phase 14 matched x86_64 WASI producer correction (2026-11-24)

Runtime/model: Codex; primary session model ID unavailable. Main CI on head `d46ec1e` reached its linked-module size
step and measured 947,969 bytes, failing only because the local update had set 947,899. The earlier 947,899-byte
reproduction used an arm64 container and `SCHWAYSTATION_BUILD_TESTS=OFF`, so it did not match the hosted x86_64
`wasm32-chd` preset. An x86_64 tests-off build measured 947,947; the tests-enabled build added the test-only card hook
and matched CI exactly at 947,969.

The local matched producer used a task-named Ubuntu 26.04 x86_64 container, Clang/LLD 21.1.8, `wasi-libc`
0.0~git20250726.3f7eb4c-4, `libclang-rt-21-dev-wasm32`, Release, CHD enabled, and test registration enabled. Its
direct CMake configure, `cmake --build ... --target schwaystation-run`, and `stat` command exited 0 with
`WASI_MODULE_BYTES=947969`. Test emulator/fixture-tool paths were placeholders for configuration only; no WASI CTest
was run locally. The exact measurement comes from both this producer and Main CI run
[45436318507](https://github.com/szTheory/schwaystation/actions/runs/45436318507). The pin is corrected to floor and
ceiling 947969. `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DMEASURED=947969 -P
tests/cmake/quality.cmake` exited 0; the focused `ctest --test-dir build/ci --output-on-failure -R
'^policy[.]quality_metrics$' --no-tests=error` passed 1/1 in 47.15 seconds. The quality JSON, SIO inventory sort and
`git diff --check` also passed before the retry push.

After confirming the hosted mismatch, obsolete Main CI [45436318507](https://github.com/szTheory/schwaystation/actions/runs/45436318507)
and Nightly [45436318741](https://github.com/szTheory/schwaystation/actions/runs/45436318741) were canceled at 323 and
292 seconds elapsed. Nightly had already passed its WASI benchmark, all three native benchmarks and vectors; its fuzz
job lasted 283 seconds wall, with the bounded-campaign step canceled after 180 seconds. Both timing endpoints returned zero billable milliseconds, so no cost savings are
claimed. The x86_64 container and x86_64 base image pulled for it were removed; the pre-existing arm64
`ubuntu:26.04` tag was restored. The corrected exact bound needs one final Main CI/Nightly attempt before Phase 14
ships. No workflow changes were made; direct exact-producer reproduction provided more relevant coverage than `act`.

### Phase 14 tail planning split (2026-11-24)

Runtime/model: Codex/OpenGSD 1.23.0; configured planner `gpt-6.1-sol`; active runtime model ID unavailable.
Planning duration was not separately measured. Plan14's 32m22s execution exceeded the 30-minute cap; the
unstarted 11,500-token Plan15 had two independent existing task boundaries. Plan15 now owns only the
compatibility ledger and generated evidence (5,000-token projection, Wave12). New Plan16 owns only measured
metrics, fixture state hashes and discoverable documentation (6,500-token projection, Wave13), then the
complete final review/verification handoff. Each single-task implementation slice targets 20–28 minutes.
The estimate-calibration query returned factor1, zero samples and low confidence; no speedup is claimed.

Both original tasks, automated verification commands, done criteria and the complete phase handoff are
preserved byte-for-byte across the split. GSD schema/structure checks, shell/Python syntax, read-only selector
inspection, dependency/ownership checks and diff checks passed. The structure parser's existing bare-diff
advisory remains appropriate for task checks against the uncommitted tree; the decision gate skips this
context's unnumbered decisions, whose grouped aliases remain covered by the source audit in Plan16.
ROADMAP and the GSD plan index now show 14/16 complete and 13 waves; only Plan15 is ready. Plans01–14,
all summaries and existing requirement statuses are unchanged. No source change, configure/build,
implementation test, hosted CI, Act, Docker or execution ran; no hosted run URL applies. The parent
orchestrator acts next on the reviewed planning split; neither remaining plan has started.

### Phase 14 Plan 14-14 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The measured
wall duration was 32m22s (11:20:03–11:52:25 UTC), 2m22s past the 30-minute plan cap. The executor stopped after
its single task commit, `39cd818`; the orchestrator completed GSD closeout without further implementation.

The CI configure/build and focused card tests passed. `libretro.cards.multitap`, `libretro.cards.persistence`
and `libretro.cards.controls` passed 3/3 in 18.34s. The direct host test was green on its first run because the
state, reset, failed-load and save-identity invariants already existed in production; no runtime change or
artificial RED commit was made. This slice adds test-only probes/assertions and save ownership documentation.
Shared SIO-01 remains open until its last declaring plan finishes. No broad suite, hosted CI, Act or Docker was
run; Plan 16 owns the final reviewed-head aggregate verification.

### Phase 14 Plan 14-15 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Wall duration
was 13m, starting at 12:06:17Z. Task commit `a05c691` records the compatibility rows and independent ratchet
expectations.

The feature data rows passed the byte-sort, field-count, duplicate-key and compatibility-schema preflights. The
existing `build/ci` was reused; CI configure and `schwaystation-test-probe` build passed. The exact compatibility
feature selector presence check passed, then `compat.features.preflight`, `compat.features.ratchet`, and
`compat.features.controls` passed 3/3 in 1.30 seconds. The compatibility document was regenerated and its
`--check` passed. Direct SIO/card/runner/libretro selectors cited by the rows passed in their owning prior plans;
they were not rerun here. No broad suite, hosted CI, Act or Docker ran. Shared SIO-01–04 remain gated until Plan
14-16 and phase verification complete.

### Phase 14 Plan 14-16 execution (2026-11-24)

Runtime/model: Codex/OpenGSD 1.23.0; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Wall duration was about 32 minutes from 12:25:11Z through GSD closeout, exceeding the 30-minute slice cap by about 2 minutes. The task commit is `b51876c`.

The reused `build/ci` stable-state producer measured 3,757,638 bytes, so `quality.json` now tracks that exact value; `core.state.stable_bytes` passed 1/1. The frame-150 and frame-180 `gpu_motion` state hashes were directly reproduced and their focused selectors passed 2/2. Controller, saves and runner pages are linked from the embedding/testing guides, which remain under their byte limits. Implementation and focused checks finished about 12 minutes into the slice; the additional wall time was GSD closeout, not implementation scope.

No local `clang-21`/`wasm-ld` or Valgrind/Cachegrind was available. WASI size awaits `.github/workflows/ci.yml` `wasm32` → `Measure linked WASI module`; Ir/frame awaits the `perf` job → `Instructions per frame` after `Cachegrind` setup. Neither bound was guessed or changed. The parent orchestrator owns source review and the single aggregate workflow after review; SIO-01–04 remain open for phase verification.

### Phase 14 Plan 14-13 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; OpenGSD 1.23.0; primary runtime model ID unavailable. Wall duration was 20m through metadata commit. Two task commits landed: `7686490` (RED persistence assertions) and `444e8af` (seven adapter sidecars).

The libretro inventory sorted cleanly, and the selector preflight found all five planned tests. Existing `build/ci` was reused; `cmake --preset ci` and the focused libretro host/core build passed. The owned card persistence, controls, multitap and memory alias/boundary selectors passed 5/5 in 5.90 seconds. The direct TAP RED run failed on missing additional sidecar paths and classified `RED_EVIDENCE_OK`. Two strict-build issues in the test hooks were corrected before accepting RED. Shared SIO-01 remains open until its other declaring plans finish. No broad suite, hosted CI, Act or Docker was run; Plan 15 owns final reviewed-head integrated verification.

### Phase 14 Plan 14-12 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Measured
wall duration was 24m. Two task commits landed: `f295c54` (special mapping RED checks) and `79e8c0c`
(special mappings and documentation).

Cheap diff, sorted inventory and exact-selector preflights passed before configure/build. Existing `build/ci`
was reused; CI configure and focused libretro host/core build passed. The `libretro.input.multitap`,
`libretro.input.special` and `libretro.input.descriptors` selectors passed 3/3. An initial green attempt
exposed a test counter incorrectly shared between players; the test now scopes observations by player and
the rerun passed. Phase-wide SIO requirements remain open until all their declaring plans finish. No broad
suite, hosted CI, Act or Docker was run; Plan 15 owns final reviewed-head aggregate verification.

### Phase 14 Plan 14-11 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Measured
wall duration was 24m. Three task commits landed: `b72c468` and `b1e8fd3` (RED coverage), then `474aa6a`
(compact multitap topology, routing and documentation).

Cheap `git diff --check`, inventory-sort and exact-selector preflights passed before configure/build. Existing
`build/ci` was reused; `cmake --preset ci` and the focused libretro host/core build passed. The focused
`libretro.input.multitap` and `libretro.input.descriptors` selectors passed 2/2. The summary's coverage block
passes GSD classification; SIO-01 remains gated until its other declaring plans finish. No broad suite, hosted
CI, Act or Docker was needed for this focused slice.

### Phase 14 Plan 14-10 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Wall interval was
9m from 09:30:01Z through focused verification and closeout. One task commit was made: `4cfe18e` (default two-pad
callback-to-serial baseline).

Before the focused build, `git diff --check`, the libretro inventory sort, CI configure reuse, and exact selector
presence checks passed. The focused build of `schwaystation-test-libretro-host` and `schwaystation_libretro`
passed. `libretro.input.multitap` and `libretro.input.descriptors` then passed 2/2 in 0.03s. The test helper is
confined to the card-test module; the descriptor selector confirmed it is not exported by the shipped module.
The baseline covers only default players 0/1 on socket A. Plan 14-11 owns tap options and expanded routing.
No broad suite, hosted CI, Act or Docker was needed.

### Phase 14 Plan 14-09 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The measured
wall interval was 60m42s (07:13:54–08:14:36 UTC); active time was not separately measured. Two tasks produced
three task commits: `ae0311f` (eight-socket RED), `1f14811` (runner card lifecycle), and `cc3d2d8` (independent
tap flags, docs and focused target correction). The explicit 30-minute split bound was exceeded; execution was
kept within Plan 14-09 and recorded transparently.

Before configure/build, the runner inventory sorted cleanly, `git diff --check` passed, and CTest selector
preflights found the three card selectors. The eight-socket RED exited 8 on the missing port 1/socket B file;
the JUnit evidence classified `RED_EVIDENCE_OK`. A typed card-byte view resolved the strict compile error in the
test hook. The focused trio passed 3/3 in 14.39 seconds (multitap 1.37s, controls 0.42s, persistence 14.33s).
After the final help assertions, the hook target rebuilt and multitap passed 1/1 in 0.55s. The pre-registered
selector used `schwaystation-test-run-stop`, omitted from the plan's original build list; the plan was corrected
to build that target. No workflow wiring changed, so no broad suite, Act or hosted CI run was needed. No Docker
resources were created. Plan 14-10 followed.

### Phase 14 Plan 14-08 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The measured
wall interval was 16m21s (06:55:13–07:11:34 UTC); active time was not separately measured. Two tasks produced
three commits: `ab475ee` (socket path RED), `3faadf8` (socket path GREEN), and `68e1c43` (eight-socket atomic
persistence vectors and documentation).

The existing card inventory was sorted and retained its pre-registered `card.files.tap_paths` selector. After
`cmake --preset ci` and focused card-files builds, the combined five-test selector passed 5/5 in 0.03 seconds,
covering the eight-name mapping, legacy aliases, unsafe/oversized inputs, atomic round-trip/failure behavior and
existing wrapper handling. Eight 128 KiB card files were independently written and read back; replacing one left
the other seven unchanged, and injected flush/rename failures preserved the selected prior file and left no
temporary files. No workflow wiring changed, so no broad suite, Act, hosted CI or Docker run was needed. The
suffix table and DEC.67 boundary are documented in `docs/saves.md`, linked from `docs/runner.md`. Plan 14-09 is next.

### Phase 14 Plan 14-07 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The measured
wall interval was 26m10s (06:23:04–06:49:14 UTC); active time was not separately measured. Two tasks produced
four task commits: `4412a0f` (card-state RED), `139fd5f` (card-state GREEN), `d1bb44c` (aggregate-state RED),
and `c0e2b82` (aggregate-state GREEN).

Before configure/build, the card, SIO and core inventories sorted cleanly and CTest selector preflights found the
required tests. `cmake --preset ci` and the focused card/SIO/state/API targets built successfully. The final focused
selection passed 15/15 in 3.67 seconds; it covered all eight card endpoints, candidate state rejection, state
round-trip/stable bytes, API v9, tap history and pending SIO byte/ACK events. The format-20 state producer reported
3,757,638 bytes. No serialized layout or quality bound changed. A mistaken non-mouse round-trip expectation was
corrected; no workflow wiring changed, so no broad suite, Act, hosted CI or Docker run was needed. The summary is
`.planning/phases/14-multitap-and-special-controllers/14-07-SUMMARY.md`. Plan 14-08 followed.

### Phase 14 Plan 14-06 execution (2026-11-24)

Runtime/model: Codex GSD executor configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Measured
wall interval was 19m23s (05:57:02–06:16:25 UTC); active time was not separately measured. Two tasks produced
four task commits: `bc4d8e1` (flight RED), `4bae1bb` (flight GREEN), `504b9e6` (Jogcon RED), and `4123d90`
(Jogcon GREEN).

Before configure/build, the changed path inventory sorted with `git diff --name-only | LC_ALL=C sort -cu`,
`git diff --check` passed, and Python selector assertions confirmed the pre-registered flight, Jogcon, tap and
DualShock tests. The first flight RED exited 8 on ID `41h` instead of `53h`; its JUnit record was classified
`RED_EVIDENCE_OK`. After the minimal flight ID/length branch, the six-test selection passed 6/6 in 0.01s.
The first Jogcon RED exited 8 on ID `41h` instead of `E3h`; its JUnit record was classified `RED_EVIDENCE_OK`.
After adding the native dial and direction bytes, the four-test selection passed 4/4 in 0.04s. Both focused SIO
test targets built successfully; save/load, direct ports, both tap methods, 0/128/255 axes, all button bits,
unsupported Jogcon commands and old DualShock configuration/rumble/watchdog cases were exercised.

The initial Task 1 green run exposed a mistaken expected high-byte vector, corrected before the passing rerun.
The first Task 2 test compile exposed duplicate local variable names, corrected before RED evidence. A relative
JUnit path was also corrected to an absolute ignored evidence path before classification. No workflow wiring
changed, so no broad workflow, Act or hosted CI run was needed; no Docker resources were created. Plan 14-05's
neGcon coverage plus this Jogcon implementation completes SIO-03. SIO-04 remains pending the later libretro
mapping. Plan 14-07 followed.

### Phase 14 Plan 14-05 execution (2026-11-24)

Runtime/model: Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The
measured wall interval was 29m16s (05:25:25–05:54:41 UTC); active time was not separately measured. Two tasks
produced five plan commits: `96b9aa7` (mouse RED), `5bc7308` (mouse implementation and shared response branch),
`5d77069` (PER.01 mouse reserved bits), `e44a17a` (neGcon vectors), and `50a9346` (protocol documentation).
The SIO inventory sorted and all three exact selectors were present before configure/build. The targeted pad/SIO
build passed; the focused mouse, neGcon and tap selection passed 3/3 in 0.03 seconds. The mouse RED evidence
classifier returned `RED_EVIDENCE_OK`. The shared response switch included neGcon behavior in Task 1's GREEN
commit, so Task 2 added and passed dedicated tests without claiming a synthetic RED. PER.01 source review
corrected the mouse's reserved button bits. No workflow wiring changed, so no broad workflow, Act or hosted run
was needed; no Docker resources were created. SIO-02 remains pending its libretro mapping, and SIO-03 remains
pending Jogcon and configuration-mode coverage. Plan 14-06 is next.

### Phase 14 Plan 14-04 execution (2026-11-24)

Runtime/model: Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Measured
wall interval was 38m59s (04:41:54–05:20:53 UTC); active time was not separately measured. Two tasks produced four
plan commits: `1084eb1` (API RED), `c3cfb29` (eight-slot API/storage), `8b23612` (routing RED), and `3158414`
(socket routing and docs). The card and SIO inventories sorted cleanly; exact selector preflights passed before
the focused builds. API configure/build and ABI checks passed. The focused tap/card selection passed 8/8 in
0.08s, followed by the affected state-restore selectors passing 3/3 in 1.00s. Both task RED-evidence checks
returned `RED_EVIDENCE_OK`. One PocketStation test expectation was corrected to match the fixture's unset flags,
and the affected selectors passed afterward. No workflow wiring changed, so no Act or hosted run was needed; no
Docker resources were created. Plan 14-03 controller-slot evidence plus this plan's independent card routing,
storage and protocol-state coverage complete SIO-01. Plan 14-05 is next.

### Phase 14 Plan 14-03 execution (2026-11-24)

Runtime/model: Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The
measured wall interval was 31m31s (04:01:30–04:33:01 UTC); active time was not separately measured. Two tasks
produced three task commits: `f9b8d11` (RED), `7f0b42c` (router/history), and `8b876d0` (edge coverage and
protocol documentation). `git diff --check`, sorted SIO inventory, and exact selector preflights passed. The
Task 1 RED selector exited 8 on the missing Method 2 routing and next-transaction Method 1 behavior; the GSD
RED-evidence checker returned `RED_EVIDENCE_OK`. The final focused selector command exited 0 with 6/6 tests
passing in 1.26 seconds, including independent ports, Method 1 response history, Method 2 socket routing,
empty/missing slots, aborts, saved deadlines, and no final ACK/IRQ. Workflow wiring did not change, so no Act
or hosted run was needed; no Docker resources were created. SIO-01 remains phase-level pending for card routing
and host persistence. Plan 14-04 is next.

### Phase 13 Plan 13-01 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The recorded wall interval was 20m43s from the saved planning-state checkpoint at 14:43:33Z to closeout at 15:04:16Z; executor start was not separately captured and active time was not measured. Task commits: `9ceab60` registered `core.profile.machine`; `067c686` added API v8 board selection and defaults.

`git diff --check` and both sorted exact inventories passed. The repository-check budget remained at its 3,901-line ceiling; the API version preflight covered 249 public header names. The first planned configure/build/CTest command exited 1 on the stale `sizeof(schway_profile) == 16` assertion; configure passed and CTest did not run. After asserting the board offset at 16 and the new size at 20, the focused configure/build/CTest command exited 0: 8/8 reported tests passed in 0.21 seconds, including the board matrix, old-size compatibility, ABI v8 surface and behavior revision check. No workflow changed, so no Act or hosted CI run was needed. PROFILE-02 remains pending until Plan 13-04 adds state validation, serialization and hash coverage; Plan 13-02 is next.

### Phase 13 Plan 13-02 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The measured wall interval from the saved STATE checkpoint at 15:11:48.258Z through the 16:11:52Z closeout checkpoint was 60m04s; executor start and active time were not separately captured. The task used the RED registration commit `77ae668` and GREEN classifier commit `4ab751c`.

The initial CI preset probe configured in 0.9s but exposed 0/4 MDEC selectors, so the checked plan was corrected to the supported MDEC-only Nightly lane. Nightly configure first discovered the four existing selectors (1.8s), then after registration the exact selector graph included only `mdec.reference.input_preflight`, `mdec.reference.color_assets`, and `mdec.reference.edge`. Archive/member extent and digest checks passed. The initial registered edge test exited 8 in 0.07s after both setup fixtures passed; it reproduced the 1,696-word movie15 DMA0 exhaustion at 1,902,230 ticks without a fault. The classifier now accepts only that pinned source count and exact MDEC/DMA signature after eight successful run/TTY empty-FIFO samples, below the unchanged 4,915,200-cycle cap.

The corrected preflight sequence passed: Nightly inventory sort and the 3,901-line budget gate each exited 0 in 0.00s; MDEC-only Nightly configure exited 0 in 1.17s; exact three-name selector assertion exited 0 in 0.00s; focused target build exited 0 in 0.12s; and the single corrected CTest selection exited 0 with 3/3 tests in 0.06s (source preflight 0.02s, assets 0.03s, edge 0.03s). Its saved log reports source complete (1,440 blocks / 240 macroblocks / zero trailing), decoder FIFO empty, guest expected-incomplete, display not compared, ticks 1,902,230/4,915,200, DMA0 idle, DMA1 waiting remain=9/phase=2, and fault none. No full unaffected 4-bit/8-bit suite, hosted CI, Act job, or Docker resource ran because this plan changed no workflow wiring. MDEC-01 remains pending until full Phase 13 verification; no playback, decoded-frame, or hardware-conformance claim is made. Plan 13-03 is next.

### Phase 13 Plan 13-03 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Wall interval was 59m22s from the saved STATE checkpoint at 16:14:55Z through the final implementation evidence checkpoint at 17:14:17Z; executor start and active time were not separately captured. Seven plan commits were measured from the persisted ledger (`fc67c35` through `cc25612`).

Task 1 preflights passed: nine board rows, two 0x0B80 entries, sorted core/GPU inventories, 3,901/3,901 line budget and exactly 14 selectors including the two disc fixtures. The initial implementation's build and 14-test run passed in 0.50s and 0.15s, but only before the first-boot register seam was found; those are pre-change evidence. After applying defaults during profile selection and adding before/after-reset assertions, the final core/GPU build passed in 0.45s and the exact selector passed 14/14 in 0.13s.

Task 2 preflights passed: firmware-generation counts were vC0/vC1/vC2/vC3 = 2/1/2/4 across nine rows, core/CD-ROM inventories were sorted, diff hygiene passed, and the line-budget gate remained 3,901/3,901. CI reconfigure passed in 0.81s. The exact selector graph was 5/5 in 0.05s, including `cdrom.profile.firmware`, `cdrom.region`, `core.profile.machine` and both disc fixtures. The targeted CD-ROM/core build passed in 0.25s and focused CTest passed 5/5 in 0.12s. A malformed selector shell wrapper exited 1 before invoking CTest; the corrected Python assertion passed. vC0 Test 22h returns INT5 `(0x11,0x10)`; vC1+ keeps the fixed-region string, and the region selector remains separate.

No hosted CI, Act, Docker, or broad suite was run because workflow wiring was unchanged. PROFILE-02 remains pending until state serialization/hash work and full Phase 13 verification. Plan 13-04 is next.

### Phase 13 Plan 13-04 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The measured wall interval was 17m02s from the saved STATE checkpoint at 17:19:44Z to final focused acceptance and the last task commit at 17:36:46Z; executor start and active time were not separately captured. Task commits were `bf503b3` (RED), `f7064ff` (GREEN), and `1391c15` (Task 2 regression assertions).

The core inventory sort and fixed 3,901-line budget passed. Task 1 configured CI in 0.81s, asserted exactly five selectors including both disc fixture setups, built the state target in 0.56s for RED, and produced the intended RED: four of five tests passed, while `core.state.profile` showed board-derived identity was not restored. The RED evidence checker passed. After implementation, the changed target build passed in 0.87s and the exact selector passed 5/5 in 0.26s. Board identity is serialized once; the shared defaults table validates board/GPU_V0 pairing and reconstructs fixed defaults after APPLY without overwriting `bus.ram_size`.

Task 2 reran the sorted inventory and line-budget checks, configured CI in 0.89s, and asserted exactly seven selectors including the disc fixtures with `core.state.record_sweep` excluded. The state target was rebuilt after test edits in 0.31s; the selected tests passed 7/7 in 0.60s. The added cases cover board zero, above-range board, a CRC-valid PU-7/GPU_V0 mismatch, prior format 18, atomic preservation of state/audio, and deterministic equal/different board state-byte hashes. Task 2's validation cases were green immediately because the validator was delivered in Task 1; no separate RED was fabricated. This sequencing deviation is documented and waived as WINDOWS entry 25.

No broad suite, hosted CI, Act, Docker, or hardware run was needed because workflow wiring did not change. PROFILE-02 remains pending until remaining Phase 13 work and final phase verification. Plan 13-05 is next.

### Phase 13 Plan 13-05 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. A trustworthy start/end interval and active duration were not recorded. Task commits are `e59ff11` (host region resolution) and `e108691` (runner machine/region options); checked plan corrections are `60723cf`, `2181560`, and `177b815`.

Task 1 passed the runner inventory sort (0.00s), the 3,901-line gate (0.02s), corrected CI configure (0.75s; another configure 0.74s), exact two-selector discovery (0.03s), fixture target (0.02s), and `policy.fixture_budget` (1/1 in 0.02s). Its first focused build exited 1 in 0.27s because the new test target lacked the build-generated include directory; adding `${PROJECT_BINARY_DIR}/generated` fixed it. The successful rebuild duration was not captured. The final region/disc selector passed 2/2 in 0.04s.

Task 2 sorted both runner and compatibility inventories, kept the line budget at 3,901/3,901, configured the reused CI tree in 0.81s, and asserted the exact 16-name selector graph including `compat.features.preflight`. The focused runner/probe target build passed in 1.00s. The exact 16-test selection passed in 0.19s, including all nine machine tokens, canonical profile acceptance, region fallback/precedence, redaction, and full identity hash preservation.

No broad suite, hosted CI, Act, Docker, or hardware run was needed because workflow wiring did not change. The summary records runner-observed region/profile evidence only; PROFILE-01 and PROFILE-02 remain pending until full Phase 13 verification. Plan 13-06 is next.

### Phase 13 Plan 13-06 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Total task duration was not reliably tracked. Task commits were `c072d8e` (libretro options and profile coverage) and `a8e803b` (option documentation); measured task commit count was two from `87eac29d1fec593efb7e019c7cbd4ef6fb3b6408` through `a8e803beb950881edc3e2073b99aa73edf618ba0`.

The initial CI configure/target-registration attempt failed before any build. The production core and test targets needed to link `host/disc_probe.c` and `runner/region.c`; the plan file list and resolver-link note were corrected alongside target wiring. The subsequent CI configure passed in 0.8s. Sorted libretro inventory and the fixed 3,901-line budget passed, fixture producers were current, and `policy.fixture_budget` passed 1/1. The changed-target build passed in 0.465s. The exact profile selector closure matched 6/6 names and all six tests passed in 0.44s.

The D-01 review changed the libretro machine default/fallback to PU-18 without changing the library default. That first profile run exposed three PM-41(2) comparisons failing across the three regions: the advertised `pm41-2` token lacked an explicit parser case and had depended on the old fallback. Adding the explicit PM-41(2) mapping fixed the isolated option-profile test and the final six-test selector. The detached first audio-test launch had no authoritative exit record, so the exact test was rerun as one managed session; selector closure was 1/1 and `libretro.host.disc_audio` passed in 2.20s with both PAL and NTSC cases. `git diff --check` passed.

No workflow wiring changed, so no broad CI, Act, hosted CI, or Docker run was used. `PROFILE-01` and `PROFILE-02` remain pending full Phase 13 verification; Plan 13-07 is next.

### Phase 13 Plan 13-07 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. Total task duration was not reliably tracked. Task commits were `16c6c7b` (fast-boot profile flag) and `ec6d167` (bounded BIOS shell handoff); measured count was two from `cfe434109311ce2244630edb5798dee06e98e9f2` through `ec6d167085be33b4b53b4ff117a7773aeac59c18`.

Sorted core inventory and the fixed 3,901-line budget passed; CI configure passed in 0.7s. After Task 1 test registration, the exact profile/state selector closure matched 7/7 names. The first Task 1 build failed in 0.55s before tests because `test_state.c` used a helper local to `test_core.c`; constructing the profile locally fixed it. The corrected target build plus CTest command passed in 0.656s, with 7/7 selected tests in 0.52s. Task 2's core target build passed in 0.359s, and the exact boot selector closure included its two disc fixtures and passed 3/3 in 0.16s. Final `git diff --check` passed.

The fast-boot bit is carried by the existing serialized profile field, so state format 19 remains unchanged. Behavior revision 19 remains appropriate because only a new explicit input was added and the default path is unchanged. No workflow wiring changed, so no broad CI, Act, hosted CI, or Docker run was used. `BOOT-01` remains pending until Phase 13 verification; Plan 13-08 is next.

### Phase 13 Plan 13-08 execution (2026-11-23)

Runtime/model: Codex GSD executor, `gpt-6-luna` xhigh; primary session model ID unavailable. Wall interval was about 39m31s from the saved STATE checkpoint at 20:39:03.978Z through closeout at 21:18:35Z; active time was not separately measured. Task commits were `1459a61` (runner) and `dd12d65` (libretro); plan preflight correction was `eac1a18`.

The sorted runner/libretro inventories, fixed 3,901-line repository budget, both 524,288-byte OpenBIOS digests, current disc fixture producer, fixture budget and pre-change disc probe passed. Runner configure/build/selector took 0.6s/0.235s/0.76s; its exact five-test closure passed. Libretro configure/build/selector took 0.78s/0.90s/0.81s; its exact four-test closure passed. `git diff --check` and the Plan 13-08 summary validator passed.

The first runner test asserted that fast mode must use fewer video frames. It failed because both boot modes completed at frame 276 with the same mailbox and frame hash; this fixture does not expose the CPU-cycle shortcut at frame granularity. The unsupported speed assertion was removed, and both host tests now assert the same guest output without claiming a speedup. The core handoff remains covered separately by the existing `core.boot.fast` test. No workflow changed, so no Act, hosted CI, broad suite or Docker work was needed. CI/CD efficiency and task-named Docker cleanup guidance is already discoverable in `AGENTS.md`, `.planning/AUTONOMOUS.md` and `docs/maintaining.md`. `BOOT-01` remains pending Phase 13 verification; Plan 13-09 is next.

### Phase 13 Plan 13-09 execution (2026-11-23)

Runtime/model: Codex GSD executor, `gpt-6-luna` xhigh; primary session model ID unavailable. Wall interval was about 19m40s from the saved STATE checkpoint at 21:18:35Z through closeout at 21:38:15Z; active time was not separately measured. Task commits were `cb94d43` (canonical profile evidence and controls) and `592d21b` (generated documentation and roadmap plan status). Plan corrections were `861f21d`, `de5d4cd`, `48196d4` and `691112a`.

The core machine matrix and libretro option matrix passed 4/4 with fixture setup before profile rows were recorded. The exact compatibility feature preflight/ratchet/negative-control closure passed 3/3 in 0.50s. All 32 feature data rows are bytewise sorted; the title ledger remains at its original 10-column schema and four rows. The generated compatibility documentation reproduces from the generator and current ledgers; the generator's changed output-prose line keeps the counted repository-check gate at exactly 3,901 lines. CI configure passed in 0.80s, documentation generation in 0.11s, focused feature preflight in 0.04s, and `git diff --check HEAD` passed.

The raw CSV sort preflight initially treated the header as data and failed. Plan 13-09 was corrected to sort from the second line; all feature rows then passed. The roadmap records 9/9 plans complete, but Phase 13 and PROFILE-01/02, BOOT-01 and MDEC-01 remain unchecked/Pending until aggregate verification. The generator change did not alter workflow wiring; the single full local CI workflow is next after code review, as planned. No Act, hosted run or Docker resource was used here. Phase 13 code review is next; no Phase 14 work starts before Phase 13 ship/release.

### Phase 13 verification and completion (2026-11-23)

Runtime/model: Codex GSD verifier, configured `gpt-6-luna` xhigh; primary session model ID unavailable. Goal-backward verification passed 32/32 plan and roadmap truths, all four requirements, and all seven tracked decisions. `verification.status` returned `passed`; the report fingerprint was regenerated and matched. The source review is clean; its two earlier findings are recorded as fixed in `13-REVIEW-DISPOSITION.md`.

The final `cmake --workflow --preset ci` run on source head `ac4bb12` exited 0 in 81 seconds: 691/691 CI tests passed, all three macOS arm64 packages were produced, and the CI Release selector passed 8/8. Targeted verification passed 21/21 CI selectors, 5/5 Nightly MDEC reference selectors, and 3/3 compatibility selectors; commands and results are in `13-VERIFICATION.md`. The preceding full-workflow attempts exposed stale profile initializers, an API-version guard omission, changed state-format hashes, missing public-symbol docs, and a playlist expectation tied to the old first member. Each was fixed and targeted before the final green workflow. The later review-disposition and verification changes are planning documents only.

The CI efficiency review measured `policy.quality_metrics` at 44.35 seconds. Its serial subprocess scenarios protect real quality and timing scripts; splitting them safely requires harness isolation/refactoring, so no small change was justified. The branch also carries the Phase 12 Release Please routing optimization. `act --dryrun --strict --container-architecture linux/arm64 --platform ubuntu-26.04=catthehatter/ubuntu:act-latest --actor szTheory -W .github/workflows/ci.yml pull_request` exited 0 and validated PR workflow/job routing; it did not execute job steps. The first attempt exited 1 at Act's interactive image prompt before dispatch. The successful run used a project-named temporary action cache under `/private/tmp`, removed on exit, and created no Docker resources. The local workflow and classifier tests provide implementation coverage; hosted runners, release automation and Actions usage remain to inspect during ship. `phase.complete` reported four summary file-list warnings because command strings were parsed as paths; these are not missing implementation files. Phase 13 is verified and marked complete, but must ship before Phase 14 planning.

### Phase 12 Plan 12-05 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The 37-minute plan run completed three tasks and three task-sized commits. The selected M3U child command test passed with the generated legacy fixture setup. The compatibility feature preflight and ratchet passed 2/2; the SBI image selector plus legacy setup/parity passed 3/3. Before the broader local check, selected test names, sorted inventories and the fixed 3,901-line budget all passed. The CD-ROM/disc-image group passed 39/39 in 0.16 seconds wall time.

The broader selector exposed that the historical `SBId` fixture marker was copied beside `disc_boot.cue` and now collided with automatic same-stem sidecar discovery. Removing only the runtime copy restored legacy parity; explicit fixture tests continue to read the marker from its source path. The test compares payloads as one buffer and stops after a failed command guard to keep failure output bounded. No workflow changed, so no hosted CI or Act run was needed; no Docker resource was created. Plan 12-05 is complete.

### Phase 12 Plan 12-06 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The 64-minute wall interval completed two tasks and one implementation commit; active time was not separately measured. The new SBI replay passed both the native CI preset and an Apple Clang 21 ASan/UBSan fuzz-preset build. Sorted inventory, workflow selector, Nightly target/count/length arrays and the fixed 3,901-line budget passed. Act dry-run traversed the Nightly fuzz job without creating Docker resources. The local Apple Clang fuzzer runtime is missing, so the mutation campaign remains hosted-only; CRYPT-01 is now complete, and Plan 12-07 remains.

### Phase 12 Plan 12-07 execution (2026-11-23)

Runtime/model: Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable. The measured 9-minute wall interval (08:02:00–08:11:27 UTC) completed one task and one implementation commit through the focused test and initial GSD closeout; active time was not separately measured. The subsequent metadata-only duration-correction commit is excluded. `libretro.host` passed 1/1 in 131.00 seconds after a 4.6-second configure and 5.2-second focused build. The source count dropped runner PCM captures from six to three while retaining six callback comparisons and all size/hash/status checks. The prior BE32 result was about 18m52s; this native run is not comparable, so no hosted speedup is claimed. All seven plans were complete; this entry preceded final review, verification, ship and release.

### Phase 12 ship closeout (2026-11-23)

Runtime/model: Codex; primary model ID unavailable. The merged release commit `8a462ee2c256a7fde7b65a1ec17b24f509c0146b` passed Main CI in [run 45168856493](https://github.com/szTheory/schwaystation/actions/runs/45168856493) and the full Release workflow in [run 45168856424](https://github.com/szTheory/schwaystation/actions/runs/45168856424); both used the same SHA. Main CI used 9,173 runner-seconds (152.88 runner-minutes), with BE32 at 4,665 seconds. Release CI used 7,893 runner-seconds (131.55 runner-minutes), with BE32 at 2,638 seconds, and published the 18 platform archives as [v0.12.0](https://github.com/szTheory/schwaystation/releases/tag/v0.12.0).

CI efficiency review found the Release Please metadata push repeats Main's full matrix after Release CI already validates and packages that exact commit. A fail-closed route now skips Main's heavy matrix only for the Release App actor and the exact three modified generated metadata files; Release CI stays full. Its code and adversarial classifier cases are committed on the Phase 13 branch, and the local `cmake --workflow --preset ci` run passed 674/674 tests and 8/8 release checks before the documentation-only note. The projected saving is about 153 Main runner-minutes per metadata-only release push, based on this run; hosted confirmation is pending the Phase 13 pull request.

Phase 7–11 reports became stale as later phases changed files they cover. The GSD phase-init check confirms this does not block Phase 13 planning. Defer refreshing those reports until the final milestone audit so a green full local workflow is not repeated five times for bookkeeping; do not hand-edit coverage digests or bypass the supported stale-verification route.

### Phase 13 discussion (2026-11-23)

Runtime/model: Codex discuss orchestrator; exact primary runtime model ID unavailable. Three independent
`gsd-advisor-researcher` reviews used `gpt-6-luna` xhigh. The measured discussion wall interval from the first
clock reading at 13:23:49 UTC through context/state closeout at 13:28:05 UTC was 4 minutes 16 seconds; active
sampling time was not separately available. Context commit `f8937a8` records the nine documented board identities,
legal GPU behavior pairs, region/title/board separation, BIOS-backed opt-in shell entry and MDEC evidence boundary.
`git diff --check` passed before commit. No tests, hosted jobs, Act jobs or Docker resources were needed for this
planning-only step. Three advisor reviews and an adversarial synthesis preferred the existing feature-ledger
profile field with canonical `region.board.gpu` values; four loosely matched todos were reviewed and kept bound
to their earlier phases or test-maintenance scope. Phase 12 release is green. No tests or hosted work ran during
discussion.

### Phase 13 research and planning (2026-11-23)

Runtime/model: Codex orchestrator; GSD planner `gpt-6.1-sol` high and plan checker `gpt-6-luna` xhigh; primary
runtime model ID unavailable. Nine plans were created with serialized waves to avoid concurrent writes to the
shared CMake build tree. Final plan review passed all structure, ownership and dependency checks; all 18
automated command bodies pass `bash -n`, and failure-direction checks report zero findings. A local CTest
show-only probe exposed that the existing build directory was unconfigured and CTest treats a missing selector
as exit 0; Plan 02 now configures first and asserts the exact discovered selector set in JSON. The fixed
repository-check budget remains 3,901/3,901 lines. No implementation build, test suite, hosted CI, Act job or
Docker resource was started. Planning wall duration was not tracked. Next: execute Plan 13-01.

### Phase 11 review, verification and transition (2026-11-22)

Runtime/model: Codex/OpenGSD 1.20.0; GSD reviewer and verifier roles resolved to `gpt-6-luna` xhigh; the primary runtime-reported model ID was unavailable. The deep review covered 40 Phase 11 implementation/integration files and found no blockers plus one metadata warning. Commit `961964a` corrected both CHD-off/on descriptions and added a regression check. Goal-backward verification passed 45/45 truths; the automation-only UAT record passed 4/4 with no owner or hardware observations claimed. The GSD completion transition marked Phase 11 complete and validated ECM-01/CCD-01. The phase still needs the authorized ship, hosted Main/Nightly/Release evidence, merge and automatic release before Phase 12 planning.

The exact code head `961964a` passed `cmake --workflow --preset ci` with exit 0: 670/670 CI tests in 105.02 seconds and 7/7 release checks in 1.46 seconds. The fixed repository-check set passed at exactly 3,901/3,901 lines. The CMake configure and focused libretro metadata positive/negative tests also passed. No local Act container or campaign ran for this exact head; the earlier `act schedule --dryrun` route check is recorded in Plan 11-12. Hosted Main, Nightly and Release results remain unknown until the PR runs.

CI efficiency review: the full suite had no duplicate broad test run; package checks were a distinct post-package gate. The slowest measured cases were `policy.quality_metrics` at 65.82 seconds and `libretro.host` at 59.36 seconds. The former preserves many isolated production-gate scenarios; no safe in-scope assertion reduction was found. A measured, quality-preserving optimization seed is `.planning/seeds/SEED-261122-vlf-reduce-quality-gate-test-runtime-without-losing-coverage.md`.

### Phase 11 ship follow-up and hosted failure correction (2026-11-22)

PR #36's first exact head was `4e381c184f3db889395743d7853a26e0983c7af8`. [Nightly run 45070735998](https://github.com/szTheory/schwaystation/actions/runs/45070735998) passed the four benchmark jobs and full-vector job, then failed its ECM/CCD fixture-closure step because CTest's `disc.image.legacy_preflight` executable had not been built. The fix adds `schwaystation-test-disc-legacy` to the existing fixture build and strengthens `tests/cmake/workflows.cmake` so the cheap route preflight catches its removal. [CI run 45070735988](https://github.com/szTheory/schwaystation/actions/runs/45070735988) exposed a separate shared compile issue: ASan and Linux x64/arm64 failed because strict C17 hid `symlink()` in `tests/disc/ccd.c`; a guarded `_POSIX_C_SOURCE=200809L` definition before headers fixes the POSIX declaration while Windows/WASI remain excluded. Other CI matrix jobs were canceled after these failures. The changed workflow/source files received a clean follow-up review.

The first-push hosted findings were reproduced and corrected locally before another broad hosted run. `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` exited 0; `act schedule --dryrun --workflows .github/workflows/nightly.yml --job fuzz ...` exited 0 and routed through fixture build, closure, and campaign steps (dry-run only; hosted Ubuntu 26.04 and its permissions/artifacts remain unverified). A fresh task-scoped fuzz build compiled the legacy test target plus ECM/CCD replay targets, and its exact four closure tests passed 4/4. The corrected CCD test passed the local CCD plus legacy preflight/parity selector 3/3, and GCC 15.3 in the named auto-removed container compiled `tests/disc/ccd.c` with the hosted Linux warning/error flags. The full local `cmake --workflow --preset ci` run after the workflow fix but before the final POSIX macro passed 670/670 in 82.38 seconds and separate release checks 7/7 in 1.44 seconds; build artifacts were reused (seven build actions). The final macro change then passed its targeted compile and tests. Do not claim hosted Main/Nightly/Release success until the final PR head runs.

No new Docker image or volume was created. The existing `gcc:15` image was used with container name `schwaystation-phase11-gcc15-ccd-check`; `--rm` removed the container after the compile. No broad Act run was attempted because the available Act image is Ubuntu 24.04/arm64 rather than the hosted Ubuntu 26.04 lanes.

### Phase 11 final-head failures, repair and local verification (2026-11-22)

Runtime/model: Codex; primary model ID unavailable. Whole follow-up duration was not tracked; the complete local CI workflow's CTest phase took 111.59s and the Release checks took 1.91s. The focused deep review found no issues in the new seed, timeout, WASI bound and gate-order changes.

The final pre-repair [Nightly run 45073362687](https://github.com/szTheory/schwaystation/actions/runs/45073362687) reached `fuzz.regress.chd` corpus creation and rejected its intentionally empty seed. The final pre-repair [CI run 45073362654](https://github.com/szTheory/schwaystation/actions/runs/45073362654) measured the linked CHD/ECM/CCD WASI runner at 906,778 bytes against the Phase 10 875,971-byte ceiling. All six native matrix legs then timed out at the 30-second `runner.disc.legacy` gate after one failure per lane; the test performs nine 1,200-frame fixture boots. The failed tail was canceled after useful failures were collected; no post-repair hosted result exists yet.

The local fix allows only the named `empty` seed to be written as a zero-byte corpus file, registers a corpus-writing CTest, and includes it in Nightly's closure selector before timed fuzz campaigns. The legacy runner timeout is now 60 seconds: it took 19.98s in isolation and 29.05s while the full local suite ran in parallel. `quality.json` now tracks the actual hosted 906,778-byte module. In `.github/workflows/ci.yml`, only the runner target builds before size measurement; the full WASI portable-test build still runs after a passing bound and before every portable test. The prior failed measurement job built 331 Ninja actions before discovering the old bound, so this reordering fails early without changing passing-path coverage.

Cheap preflights passed: sorted fuzz inventory, `disc_registration` CI/Nightly wiring, 3,901/3,901 counted lines, and `git diff --check`. A fresh local CHD replay build plus its corpus and ordinary replay tests passed 5/5; the corpus contained 12 files and `empty.exe` was zero bytes. The CHD-OFF `library` preset also rebuilt the replay and passed the seed-corpus CTest 1/1 without a baseline CHD fixture. `cmake --workflow --preset ci` passed 671/671 tests, packaged all three deliverables, and passed Release checks 7/7. Act's updated Nightly fuzz `--dryrun` exited 0 after its task-specific cache was placed under `/private/tmp`; it routed through the closure and campaign step without creating containers. The dry run maps hosted Ubuntu 26.04 to an Ubuntu 24.04/arm64 image and proves routing only. The actual WASI module measurement is hosted evidence; this machine did not build the WASI target.

Next: commit and push the reviewed repair to PR #36, then inspect one final-head CI and Nightly run and complete auto-merge, main CI and the automatic release before Phase 12 planning. The v2.0 roadmap still has Phases 12–16 to execute and ship after this boundary.

### Phase 11 Plan 11-15 execution (2026-11-22)

Runtime/model: Codex/OpenGSD 1.20.0; configured executor `gpt-6-luna`; primary runtime model ID unavailable. Measured
duration was 6 minutes from 2026-11-22T20:05:25Z through the plan state closeout at 20:11:54Z. The line-budget
preflight passed at 3,899/3,901; all 10 focused selectors were discovered; installed SDL2 2.32.70 was available.
The staged package and explicit `schwaystation_DIR` configured the SDL2 example, which built with ECM/CCD sources
and was not launched, tested or packaged. CHD-ON configuration and runner, libretro and two provider targets built;
the focused provider/info/install selector passed 10/10 and the compatibility feature checks passed 2/2. Local
example/CHD-ON evidence was appended without changing hosted Main, Nightly or Release status. Requirements remain
open. No full suite, hosted workflow or campaign ran. Task commit is `cbc347c`; code review and exact-head phase
verification are next.

### Phase 11 Plan 11-14 execution (2026-11-22)

Runtime/model: Codex/OpenGSD 1.20.0; configured executor `gpt-6-luna`; primary runtime model ID unavailable. Measured
duration was 13 minutes from 2026-11-22T19:48:06Z through local package verification and state closeout. The 3,899/3,901
line budget, package selector discovery and feature-row order checks passed. The CHD-OFF CI preset configured, and
`schwaystation-run` plus `schwaystation_libretro` built with both ECM and CCD provider objects present in the disc-image
archive. Libretro vendored/info claims passed 4/4; hygiene `policy.notices` passed 2/2; CPack generated library, runner
and libretro macOS arm64 Release archives; archive-content and notice checks passed 4/4. The feature-ledger schema and
both ECM/CCD feature tests passed. Local package evidence is recorded separately from unknown Main, Nightly and Release
hosted evidence; ECM-01 and CCD-01 remain open. No hosted workflow, timed campaign or broad suite ran. Task commit is
`7f31a0f`; Plan 14 summary records the evidence. Plan 15 owns the installed SDL2 example and CHD-ON consumers.

### Phase 11 Plan 11-12 execution and bounded follow-up split (2026-11-22)

Runtime/model: Codex with OpenGSD 1.20.0; configured executor `gpt-6-luna`; primary runtime model ID unavailable.
Measured elapsed time was 33m25s from 2026-11-22T19:00:32Z to 19:33:57Z. Task 1 is committed as
`ef50063`; the CHD-OFF package checks and installed-example/CHD-ON checks were not started because the remaining
concrete work would make this plan exceed the project's 30-minute split rule. No requirement is marked complete.

The 3,899/3,901 counted-line preflight passed. The positive `disc_registration` check passed, and temporary
mutations removing the legacy fixture producer and moving the loader campaign before preflight were both rejected
(exit 1). CI/Nightly actionlint passed. The all-workflow actionlint command exited 1 only on the unchanged
`release.yml` `concurrency.queue: max` warning. The explicit `act schedule --dryrun` route passed and showed the
fuzz producer, ECM/CCD source/replay preflight, and seed/campaign step without running a campaign. A fresh
ASan/UBSan configure and build of the legacy test/fixture targets passed; the focused `legacy_preflight` and
`legacy_parity` selector passed 2/2 in 0.25s after NUL-terminating the manifest buffer. This directly fixes the
pre-existing fixture harness fault recorded during Plan 11-10 and required by the new Nightly preflight.

No timed campaign, full suite, hosted CI or Docker run occurred. Plan 12 is now scoped to its one completed Task 1
and has `requirements-completed: []`. Its CHD-OFF package/evidence work is in Plan 14; installed SDL2
example/CHD-ON checks are in Plan 15. Cross-platform evidence and hosted Main/Nightly/Release checks remain with
final phase review. Plan 14 is next, followed by Plan 15; neither plan runs hosted ship checks. Phase 11 code
review and final verification remain outstanding.

### Phase 11 Plan 12 closeout and Plans 14-15 (2026-11-22)

Runtime/model: Codex GSD planner; primary model identifier unavailable. Planning duration was not reliably captured.
The split preserves the 3,901-line cap, leaves the generic scanner deferred, records Plan 12 Task 1 only, and
assigns CHD-OFF package checks to Plan 14 followed by the installed SDL2 example and CHD-ON consumers in
Plan 15. Cross-platform evidence and hosted Main/Nightly/Release checks remain with final phase review. Plan
frontmatter and structure checks passed for Plans 12, 14 and 15. The local line-budget check
reported 3,899/3,901; CTest discovery confirmed all selected package, provider, extension and install selectors.
No tests/builds, hosted workflows or campaigns were run during planning. Next: execute Plan 14; source review
and the distinct hosted Main/Nightly/Release checks remain for final phase review.

### Phase 11 Plan 11-09 execution (2026-11-22)

Runtime/model: Codex with OpenGSD 1.20.0; configured executor model `gpt-6-luna`; primary runtime model ID unavailable.
Measured duration was 28 minutes (from 2026-11-22T16:47:08Z to final closeout at 17:15 UTC). Four task commits are recorded in
`11-09-SUMMARY.md`. The inventory and fixed line-budget preflights passed at 3899/3901. The targeted host,
libretro, card-test and fixture build exited 0. The `disc.image.ccd` + `libretro.content.legacy` selector passed
5/5 in 10.04 seconds, including legacy preflight/parity, the existing Q/GetlocP vector, both source-CUE
comparisons, rejected replacement preservation and disk replacement. The two info-claim tests passed 2/2 in
0.31 seconds. Both TDD RED records passed `gsd-tools check tdd-red-evidence`; no hosted CI, Docker or broad
suite was run. ECM-01 and CCD-01 remain phase-level requirements pending remaining plans and final verification.

### Phase 11 Plan 11-10 execution (2026-11-22)

Runtime/model: Codex with OpenGSD 1.20.0; configured executor model `gpt-6-luna`; primary runtime model ID unavailable.
Measured wall duration was 1h 10m 25s from 2026-11-22T17:16:28Z to 18:26:53Z. The 3899/3901 line budget and
sorted fuzz inventory passed. The CI-preset ECM/CCD selector passed 4/4. A disposable local Ubuntu 26.04
container built both Clang 21 ASan/UBSan campaign targets, both replay targets and legacy fixtures (104 Ninja
steps, exit 0); each campaign loaded its ten named seeds and completed a bounded `-runs=1` smoke check (11 runs
per format, exit 0). No 60-second campaign, broad suite or hosted CI ran. An exploratory ASan selector
failed in the inherited `tests/disc/legacy.c:replace_manifest_field` preflight; this unrelated finding is logged
in `deferred-items.md` for Plan 11-12 triage if its Nightly lane exercises that setup. The disposable container
was removed. Task commits `12008bd` and `2aee10a` are recorded in `11-10-SUMMARY.md`. ECM-01 and CCD-01 remain
phase-level requirements pending remaining plans and final verification. Plan 11-11 is next.

### Phase 11 Plan 11-11 execution (2026-11-22)

Runtime/model: Codex with OpenGSD 1.20.0; configured executor `gpt-6-luna` xhigh; primary runtime model ID was not
exposed. Measured duration was 23m27s from 2026-11-22T18:30:57Z through final focused verification at
18:54:24Z. The sorted disc inventory and fixed line-budget preflights passed at 3,899/3,901. Independent source
preflight/parity confirmed ECM 64/64 and CCD 222/222 sectors, with 21,312 SUB bytes. The final bounded guest
selector passed 12/12 in 13.36s, including all seven ECM/CCD guest cases and their fixture gates. The protected
feature selector passed 2/2 in 0.53s, and generated docs, ledger schema/order and whitespace checks passed.
Two stale ignored test executables were rebuilt by target after their focused checks identified the stale output.
Task commits `2193ce9`, `53fc4e4` and `7226071` are recorded in `11-11-SUMMARY.md`. No broad suite, hosted CI,
Docker or hardware run was made. The feature ledger leaves hosted CI, package and fuzz evidence unknown until
Plan 12; phase requirements ECM-01 and CCD-01 remain open pending Plan 12, code review and final verification.
Plan 12 is next.

### Phase 11 Plan 11-02 budget disposition (2026-11-22)

Runtime/model: Codex primary model identifier unavailable; OpenGSD 1.20.0. Read-only audits and plan checking used
`gpt-6-luna` xhigh. This planning and closeout step took about 27 minutes; its exact start time was not retained.
Two independent audits found the complete metadata-only Nightly controls could not fit the fixed cap: 3898/3901
current lines plus 30 route-control lines would reach 3928 before classifier/workflow changes, with no safe,
measured reduction identified. D-12 supersedes only the shortcut portion of D-08; todo 051 remains pending. The
current full-Nightly fallback, all tests and lanes, and Main/Release separation are unchanged. Plan-structure
validation passed, the line-budget check exited 0 at 3898, and `ci.classify_policy` passed 1/1 in 0.15 seconds.
No hosted run or workflow change was made. Plan 02 is closed as a decision record, not as an implemented CI
optimization; ECM-01 and CCD-01 remain open. Next: execute Plan 05.

### Phase 11 Plan 11-13 execution (2026-11-22)

Runtime/model: OpenGSD 1.20.0 GSD executor, `gpt-6-luna` xhigh. Plan 13 took 11 minutes 20 seconds, from
2026-11-22T13:18:19Z to 13:29:39Z, and changed two files. RED `d2648c2` and GREEN `6696aec` are committed.
The 3,898/3,901 line-budget preflight, sorted inventory and selector discovery passed. CI preset configure and
focused CCD target build passed; the red JUnit evidence checker returned `RED_EVIDENCE_OK` for the dangling-SUB
canonicalization status assertion. The rebuilt and committed CCD selector passed 1/1, including optional SUB,
path escape and active-reader preservation cases. No broad suite, Windows/WASI hosted lane, Act job or hosted CI
was run. Plan 13 and Plan 05 are complete. Plan 02 is closed as an evidence-based D-08 deferral; D-12 leaves the
full Nightly route and all cases intact. Plan 06 is next after its Plan 05 and Plan 13 dependencies.

### Phase 11 Plan 11-04 execution and split (2026-11-22)

Runtime/model: OpenGSD 1.20.0; executor `gpt-6-luna` (xhigh), planning follow-up `gpt-6-luna` (high). Plan 11-04 Task 1 took about 33 minutes; its exact start time was not retained. RED `5780f40` and implementation `043403f` are committed. Post-commit CI-preset configure and target build exited 0; the focused CCD, CUE-rejection and playlist selector passed 3/3. No broad suite or hosted CI was run. The unstarted companion confinement, SUB/missing-file, IO and failure-atomic scope was moved to Plan 13 so the verified work remains intact and Plan 11-04 stays within the 30-minute plan rule. Plan 11-06 depends on Plans 11-05 and 11-13 and moves to Wave 5. Plan 11-02 is closed as a D-08 deferral under D-12; the full Nightly route and all cases remain unchanged. The independent plan-split review passed, and `11-04-SUMMARY.md` records the verified slice. Next: complete dependency-ready Plan 06. Planning split duration was not measured.

### Phase 11 Plan 11-05 execution (2026-11-22)

Runtime/model: OpenGSD 1.20.0 GSD executor, `gpt-6-luna` xhigh. Plan 05 took 29 minutes, from
2026-11-22T14:05:13Z to 14:34:10Z, and changed five files. Task commits are `8afd490` (modeled RED),
`10c9bd7` (modeled GREEN), `3b23e90` (CUE RED), and `90cc6d1` (CUE GREEN). The 3,898/3,901 line-budget,
sorted inventory and selector preflights passed. Both intentional RED runs exited 8 and passed the RED evidence
classifier; configure and focused builds passed. The final ECM/CUE/playlist selector passed 9/9, then the ECM
selector passed 1/1 after duplicate-source coverage. No hosted CI or broad suite ran. ECM type 1–3 records now
match independent trailer vectors, checkpoint reads and the full decoded EDC. Raw MODE2/2352 and AUDIO CUE
sources use decoded extents and preserve the raw CUE TOC and boundary reads; cooked ECM CUE sources are rejected.
ECM-01 remains incomplete until phase-level verification. Plan 06 is dependency-ready; no user action is needed.

### Phase 11 Plan 11-06 execution (2026-11-22)

Runtime/model: OpenGSD 1.20.0 Codex executor, configured `gpt-6-luna` xhigh; the primary runtime model
identifier is not exposed. Plan 06 took about 27 minutes overall, from 2026-11-22T14:36:57Z through
2026-11-22T15:03:34Z; implementation took about 19 minutes and changed two source/test files. Task commits are `b13e65f` (track-boundary RED),
`37f3efe` (track/index GREEN), `0e55424` (SUB/Q RED), and `065b064` (SUB transpose GREEN). The
3,898/3,901 line-budget preflight, sorted disc inventory, exact CCD selector discovery and diff checks
passed. Both planned RED CTest runs exited 8 on their expected behavior assertions and their JUnit
records passed `tdd-red-evidence`; the final CI-preset CCD target build and selector passed, 1/1.
Missing/empty SUB remains zero-filled, exact-size SUB transposes into callback P-W bits, and malformed
track indexes/entries reject. No broad suite, Act job, or hosted CI run was needed. The parser/index
reconciliation and independent byte vectors were the main work; configure, focused build, and CTest
completed in seconds. CCD-01 remains open until Phase 11 verification. Plan 07 is next and is
dependency-ready after Plans 05 and 06.

### Phase 11 Plan 11-07 execution (2026-11-22)

Runtime/model: OpenGSD 1.20.0 Codex executor, configured `gpt-6-luna` xhigh; the primary runtime model ID
was not exposed. Plan 07 took 48 minutes 17 seconds, from 2026-11-22T15:07:34Z to 15:55:51Z, and changed
eight files. Task commits are `64668c1` (fixture generators), `1eb224d` (preflight/parity tests), `cbefda7`
(IMG-backed identity CUE producer), and `14bd092` (unused-value cleanup and digest update). The sorted
inventory, 3,898/3,901 line cap, generator provenance hash, configure, fixture/test target builds, and
focused legacy preflight/parity selector passed; CTest passed 2/2 in 0.14 seconds on the final producer.
All five negative controls remain. The task's earlier machine-classified RED was semantically invalid because
the source identity lacked SUB while the CCD image included generated SUB; the test oracle was corrected and
the summary does not claim canonical RED evidence. No guest, broad suite, Act job, hosted CI or Docker ran.
The identity-oracle diagnosis was the main bottleneck and put the plan over 30 minutes; the concrete remaining
work did not warrant a split. ECM-01 and CCD-01 remain open pending phase-level verification. Plan 08 is next.

### Phase 11 Plan 11-08 execution (2026-11-22)

Runtime/model: OpenGSD 1.20.0 Codex GSD executor, configured `gpt-6-luna` xhigh; primary runtime model ID was
not exposed. Plan 08 took 33 minutes 32 seconds through final targeted verification, from 2026-11-22T16:00:47Z
to 16:34:19Z, and changed four files. Task commits are `23d1c48`, `c7126f8`, `2123375`, and `cdcd1b3`.
Inventory, fixture extents/counts, fixture edges, and the 3,898/3,901 cap passed. Configure and focused runner
build passed; the final legacy and malformed-media selector passed 4/4 in 19.30 seconds, and diff checks passed.
The direct ECM RED exited 8 and classified as `RED_EVIDENCE_OK`. Expanded checks exposed an ECM-CUE routing gap
and a CCD save-identity oracle that compared different SUB bytes; the route and test comparator were corrected.
The 30-minute target was exceeded during diagnosis, with no remaining implementation slice to split. PBP and CHD
refusal controls remain. No broad suite, Act execution, Docker, or hosted CI ran. Phase-wide ECM-01/CCD-01 remain
open pending review and final verification; dependency-ready Plan 09 is next.

### Phase 11 Plan 11-01 execution (2026-11-22)

Runtime/model: OpenGSD 1.20.0 in the Codex GSD executor; configured model `gpt-6-luna`. The primary
runtime model identifier is not exposed. Three tasks took 37 minutes (started 2026-11-22T01:34:33Z,
completed 2026-11-22T02:11:22Z), producing four task commits and changing ten source, CMake, and test
files. The route controls pass with 3898/3901 counted lines. Commands exited 0 for the route check,
the classifier test, the host target build, and the final focused ECM/CCD configure/build/CTest run
(2/2 tests). The planned RED command exited 8 on the truncated ECM count assertion; the RED evidence
classifier exited 0 with `RED_EVIDENCE_OK`. The committed-state tracer rerun exited 0. Configure,
build, and the two focused tests completed in under one second; evidence/commit bookkeeping was the
longer part of the plan. No hosted CI run was started. The intentionally incomplete positive ECM/CCD
provider seams are listed in `.planning/WINDOWS.md` and owned by Plans 03/04. ECM-01 and CCD-01 remain
incomplete until phase-level verification. Plan 02 is closed as a D-08 deferral under D-12; Plan 06 is next.

### Phase 11 Plan 11-03 execution (2026-11-22)

Runtime/model: OpenGSD 1.20.0 in the Codex GSD executor; configured model `gpt-6-luna`. The primary
runtime model identifier is not exposed. Two tasks took 36 minutes (started 2026-11-22T02:37:28Z,
completed 2026-11-22T03:13:26Z), changing five host and test files. Task commits are `9c00a79` and
`99fbc64`; each followed its recorded TDD RED evidence and GREEN checks, with test and implementation
changes together in the task commit. No history rewrite was made. The committed-state checks exited 0:
CI preset configure, focused ECM/image target build, focused ECM/CUE-rejection/playlist CTest (3/3),
then the ECM target build and ECM bounds CTest (1/1). Inventory ordering, focused test discovery,
`git diff --check`, and the 3898/3901 line budget also exited 0. The two planned RED CTest commands
exited 8 on their expected assertions and both `tdd-red-evidence` checks exited 0. No hosted CI run
was started. Literal type-0 direct and M3U ECM reading is implemented; ECM-01 remains incomplete
pending phase scope and final verification. Plan 02 was closed as an evidence-based D-08 deferral;
Plan 04 continued independently and Plans 05 and 13 are now complete.

### Phase 11 planning (2026-11-22)

Runtime/model: Codex planner, inherited model identifier unavailable; OpenGSD 1.20.0. A dedicated
gsd-phase-researcher established primary ECM byte-stream mapping and the independent CloneCD SUB vector.
Twelve plans cover both requirements and every locked decision; wave ownership is recorded in
`11-PLAN-COVERAGE.md`. All plan frontmatter/structure validators passed. The one-shot balanced-tag,
shell-syntax, same-wave ownership, command-path and failure-direction checks passed without findings;
the command-path probe reports CMake forms as not applicable rather than claiming target resolution.
The 3901-line cap, generic SPDX deferral, separate Main/Release checks and cheap preflights are preserved.
No source implementation or hosted run was performed. The independent checker passed after adding explicit
CHD-ON consumer and installed-SDL2 example verification. Decision and post-planning coverage gates pass;
the unchanged counted source was exactly 3901/3901 lines. Planning duration was not reliably measured.
Plan 11-01 has since completed; Plan 02 is closed as a D-08 deferral under D-12. Plan 06 is next.

### Phase 11 discussion (2026-11-22)

Runtime/model: Codex session (primary model ID unavailable), OpenGSD 1.20.0. Read-only fan-out completed with
`gsd-assumptions-analyzer` (`gpt-6.1-sol`, high), `gsd-domain-researcher` (`gpt-6-luna`, xhigh), and
`gsd-advisor-researcher` (`gpt-6-luna`, xhigh). Their independent notes were reconciled and adversarially
reviewed; no source code or tests changed. The discussion pins the ECM author specification as SCM.32, records
the CloneCD confidence boundary, and resolves the source/callback SUB conversion with an independent known-answer
test. The owner selected the unchanged 3,901-line cap and deferred generic SPDX scanning. Phase 11 is ready for
planning. The narrow metadata-only Nightly route is an implementation item grounded in Phase 10's measured
419-runner-second false positive; all unknown and mixed changes stay on the full route.

### Phase 10 release closeout (2026-11-22)

Runtime/model: Codex session (primary model ID unavailable), OpenGSD 1.20.0. The interval from the saved STATE
checkpoint at 2026-11-21T20:36:14Z to release and current-main CI closeout at 2026-11-22T00:50:02Z was about
4h14m wall, including hosted waits; active time was not measured. PR #34 merged at `9bf39aa`; release PR #35
merged and published `v0.10.0` at 00:09:45Z. The release is Latest with all 19 expected archives/checksum
assets; asset, signature and download verification passed, and `install-line-latest` passed. Release workflow
[44961621930](https://github.com/szTheory/schwaystation/actions/runs/44961621930) passed on
`636ac89d4add3192520084a73fa61295a8422bbc`. Current-main CI
[44961621903](https://github.com/szTheory/schwaystation/actions/runs/44961621903) passed on that same SHA;
it took 85m51s wall and about 9,200 summed job-seconds, with BE32/QEMU at 5,095 seconds (84m55s) as the
critical path. Release CI used about 6,150 job-seconds (102.5 runner-minutes) for the full artifact matrix; its
BE32 lane took 2,520 seconds (42m). Dollar billing data was not exposed.

The release PR's metadata-only CI route correctly skipped the heavy matrix. Its Nightly classifier did not
recognize the exact generated release-metadata files and launched the full workload; the run was cancelled
after 419 elapsed job-seconds (about 7 runner-minutes). A cheap local invocation reproduced
`nightly_full=true` for `.release-please-manifest.json`, `CHANGELOG.md`, and `version.txt`. Add a narrow
classifier/test change to the Phase 11 plan so only this complete exact metadata set skips PR Nightly; unknown,
mixed and incomplete lists must retain the full route.

The main run on superseded ancestor `9bf39aa` was cancelled after `636ac89` advanced main with only the three
generated release metadata files and full CI plus Release CI were running on `636ac89`. The cancelled run had
already consumed 6,634 elapsed job-seconds (110.6 runner-minutes); this is accrued work, not the counterfactual
savings. Keep both full current-SHA main CI and Release CI: the completed artifact-reuse review found their 18
product archives differ, so reusing one matrix would weaken release verification. No coverage or test cases were
removed. The standing CI/CD efficiency guidance is already discoverable in `AGENTS.md`,
`.planning/AUTONOMOUS.md`, and `docs/maintaining.md`.

### Phase 10 verification (2026-11-21)

Runtime/model: GSD verifier `gpt-6-luna` xhigh. Goal-backward verification passed 2/2 roadmap truths; implementation artifacts, wiring, data flow, requirements coverage, and all 13 plan artifact/key-link audits passed. Context decision coverage was 12/12. The local final workflow evidence already recorded above passed 655 standard tests and 7 release checks; verification did not rerun tests. The verifier corrected four stale Plan 10-09 references from `libretro/schwaystation_libretro.c` to `libretro/libretro.c` and confirmed that plan's artifact/key-link audit passed. Report: `.planning/phases/10-pbp-disc-images/10-VERIFICATION.md`; `verification.status` returned `passed` (exit 0).

The pre-repair exact-head hosted attempt failed and was cancelled after exposing fixable CI issues. The WASI producer measured the linked Phase 10 module at 875,971 bytes on run 44924014258; `quality.json` records that value. Exact-head CI 44947489771 and Nightly 44947489816 passed. Their critical path and runner-time review is recorded in `10-CI-EFFICIENCY.md`; merge and the automatic release remain mandatory before Phase 11.

### Phase 10 Plan 10-13 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; executor `gpt-6-luna` xhigh; primary model ID unavailable. The wall
interval from the prior saved state at 15:15:19Z through the closeout at 15:53:05Z was about 38 minutes; active
time was not measured. The reviewed Phase 10 test correction passed its focused compatibility selector (2/2,
0.07s). The final `cmake --workflow --preset ci` passed all 655 regular tests plus 7 release checks in 78s;
its CTest critical path was 75.65s and the release checks took 1.35s. The local build was incremental.

The initial full run exposed one stale compatibility expectation for PBP, corrected in `e9ce168` and independently
reviewed before the passing rerun. Cheap inventories, workflow pin/timeout and wiring checks, and the 3,900/3,901
line-budget preflight passed. The Nightly route dry-run passed. Prior Phase 9 measurements identify BE32/QEMU as
the historical hosted critical-path candidate. The first Phase 10 hosted CI/Nightly attempt is recorded in
`10-CI-EFFICIENCY.md`; it was cancelled after failures. The actual WASI module measured 875,971 bytes and the
quality bound now tracks it. Final exact-head CI 44947489771 and Nightly 44947489816 passed. CI took 63m30s
wall/8,250 summed runner-seconds with BE32 at 62m18s; Nightly took 9m43s wall/908 summed runner-seconds. The
successful WASI comparator confirmed the recorded bound but did not print the new exact measurement. No safe
coverage-preserving workflow optimization was supported by the run-to-run timing; see `10-CI-EFFICIENCY.md`.
The phase PR and automatic release remain before Phase 11.

### Phase 10 ship-fix follow-up (2026-11-21)

Runtime/model: Codex GPT-6 session; independent `gsd-code-reviewer` used `gpt-6-luna` xhigh. The previous STATE
checkpoint was 16:20:53Z; closeout is at 16:55Z (about 34 minutes wall, including tool waits; active time was
not measured). The cancelled run failed the hygiene classifier assertion, WASI size bound, BE32 pointer cast,
Windows `fopen` warning, and protected-base sparse-checkout dependency. These were fixed in one batch; the missing
`date_days.cmake` input was added to the CI sparse checkout and its wiring is now checked. Focused classifier,
PBP, workflow-closure, full-base ratchet, quality and line-budget preflights passed; the 3,901-line cap is exactly
full. One complete local CMake workflow passed 655 standard tests and 7 release checks in 78.09s CTest time.
`act pull_request --dryrun` passed with a writable `/private/tmp` action cache; it establishes routing only. The
old hosted CI/Nightly tails were cancelled after diagnosis; summed elapsed job time was 26.5 CI runner-minutes
and 9.7 Nightly runner-minutes. No dollar billing estimate is available. No local BE32 toolchain/QEMU or WASI
sysroot is installed, so those hosted gates remain required. The correction diff had a clean independent review.
Continue with one repaired-head PR/Nightly attempt, inspect live and final job timing, then merge/release only
after all required lanes pass.

### Phase 10 ship-fix follow-up 2 (2026-11-21)

Runtime/model: Codex GPT-6; independent code review dispatched to `gsd-code-reviewer` gpt-6-luna xhigh.
The previous state checkpoint was 16:55Z; this repair reached local verification at 17:23Z (28 minutes wall,
including CI and tool waits; active time was not measured). PR CI 44927726830 failed in the SDL2 example because
its source-built host image layer omitted the PBP provider/inflater. Nightly 44927726826 failed because the full
fuzz build preset did not include the PBP fixture; the regression seed check occurred after earlier 60-second
campaigns. The obsolete CI tail was cancelled after its deterministic link failure was reproduced and fixed.
The Nightly run had already ended.

The example now links the PBP reader and inflater; its local configure/build passed. Nightly now builds the PBP
fixture and validates all seeds before timed campaigns. The local replay preflight wrote the expected 42 EXE
seeds and 5/5/7/15/12/6/14 subsystem seeds. Workflow closure, actionlint, and the 3,901/3,901 line budget pass;
`act pull_request --dryrun` parsed the Nightly route but did not execute it. The two attempts consumed 79.22 CI
and 14.17 Nightly summed runner-minutes; no Actions dollar data was available. Keep every platform, case,
campaign and assertion. Independent review found one gap in the seed-order assertion; the next subsection records
the correction and post-review checks before the exact-head CI and Nightly attempt.

### Phase 10 seed-order assertion follow-up (2026-11-21)

Runtime/model: Codex GPT-6 session; independent `gsd-code-reviewer` `gpt-6-luna` xhigh. The interval from the
17:23Z checkpoint through the 17:39Z local checks was about 16 minutes wall; active time was not measured. Review
found that the Nightly regression assertion could accept a campaign inside the replay seed loop: the loader
fragment preceded seed validation, and the route's literal `||` was interpreted as regex alternation. The check
now treats the operator literally and requires the loader command after loop closure. The real workflow check
passes, and a temporary inside-the-loop campaign fixture fails as expected. The gate remains 3,901/3,901 lines;
actionlint and diff checks pass. The earlier CI/Nightly failures remain diagnosed and linked above. Commit this
reviewed fix, run one consolidated exact-head CI/Nightly attempt, inspect its timing/cost while active and after
completion, then release before Phase 11.

### Phase 10 generated-media graph follow-up (2026-11-21)

Runtime/model: Codex GPT-6 session; independent `gsd-code-reviewer` `gpt-6-luna` xhigh. From the 17:42Z checkpoint
through the 18:02Z preflight was about 20 minutes wall; active time was not measured. Exact-head Nightly
44931682921 failed after 61 seconds in the fuzz fixture build: the PBP source image named `xa.xa` and `movie.str`
as inputs, but the media-generation rule declared only `media.sha256`. A fresh local CMake configure with
`SCHWAYSTATION_DISC_MEDIA_DIR` unset reproduced the clean-host build graph. The fix declares all three generated
media files as byproducts. Ninja now finds the producer for each file; the route check and a negative misplaced-
byproduct fixture pass, and the gate remains 3,901/3,901 lines.

The paired CI 44931682851 had all six native builds and WASI pass, plus the listed quick gates; it was cancelled
after the Nightly failure while BE32/ASan and dependent checks were still running. The partial CI run used 75
runner-minutes; Nightly used 6.28. These are elapsed job sums, not billed dollars. Review found the first version
of the new static guard could associate byproducts with an unrelated command; it now extracts only the command
whose output is `media.sha256`, and a negative control rejects declarations attached elsewhere. Independent
re-review of this narrowed guard was clean. Commit the fix, run one exact-head CI/Nightly pair, and inspect the
live and final timing before release and Phase 11.

### Phase 10 cross-platform PBP hash follow-up (2026-11-21)

Runtime/model: Codex GPT-6 session (exact primary model ID not exposed); independent `gsd-code-reviewer`
`gpt-6-luna` xhigh. The exact-head [CI run 44940420226](https://github.com/szTheory/schwaystation/actions/runs/44940420226)
failed only at hash equality: Windows x64's generated PBP differed from the other seven platform outputs, while
the normalized CUE and all other 112 records matched. The paired [Nightly run 44940420161](https://github.com/szTheory/schwaystation/actions/runs/44940420161)
passed its full route. CI used 8,481 summed runner-seconds over 63m32s, with BE32 at 3,744s; Nightly used 860
runner-seconds over 10m20s. The CI efficiency artifact records that no coverage reduction is supported.

The fixture generator now emits fixed-Huffman raw DEFLATE using deterministic distance-one byte runs instead of
host zlib compression, retaining zlib only as an independent decoder check. Its 64-sector output is 1,132,683
bytes under the 1,572,864-byte cap; all four PBP blocks remain compressed. `python3 -m py_compile tools/gen_pbp.py`
and the synthetic 1,000 run-length/decode checks passed. `cmake --build --preset ci --target
schwaystation-test-disc-pbp schwaystation-pbp-fixtures` passed. The ten `disc.image.pbp` provider tests passed,
and the nine guest/fuzz selector tests passed, including PBP boot/read/FMV and fuzz replay. The separate runner
and libretro PBP integration tests passed as well. The line-budget, inventory sort, workflow closure and diff
preflights passed at 3,901/3,901.
Independent standard review found no blockers or warnings. No workflow changed, so Act was not repeated; local
tests covered the generator and consumers, while only hosted platforms can establish cross-platform hash parity.
The repaired-head CI/Nightly pair and Phase 10 release remain pending; Phase 11 has not started.

### Phase 10 discussion (2026-11-21)

Runtime/model: Codex session; primary model ID is not exposed. OpenGSD CLI 1.20.0; domain, security/architecture,
CI/DevOps, and pattern-map advisors used gpt-6-luna xhigh. The elapsed wall interval from the previous saved
STATE checkpoint at 01:24:41Z through discussion closeout at 05:04:26Z was 3h39m; active time was not measured.
Phase 9's release and exact-main CI completed green before this discussion. The PBP boundary is limited to
fixture-proven unencrypted PSISO layouts; malformed structures fail as bad media, unknown/encrypted variants
remain unsupported, and the provider stays host-only with no system zlib dependency. Folded TODOs 006 and 051;
retained the existing 3,901-line ceiling. The four pinned miniz files inspected contain no SPDX identifier,
so TODO 005 remains deferred per the owner's choice; TODO 002's and TODO 003's recorded first triggers predate
this phase. The stale Unlicense wording in upstream `miniz.h` is excluded; use the three modular MIT files,
project-owned shim, generated export header, and private symbol aliases. The cheap line-budget preflight passed
at 3,901/3,901; no build, broad suite, `act`, or hosted workflow ran during discussion. The original context
commit is `a66a49b`; the corrected scope and research are being committed before planning continues.

### Phase 10 plan and independent review (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; primary model ID unavailable; planner model ID not preserved; independent
`gsd-plan-checker` gpt-6-luna xhigh and source researcher gpt-6-luna xhigh. Wall time from the discussion
checkpoint at 05:04:26Z through final plan review at about 05:43Z was roughly 39 minutes; active time is not
reliably attributable. The checker first found unresolved Q1–Q4 in the research and an advisory 10-file Plan
10-02 footprint. The research now records each resolved choice, including all four emitted inflater aliases
under `MINIZ_NO_MALLOC`; the two Plan 10-02 tasks each own five files, so no evidence supported splitting it.
The focused independent re-review passed. All 13 plan structures, 11/11 decisions, 27/27 failure-direction
checks, sorted inventories, `git diff --check`, and the 3,901-line ceiling passed. The roadmap already contains
its wave map and cross-cutting constraints; the annotation helper skipped its existing em-dash checklist syntax.
No implementation build, suite, `act`, or hosted CI ran during planning. Plan 10-01 is next.

### Phase 10 Plan 10-01 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna; primary model ID and reasoning effort were not exposed.
Measured wall time was 24m53s from 05:47:33Z through final verification at 06:12:26Z; active time was not
measured. The quality and triage control drivers were reduced by 37 lines with their cases preserved, leaving
the repository gate at 3,864/3,901. Both tracer runs passed all three control selectors and the line-budget
check; `policy.quality_metrics` took 31.10–33.39s. The CHD-OFF host image target built, the sorted 30-row
inventory passed, and `disc.image.pbp_headers` passed 1/1. Branch-wide `git diff --check` initially caught an
extra blank line in each Phase 10 plan; commit `875f147` removed only those trailing blank lines, after which
the full range check passed. No workflow changed, so no `act` or hosted CI run applied. This proves only the
host-only outer-envelope boundary; PBP-01 remains phase-wide pending. Task commits: `2df74ad`, `7349f35`,
`9dfccfb`; planning-ending correction: `875f147`. Plan 10-02 followed.

### Phase 10 Plan 10-02 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; executor `gpt-6-luna` xhigh. The wall interval from the previous saved
STATE checkpoint at 06:16:36Z through this plan's closeout was about 1h05m; active work time was not measured.
The three exact pinned modular miniz files, the project shim and the MIT grant are recorded in provenance and
notices; the upstream `miniz.h` remains excluded. Cheap pin/hash/license, notice, mutation, selector, diff and
budget checks passed before the build. The standalone target first failed under `-Werror` on undefined upstream
portability macros and conversion warnings; setting the three research-verified macros and suppressing only
conversion warnings for the unchanged vendor C file made the focused build pass. The `library` preset confirmed
CHD and its tool OFF, and the image target compiled with the standalone inflater. The symbol audit found all four
private definitions and no unresolved tinfl, allocator or zlib symbols. The vendored CTest selectors passed 2/2
in 0.11s; direct notice and digest scripts passed, including both expected negative mutations. Gate total is
3,869/3,901. No workflow changed, so no `act` or hosted run applied. A `cmake --preset ci -LAH` cache inspection
reconfigured build/ci and restored its preset defaults; use `cmake -LA -N build/ci` for read-only inspection.
Task commits are `8e64f5c` and `07fd276`; summary is `.planning/phases/10-pbp-disc-images/10-02-SUMMARY.md`.
Plan 10-11 is the other ready Wave 2 item and is next under the sequential executor.

### Phase 10 Plan 10-11 execution (2026-11-21)

Runtime/model: Codex session; primary model ID is not exposed. Wall interval from the prior STATE checkpoint at
07:26Z through closeout at 09:31Z was about 2h05m; active time was not measured. The Nightly classifier keeps
existing CI outputs intact, routes exact PBP-only lists to fuzz, skips only complete docs/planning-only lists,
and sends uncertain changes to full Nightly. A focused review found the missing failed-classifier fallback and
recommended using 100-file REST pages; both were implemented and re-reviewed. Route tests, the registered
classifier CTest (1/1), workflow wiring, action pins/timeouts, actionlint, `act` PR dry-run, diff checks and the
3,900/3,901 gate passed. The workflow_dispatch act attempt failed locally at artifact upload because no
`ACTIONS_RUNTIME_TOKEN` is available; no hosted run or cost comparison was made. TODO 051 stays pending until
the exact-head hosted run in Plan 10-13. Task commits: `bc4e922`, `9d74ab0`; summary:
`.planning/phases/10-pbp-disc-images/10-11-SUMMARY.md`. Plan 10-03 was completed before this checkpoint; Plan 10-04 followed.

### Phase 10 Plan 10-04 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; primary model ID is not exposed. The wall interval from the saved
09:49:43Z STATE checkpoint through closeout at 11:28:58Z was about 1h39m and includes inherited idle time;
active time in the resumed segment was not separately measured. Cheap inventory, whitespace, Python syntax and
3,900/3,901 budget checks passed before configure/build. The deterministic generator produced identical
1,124,404-byte PBP output from 64 owned sectors in four blocks. The focused build passed, and fixture preflight
plus all negative controls passed 2/2 in 0.04s. The focused checks were not the source of the inherited pause;
no process was interrupted. No workflow changed, so `act` and hosted CI were not applicable. Task commits are
`68f8173` and `54679e1`; the plan summary is `.planning/phases/10-pbp-disc-images/10-04-SUMMARY.md`.
Plan 10-05 is next.

### Phase 10 Plan 10-05 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; executor `gpt-6-luna` xhigh; primary model ID is not exposed. The wall
interval from the prior saved STATE checkpoint at 11:28:58Z through closeout at 12:02:59Z was about 34 minutes;
active time was not separately measured. Cheap inventory, diff and repository-budget checks passed before the
focused build. The metadata parser maps the fixture's absolute `00:02:00` start through the 150-frame lead-in
and checks cumulative compressed extents. An initial `-Werror`, legacy test header and maximum-BCD test
expectation each surfaced at focused checks and were corrected. All six PBP selectors passed 6/6 in 0.03s.
Valid BCD MSF cannot encode a logical image over 1 GiB; the maximum legal table and oversized file-size guard
are tested separately. The gate remains 3,900/3,901. No workflow changed, so `act` and hosted CI were not
applicable. Task commits: `5db27a6`, `826cb13`; summary:
`.planning/phases/10-pbp-disc-images/10-05-SUMMARY.md`. Plan 10-06 is next.

### Phase 10 Plan 10-06 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; executor `gpt-6-luna` xhigh; orchestration model ID and effort were not
exposed. The measured wall interval from the prior STATE checkpoint at 12:02:59Z through root closeout at
12:27:39Z was 24m40s; active executor time was not separately measured. Root review caught a missing verbatim
0x9300-byte block path; it was implemented and tested before acceptance. Root also corrected the raw fixture's
PSAR length metadata. Sorted inventory, diff, selector and 3,900/3,901 line-budget checks passed before the
focused rebuild. The raw selector plus fixture preflight passed 2/2 in 0.02s. The executor's PBP raw/metadata/
bounds/parity selection passed 5/5 in 0.02s, and open/boundary/parity plus playlist passed 10/10. A malformed
compressed block is rejected before reader publication. The measured open/scan process time was 0.002783s.
No workflow changed, so `act` and hosted CI did not apply. Task commit: `83d7bf0`; Plan 10-06 summary is
`.planning/phases/10-pbp-disc-images/10-06-SUMMARY.md`. Plan 10-07 completed at the 13:16Z checkpoint; Plan 10-08 followed.

### Phase 10 Plan 10-08 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.20.0; executor `gpt-6-luna`; reasoning effort was not exposed. The wall interval
from the saved STATE checkpoint at 13:16Z through closeout was about 24 minutes; active time was not measured.
Runner and disc inventories, diff checks, selector wiring, fixture preflight and the 3,900/3,901 gate passed
before the focused configure and run. The PBP runner/probe check, source-CUE parity, M3U compatibility and docs
budget passed 5/5 in 4.85s. `runner.disc.pbp` took 4.83s; PBP fixture preflight and provider parity each took
about 0.01s. The first mailbox-stop assertion reached the fixture's 1,200-frame cap; fixed-frame PBP and source
CUE runs both exited 0 and produced the same hash, so the runner test now checks that bounded result. A PBP
playlist guard keeps M3U membership at its documented ISO/CUE scope; broken-windows entry 15 is fixed. No
workflow changed, so `act` and hosted CI did not apply. Task commits are `b8f43b1` and `b5b0076`; summary:
`.planning/phases/10-pbp-disc-images/10-08-SUMMARY.md`. PBP-01 remains phase-wide pending; Plan 10-09 is next.

### Phase 10 Plan 10-09 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.21.0; executor `gpt-6-luna` xhigh. Work began at 13:45:05Z; wall time through
closeout was about 25 minutes, with active time not separately measured. Sorted libretro inventory, fixture
preflight, selector wiring and the 3,900/3,901 line-budget check passed before the focused build. The PBP/CUE
libretro host test passed with both fixture prerequisites in 6.00s; generated metadata checks passed 2/2 in
both CHD-OFF and CHD-ON configurations. The 1,200-frame PBP/CUE host comparison is the local critical path;
its measured host test took 5.97s. A bounded test timeout of 60s keeps over 10x allowance for slower CI hosts.
The provider exposes no serializable disc identity, so failure-atomic state evidence compares exact RAM and
card bytes alongside the frame/audio traces and active path. No workflow changed, so `act` and hosted CI did
not apply. Task commits are `47d233e` and `d354bfe`; summary: `.planning/phases/10-pbp-disc-images/10-09-SUMMARY.md`.
PBP-01 remains phase-wide pending; Plan 10-10 is next.

### Phase 10 Plan 10-10 execution (2026-11-21)

Runtime/model: Codex/OpenGSD 1.21.0; primary GPT-6 model ID was not exposed. The wall interval from the saved
STATE checkpoint at 14:09:41Z through focused verification was about 19 minutes; active time was not measured.
Pin/digest, notice, sorted inventories, selector wiring and the 3,900/3,901 line budget passed before packaging.
CHD-OFF PBP, runner and libretro checks passed 7/7; CHD-ON PBP and original CHD parity plus package checks
passed 9/9. All three packages in both configurations retained the exact notices and correct .info claims.
The arm64 native libretro bundle measured 359,952 bytes OFF and 612,128 bytes ON; these are not WASI evidence.
The ci preset excluded release-labeled archive tests, so the existing ci-release selector ran all four cases.
A stale ON-built helper initially failed the OFF host test; rebuilding configuration-sensitive helpers made
the 7/7 OFF selector pass. No workflow changed, so no hosted CI or act run applied. Task commits are d54349b
and 32b4104; evidence and summary are in .planning/phases/10-pbp-disc-images/. PBP-01 remains phase-wide
pending; Plan 10-12 is next.

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

### Phase 9 Plan 09-13 execution (2026-11-20)

Runtime/model: Codex GPT-6 session; exact model ID unavailable. Both configured package legs passed before
closeout. ON metadata and notice preflights passed 2/2 each, its build completed 229 Ninja actions, and
`release.archive_contents`, its negative control, `release.notices` and its negative control passed 4/4 in
about 0.13 seconds. OFF repeated the preflights at 2/2, then rebuilt in 27 Ninja actions and passed the same
release selectors 4/4 in 0.10 seconds. CPack produced the same library, libretro and runner macOS arm64 ZIP
names in each leg. Inventory sorting, diff checks and the 3900/3901 budget passed. Wall and active duration
were not measured. No broad suite, `act` or hosted CI run applied because no workflow changed. The corrected
plan routes notice checks through `hygiene`; Plan 09-11 retains `act --dryrun` and producer-path timing.
Todo 005 remains bound to the first tracked SPDX-bearing source in Phase 10. Task implementation commit is
`d275d99`; the ON checkpoint and plan selector correction are in `5edcbd1`; plan summary/state closeout follows.

### Phase 9 Plan 09-15 execution (2026-11-20)

Runtime/model: Codex gsd-executor gpt-6-luna xhigh; primary session model ID unavailable. Wall time from the
executor checkpoint at 18:47:57Z through closeout at 19:10:00Z was about 22 minutes; active time was not
measured. Cheap preset/inventory/budget checks passed in 0.01 seconds and the line budget remained 3900/3901.
External CHD mode configured in 8.8 seconds and its focused fixture/test build took about 16 seconds. Existing
generator pins, tool digest and media provenance passed preflight; enabled selectors passed 7/7 in 0.14 seconds.
Reconfiguring CHD OFF removed both outputs immediately; the disabled selector passed 2/2 in 0.03 seconds.
The fixture glob check found CHD and excluded the relocated manifest. The first in-tree selector attempt exited 8
because its build omitted the CHD test executable; rebuilding that target fixed the stale-binary issue, and the
same selector passed 7/7 in 0.09 seconds. Local BE32/WASI compilers are unavailable (tool check exit 1); hosted
cross-lane validation is assigned to Plan 09-11. No workflow changed, so no `act` or hosted CI run applied.
Task commits are `9a35d55` and `4bdeece`; Plan 09-11 is next by dependency order.

### Phase 9 Plan 09-11 execution (2026-11-20)

Runtime/model: Codex GPT-6 session; exact model ID unavailable. The inherited executor completed the Plan 11
implementation checks but did not preserve a reliable start timestamp or close the summary. The parent recovered
the committed working tree and completed the plan closeout; wall and active duration are not reliably measured.
The existing CI `changes` and nightly fuzz jobs now run pin, timeout, sorted-inventory, 3901-line budget and route
preflights. CHD is enabled in required CI/package presets; BE32 and WASI generate checked fixtures with a copied
native tool, and nightly adds a CHD campaign with 12 seeds and a 16,384-byte maximum input under the existing
60-second bound. Main and Release remain separate based on Plan 10's measured
18-archive digest mismatch and 24m25s serialization cost; todo 050 is completed. Maintainer docs explain Act's
classifier/matrix limitation and required hosted-only evidence.

Cheap post-change route, pin, timeout, preset, sorted-inventory, diff and budget checks passed; the gate remains
3901/3901 lines. Both Act dry runs passed using `/tmp` for its cache. Act did not execute workflow steps or prove
dependent matrix routing. Nightly now uses its already-configured build preset after fixture generation, avoiding
a duplicate CMake configure; time saved was not measured because clang-21 is unavailable locally. The WASI
module size remains unknown and `quality.json` stays at 608,540 bytes until the hosted CHD-enabled module is
measured. No full suite or hosted CI was started; final exact-head verification follows Plan 14 and source review.
Task commits are `0fc7f50` and `6e60d85`; the shared selector diagnostic follow-up is `82606fc`.

### Phase 9 Plan 09-12 execution (2026-11-20)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh; primary session model ID unavailable. Wall time
was 38m37s from 19:53:15Z to 20:31:52Z; active time was not measured. Sorted runner/libretro inventories,
fixture inputs and `git diff --check` passed before targeted builds. The CHD-on configure and affected target
build passed (0.77s); the focused runner/libretro selector passed 14/14 (0.91s). The CHD-off configure and
affected target build passed (0.73s); the unsupported-path selector passed 6/6 (0.36s), then the local build
tree was restored to CHD on. The initial runner selector exposed a parallel scratch-directory collision;
isolating the CHD case removed it. The bounded runner probe now reads CHD through shared disc callbacks, while
libretro's path acceptance and extension reporting follow the build feature. No broad suite, `act`, or hosted
CI run was needed for this focused provider integration; Plan 09-14 still owns cross-lane/hosted verification
and the final reviewed head. Task commits are `69c00c9` and `8a0637d`; planning closeout remains unstaged for
the root executor.

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
- Total plans completed: 320
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
| 9 | 15 | - | - |
| 10 | 13 | - | - |
| 11 | 15 | - | - |
| 12 | 7 | - | - |
| 13 | 9 | - | - |
| 14 | 16 | - | - |
| 15 | 12 | - | - |
| 16 | 18 | - | - |

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
| Phase 09 P12 | 38m37s | 2 tasks | 9 files |
| Phase 09 P14 | 22m | 2 tasks | 7 files |
| Phase 10 P01 | 24m53s | 3 tasks | 22 files |
| Phase 10 P07 | 44m | 2 tasks | 9 files |
| Phase 10 P08 | 24m | 2 tasks | 8 files |
| Phase 10 P09 | 25m | 2 tasks | 7 files |
| Phase 10 P10 | 19m | 2 tasks | 1 files |
| Phase 10 P12 | 41m | 2 tasks | 12 files |
| Phase 10 P13 | 38 | 2 tasks | 7 files |
| Phase 11 P01 | 37min | 3 tasks | 10 files |
| Phase 11 P03 | 36min | 2 tasks | 5 files |
| Phase 11 P13 | 11min | 1 tasks | 2 files |
| Phase 11 P05 | 29min | 2 tasks | 5 files |
| Phase 11 P06 | 27min | 2 tasks | 2 files |
| Phase 11 P07 | 48m | 2 tasks | 8 files |
| Phase 11 P08 | 33m32s | 2 tasks | 4 files |
| Phase 11 P09 | 26m | 2 tasks | 7 files |
| Phase 11 P10 | 1h 10m 25s | 2 tasks | 6 files |
| Phase 11 P11 | 23m27s | 2 tasks | 8 files |
| Phase 11 P14 | 13m | 2 tasks | 3 files |
| Phase 11 P15 | 6m | 2 tasks | 3 files |
| Phase 12 P01 | 8m | 2 tasks | 9 files |
| Phase 12 P02 | 4m | 2 tasks | 3 files |
| Phase 12 P03 | 12m | 3 tasks | 5 files |
| Phase 12 P03 | 16m | 3 tasks | 5 files |
| Phase 12 P04 | 22m | 3 tasks | 5 files |
| Phase 12 P05 | 37min | 3 tasks | 5 files |
| Phase 12 P06 | 64min | 2 tasks | 8 files |
| Phase 12 P07 | 9min | 1 tasks | 1 files |
| Phase 13 P01 | 20m43s | 2 tasks | 12 files |
| Phase 13 P02 | 60m | 1 tasks | 3 files |
| Phase 13 P03 | 59m22s | 2 tasks | 9 files |
| Phase 13 P04 | 17m02s | 2 tasks | 4 files |
| Phase 13 P05 | not tracked | 2 tasks | 11 files |
| Phase 13 P06 | not reliably tracked | 2 tasks | 7 files |
| Phase 13 P07 | not reliably tracked | 2 tasks | 8 files |
| Phase 14 P01 | 15m35s | 3 tasks | 11 files |
| Phase 14 P02 | 36m | 3 tasks | 10 files |
| Phase 14 P03 | 31m31s | 2 tasks | 4 files |
| Phase 14 P04 | 39min | 2 tasks | 9 files |
| Phase 14 P05 | 29m16s | 2 tasks | 5 files |
| Phase 14 P06 | 19m09s | 2 tasks | 5 files |
| Phase 14 P07 | 26m10s | 2 tasks | 7 files |
| Phase 14 P08 | 16m21s | 2 tasks | 5 files |
| Phase 14 P09 | 61m | 2 tasks | 4 files |
| Phase 14 P10 | 9m | 1 tasks | 2 files |
| Phase 14 P11 | 24m | 1 tasks | 3 files |
| Phase 14 P12 | 24m | 1 tasks | 3 files |
| Phase 14 P13 | 20m | 1 tasks | 2 files |
| Phase 14 P14 | 32m22s | 1 tasks | 3 files |
| Phase 14 P15 | 13m | 1 tasks | 3 files |
| Phase 14 P16 | 12m | 1 tasks | 4 files |
| Phase 15 P01 | 14 min | 3 tasks | 9 files |
| Phase 15 P02 | 20m 10s | 2 tasks | 7 files |
| Phase 15 P03 | 13m 25s | 2 tasks | 5 files |
| Phase 15 P04 | 13m | 2 tasks | 3 files |
| Phase 15 P05 | 2h28m | 2 tasks | 4 files |
| Phase 15 P6 | about 3h 52m wall time; active time not isolated | 2 tasks | 5 files |
| Phase 15 P07 | 24m commit window; active start not separately recorded | 2 tasks | 4 files |
| Phase 15 P08 | 29m 26s | 2 tasks | 4 files |
| Phase 15 P09 | 38m | 2 tasks | 9 files |
| Phase 15 P10 | 60m | 2 tasks | 11 files |

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
- [Phase 09]: Read CHD probe metadata and track identity through shared disc callbacks rather than duplicating media parsing.
- [Phase 09]: Gate libretro CHD acceptance and extension advertisement with SCHWAYSTATION_CHD.
- [Phase 09]: Leave CHD playlist membership out of Plan 09-12 because the existing provider rejects CHD playlist children.
- [Phase 09]: Plan 09-14 reuses the bounded boot/read/audio/FMV guest paths and pins for CHD input.
- [Phase 09]: The local CHD compatibility predicate passes on provider, host, package and guest evidence; hosted lane/hash/fuzz and WASI size remain unknown.
- [Phase 10]: Keep the PBP fuzz corpus as 14 generated mutation scripts capped at 256 bytes, with build-owned raw and compressed baselines and no committed disc bytes.
- [Phase 10]: Keep all Nightly fuzz cases; skip CHD and non-PBP fuzz work only when the classifier proves a pull request is PBP-only.
- [Phase 10]: Standalone PBP support leaves M3U playlist membership limited to the existing ISO/CUE contract.
- [Phase 10]: Keep PBP always available in the host path and gate its metadata claim on the registered libretro.content.pbp integration test.
- [Phase 10]: Keep the supported PBP claim limited to unencrypted single-data-track PSISO images and preserve CHD license derivation from the linked closure.
- [Phase 10]: Used the ci-release preset for release-labeled package tests; all existing archive and notice cases remained enabled.
- [Phase 10]: Rebuilt configuration-sensitive host/card helpers after toggling CHD in the reused build tree so tests matched the configured provider.
- [Phase 10]: The compatibility pass covers only the exercised 64-sector PBP layout; audio, untested track/profile variants and hosted verification remain explicitly unclaimed.
- [Phase 10]: PBP guest evidence uses a distinct one-data-track NTSC-U guest source and omits the CUE/CHD-only CDDA path; original two-track predicates remain unchanged.
- [Phase 10]: Keep the 3,901-line cap and defer the generic SPDX scanner.
- [Phase 10]: Keep the WASI size bound unchanged until the Phase 10 hosted lane reports the actual linked module size.
- [Phase 10]: Run exact-head CI and Nightly once at the ship PR to avoid duplicate hosted workloads.
- [Phase 11]: Plan 11-01 keeps ECM and CCD private and fail-closed until their positive readers validate complete media.
- [Phase 11]: A missing ECM continuation byte is malformed and must leave the output reader null.
- [Phase 11]: Workflow assertions were consolidated while preserving all controls under the unchanged 3901-line cap.
- [Phase 11]: Type-0 ECM literal streams are the supported direct and M3U path in Plan 11-03; ECM remains unsupported as a CUE source until Plan 11-05.
- [Phase 11]: The ECM open scan validates exact extent and global EDC before publishing; checkpoint memory and per-read work use independent fixed bounds.
- [Phase 11]: Keep ECM type 2/3 output at 2336 bytes and require CUE raw 2352 geometry so no missing sector header is invented.
- [Phase 11]: Plan 11-06 maps an omitted INDEX 0 to INDEX 1, preserving no stored pregap without inventing geometry.
- [Phase 11]: Plan 11-06 accepts nonempty SUB only at exactly 96 bytes per IMG sector; missing or empty SUB remains zero-filled.
- [Phase 11]: Plan 11-07 generates legacy media only from owned raw fixture sources and records generator digests and counts.
- [Phase 11]: Compare save/content identity through the IMG-backed CUE view and independently compare all CloneCD SUB bytes.
- [Phase 11]: Runner probes ECM-containing CUEs through the shared image reader; ordinary raw CUE handling remains unchanged.
- [Phase 11]: Compare CCD save identity only against a matching SUB base; keep generated SUB byte parity as a separate check.
- [Phase 11]: Libretro runtime and generated metadata advertise ECM and CCD from the same inventory-backed loader, retaining the CHD conditional.
- [Phase 11]: Keep ECM-01 and CCD-01 phase requirements open until all remaining Phase 11 evidence and final verification are complete.
- [Phase 11]: Use a named disposable local Linux container to build Clang 21 libFuzzer targets when the host Apple Clang lacks its Fuzzer runtime; keep long campaigns separate from bounded build/replay checks.
- [Phase 11]: The owned one-data-track ECM fixture supports boot, read/replay and FMV guest predicates, but no CD-DA claim.
- [Phase 11]: ECM and CCD compatibility rows report only local fixture, runner and libretro evidence; hosted CI, package and fuzz remain unknown until Plan 12.
- [Phase 11]: Keep local CHD-OFF package evidence separate from unknown hosted Main, Nightly and Release evidence.
- [Phase 11]: Record local installed-example and CHD-ON evidence separately from unknown hosted workflow results
- [Phase 12]: Reuse the generated legacy CCD/IMG/SUB fixture and patch only selected test sectors instead of adding another CCD generator.
- [Phase 12]: Keep compatibility evidence limited to selected-provider binding, bad-Q data availability, and previous-good GetlocP; make no retail-title claim.
- [Phase 12]: Keep the legacy SBId marker out of the runtime media directory so it cannot be mistaken for a real same-stem SBI sidecar.
- [Phase 12]: Use the parser's 65,536-byte maximum as the SBI corpus cap, including an 8,193-record boundary case.
- [Phase 12]: Register both SBI binaries in the explicit fuzz build preset so the existing Nightly job builds them.
- [Phase 12]: Capture one region-specific runner PCM file and reuse it for both callback comparisons after its existing size and SHA-256 checks pass.
- [Phase 13]: API v8 appends board identity; behavior revision is 19, while state serialization/hash updates remain for Plan 13-04.
- [Phase 13]: Old-sized profiles and board zero inherit current board defaults; GPU_V0 stays a legacy override, while explicit boards must match their documented GPU pairing.
- [Phase 13]: New instances default to the PU-18 board profile.
- [Phase 13]: Plan 13-02 records the exact movie15 expected-incomplete signature as diagnostic-only; MDEC-01 remains pending until full Phase 13 verification.
- [Phase 13]: Apply board RAM_SIZE at profile selection and power reset so explicit profiles are correct before the first frame.
- [Phase 13]: Gate only vC0 Test 22h to the documented INT5 (11h,10h); leave Test 20h and unpinned Test 51h behavior unchanged.
- [Phase 13]: Serialize board identity once and reconstruct fixed board-derived defaults from the canonical table after APPLY.
- [Phase 13]: Preserve mutable bus.ram_size during board-identity restoration.
- [Phase 13]: Resolve automatic region from serial, disc licence, and executable marker while keeping explicit selection authoritative.
- [Phase 13]: Keep board selection separate from video region and title serial; pair PU-7 and PU-8 early with GPU_V0.
- [Phase 13]: Libretro machine options default to and fall back to PU-18 per D-01 while the library default remains unchanged.
- [Phase 13]: Machine and region remain independent options applied at content load; auto region identity uses the shared host resolver.
- [Phase 13]: Full BIOS boot remains the default; only the explicit fast-boot profile flag enables the shell handoff.
- [Phase 13]: State format and behavior revision remain 19 because profile flags were already serialized and default behavior is unchanged.
- [Phase 14]: Keep the released 28-byte caller layout fixed while testing it against the current API.
- [Phase 14]: Register future peripheral tests as gating selectors and leave their behavior to the owning plans.
- [Phase 14]: API v9 preserves the 28-byte v8 input prefix and validates a zero-filled, caller-sized snapshot before apply.
- [Phase 14]: Frame-start latching resolves tap precedence across eight stable endpoints; rumble results remain tied to socket A.
- [Phase 14]: The new serialized endpoint shape uses unreleased state format and behavior revision 20.
- [Phase 14]: Method 1 REQ is request history for the next transaction and is evaluated with the previous response kind.
- [Phase 14]: Tap routing and request history remain independent per physical SIO port.
- [Phase 14]: Existing pad byte FSMs produce socket data and preserve each device's response length.
- [Phase 14]: Legacy card 1/2 APIs remain fixed aliases for port 1/2 socket A, independent of tap configuration.
- [Phase 14]: Serialize selected SIO card endpoint state, never card image bytes or generations.
- [Phase 14]: Mouse movement is latched and saturated once at frame start; subsequent frames without fresh host input report zero movement while retaining button levels.
- [Phase 14]: A mouse startup ACK-low hold is released only by an addressed 01h access at the same endpoint, including port 2 and tap sockets.
- [Phase 14]: neGcon twist, I, II and L inputs are reported raw, without a response curve or deadzone.
- [Phase 14]: Dual Analog flight uses the existing axis order and ID 5A53 without redirecting DIGITAL or DUALSHOCK behavior.
- [Phase 14]: Jogcon supports only its documented native 42h payload; wire bit 0 of the raw dial halfword is cleared and its physical interpretation is not inferred.
- [Phase 14]: Jogcon direction flags are turned-left and turned-right; software configuration and feedback commands return FF with no ACK.
- [Phase 14]: Plan 14-07: cross-validate candidate pad, SIO and card protocol states before apply; preserve receiver-owned card media across restore. Runtime Codex GSD executor, configured gpt-6-luna xhigh; primary runtime model ID unavailable; measured duration 26m10s.
- [Phase 14]: Scope legacy disc fixture prerequisites to their actual CTest consumers — The shared-directory TESTS property had added legacy setup/parity tests to unrelated focused selectors. Keep both fixture tests in the full inventory and retain dependencies on consumers; local configure and selector checks confirm unrelated state checks run alone while ECM guest and runner legacy checks still pull both prerequisites.
- [Phase 14]: Plan 14-08: preserve A card filename aliases and map all eight stable port/socket suffixes; reject invalid or undersized paths atomically. Runtime Codex GSD executor, configured gpt-6-luna xhigh; primary runtime model ID unavailable; measured duration 16m21s.
- [Phase 14]: Runner card persistence enumerates all eight physical port/socket identities and flushes only changed generations.
- [Phase 14]: Per-port multitap options default off and are reapplied after each movie snapshot before frame latching.
- [Phase 14]: Phase 14 Plans 10–14 split at behavior boundaries after Plan 14-09 took 60m42s; Codex GSD planner gpt-6.1-sol high and checker gpt-6-luna xhigh; planning/check review took about 8 minutes. — The plan checker passed requirements and dependency coverage. Keep each implementation slice below the 30-minute bound while preserving all selectors, metrics, and final verification.
- [Phase 14]: Split Plan 14-10 again after selector evidence showed its default port 0/1 probes pass; callback order queried both players and disabled sockets caused the remaining expected tap RED. Plan 14-10 is now a green default two-pad test seam, Plan 14-11 owns topology. Codex planner gpt-6.1-sol high, checker gpt-6-luna xhigh; follow-up planning/check about 5 minutes. — Separating harness correctness from product behavior prevents invalid RED evidence and keeps implementation slices inside the 30-minute plan bound. Full topology and all final quality/CI gates remain required.
- [Phase 14]: Keep Plan 14-10 limited to the default socket-A callback baseline; Plan 14-11 owns tap options and expanded routing.
- [Phase 14]: Apply multitap topology only after successful content load — A rejected replacement must leave the currently running frontend descriptors and serial topology unchanged.
- [Phase 14]: Accept only exact NONE, JOYPAD, and ANALOG controller IDs — Masking arbitrary subclasses would make the frontend claim support for devices that have no implemented input mapping.
- [Phase 14]: Match supported special libretro controllers by exact device ID and map their host inputs with bounded integer conversions.
- [Phase 14]: Libretro keeps port1A exclusively frontend-owned SAVE_RAM and persists only the seven remaining sockets.
- [Phase 14]: Build sidecar paths once from the original content basename and advance saved generations only after atomic replacement succeeds.
- [Phase 14]: Use the existing ntsc-u.pu8-late.v2 profile token for the six tested controller feature rows.
- [Phase 14]: Keep compatibility claims limited to passing selector evidence; leave retail-game support and undocumented Jogcon behavior unclaimed.
- [Phase 14]: Set state.bytes to the exact 3,757,638-byte local stable-state producer measurement.
- [Phase 14]: Leave WASI-size and Ir/frame bounds unchanged pending their exact hosted producers.
- [Phase 15]: Freeze the complete released v9 input and endpoint layouts locally so API v10 cannot move the 228-byte prefix or 24-byte endpoint stride.
- [Phase 15]: Register planned gun behavior in existing test targets and inventories; leave future behavior selectors gating until their owning plans implement them.
- [Phase 15]: Keep the released 228-byte API v9 input prefix and 24-byte controller endpoint stride unchanged.
- [Phase 15]: Append two explicit 16-byte direct-port gun records at offset 228 in API v10.
- [Phase 15]: Normalize only size-tagged caller bytes and validate the complete snapshot before replacing pending input; reject guns on enabled multitaps.
- [Phase 15]: Keep pending host input outside saved state; serialize only hardware-visible latched continuation in Plan 09.
- [Phase 15]: Pad owns gun protocol/result state and invalidates GPU-owned per-port beam events through the instance.
- [Phase 15]: GunCon unavailable or unsampled input returns X=0001h/Y=000Ah; explicit offscreen or reload returns X=0001h/Y=0005h as a deterministic model choice.
- [Phase 15]: GunCon reports only the documented active-low A, trigger, and B wire bits; unsupported bits remain released.
- [Phase 15]: Use one bounded GPU scheduler deadline per latched direct-port aim and sample live VRAM at that beam point.
- [Phase 15]: Use RGB5 channel sum >= 48 as an uncalibrated threshold, decode RGB24 with scanout byte addressing, and truncate the regional 8 MHz X conversion.
- [Phase 15]: Raise IRQ10 from the GPU beam callback only for an enabled bright Justifier sample. — The existing scheduler synchronizes Timer0 and Timer1 before the callback. Preserve its timer-to-GPU-to-DMA order instead of changing timers.c or globally reordering devices.
- [Phase 15]: Reuse the existing serialized byte-staging lane for Justifier sensing enable. — This preserves the established continuation format and avoids adding a state-format field or revision.
- [Phase 15]: Use official LIGHTGUN base ID and Justifier subclass 2 on direct ports; do not offer guns behind multitaps.
- [Phase 15]: Treat capability as device-type support only; zero input_state values remain neutral and cannot identify absent controls.
- [Phase 15]: Prefer LIGHTGUN when both device types are advertised; POINTER is a limited fallback without forced reload.
- [Phase 15]: Keep GunCon and Justifier on direct physical ports; multitap sockets reject guns.
- [Phase 15]: Pending host input remains outside serialized machine state; the host submits it through the normal input API after load.
- [Phase 15]: State format and behavior revision 21 carry gun protocol fields; state.bytes tracks the exact 3757754-byte producer result.
- [Phase 15]: Keep gun compatibility evidence synthetic; no retail-game or physical calibration claim is supported.
- [Phase 15]: Set exact WASI and Cachegrind bounds from measured current-head producers; preserve the 100-basis-point relative limit.
- [Phase 15]: Limit frame-start GPU synchronization to direct-gun configurations so the no-gun OpenBIOS timing path stays unchanged while armed sensors retain mode-zero sampling.
- [Phase 15]: Reject GunCon and Justifier in every saved multitap endpoint and guard direct-gun polling against endpoint indexes outside its two-port arrays.

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
- [Phase 11]: ECM and CCD are host-side bounded providers on the shared disc contract; fixtures and consumer tests preserve the documented subsets.
- [Phase 11]: Keep full Nightly routing and every campaign case; D-12 defers the metadata-only shortcut until a measured gate refactor fits the fixed line budget.
- [Phase 11]: The final local CI run is green; hosted Main/Nightly/Release evidence must be recorded at ship before Phase 12 planning.
### Blockers/Concerns

- [Phase 4]: Research flag: GPU blend width, size rejection, stepper precision, GPU clocks per line, and the adapter's stub-kernel path for PS-EXE content in RetroArch.
- [Phase 6]: Research flag: the CD seek model and INT1 period, and whether OpenBIOS boots the own mkpsxiso disc without licence data.
- [Phase 7]: Closed; hosted verification, main CI, release v0.7.0 and triage artifact passed. Follow-up efficiency investigation is in todo 050, bound to Phase 9.
- [Phase 11]: D-08's Nightly shortcut is deferred under D-12; the full Nightly route and all cases remain active. This is a closed scope disposition, not an active blocker. Phase 11 hosted Main/Nightly/Release results remain unknown until its PR runs.

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
| 2026-11-25 | /gsd-execute-plan 15-05 | Codex/OpenGSD executor gpt-6-luna; 2h28m wall from 06:01:43Z to 08:29Z. Focused ABI/budget preflight passed 2/2; beam timing controls passed 3/3; geometry matrix passed 28 assertions and the required selector set passed 4/4 in 0.03s. Task commits: `6d3fba9`, `b519b6a`, `bc72f56`. No broad suite, hosted CI, Act or Docker. The 20–28m low-confidence estimate was exceeded while deriving scheduler deadline/retarget behavior and adding edge coverage; next plans should compile the smallest visible path before expanding geometry cases. | gpt-6-luna |
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
| 2026-11-21 | /gsd-execute-phase 10 plan 10-07 | Codex, OpenGSD 1.21.0; focused PBP replay/bounds, 14-seed ASan/UBSan campaign in native ARM64 Ubuntu Docker (54,946 inputs/61s), Act routing dry-run, 3,900/3,901 line budget; 44m wall | gpt-6-luna xhigh |

## Session Continuity

Last session: 2026-11-26T16:18:55Z
Stopped at: Phase 16 complete — all phases complete
Resume file: .planning/phases/16-patches-cheats-and-compatibility-evidence/16-VERIFICATION.md

Resume check (Codex/OpenGSD 1.23.0, primary model ID unavailable): `cmake "-DROOT=$PWD" '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` exited 0 at 3,900 lines in the phase worktree and 1 at 4,694 lines in the isolated Plan 16-14 worktree. The draft is present and uncommitted. Plans 16-15 through 16-18 depend on 16-14 directly or transitively. The owner approved the measured ceiling after this check.

Runtime/model: Codex/OpenGSD 1.23.0 coordinator (primary model ID unavailable); assumption analysis gpt-6.1-sol high, phase researcher gpt-6-luna xhigh, planner and revision gpt-6.1-sol high, independent plan checker and rechecks gpt-6-luna xhigh.

Phase 16 execution started on `phase/16-patches-cheats-compatibility`. Runtime/model: Codex/OpenGSD 1.23.0 coordinator (primary model ID unavailable); GSD executor `codex exec` pinned to gpt-6-luna xhigh. Parallelization and GSD worktrees are enabled to follow the project’s disjoint-wave execution rule.

Plan 16-01 completed on 2026-11-26 in 17 minutes. Runtime/model: Codex/OpenGSD 1.23.0; typed `gsd-executor` gpt-6-luna xhigh. The released API-v10 caller and exact patch, cheat, compatibility and fuzz selector graph are committed; the six intentional fail-first stubs are tracked for their owning plans. `git diff --check HEAD`, all six inventory sort checks, `cmake --preset ci`, the four target build, and the focused `abi.api_v10` plus `runner.cheats.memory_read` CTest selection exited 0. The 24-selector preflight exited 0 after accounting for CTest’s `compat.features.preflight` fixture dependency. GSD worktree cleanup/merge exited 0. The broad workflow remains for the phase closeout because Plan 01 deliberately registers gating selectors that their later implementation plans make pass; no test is removed or made non-gating.

The GSD `verify-summary` helper exited 0 but returned `passed: false`: its matcher treated `## Verification Evidence` as the self-check section and then classified the later `fail-first` notes as a failure. The actual `## Self-Check: PASSED`, first two created files and plan-scoped commits were verified. This is a validator false positive, not an implementation failure. Process-spawned `codex exec` could not initialize its in-process app-server in this environment (`Operation not permitted`), so the plan ran through the typed GSD executor in a GSD-created worktree. Continue wave dispatch with typed executors and explicit worktree paths. Next: plans 16-02 and 16-07.

Phase 15 release boundary: `gh run view 45697774644 --json status,conclusion,jobs` exited 0 (Main CI passed); [run 45697774644](https://github.com/szTheory/schwaystation/actions/runs/45697774644). `gh run view 45698033992 --json status,conclusion,jobs` exited 0 (Release passed); [run 45698033992](https://github.com/szTheory/schwaystation/actions/runs/45698033992). `gh release view v0.15.0 --json isDraft,url` exited 0 with `isDraft=false`; [v0.15.0](https://github.com/szTheory/schwaystation/releases/tag/v0.15.0).

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

- Push the reviewed Phase 9 corrections and complete one exact-head hosted CI/Nightly rerun. If both pass, write Phase 9 verification, ship PR #32, and confirm the release and main CI before discussing Phase 10. Keep todo 005 for the first tracked SPDX-bearing source and todo 051 bound to Phase 10's first Nightly routing change. No owner UAT is needed.

### v2.0 roadmap creation (2026-11-18)

Runtime/model: Codex, OpenGSD 1.20.0, gsd-roadmapper gpt-6.2-sol high. The approved milestone has 10 phases and 34 requirements. Phase 9 establishes the shared media controls and feature-ledger checks with the first provider. Later feature phases record their own evidence. Phase 16 reconciles the pinned DAT inventory and gates libretro metadata on the required-capability matrix. The v1.0 MDEC evidence boundary and the standing 300-line real-host limit carry forward. No implementation or phase verification is claimed here.

### Phase 7 verification refresh (2026-11-20)

Runtime/model: Codex/OpenGSD; primary GPT-6 model ID unavailable; gsd-verifier gpt-6-luna xhigh. The Phase 7 review scope contained only release-version metadata, so no new source review was needed. Refreshed `07-VERIFICATION.md`; 5/5 implementation truths and 27/27 plan artifacts passed, and `verification.status` returned `passed`. The baseline note now identifies the initial 595,820-byte WASI and 3,755,020-byte state measurements as superseded by Phase 8 producer evidence: 608,540 and 3,755,328 bytes. `git diff --check` passed. The 24 Phase 8 regression selectors were present; full suite execution was not repeated because exact-head Main CI [44645037841](https://github.com/szTheory/schwaystation/actions/runs/44645037841) and Release [44645037923](https://github.com/szTheory/schwaystation/actions/runs/44645037923) passed on `830bf4c`. BE32 took 4,694 seconds on Main CI and 3,402 seconds in the concurrent Release matrix; todo 050 remains bound to Phase 9 for measured reuse analysis. Phase 7 is complete and its existing v0.7.0 release remains valid.


### v1.0 milestone closeout (2026-11-18)

All six phase verification reports passed; milestone audit found 30/30 requirements, 30/30 integration paths and 7/7 flows, with accepted MDEC conformance limits and non-blocking follow-ups recorded in `.planning/milestones/v1.0-MILESTONE-AUDIT.md`. The closeout scanner found 29 pending todos, 11 dormant seeds and two existing deferred notes. All 42 were acknowledged as deferred, not marked resolved; the details are listed in the Deferred Items table above. The phase trees and one completed quick task are archived under `.planning/milestones/v1.0-phases/` and `.planning/milestones/v1.0-quick/`.

Hosted evidence reused: [release-PR CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074), [nightly full-vector job 134564625732](https://github.com/szTheory/schwaystation/actions/runs/44233700921/jobs/134564625732), and [v0.6.0 release run 44262491280](https://github.com/szTheory/schwaystation/actions/runs/44262491280). Planning-only edits received `git diff --check`; no implementation tests were added or rerun.

### Phase 9 Plan 09-14 execution (2026-11-20)

Runtime/model: Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh. Work started at 20:42:42Z and the state closeout completed at 21:04:54Z. The owned CHD source/geometry preflight, 222-sector forward/reverse parity, feature-key check, and boot/read/audio/FMV guest cases all passed. The focused CHD selector passed 14/14 in 10.71 seconds; an identical extra rerun passed 14/14 in 11.45 seconds but added no coverage and was redundant. The feature ratchet passed 2/2, documentation freshness passed, and both diff checks were clean. The four guest tests shared the existing pins and bounds and ran in parallel after setup fixtures. No broad Phase 9 suite, hosted CI, `act`, or source review ran in this plan. The local CI build remains CHD ON. Task commits: `1d48028`, `ab91147`, `938cca6`; summary: `.planning/phases/09-chd-disc-images/09-14-SUMMARY.md`.

The feature row now passes only the demonstrated local provider, runner, libretro, package and guest predicate. Hosted eight-lane/hash/fuzz evidence and the actual WASI module size remain unknown for final Phase 9 verification. The source review passed; final verification is next, with no owner action needed.

### Phase 9 source review (2026-11-20)

Runtime/model: GSD `gsd-code-reviewer`, gpt-6-luna xhigh, standard depth. Reviewed 29 source files changed since
the preceding review commit `0f81b9a`; no new defects were confirmed and the prior resolved finding was not
repeated. The review itself ran no tests. Artifact: `.planning/phases/09-chd-disc-images/09-REVIEW.md`.
Plan 09-14's bounded 14-test selector ran twice (10.71s and 11.45s); the second identical pass added no
coverage. Future closeout should keep the first green result and avoid a tracer-only rerun. Next: final local
preflight and one exact-head hosted verification run, recording the WASI module size and CI critical path.

### Phase 9 final local workflow and CI-efficiency review (2026-11-20)

Runtime/model: Codex/OpenGSD 1.20.0; primary GPT-6 model ID unavailable. The Phase 9 final preflight ran
`LC_ALL=C sort -cu` for the disc, compatibility and fuzz inventories, `git diff --check origin/main...HEAD`, and
the six-test media/feature selector; all exited 0, with the selector completing in 0.49 seconds. The final local
`cmake --workflow --preset ci` workflow exited 0: 639/639 CI tests passed, the separate release selector passed
7/7, and CTest reported 74.78 seconds real time. The workflow also generated all three local macOS packages.

The preceding local attempt exposed two corrective issues. `disc.playlist` still passed the 255-byte boundary,
status and line assertions; only its pinned CHD message was stale after CHD became a supported parser path. The
assertion now checks status and line without binding to that message. `docs/testing.md` exceeded its 61,440-byte
page budget; the unchanged media-preflight details moved to `docs/media-testing.md`, leaving the original page at
60,814 bytes. The exact targeted selector passed 2/2 in 0.06 seconds and the byte-budget check passed.

Five failing libchdr test registrations came from an old ignored `build/ci/pins/libchdr/build/tests/CTestTestfile.cmake`
left by a prior configuration, despite the current forced `CHDR_WANT_TESTS=OFF`. Removing only that generated
CTest file restored the intended test inventory; the current configure retains 639 CI tests plus seven separate
release checks. No tracked CMake or workflow change was needed. Fresh hosted runners do not inherit this local
build metadata.

Two read-only CI-efficiency reviews found no evidence-backed workflow reduction to apply: the cross-lane generator
sharing idea lacks timing evidence, the nightly job already reuses its configure, and Main/Release remain separate
because measured archive digests differ and serialization added 24m25s. The standing preflight, Act, live-run and
post-run cost guidance is discoverable in `AGENTS.md`, this file, and `docs/maintaining.md`. The workflow files did
not change in this closeout, so the already-passing Act dry-run was not repeated. The 09-14 summary now labels its
second identical focused run as redundant rather than required.

### Phase 9 first exact-head hosted CI follow-up (2026-11-20)

Runtime/model: Codex/GPT-6 session (primary model ID unavailable); gsd-code-reviewer gpt-6-luna xhigh. The first
hosted CI head `e57ad5f` exposed three concrete integration gaps. The CHD-enabled WASI module was valid and
measured 859,379 bytes, above the prior 608,540-byte CHD-off bound. Both Windows C++ lanes failed because pinned
sources reached MSVC `<limits>` before the CRT floating-point macros were included. Nightly built the pinned CHD
generator, then its owned media fixture failed because the job lacked the FFmpeg development packages required
by the pinned encoder. These results are in [CI run 44827711416](https://github.com/szTheory/schwaystation/actions/runs/44827711416)
and [Nightly run 44827711388](https://github.com/szTheory/schwaystation/actions/runs/44827711388).

After confirming those failures, the known-failing CI tail was canceled. That attempt used 1,684 raw runner
seconds (28.1 runner-minutes) over 268 seconds wall time; Nightly used 351 raw runner seconds (5.9 runner-minutes)
over 85 seconds wall time. These are unweighted runner times, not billing cost. The follow-up set the WASI bound
to 859,379 and moved sanitizer/FFmpeg dependency installation ahead of Nightly compilation. Cheap route, pin,
timeout, inventory, budget, diff and Act dry-run checks passed.

A second hosted CI attempt on `83a27a4` showed `/FIfloat.h` was present but resolved to pinned FLAC's
`include/private/float.h` because that directory preceded MSVC's implicit headers. Both Windows architectures
therefore repeated the missing `FLT_*`/`DBL_*` failure. [CI run 44830485927](https://github.com/szTheory/schwaystation/actions/runs/44830485927)
was canceled after these failures; it consumed 1,654 raw runner-seconds (27.6 runner-minutes) over 380 seconds
wall time. The same exact head's [Nightly run 44830486029](https://github.com/szTheory/schwaystation/actions/runs/44830486029)
passed the bounded fuzz campaign. Nightly consumed 825 raw runner-seconds (13.8 runner-minutes) over 503 seconds
wall time. Its four benchmark jobs plus full-vector job used 322 seconds (5.4 runner-minutes) unrelated to CHD;
keep the Phase 10 routing todo 051 bound to the first Nightly-routing change.

The current CMake correction removes FLAC private/protected include directories from the whole target and applies
them only to FLAC C sources. The generated compile database shows all 21 pinned C++ files exclude those paths
and all 17 FLAC C files retain them; the native CHD tool rebuilt successfully (85/85 actions). The updated
32-file source review is clean. Hosted Windows/WASI and eight-lane checks must still confirm the correction on
the next head; no Phase 9 verification or ship claim is made.

### Phase 9 verification and completion (2026-11-20)

Runtime/model: Codex with OpenGSD 1.20.0; primary model ID not exposed. A typed verifier handoff made no artifact
or state change and was stopped; the orchestrator completed goal-backward verification inline from the code,
phase artifacts and exact-head CI evidence. The wall interval from the saved STATE checkpoint at 22:48:36Z to
closeout at 01:24Z was about 2h36m; active time was not measured. The Phase 9 report passed 5/5 observable truths
with no human checks and records its fingerprint as
`v4:sha256:c8d13054d5c31b7f5d051488ebe7bdd6bc3e2fbb1d20bb171dd15f2460b096a7`.

The exact-head [Main CI run 44843623296](https://github.com/szTheory/schwaystation/actions/runs/44843623296)
passed on `16dc50920b793316ad1295d47168bdb39d2cc87b`, including all six package lanes, BE32, WASI, hash equality,
RetroArch and release rehearsal. The linked WASI module measured exactly 859,379 bytes. `CI required` measured a
2,419-second critical path and the workflow used 6,571 raw runner-seconds (109.52 runner-minutes); the run
fingerprint differed from the stored baseline, so no speedup is claimed. Hosted CHD fuzz passed on
[run 44830486029](https://github.com/szTheory/schwaystation/actions/runs/44830486029); the redundant Nightly
attempt on the final head was canceled. `gsd-tools query verification.status` returned `passed` (exit 0),
`git diff --check` passed, and `gsd-tools query phase.complete 9` advanced ROADMAP/STATE/REQUIREMENTS to Phase 10.
Its six evidence-path warnings are command strings parsed as file paths plus one old todo path after the todo
moved to `completed/`; no Phase 9 source or verification gap was reported.

The Phase 9 review is clean. Existing CI/CD guidance is discoverable in `AGENTS.md`, `.planning/AUTONOMOUS.md`
and `docs/maintaining.md`; no extra workflow change or repeated full local suite was needed. Before Phase 10
planning on its branch, refresh the stale Phase 7 and Phase 8 verification reports using the exact main/release
evidence already available, then continue Phase 10 discussion and planning. The roadmap now marks Phase 9
complete; the Phase 9 report and tracking edits still need to be committed and shipped through PR #32.

### Phase 15 gap-plan execution and final local workflow (2026-11-25)

Runtime/model: Codex/OpenGSD 1.23.0; primary and typed agent model IDs were not exposed. Plan 15-11 updated only the Format 21 `gpu_motion` state hashes at frames
150 and 180 to the exact runner values. `git diff --check`, `cmake --preset ci`, selector-count preflight, and
the two focused state selectors passed; the selectors took 0.67 seconds.

The first full `cmake --workflow --preset ci` attempt on `c79b694` exited 8 after 49 seconds (CTest 48.32s).
It exposed one remaining stale test assumption: `libretro.input.controls` still called the now-supported
`RETRO_DEVICE_LIGHTGUN` an invalid device selection. The correct fix was in the host test, not the emulator.
Plan 15-12 changes only that negative control to `RETRO_DEVICE_KEYBOARD`, retaining LIGHTGUN tests and the
serialized-state assertions. A focused CTest run before rebuilding the changed host test used the stale binary
and failed in 1.34 seconds. The plan was corrected to rebuild `schwaystation-test-libretro-host` first; the
build passed and the focused selector then passed 1/1 in 1.33 seconds.

Code review ran before final verification and found no active issues in either gap fix. On reviewed head
`97606163409f6be78c08b8a2249b85bb68930619`, `time cmake --workflow --preset ci` exited 0 in 74.50 seconds.
CTest's JUnit report records 717/717 passing cases and 71 seconds suite time; the release selector passed 8/8
in 1.54 seconds. The slowest cases were `policy.quality_metrics` (39.50s), `libretro.host` (35.56s),
`libretro.input.movies` (25.84s, serial tail), `runner.disc.legacy` (20.48s) and `runner.hashes_file`
(16.48s). The required cases and bounds remain intact. No workflow files changed, so Act would add no coverage
beyond the complete local workflow. Hosted Main and Release checks remain required during ship. This was a
local workflow, so there is no run URL.

All bounded CTest and workflow commands returned; CTest waited for its child processes, and no task-owned
fuzzer or Docker resource was created in this closeout. The earlier stopped fuzz jobs remain covered by the
durable cleanup rule in `AGENTS.md` and `.planning/AUTONOMOUS.md`. Goal-backward Phase 15 verification and ship
are next; previous Phase 7–14 verifier fingerprints currently need refresh after shared-file changes.
