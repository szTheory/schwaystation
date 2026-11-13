---
phase: 04-games-play-in-retroarch
plan: "30"
subsystem: testing
tags: [gpu, vram, png, runner, ctest]
requires:
  - phase: 04-12
    provides: Read-only access to the instance's native VRAM
  - phase: 04-40
    provides: Runner PNG writing and SHA-256 support
provides:
  - Canonical whole-VRAM SHA-256 through the runner
  - Synthetic full-frame PNG comparison with registered edge controls
affects: [04-46]
actuals:
  tokens: 4702
  tasks: 2
  commits: 3
  plan_head_before: ab46cabfcc9a5aa3b63b54b51e6d697c6faadd9c
  plan_head_after: 75049e6fe86c7b698e4d3643e62d9fd4938d235d
tech-stack:
  added: []
  patterns:
    - Serialize canonical VRAM words explicitly as row-major little-endian RGB555 with bit 15 cleared
    - Compare every synthetic PNG pixel after independent per-channel five-bit quantization
key-files:
  created: []
  modified:
    - runner/main.c
    - tests/gpu/reference.c
    - tests/gpu/tests.cmake
    - tests/gpu/inventory.txt
    - docs/runner.md
key-decisions:
  - Hash all 1024x512 VRAM words after clearing bit 15, with each word serialized low byte first.
  - Keep the reference decoder bounded to the runner's RGB8, filter-0, stored-DEFLATE synthetic PNGs; captured formats remain a downstream extension.
patterns-established:
  - Use a fixed independent full-frame known answer and zero-tolerance RGB555 quantization for the reference comparison.
requirements-completed: []
coverage:
  - id: D1
    description: The runner reports a canonical whole-VRAM hash and rejects duplicate or incompatible options.
    requirement: GAME-01
    verification:
      - kind: integration
        ref: tests/gpu/tests.cmake#gpu.reference.cli
        status: pass
    human_judgment: false
  - id: D2
    description: The synthetic reference comparison checks all pixels and controls channel order, quantization, bit-15 masking, one-pixel changes, dimensions, and missing output.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: tests/gpu/tests.cmake#gpu.reference.quantization
        status: pass
      - kind: unit
        ref: tests/gpu/tests.cmake#gpu.reference.controls
        status: pass
    human_judgment: false
duration: "at least 19 min; the recovered executor start time was unavailable"
completed: 2026-11-12
status: complete
---

# Phase 4 Plan 30: Canonical GPU Reference Summary

**The runner now hashes the complete canonical VRAM domain, and an independent synthetic PNG judge checks every RGB555 pixel plus its invalid-input controls.**

## Performance

- **Duration:** At least 19 minutes; the executor began before the earliest retained task evidence, and its start timestamp was unavailable after recovery.
- **Started:** Before 2026-11-12T22:52:51Z; exact time unavailable.
- **Completed:** 2026-11-12T23:12:19Z.
- **Runtime/model:** Codex / OpenGSD 1.20.0, gpt-6-luna xhigh, sequential shared phase checkout.
- **Tasks:** 2.
- **Files modified:** 5.

## Accomplishments

- Added `--hash-vram`, which hashes all 1024x512 VRAM words in row-major order after clearing bit 15 and serializing each word low byte first. Its expected frame-1 test-card digest is fixed independently of observed output.
- Added a full-frame synthetic PNG comparison at zero RGB555 bucket tolerance, with a fixed primary-color order known answer and controls for quantization, mask-bit independence, a one-pixel bucket change, wrong dimensions, and missing image data.
- Registered the CLI, quantization, and controls tests in ordinary CI and added the runner hash domain to `docs/runner.md`.

## Task Commits

1. **Task 1 RED:** `0d4647b` — registered the failing canonical VRAM CLI known-answer test.
2. **Task 1 GREEN:** `fa3c71f` — implemented `--hash-vram` and the synthetic full-frame judge.
3. **Task 2:** `75049e6` — added controls, exact inventory registration, and hash-domain documentation.

## Files Created/Modified

- `runner/main.c` — parses and prints the canonical whole-VRAM digest.
- `tests/gpu/reference.c` — independent synthetic PNG reader, full-frame comparator, and color/invalid-input controls.
- `tests/gpu/tests.cmake` and `tests/gpu/inventory.txt` — register the new gating reference checks and exact inventory entries.
- `docs/runner.md` — documents the hash output and its stable byte domain.

## Decisions Made

- Keep bit 15 outside the hash and color-compare domains because it is a mask flag rather than an RGB channel.
- Keep this judge limited to a small, bounded synthetic PNG subset; it makes no claim that the decoder accepts captured images.

## TDD Gate Compliance

The RED command `ctest --test-dir build/ci -R '^gpu\.reference\.cli$' --no-tests=error --output-on-failure --output-junit build/ci/gpu-reference-red.xml` exited 8 because the pre-change runner rejected `--hash-vram` with its usage error. The captured target failure was classified as `RED_EVIDENCE_OK` (`target_test_failed`); semantic inspection confirmed that the assertion expected the independent canonical digest. RED commit: `0d4647b`.

The GREEN tracer rebuilt `schwaystation-run` and `schwaystation-test-gpu-reference`, then ran `gpu.reference.quantization` and `gpu.reference.cli`; both passed. The passing tracer was rerun after the GREEN commit before controls were expanded. Task 2 was a regression expansion and was not falsely reported as RED.

## Evidence

- Task 2 focused verification: `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-test-gpu-reference --parallel 4 && ctest --test-dir build/ci -R '^gpu\.reference\.controls$' --no-tests=error --output-on-failure --parallel 4` — exit 0; the registered controls case passed.
- Full local CI: `cmake --workflow --preset ci` — exit 0; all 512 tests and all 5 release archive checks passed.
- Full local hygiene: `cmake --workflow --preset hygiene` — exit 0; all 23 checks passed.
- The new CLI/reference tests also passed in the full CI workflow. No hosted run was launched for these new sources, and the prior immutable hosted run does not verify this head.

## Issues Encountered

- The first CLI output regex had an extra escape and rejected the expected line. It was corrected and the focused CLI and tracer checks passed before task 2 began.

## Deviations from Plan

None — the two planned tasks were completed. The decoder stays synthetic-only as scoped; this is not a claim of captured-image support.

## Next Phase Readiness

Inspection of the pinned build-158 capture archive found `gpu/clipping/vram.png` uses a 4-bit indexed PNG with dynamic DEFLATE. The synthetic reader intentionally does not decode that format. Plan 04-46 needs to adapt or extend the decoder before running its captured clipping oracle; success here does not establish captured-oracle support.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-12*

## Self-Check: PASSED

The summary file exists and all three task commits are ancestors of the current plan head.
