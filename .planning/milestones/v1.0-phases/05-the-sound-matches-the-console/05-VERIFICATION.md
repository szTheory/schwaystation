---
phase: 05-the-sound-matches-the-console
verified: 2026-11-18T11:40:30Z
status: passed
score: 74/74 must-haves verified
tested_revision: 35a19da726e9fed0b9c0e4dca8c418aca29dbaff
tested_tree: 058ac8996b00488ce9bd79092b4b750ae4d33214
runtime: "Codex, OpenGSD 1.20.0, execute-phase stale-verification refresh"
model: "gsd-verifier gpt-6-luna xhigh; primary session model ID unavailable"
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .planning/phases/05-the-sound-matches-the-console/05-01-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-01-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-02-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-02-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-03-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-03-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-04-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-04-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-05-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-05-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-06-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-06-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-07-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-07-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-08-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-08-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-09-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-09-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-10-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-10-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-11-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-11-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-12-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-12-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-13-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-13-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-14-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-14-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-15-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-15-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-16-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-16-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-17-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-17-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-18-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-18-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-19-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-19-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-20-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-20-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-21-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-21-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-22-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-22-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-23-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-23-SUMMARY.md
  - .planning/phases/05-the-sound-matches-the-console/05-24-PLAN.md
  - .planning/phases/05-the-sound-matches-the-console/05-24-SUMMARY.md
  - CMakePresets.json
  - cmake/SPUReferences.cmake
  - libretro/libretro.c
  - runner/main.c
  - src/bus.c
  - src/dma.c
  - src/frame.c
  - src/internal.h
  - src/sched.c
  - src/spu.c
  - src/state.c
  - tests/inventory/ci.txt
  - tests/inventory/nightly.txt
  - tests/libretro/host.c
  - tests/libretro/tests.cmake
  - tests/nightly/tests.cmake
  - tests/perf/tests.cmake
  - tests/runner/tests.cmake
  - tests/spu/fixture.c
  - tests/spu/inventory.txt
  - tests/spu/model.c
  - tests/spu/reference.c
  - tests/spu/tests.cmake
  - tests/tests.cmake
covered_digest: "v4:sha256:c5dd171aeb1deac661f2e1df837884f6db096561096ac48b732c50fae2868f71"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 10
  total: 10
  not_honored: []
re_verification:
  previous_status: passed
  previous_score: 74/74
  gaps_closed: []
  gaps_remaining: []
  regressions: []
---

# Phase 5: The sound matches the console — Verification Report

**Phase Goal:** The SPU matches its documented hardware transfer cases, audio hashes are identical across all platforms including be32 and wasm32, and libretro delivers the runner PCM stream.

**Verified:** 2026-11-18T11:40:30Z
**Status:** passed
**Refresh:** The previous report passed 74/74 and had no gaps. It became stale after later workflow changes. Current code and test paths were checked against the exact current source tree and the relevant hosted Nightly result; no local tests were rerun.

## Goal Achievement

### Observable Truths

| # | Roadmap success criterion | Status | Evidence |
|---|---|---|---|
| 1 | The spu_tone stream has the expected per-frame sample count, its hash is identical on all eight lanes, and the CPU-free block-table path produces the guest PCM. | VERIFIED | The exact-current-source [CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) passed the all-platform matrix, audio tests, eight-lane hash-equality job, be32 and wasm32 jobs. The prior verifier traced the sample-count and CPU-free equality checks through tests/spu/fixture.c and the audio model tests; these files match the tested source tree. |
| 2 | The audio tests cover the SPU model and the pinned memory-transfer log comparator passes its seven hardware cases. | VERIFIED | The exact-source [Nightly run 44233700921](https://github.com/szTheory/schwaystation/actions/runs/44233700921) ran cmake --workflow --preset nightly; its vectors-full job log shows spu.reference.memory_transfer passed and 30/30 nightly tests passed. The SPU model, reference comparator, pins, bus/DMA/scheduler paths, and nightly registration are unchanged from that run to current origin/main. The exact-tree CI also passed the recurring audio-label tests. |
| 3 | Runner and libretro batch/single audio are equal, RetroArch boots spu_tone.exe, and idle SPU work is bounded. | VERIFIED | The exact-current-source CI run passed libretro.host, the audio hash-equality job, core.no_busy_loop coverage, and the hosted macOS RetroArch tone-content smoke; the RetroArch screenshots matched runner frames. Current wiring in libretro/libretro.c uses schway_get_audio for batch delivery and the sample callback fallback; tests/libretro/host.c compares both paths against runner PCM. |

**Score:** 74/74 must-haves verified (71 plan-specific truths and the three roadmap criteria); zero behavior-unverified truths.

### Current Hosted Evidence

| Purpose | Command or job | Result | Run |
|---|---|---|---|
| Current all-platform source regression, sample/hash/host tests and content smoke | cmake --workflow --preset ci; six platform legs, be32, wasm32, hash-equality, libretro.host, core.no_busy_loop and RetroArch tone-content jobs | Exit 0 on the exact current source tree (tree 058ac8996b00488ce9bd79092b4b750ae4d33214) | [Release-PR CI 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) |
| Pinned SPU hardware-transfer comparator | cmake --workflow --preset nightly; ctest inventory includes spu.reference.memory_transfer | Exit 0; the comparator passed, and the inventory check passed | [Nightly 44233700921](https://github.com/szTheory/schwaystation/actions/runs/44233700921) |

Nightly 44233700921 ran on commit 7d24bed34eb11dce8e223eefb40fb2098e41a095. Its hardware test inputs and relevant SPU source, reference-test code, pins, CMake wiring and nightly inventory match current origin/main. The working milestone branch adds planning files only. The all-platform CI run is on a pull-request commit whose tree exactly matches current origin/main.

### Implementation and Wiring

| Contract | Current implementation and evidence | Status |
|---|---|---|
| Scheduled SPU to public PCM ring | src/sched.c advances src/spu.c; SPU output is mixed into the instance-owned ring drained by schway_get_audio in src/frame.c. The audio model/fixture tests assert pair counts, regional cadence and CPU-free PCM replay. | VERIFIED |
| Canonical runner digest | runner/main.c drains schway_get_audio and hashes canonical signed 16-bit stereo PCM for --hash-audio. The all-lane hash job compares the expected eight lane artifacts. | VERIFIED |
| Libretro delivery | libretro/libretro.c drains the same public PCM API and selects batch or sample callback; tests/libretro/host.c runs both against the runner byte stream. | VERIFIED |
| Hardware transfer evidence | tests/nightly/tests.cmake registers the pinned spu.reference.memory_transfer test, which validates the ordered seven-case log; the successful nightly log records the test pass. | VERIFIED |
| Lane/test wiring | SPU audio tests and inventories feed the normal CI matrix; the later CI workflow additions retain these registrations. The refreshed code review found no new Phase 05 issue. | VERIFIED |

### Plan and Requirement Coverage

All 24 Phase 05 plan/summary pairs were included in the covered-file fingerprint. The previous goal-backward review mapped 71 plan-specific truths to implementation, test registrations and evidence; the refreshed current-tree check found no implementation or test delta beyond the reviewed workflow changes. Current exact-source CI and the unchanged nightly comparator evidence continue to cover those paths. No requirements are orphaned.

| Requirement | Status | Evidence |
|---|---|---|
| SOUND-01: scheduled integer SPU, fixed sample cadence, CPU-free PCM replay, cross-platform hashes, audio behavior tests, pinned seven-case hardware transfer log, and fuzz replay | SATISFIED | Exact-tree CI audio tests and eight-lane equality; successful nightly run #44233700921 passed spu.reference.memory_transfer. No full-silicon PCM capture is claimed. |
| SOUND-02: batch/single libretro PCM equals runner PCM, tone content smoke, and bounded idle SPU | SATISFIED | Exact-tree CI libretro.host, RetroArch tone smoke, and core.no_busy_loop checks passed on the tested tree. |

### Test Quality and Anti-Patterns

The prior 74/74 report found no disabled tests, circular expected values, unreferenced TBD/FIXME/XXX markers or empty production implementations. The linked SPU tests, comparator, host tests and lane inventories are unchanged from the successful hosted evidence. The exact-source test matrix and the passing nightly comparator provide behavioral evidence; no local test suite was rerun.

### Evidence Limits and Human Verification

The project does not claim that the self-authored spu_tone digest is a full silicon waveform oracle. Cross-lane hashes establish determinism, runner/libretro equality establishes delivery, and the pinned transfer log establishes only its seven named hardware cases. The Phase 5 context explicitly records that no full hardware PCM capture exists. No owner hardware capture or manual audio sign-off is required.

### Gaps Summary

No implementation gaps remain. All three roadmap criteria, SOUND-01/02 and 71 plan-specific truths are verified. The only evidence limitation is the already-documented absence of a full hardware PCM waveform capture; the project does not claim that coverage.

### Decision Coverage

All 10 trackable decisions in 05-CONTEXT.md are honored (10/10).

---

Verified: 2026-11-18T11:40:30Z
Verifier: gsd-verifier
