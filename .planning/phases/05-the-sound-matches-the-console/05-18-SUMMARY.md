---
phase: 05-the-sound-matches-the-console
plan: "18"
subsystem: testing
tags:
  - spu
  - pcm
  - sha256
  - fixtures
requires:
  - phase: 05-the-sound-matches-the-console
    provides:
      - Sample-clock SPU registers, RAM, voices and event scheduling
      - Stub-booted tone guest and public PCM drain
provides:
  - CPU-free replay of the authored BRR table and SPU register events
  - Per-region complete PCM and frame-600 mailbox regression expectations
affects:
  - Phase 5 verification
  - SPU fixture coverage
actuals:
  tokens: 8855.25
  tasks: 2
  commits: 4
  plan_head_before: 8afb19b452d8a4d07b1d6a30b430af716892f495
  plan_head_after: ed528d3340b9f0944b30205df8d47d04ef91e023
tech-stack:
  added: []
  patterns:
    - Independent little-endian PCM SHA-256 over every stereo pair
    - Source-driven CPU-free SPU event replay compared with guest output
key-files:
  created:
    - tests/spu/expected.txt
  modified:
    - tests/spu/fixture.c
    - tests/spu/tests.cmake
    - tests/spu/inventory.txt
    - tests/diag/expected.csv
key-decisions:
  - "Store frame-600 PCM lengths and digests as self-authored regression baselines, not hardware waveform claims."
  - "Require a sealed three-record mailbox and compare both ENVX values and the guest VBlank count."
patterns-established:
  - "Read authored BRR blocks and register writes from source files instead of duplicating fixture values."
  - "Keep guest setup silence and sample-tick ordering in CPU-free comparisons."
requirements-completed: []
coverage:
  - id: D1
    description: CPU-free authored-table replay matches the complete guest PCM stream in NTSC-J, NTSC-U and PAL.
    requirement: SOUND-01
    verification:
      kind: unit
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-fixture && ctest --test-dir build/ci -R '^spu\.fixture$' --no-tests=error --output-on-failure"
      status: pass
    human_judgment: false
  - id: D2
    description: Frame-600 audio length/digest and sealed mailbox ENVX records reject wrong, empty, truncated and incomplete results.
    requirement: SOUND-01
    verification:
      kind: unit
      ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-fixture && ctest --test-dir build/ci -R '^spu\.fixture$' --no-tests=error --output-on-failure"
      status: pass
    human_judgment: false
duration: "at least 36 min"
completed: 2026-11-14
status: complete
---

# Phase 05 Plan 18: CPU-free SPU frame-600 expectations Summary

**CPU-free SPU replay matches the complete guest PCM stream in NTSC-J/U/PAL and freezes per-region audio and ENVX regression expectations.**

## Performance

- **Duration:** At least 36 minutes from the persisted plan baseline through metadata closeout; exact executor start time was not retained across continuation.
- **Started:** 2026-11-14T08:38:59Z (persisted plan baseline ledger; executor start may have been earlier)
- **Completed:** 2026-11-14T09:15:54Z (summary and state closeout)
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Added a CPU-free replay that reads the authored two-block BRR table and SPU register event source, preserves reset/setup silence, and matches every guest PCM pair at the region-specific guest VBlank 600 boundary.
- Added frame-600 audio pair counts and SHA-256 baselines for NTSC-J, NTSC-U and PAL; the byte stream is signed PCM pairs serialized little-endian.
- Added independently judged mailbox ENVX/VBlank records and negative controls for wrong digests, zero or truncated PCM, wrong ENVX, incomplete expectations, unsealed mailboxes and missing records.
- Labeled all tone audio and ENVX baselines as self-authored model regression evidence, not captured hardware waveforms.

## Task Commits

1. **Task 1 RED:** 3e4669e — CPU-free equality assertion failed before replay existed.
2. **Task 1 GREEN:** b4b561b — authored table replay matches guest PCM in all three profiles.
3. **Task 2 RED:** 666bfc8 — expectation input assertion failed because the audio baseline had not yet been added.
4. **Task 2 GREEN:** ed528d3 — frozen audio and mailbox expectations pass with rejection controls.

## Files Created/Modified

- tests/spu/fixture.c — guest capture, source parsing, CPU-free replay, full-stream hash and sealed mailbox checks.
- tests/spu/tests.cmake — fixture registration and expectation file paths.
- tests/spu/inventory.txt — exact audio/portable test inventory entry.
- tests/spu/expected.txt — per-region guest frame count, PCM pair count and SHA-256.
- tests/diag/expected.csv — per-region voice ENVX and guest VBlank expectations.

## Decisions Made

- Hash every complete PCM stream as little-endian stereo pairs and retain the host-frame/pair counts so truncation cannot pass by digest comparison alone.
- Validate mailbox magic, done status, protocol version, fixture id, record ids/flags, count and CRC before comparing the two ENVX values and guest frame count.
- Preserve SOUND-01 as pending until final Phase 5 verification, as the phase-level evidence is not complete in this plan.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected the independent scalar BRR sample expectation**

- **Found during:** Task 1, GREEN implementation.
- **Issue:** The first draft of the independent reference helper used the wrong header nibble and packed-sample order, so it expected 4096/16384 instead of the authored blocks' decoded first samples, 512/1280.
- **Fix:** Read shift from the low header nibble, filter from the high header bits and the first packed sample from the low data nibble.
- **Files modified:** tests/spu/fixture.c.
- **Verification:** The exact focused spu.fixture command passed for all three regions.
- **Committed in:** b4b561b.

**2. [Rule 3 - Blocking issue] Passed expected-file paths to the registered fixture**

- **Found during:** Task 2, RED.
- **Issue:** Task 2 required parsing tests/spu/expected.txt and tests/diag/expected.csv, but its file list omitted the CTest command wiring needed to locate those source files.
- **Fix:** Added both absolute source paths as fixture arguments.
- **Files modified:** tests/spu/tests.cmake.
- **Verification:** The exact focused command failed on the intended missing-audio-expectation TAP assertion in RED and passed after the files and parsers were complete.
- **Committed in:** 666bfc8.

**Total deviations:** 2 auto-fixed (one test-reference bug, one missing test-path wiring).
**Impact on plan:** Both changes were required to make the independent checks reliable; no product behavior or phase scope was added.

## TDD Gate Compliance

| Task | RED evidence | GREEN evidence | Result |
| --- | --- | --- | --- |
| 1 | spu.fixture failed on the intended complete CPU-free PCM equality assertion; RED classifier returned RED_EVIDENCE_OK. | Focused command exit 0; 1/1 selected test passed. | Pass |
| 2 | spu.fixture failed on the intended missing audio@600 expectation assertion; direct TAP run exit 1 and RED classifier returned RED_EVIDENCE_OK. | Focused command exit 0; 1/1 selected test passed. | Pass |

## Commands and Evidence

Task 1 RED focused command exited 8 in CTest; the CPU-free equality assertion was the intended failure. Task 1 GREEN exited 0 and passed 1/1.

```sh
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-fixture && ctest --test-dir build/ci -R '^spu\.fixture$' --no-tests=error --output-on-failure
```

Task 2 RED used the same focused CTest command and exited 8 on the intended missing expectation assertion. Its direct TAP report exited 1 and was accepted by the RED evidence classifier as RED_EVIDENCE_OK. Task 2 GREEN exited 0 and passed 1/1 in 13.05 seconds.

```sh
build/ci/tests/schwaystation-test-spu-fixture build/ci/fixtures/spu_tone.exe tests/fixtures/src/spu_blocks.inc tests/fixtures/src/spu_tone.asm build/ci/schwaystation-run tests/spu/expected.txt tests/diag/expected.csv
```

```sh
cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu-fixture && ctest --test-dir build/ci -R '^spu\.fixture$' --no-tests=error --output-on-failure
```

No hosted run was required or applicable to this local deterministic test plan.

## Issues Encountered

The first build after adding expectation parsing found a strict-warning initializer error in the test fixture; replacing the aggregate initializer with explicit zero-initialization fixed it. No product-code defect was found.

## User Setup Required

None.

## Next Phase Readiness

Plan 05-18 is complete. SOUND-01 and SOUND-02 remain pending until final Phase 5 verification. No Phase 5 ship or Phase 6 work was started.

## Self-Check: PASSED

The summary file exists, and all four task commits are ancestors of HEAD.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*
