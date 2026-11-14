---
phase: 05-the-sound-matches-the-console
plan: "23"
subsystem: audio
tags: [spu, dma4, cmake, nightly]
requires:
  - phase: 05-the-sound-matches-the-console
    provides: "SPU-owned DMA4 readiness and the bounded seven-case transfer guest comparator from plan 05-21"
provides:
  - "SPU ATTR is the single owner and serializer used by DMA4 readiness"
  - "Nightly registers the full pinned seven-case SPU memory-transfer guest"
affects: [phase-05-verification]
actuals:
  tokens: 523
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - "DMA4 readiness is queried from SPU current or pending ATTR state"
    - "Nightly reuses the pinned SPU asset helpers without requiring CPU vectors"
key-files:
  created: []
  modified:
    - src/bus.h
    - src/bus.c
    - src/dma.c
    - tests/bus/bus_tests.c
    - tests/dma/dma_tests.c
    - tests/nightly/tests.cmake
    - tests/nightly/inventory.txt
key-decisions: []
requirements-completed: []
coverage:
  - id: D1
    description: "DMA4 readiness and bus state use SPU-owned ATTR, with no duplicate bus reset or serialized field."
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-bus schwaystation-test-dma schwaystation-test-spu && ctest --test-dir build/ci -R '^(bus\\.|dma\\.spu\\.|spu\\.model$)' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "Nightly runs the full pinned SPU transfer guest and preserves the existing GPU registrations."
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: "cmake -S . -B build/spu-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_GPU_REFERENCE_ONLY=ON -DSCHWAYSTATION_BUILD_LIBRETRO=OFF -DSCHWAYSTATION_FIXTURES=OFF && cmake --build build/spu-reference --target schwaystation-test-spu-reference && ctest --test-dir build/spu-reference -R '^spu\\.reference\\.memory_transfer$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
commits: 2
plan_head_before: 2e829420807cfb437aeecf41ba141b2dae73b543
plan_head_after: 066131917c1d1342869c62f4cfb824d9e611c696
duration: 12 min
completed: 2026-11-14
status: complete
---

# Phase 5 Plan 23: SPU ATTR Ownership and Nightly Transfer Summary

**The SPU now owns ATTR state used by DMA4 readiness, and nightly checks all seven pinned memory-transfer cases without CPU vectors.**

## Performance

- **Duration:** 12 minutes
- **Started:** 2026-11-14T04:00:13Z
- **Completed:** 2026-11-14T04:12:27Z
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex / OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Removed the bus ATTR mirror from the bus struct, reset path, save-state visitor, and SPU-write path. DMA4 readiness and bus/DMA assertions now use SPU-owned state.
- Registered `spu.reference.memory_transfer` in nightly using the full guest comparator and the existing pinned SPU asset helpers. The inventory adds one byte-sorted row; every GPU nightly row remains present.
- `SCHWAYSTATION_GPU_REFERENCE_ONLY=ON` configured successfully with no CPU vector blobs fetched.

## Task Commits

1. **Task 1: Remove the duplicate bus ATTR mirror** - `0f02143` (`refactor`)
2. **Task 2: Register the full transfer guest in nightly** - `0661319` (`test`)

## Files Modified

- `src/bus.h`, `src/bus.c` - Removed duplicate ATTR storage, reset, state serialization, and bus-write mirroring.
- `src/dma.c` - Updated the readiness comment to describe SPU-owned current and pending ATTR.
- `tests/bus/bus_tests.c`, `tests/dma/dma_tests.c` - Assert through the SPU DMA-readiness interface.
- `tests/nightly/tests.cmake`, `tests/nightly/inventory.txt` - Added the full transfer guest registration and inventory entry.

## Decisions Made

None - followed the settled SPU ATTR ownership and nightly test decisions.

## TDD Notes

The plan uses `type: execute` with automatic tasks. Existing assertions were updated before the source cleanup. This refactor preserves observable behavior, and the CTest registration is configuration; neither has an honest behavior-failing pre-change assertion. No RED was claimed. The first Task 1 run used `schway__spu_reg_peek` for a pending ATTR value and failed; the check was corrected to use `schway__spu_dma_ready`, which observes the SPU's pending value.

## Commands and Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-bus schwaystation-test-dma schwaystation-test-spu && ctest --test-dir build/ci -R '^(bus\.|dma\.spu\.|spu\.model$)' --no-tests=error --output-on-failure` - final run exit 0; all 23 selected tests passed.
- `cmake -S . -B build/spu-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_GPU_REFERENCE_ONLY=ON -DSCHWAYSTATION_BUILD_LIBRETRO=OFF -DSCHWAYSTATION_FIXTURES=OFF && cmake --build build/spu-reference --target schwaystation-test-spu-reference && ctest --test-dir build/spu-reference -R '^spu\.reference\.memory_transfer$' --no-tests=error --output-on-failure` - exit 0; the full seven-case nightly guest passed 1/1.
- `ctest -N -R '^gpu\.reference\.'` listed all 13 existing GPU registrations; the nightly inventory diff adds only the SPU row.
- `git diff --check` - exit 0.

## Issues Encountered

The initial Task 1 assertion read committed ATTR instead of pending ATTR, so three CTest cases failed across four assertions. After switching those checks to the SPU readiness interface, the exact focused sequence passed. No implementation defect remained.

## Deviations from Plan

None. The adjacent `src/dma.c` comment was updated with the ownership change because it named the removed bus mirror.

## Requirements Status

SOUND-01 and SOUND-02 remain Pending in `REQUIREMENTS.md`; this plan does not complete either phase-level requirement.

## Plan Boundary

Plan 05-22 and all later plans remain unstarted in this execution. No Phase 6 work or shipping was started.

## User Setup Required

None.

## Self-Check: PASSED

- `05-23-SUMMARY.md` exists at the required phase path.
- Task commits `0f02143` and `0661319` are present in `HEAD` ancestry.
