---
phase: 05-the-sound-matches-the-console
plan: "05"
subsystem: audio
tags: [spu, dma4, fifo, reference-guest]
requires:
  - phase: 05-the-sound-matches-the-console
    provides: "SPU FIFO, RAM, scheduled writes, and pinned transfer assets from plans 05-04 and 05-20"
provides:
  - "DMA4 upload and readback through the SPU FIFO and actual SPU RAM"
  - "A CI-registered check for the first four pinned transfer cases"
affects: ["05-21", "05-11", "05-14", "05-18", "05-19"]
actuals:
  tokens: 7750
  tasks: 1
  commits: 3
commits: 3
plan_head_before: 95053c9241e782d60085864796470b445b2ec8a8
plan_head_after: ab509b20e6a796ae1f06eb91a40af99644f28d1f
tech-stack:
  added: []
  patterns:
    - "DMA4 consumes SPU chip readiness and data paths instead of the previous sink/source shims"
    - "The first pinned guest slice runs as a bounded audio-labeled CI integration test"
patterns-established:
  - "The SPU owns transfer readiness and data; DMA4 uses its chip interface for FIFO traffic."
key-files:
  created:
    - .planning/phases/05-the-sound-matches-the-console/05-21-PLAN.md
  modified:
    - src/dma.c
    - src/spu.c
    - tests/spu/reference.c
    - tests/fixtures/src/stub/stub.asm
    - tests/spu/tests.cmake
key-decisions:
  - "DMA4 readiness sees the latest queued ATTR write while the public status copy remains scheduled."
  - "The own stub supports only literal and %s output for the pinned transfer guest's A(3Fh) printf path."
  - "The manual FIFO drains at the scheduled transfer tick so the guest's bounded manual-write/readback cases complete."
requirements-completed: []
coverage:
  - id: D1
    description: "DMA4 transfers ordered words into SPU RAM and reads the values back through the chip path."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^dma\\.spu\\.(request_blocks|request_controls)$|^spu\\.model$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "The first four pinned manual and DMA transfer cases pass through the stub-booted guest."
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^spu\\.reference\\.first_upload$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: "44 min"
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 5: First DMA4 upload through SPU RAM

**DMA4 now moves a source block through the SPU FIFO into RAM, and the first four pinned guest transfer cases pass.**

## Performance

- **Duration:** 44 minutes; the 30-minute estimate was exceeded when the pinned guest exposed separate timing and mode-0 issues, now planned in 05-21.
- **Started:** Approximately 2026-11-14T00:43Z; the exact shell timestamp was not persisted.
- **Completed:** 2026-11-14T01:27Z.
- **Tasks:** 1
- **Files modified:** 13 implementation, test, and reference files.
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh.

## Accomplishments

- Replaced DMA4's data sink/source shims with chip FIFO writes, RAM readback, and readiness based on SPU transfer mode.
- Made queued ATTR changes visible to DMA scheduling while keeping the guest-visible status copy scheduled.
- Added a CI test for DTC readback, manual SPU RAM writes, DMA reads, and DMA writes using the pinned guest.
- Added the bounded stub printf subset the pinned guest uses to report its case results.
- Created plan 05-21 for DEV4 insertion, DMA timing and mode-0 behavior, removal of the bus ATTR mirror, and the complete seven-case nightly check. Dependent tone, state, PCM-equality, and documentation plans now wait for 05-21.

## Task Commits

1. **Task 1 RED:** `9357433` — `test(05-05): add failing DMA4 SPU RAM round trip checks`
2. **Task 1 GREEN:** `3e92bfb` — `feat(05-05): wire DMA4 to the SPU FIFO and guest`
3. **Task 1 CI registration:** `ab509b2` — `test(05-05): gate the initial pinned transfer guest`

The three task commits and plan head range above are measured from the persisted plan ledger. The final metadata commit follows this summary.

## Verification

- RED command: `ctest --test-dir build/ci -R '^(spu\.model|dma\.spu\.request_blocks)$' --no-tests=error --output-on-failure --output-junit "$PWD/build/ci/tdd/05-05-task1-red.xml"` — exit 8. Both named tests ran; the DMA assertion found destination SPU RAM still zero instead of the source low halfword.
- RED classifier: `gsd_run check tdd-red-evidence build/ci/tdd/05-05-task1-red.json --raw` — `RED_EVIDENCE_OK`. The target failed on the planned SPU RAM payload assertion; compilation and test discovery succeeded.
- Focused verification: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-spu schwaystation-test-spu-reference schwaystation-test-dma && ctest --test-dir build/ci -R '^spu\.(model|reference\.controls|reference\.first_upload)$|^dma\.spu\.(request_blocks|request_controls)$' --no-tests=error --output-on-failure` — exit 0; all five selected tests passed.
- The reference guest check stops after its first four planned cases. An exploratory full run showed later failures in DMA write/read timing and a sync-mode-0 case; those unchanged assertions are carried into 05-21. The seven-case check has not been claimed complete.
- Hosted verification was not required for this task.

## Files Created/Modified

- `src/spu.c`, `src/dma.c`, and `src/bus.c` — SPU DMA data path, request state, and ATTR scheduling order.
- `tests/spu/model.c`, `tests/dma/dma_tests.c` — DMA RAM round trip and readiness controls.
- `tests/spu/reference.c`, `tests/spu/tests.cmake`, `tests/spu/inventory.txt` — bounded first-four guest path registered in CI.
- `tests/fixtures/src/stub/stub.asm`, `src/stub_kernel.c`, `tests/fixtures/manifest.txt`, `PROVENANCE.md`, and `docs/hardware/cpu.md` — pinned stub formatter support and its generated identity/documentation.
- `.planning/phases/05-the-sound-matches-the-console/05-21-PLAN.md` — remaining DMA4 timing, DEV4, full-log, and nightly scope.

## TDD Gate Compliance

- **RED:** Commit `9357433`; the named test `dma.spu.request_blocks` failed because DMA4 left SPU RAM unchanged. The evidence file is `build/ci/tdd/05-05-task1-red.json`, and the classifier accepted it as `RED_EVIDENCE_OK`.
- **Semantic assessment:** The target test executed and failed at the intended RAM payload assertion. There was no compile, fixture, or discovery failure.
- **GREEN:** Commit `3e92bfb`; DMA4 writes and reads through the SPU interface. The focused model, DMA, and first-upload tests pass.
- **REFACTOR:** No separate refactor commit was needed.

## Deviations from Plan

**1. [Rule 1 - Bug] Fixed the reference runner setup and first-upload boundary.** `schway_create` requires allocator hooks, which the guest runner had omitted. The first-upload comparator also continued into later cases after its four-case target. Added the hooks and returned immediately after the fourth expected pass line. Verified by the registered `spu.reference.first_upload` test. Files: `tests/spu/reference.c`; commit `3e92bfb`.

**2. [Rule 2 - Missing critical functionality] Added the stub output path required by the pinned executable.** The guest uses BIOS A(3Fh) printf; the own stub only handled putchar, so no guest transcript was observable. Added a bounded literal/%s path and recorded the generated stub identity. Files: `tests/fixtures/src/stub/stub.asm`, `src/stub_kernel.c`, `tests/fixtures/manifest.txt`, `PROVENANCE.md`, and `docs/hardware/cpu.md`; commit `3e92bfb`.

**3. [Rule 1 - Bug] Corrected DTC readback and manual transfer completion.** The guest reads DTC immediately and checks that manual transfers finish within its bounded status poll. Reads now observe queued DTC state, and the FIFO drains at the next scheduled transfer tick. Updated the model assertion; the first four guest cases pass. Files: `src/spu.c`, `tests/spu/model.c`; commit `3e92bfb`.

**4. Split the remaining task at the plan timebox.** The real guest exposed additional DMA timing, mode-0, DEV4 insertion, mirror ownership, and full-log/nightly work. Per parent direction, preserved these assertions in plan 05-21 and made the affected downstream plans depend on it. No later timing or DEV4 assertion was weakened.

## User Setup Required

None. All verification is automated and agent-owned.

## Next Phase Readiness

The initial DMA4 upload/readback slice is complete. Plan 05-21 is the next work item for the remaining bidirectional timing and full seven-case nightly path. SOUND-01 and SOUND-02 remain Pending until Phase 5 verification.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

- The summary, narrowed 05-05 plan, and follow-up 05-21 plan exist.
- Task commits `9357433`, `3e92bfb`, and `ab509b2` are ancestors of the plan head.
- The three plan task commits are measured from `95053c9241e782d60085864796470b445b2ec8a8` through `ab509b20e6a796ae1f06eb91a40af99644f28d1f`.
