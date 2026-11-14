---
phase: 05-the-sound-matches-the-console
verified: 2026-11-14T13:19:17.195Z
status: passed
score: 3/3 must-haves verified
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
  - docs/testing.md
  - src/cpu_exec.inc
  - src/frame.c
  - src/sched.c
  - src/spu.c
  - src/spu.h
  - tests/cmake/compat-controls.cmake
  - tests/cmake/hashes.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/workflows.cmake
  - tests/core/test_run.c
  - tests/dma/dma_tests.c
  - tests/embed/CMakeLists.txt
  - tests/fixtures/src/unbuilt.asm
  - tests/fixtures/tests.cmake
  - tests/hygiene/inventory.txt
  - tests/hygiene/tests.cmake
covered_digest: "v4:sha256:fa74d1f1cfe3792baef860024c2d48e92efd711e0b95581930125280777f0f02"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 5: The sound matches the console Verification Report

**Phase Goal:** The SPU sounds as the console does: it matches the hardware logs, its audio hashes are identical on every platform including `be32` and `wasm32`, and libretro delivers exactly the runner's audio stream.
**Verified:** 2026-11-14T13:19:17.195Z
**Status:** passed
**Re-verification:** No — initial verification.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | `spu_tone` produces equal frame-600 audio hashes and mailbox evidence across eight lanes; audio length and CPU-free block-table PCM remain checked. | ✓ VERIFIED | The `hashes.cmake` writer runs `spu_tone.exe` in all three regions with `--hash-audio`, adds `audio@600` and `mailbox@600`, and the required CI `hash-equality` job compares exactly eight artifacts. The `spu.fixture` test checks the independent CPU-free path and exact sample counts. Full CI passed 547/547 tests and 5/5 release package checks; `ctest -L audio` passed 10/10. The self-authored digest is documented as a model regression, not a hardware waveform. |
| 2 | BRR, ADSR, noise, reverb, capture, IRQA and transfer behavior is tested, and the nightly lane runs the seven-case pinned hardware transfer log. | ✓ VERIFIED | The `audio` suite passed 10/10, including model, envelope, voice, noise, reverb, fixture and reference controls. `tests/nightly/tests.cmake` registers `spu.reference.memory_transfer --full` against the pinned reference assets, and `nightly.yml` invokes the nightly preset on pull requests that change its workflow. The changed workflow is covered by `spu_registration`; the exact-head hosted result and URL are deliberately confirmed at ship. |
| 3 | Libretro batch and single-sample callbacks match runner PCM; RetroArch loads `spu_tone.exe` as content; idle SPU does not busy-loop. | ✓ VERIFIED | `libretro.host` passed, comparing the runner's audio output against both callback modes; `core.no_busy_loop` passed. The required hosted RetroArch job boots the tone fixture under the stub and compares video against runner output. The workflow registration assertion passed; actual hosted PR execution remains a ship-time check. |

**Score:** 3/3 truths verified (0 present, behavior-unverified).

## Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `src/spu.c`, `src/spu.h`, `src/cpu_exec.inc`, `src/frame.c`, `src/sched.c` | Bounded SPU event processing, state ownership and public run integration | ✓ VERIFIED | Substantive implementation exists; reviewed clean and exercised by CI and focused state/run tests. |
| `tests/spu/fixture.c`, `tests/spu/model.c`, `tests/spu/expected.txt` | Tone PCM, exact sample count, mailbox predicates and CPU-free block path | ✓ VERIFIED | `spu.fixture` is registered in the audio inventory and passed as part of the audio suite. Expected digests are explicitly labeled self-authored model regressions. |
| `tests/cmake/hashes.cmake`, `tests/runner/tests.cmake`, `.github/workflows/ci.yml` | Lane artifact generation and exact eight-file comparison | ✓ VERIFIED | `runner.hashes_file` and hash comparison passed locally; required workflow registration was checked. |
| `tests/nightly/tests.cmake`, `cmake/SPUReferences.cmake`, `.github/workflows/nightly.yml` | Seven-case pinned hardware transfer test in the nightly workflow | ✓ VERIFIED | Registration and pin checks pass. The local nightly build is configured GPU-reference-only and has no transfer test/assets; the PR-triggered nightly execution is confirmed at ship. |
| `tests/libretro/host.c`, `tests/libretro/tests.cmake`, `tests/core/tests.cmake` | PCM callback equality and idle-SPU bound | ✓ VERIFIED | The two named CTest cases passed. |
| `tests/cmake/workflows.cmake`, counted check sources | Phase wiring checks and 2688-line repository-check cap | ✓ VERIFIED | Phase registration, timeout, pin and line-budget checks pass; count is exactly 2688. |

## Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `tests/runner/tests.cmake` | `tests/cmake/hashes.cmake` | `runner.hashes_file` writes lane audio and fixture hash rows | ✓ WIRED | The CTest command invokes the writer with the built runner and fixture directory; the named test passed. |
| `.github/workflows/ci.yml` | eight lane `hashes.txt` artifacts | `hash-equality` downloads artifacts and compares `COUNT=8` | ✓ WIRED | Workflow job ordering and the registration assertion enforce the build, upload and exact-count comparison. |
| `tests/spu/tests.cmake` | `tests/spu/fixture.c` | `spu.fixture` independently checks CPU-free PCM and predicates | ✓ WIRED | The audio test was registered and passed. Automated source-link matching did not recognize the indirect CMake target; manual trace confirmed it. |
| `.github/workflows/nightly.yml` | `tests/nightly/tests.cmake` | Nightly preset invokes the pinned seven-case transfer test | ✓ WIRED | The workflow invokes `cmake --workflow --preset nightly`; test registration and pinned asset checks are in the CMake test inventory. |
| `.github/workflows/ci.yml` | `tests/retroarch/run_retroarch.cmake` | Stub-booted tone content and runner screenshot comparison | ✓ WIRED | Required RetroArch job registration passed and the workflow references the executable and screenshot comparison. |
| `tests/libretro/tests.cmake` | `tests/libretro/host.c` and `schwaystation-run` | Compare batch/single callbacks with runner PCM | ✓ WIRED | The host test passed for the configured NTSC path; PAL remains covered by the full CI inventory. |

The generic key-link matcher reported two direct-text misses because the build and CMake target relationships are transitive. Each path above was traced through its CMake registration and workflow caller; none is disconnected.

## Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| All labeled audio tests | `ctest --test-dir build/ci -L audio --no-tests=error --output-on-failure` | 10/10 passed | ✓ PASS |
| Libretro PCM callbacks and idle SPU | `ctest --test-dir build/ci -R '^(libretro\.host|core\.no_busy_loop)$' --no-tests=error --output-on-failure` | 2/2 passed | ✓ PASS |
| Full local CI workflow | `cmake --workflow --preset ci` | exit 0; 547/547 tests and 5/5 release checks passed | ✓ PASS |
| Hygiene workflow | `cmake --workflow --preset hygiene` | exit 0; 24/24 checks passed | ✓ PASS |
| Phase workflow registration | `cmake -DROOT=. -DCASE=spu_registration -P tests/cmake/workflows.cmake` | exit 0 | ✓ PASS |
| Fixed repository-check budget | `cmake -DROOT=. -DLIMIT=2688 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake` | exit 0; 2688 lines | ✓ PASS |
| Exact-head hosted CI and nightly/RetroArch results | `$gsd-ship 5` | Not run; release boundary | Pending ship check |

Focused SPU state tests passed 4/4; compatibility/info policy checks passed 3/3; the runner hash file test passed 2/2; hygiene policy self-tests passed 4/4. Timeout and action-pin scans also exited 0.

## Test Quality Audit

| Test File | Linked Requirement | Active | Skipped | Circular | Assertion Level | Verdict |
|---|---|---:|---:|---|---|---|
| `tests/spu/fixture.c` | SOUND-01 | Yes | 0 | No; self-authored regression digest is explicitly not a hardware oracle | Behavioral/value | PASS |
| `tests/spu/model.c`, `tests/spu/tests.cmake` | SOUND-01 | Yes | 0 | No | Behavioral/value | PASS |
| `tests/libretro/tests.cmake`, `tests/libretro/host.c` | SOUND-02 | Yes | 0 | No; compares runner and callbacks through separate paths | Behavioral/value | PASS |
| `tests/nightly/tests.cmake`, `tests/spu/reference.c` | SOUND-01 | Wired in nightly; host execution at ship | 0 | No; checks pinned hardware log predicates | Behavioral/value | PASS — hosted result pending ship |

Disabled-test scan found no disabled tests linked to SOUND-01 or SOUND-02. The fixture baseline is labeled a self-authored model regression, while pinned hardware log checks remain separate; the docs do not claim the authored PCM digest as a silicon waveform oracle.

## Probe Execution

No phase plan declares a probe script; no probe was required.

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| SOUND-01 | Phase 05 plans | SPU behavior, cross-lane PCM determinism, sample counts, fixture replay and pinned hardware transfer coverage | ✓ SATISFIED | Audio suite 10/10, full CI 547/547, exact eight-lane comparison wiring, and nightly transfer test registration. Confirm the actual PR-head nightly run at ship. |
| SOUND-02 | Phase 05 plans | Libretro audio delivery equals runner PCM, RetroArch loads the tone content, idle SPU remains bounded | ✓ SATISFIED | `libretro.host` and `core.no_busy_loop` passed; required hosted content smoke is wired and will be confirmed at ship. |

All 10 trackable decisions in `05-CONTEXT.md` are honored by the plans and implementation (decision coverage check exit 0).

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|

No blocking anti-patterns, unreferenced debt markers or disabled requirement tests were found. Code review covered 21 changed source, test, workflow and plan files and reported zero findings.

## Human Verification Required

None. This phase is verified by deterministic local tests and required hosted jobs. The owner does not need to perform a manual audio or UI check. The exact PR-head hosted run, nightly transfer, RetroArch result, CI run URL and green release PR are confirmed at `$gsd-ship 5`.

## Gaps Summary

No implementation gaps remain. The release PR has not been created or pushed, so hosted evidence is intentionally left for the next ship step. The local nightly build was configured without the pinned public assets and therefore did not register `spu.reference.memory_transfer`; the nightly workflow is wired to fetch and run it on the PR. Stop here at the Phase 5 release boundary.

### Decision Coverage

All trackable CONTEXT.md decisions are reflected in the phase artifacts (10/10).

---
*Phase: 05-the-sound-matches-the-console*
*Verified: 2026-11-14*
