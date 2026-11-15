---
phase: 05-the-sound-matches-the-console
plan: "11"
subsystem: testing
tags: [spu, brr, armips, dma4, pcm, fixtures]
requires:
  - phase: 05-02
    provides: SPU registers, RAM, voice and scheduled sample stepping
  - phase: 05-20
    provides: stub-booted executable and guest mailbox support
provides:
  - Authored, stub-booted SPU tone fixture with PIO and DMA4 uploads
  - Regression checks for stereo PCM and frame-600 per-voice ENVX records
affects: [phase-05-plan-15, phase-05-plan-18, sound-fixtures]
actuals:
  tokens: 3337
  tasks: 2
  commits: 5
  plan_head_before: e7116fbf4d50b19811cd7867a2e2ba1f66044056
  plan_head_after: 824b59ca05e517f388b577c0d675900a7f31f41c
tech-stack:
  added: []
  patterns:
    - Integer-generated BRR fixture data assembled from authored source
    - Real guest mailbox records verified through the public execution path
key-files:
  created:
    - tests/fixtures/src/spu_tone.asm
    - tests/fixtures/src/spu_blocks.inc
  modified:
    - tests/fixtures/tests.cmake
    - tests/fixtures/manifest.txt
    - tests/spu/model.c
key-decisions:
  - "Keep fixture content authored and integer-generated; commit source and manifest digest, never the assembled executable."
  - "Use guest vblank counting and a sealed frame-600 mailbox to verify actual ENVX values without BIOS calls."
patterns-established:
  - "SPU fixture regression: assemble from source, run with the stub, and inspect PCM and guest mailbox through the public API."
requirements-completed: []
coverage:
  - id: D1
    description: "A stub-booted guest uploads authored BRR data through PIO and DMA4, keys two voices, and produces asymmetric stereo PCM."
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-fixtures schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\\.model$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "The guest records both voices' actual ENVX values at frame 600 and continues bounded output with exact sample counts."
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: "spu.model: 621 NTSC-U frames; exact per-frame sample counts, SPU RAM uploads, sealed mailbox frame 600 and ENVX values"
        status: pass
    human_judgment: false
  - id: D3
    description: "The guest draws deterministic own video; the runner produces the expected stable red and green stripe pattern."
    requirement: SOUND-01
    verification:
      - kind: other
        ref: "schwaystation-run --exe build/ci/fixtures/spu_tone.exe --frames 621 --hash-vram --png 621:build/ci/spu-tone-frame-621.png; exit 0, VRAM hash 12dfc274a765ab39ec929dc04e5d7eabd7f984a9ad78eb3e8573ed0173a8"
        status: pass
    human_judgment: false
duration: 27min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 11: Boot the authored SPU tone fixture Summary

**A first-party stub guest now exercises both SPU upload paths, emits stereo PCM, and records real frame-600 voice levels.**

## Performance

- **Duration:** 27 minutes
- **Started:** 2026-11-14T05:20:47Z
- **Completed:** 2026-11-14T05:47:19Z
- **Tasks:** 2
- **Files modified or created:** 5
- **Runtime/model:** Codex/OpenGSD 1.21.0; gpt-6-luna xhigh

## Accomplishments

- Added armips-authored, integer-generated BRR data and a stub-booted guest that uploads it by PIO and DMA4, keys two pitched voices, and produces nonzero asymmetric PCM.
- Added a sealed guest mailbox with actual ENVX readings at vblank 600; the guest continues through frame 620 and remains bounded.
- Verified exact sample counts over 621 NTSC-U frames and inspected the deterministic runner image showing red and green stripes.

## Task Commits

1. **Task 1 RED:** `ccd8a40` (test: assert guest SPU PCM)
2. **Task 1 GREEN:** `ef46187` (feat: trace authored BRR through tone guest)
3. **Task 2 RED:** `ac1a58f` (test: assert DMA and frame-600 ENVX)
4. **Task 2 GREEN:** `b712a8b` (feat: verify dual uploads and ENVX mailbox)
5. **Verified naming refactor:** `824b59c` (refactor: name deterministic stripe pattern accurately)

## Commands and Evidence

- **Task 1 RED:** `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK`; the focused test failed on the intended zero-PCM assertion, with no setup or harness failure.
- **Task 1 GREEN and tracer feedback gate:** configure, fixture/SPU test build, and focused `spu.model` CTest passed (1/1).
- **Task 2 RED:** `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK`; the guest ran without fault and failed the intended second-upload, voice-2, and frame-600 mailbox assertions.
- **Final source:** `cmake --preset ci && cmake --build --preset ci --target schwaystation-fixtures schwaystation-test-spu && ctest --test-dir build/ci -R '^spu\\.model$' --no-tests=error --output-on-failure` exited 0; CTest passed 1/1 in 4.57 seconds.
- **Runner visual check:** Command: schwaystation-run --exe build/ci/fixtures/spu_tone.exe --frames 621 --hash-vram --png 621:build/ci/spu-tone-frame-621.png; exit 0. Inspected output was a stable vertical red/green stripe pattern, with VRAM hash `12dfc274a765ab39ec929dc04e5d7eabd7f984a9ad78eb3e8573ed0173a8`.
- No hosted run applies to this plan. Plan 18 owns the independent CPU-free stream judgment.

## Files Created/Modified

- `tests/fixtures/src/spu_tone.asm` — guest initialization, PIO/DMA4 uploads, voice setup, video pattern, vblank count, and sealed mailbox.
- `tests/fixtures/src/spu_blocks.inc` — authored integer BRR block generation.
- `tests/fixtures/tests.cmake` — fixture target and include dependency.
- `tests/fixtures/manifest.txt` — source fixture digest.
- `tests/spu/model.c` — PCM, exact sample count, upload, voice, and mailbox assertions.

## Decisions Made

- Kept generated fixture bytes out of version control; the authored source and its manifest digest are the fixture provenance.
- Measured voice levels from the guest at frame 600 rather than inferring them from host audio output.
- Renamed the deterministic video helper to describe its observed stripe pattern accurately.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected SPU register offsets and queued-mode settling in the guest.**
- **Found during:** Task 1
- **Issue:** An initial guest draft addressed SPU MMIO without its `0x100` offset and polled status before the queued mode write had taken effect.
- **Fix:** Corrected the addresses and added a bounded guest delay before polling.
- **Files modified:** `tests/fixtures/src/spu_tone.asm`
- **Verification:** Focused guest PCM test and tracer feedback gate passed.
- **Committed in:** `ef46187`

**2. [Rule 3 - Blocking issue] Added local mailbox decoding helpers after the first test draft did not compile.**
- **Found during:** Task 2
- **Issue:** The new assertions referenced mailbox helpers not yet defined in the test file.
- **Fix:** Added small local little-endian word, record, and mailbox-seal helpers in the SPU model test.
- **Files modified:** `tests/spu/model.c`
- **Verification:** Task 2 produced valid RED evidence, then the focused final test passed.
- **Committed in:** `ac1a58f`

---

**Total deviations:** 2 auto-fixed (one Rule 1, one Rule 3)
**Impact on plan:** Both fixes were needed to execute the planned guest behavior and its test; scope and assertions were preserved.

## Issues Encountered

The output image showed stripes rather than a checkerboard, so the guest routine and comment were renamed to match the rendered result. No behavior or assertion was removed.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The authored fixture and focused regression are ready for later Phase 5 integration. SOUND-01 and SOUND-02 remain Pending; this plan does not complete either requirement. Plan 18 still owns the independent CPU-free stream judgment. No other plan, Phase 6, or shipping work was started.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-14*

## Self-Check: PASSED

- Summary file exists.
- All five task commits are ancestors of the current plan HEAD.
