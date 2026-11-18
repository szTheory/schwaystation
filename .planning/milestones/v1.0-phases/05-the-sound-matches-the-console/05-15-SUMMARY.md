---
phase: 05-the-sound-matches-the-console
plan: "15"
subsystem: SPU behavior and hosted acceptance wiring
tags:
  - SPU
  - PCM
  - deterministic-hashes
  - CI
requires:
  - phase: 05-the-sound-matches-the-console
    provides:
      - SPU event, state, voice and PCM paths
provides:
  - CPU-boundary SPU catchup and instance-owned state serialization scratch
  - Eight-lane audio hash and mailbox comparisons plus required hosted tone smoke wiring
  - Updated run, DMA, fixture, workflow and hygiene checks
affects:
  - Phase 5 verification and ship
  - Hosted audio acceptance
actuals:
  tasks: 2
  commits: 1
tech-stack:
  added: []
  patterns:
    - Keep SPU scratch in instance state so large state operations avoid platform stack probes.
    - Make state validation read-only; decode into scratch only in write and visit modes.
    - Reuse counted CMake check helpers while preserving the fixed repository-check ceiling.
key-files:
  created:
    - .planning/phases/05-the-sound-matches-the-console/05-15-SUMMARY.md
  modified:
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
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
key-decisions:
  - Catch up the SPU at CPU instruction and public run boundaries without adding host audio-device code.
  - Keep state validation failure-atomic and avoid stack-resident arrays proportional to SPU RAM.
  - Keep the RetroArch tone smoke video-only; PCM equality is established by the deterministic runner and lane hash checks.
requirements-completed:
  - SOUND-01
  - SOUND-02
coverage:
  - id: P15-AUDIO
    description: Eight workflow lanes compare the frame-600 tone PCM and mailbox evidence; runner hash and fixture cases reject missing, changed, truncated or empty output.
    requirement: SOUND-01
    verification:
      kind: integration
      ref: "cmake --workflow --preset ci (exit 0; 547/547 tests and 5/5 release package tests)"
      status: pass
    human_judgment: false
  - id: P15-HOST
    description: Required workflow wiring boots the tone executable under the stub and keeps the recurring RetroArch check video-only; nightly transfer wiring is registered.
    requirement: SOUND-02
    verification:
      kind: other
      ref: "cmake -DROOT=. -DCASE=spu_registration -P tests/cmake/workflows.cmake (exit 0); hosted run is pending ship"
      status: pass
    human_judgment: false
  - id: P15-STATE
    description: SPU state validation remains read-only and staged state operations use per-instance scratch; affected run, DMA, GPU, card and state paths pass.
    requirement: SOUND-01
    verification:
      kind: unit
      ref: "cmake --build --preset ci --target schwaystation-test-gpu-beam schwaystation-test-card schwaystation-test-state schwaystation-test-run && ctest --test-dir build/ci -R '^(gpu\\.beam\\.reject_malformed_state|card\\.state\\.(import_atomic|pending_edges)|core\\.run\\.budget_resume)$' --no-tests=error --output-on-failure (exit 0; 4/4)"
      status: pass
    human_judgment: false
  - id: P15-GATES
    description: The repository check budget remains exactly at the fixed 2688-line ceiling, and hygiene passes with the final inventory and workflow checks.
    requirement: SOUND-01
    verification:
      kind: other
      ref: "cmake --workflow --preset hygiene (exit 0; 24/24); line-budget check (exit 0; 2688 lines)"
      status: pass
    human_judgment: false
duration: "about 2 hours 30 minutes wall across restart; exact active time unavailable"
completed: 2026-11-14
status: complete
---

# Phase 05 Plan 15: Audio acceptance and hosted tone wiring Summary

**The reviewed SPU path and recurring audio acceptance checks are complete locally; actual hosted PR execution remains for ship.**

## Performance

- **Duration:** About 2 hours 30 minutes wall across restart; exact active time unavailable.
- **Tasks:** 2.
- **Files modified:** 19 implementation, test, documentation and workflow files.
- **Runtime/model:** Codex, OpenGSD 1.21.0; GPT-6 session (exact model ID not exposed); configured executor profile gpt-6-luna xhigh.

## Accomplishments

- Added SPU catchup at CPU instruction and public run boundaries and moved state serialization scratch into the SPU instance. Validation remains failure-atomic.
- Wired frame-600 tone PCM and mailbox evidence into eight lane artifacts, retained independent fixture negative controls, and registered the required video-only RetroArch tone smoke and nightly transfer coverage.
- Reused existing CMake helpers to preserve checks within the fixed 2688-line budget.
- Fixed CI regressions in synthetic state timing, DREQ test setup, fixture channel selection, lane hash expectations and the embed target allowlist.

## Checks Run

- `cmake -DROOT=. -DCASE=spu_registration -P tests/cmake/workflows.cmake` — exit 0.
- `cmake --workflow --preset ci` — exit 0; 547/547 tests passed and 5/5 release package tests passed.
- `cmake --workflow --preset hygiene` — exit 0; 24/24 checks passed.
- `cmake -DROOT=. -DLIMIT=2688 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake && cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake && cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake` — exit 0; 2688 lines, 24 timeout entries and 50 pinned action entries passed.
- Focused SPU state, compatibility, runner audio-hash and hygiene self-tests passed; see this plan's coverage entries and phase review for commands and outcomes.
- Code review: `.planning/phases/05-the-sound-matches-the-console/05-REVIEW.md` — clean, zero findings.
- No hosted PR run URL is recorded; the release branch has not been pushed.

## Deviations

- State validation initially exposed a large stack frame on Darwin; moving the scratch into the per-instance SPU state resolved it without changing the validation contract.
- The first CI attempts exposed synthetic timing, DMA request setup, fixture channel and allowlist mismatches. Those were corrected and the final full local workflows passed.

## Next Phase Readiness

- Phase 5 verification passed with no gaps or owner checks. Stop at the release boundary; `$gsd-ship 5` confirms the exact PR-head hosted run and its result.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*
