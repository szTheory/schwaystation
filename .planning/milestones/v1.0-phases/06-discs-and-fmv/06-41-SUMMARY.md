---
phase: 06-discs-and-fmv
plan: "41"
subsystem: testing
tags: [mdec, input-preflight, regression, reference]
requires: ["06-40"]
provides:
  - "Bounded complete-mono-input guard before the pinned model guest executes."
  - "Separate 64-byte deterministic 8-bit model and pinned-reference regression comparator."
affects: ["DISC-03 evidence", "Phase 6 plans 06-42 through 06-45"]
tech-stack:
  added: []
  patterns:
    - "Keep hardware bytes as evidence and implementation regression bytes in a separately named baseline."
    - "Preflight compressed input structure before executing a pinned model guest."
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-41-SUMMARY.md
  modified:
    - tests/mdec/input_coverage.h
    - tests/mdec/model.c
    - tests/mdec/tests.cmake
    - tests/mdec/inventory.txt
    - tests/mdec/reference.c
    - .planning/ROADMAP.md
    - .planning/STATE.md
key-decisions:
  - "Preserve the exact pinned 4-bit and original 8-bit hardware values; use a separate deterministic baseline for current 8-bit implementation output."
  - "Do not run compressed pinned reference guests until Plan 06-42 installs the in-process source guard; nightly selector registration remains with 06-43."
actuals:
  tokens: 5401
  tasks: 2
  commits: 7
commits: 7
plan_head_before: e5b249a7ac5b178fc77043020a5e4bacf37fba9d
plan_head_after: 8a0f4573f28fa7dbd851f0e01c079517c36e8155
duration: 36min
completed: 2026-11-17
status: complete
requirements-completed: []
coverage:
  - id: MDEC-INPUT-PREFLIGHT
    description: "The test-only halfword scan gates the pinned mono model input and distinguishes complete, truncated, odd-sized, and empty input spans."
    requirement: DISC-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure (exit 0; 12/12 tests)"
      status: pass
    human_judgment: false
  - id: MDEC-8BIT-MODEL-REGRESSION
    description: "The unchanged integer decoder matches all 64 frozen regression bytes through DMA0 input, ready FIFO output, and scheduled DMA1 RAM; split execution and a one-byte negative control are covered."
    requirement: DISC-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mdec[.]guest_8bit_regression$' --no-tests=error --output-on-failure (exit 0; 1/1); full MDEC label (exit 0; 12/12)"
      status: pass
    human_judgment: false
  - id: MDEC-8BIT-REFERENCE-COMPARATOR
    description: "The pinned-reference comparator maps the new mode to the existing 8-bit guest/log identity, validates the original hardware log, and rejects a one-byte regression mutation without running a guest."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/mdec-reference -R '^mdec[.]reference[.]controls$' --no-tests=error --output-on-failure (exit 0; 1/1; no compressed guest executed)"
      status: pass
    human_judgment: false
  - id: MDEC-EXTERNAL-GUEST-GATES
    description: "Compressed pinned reference guests and nightly selector registration remain pending until the source guard and registration plans are complete."
    requirement: DISC-03
    verification:
      kind: other
      ref: "Plan 06-42 installs the in-process input guard; Plan 06-43 owns selector registration. No external compressed guest was run in 06-41."
      status: pending
    human_judgment: false
---

# Phase 6 Plan 06-41: Add the bounded MDEC 8-bit regression Summary

**The MDEC model now keeps the pinned hardware bytes intact while checking a separate 64-byte deterministic 8-bit regression through the scheduled DMA path.**

## Performance

- **Duration:** 35m31s wall time from the 2026-11-17T18:46:31Z execution marker through closeout preparation at 2026-11-17T19:22:02Z; active time was not measured.
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh.
- **Tasks:** 2.
- **Files modified:** 5 implementation/test files, plus roadmap and state bookkeeping.
- **Commits:** 7 measured from `e5b249a7ac5b178fc77043020a5e4bacf37fba9d` through `8a0f4573f28fa7dbd851f0e01c079517c36e8155`; changed-source diff measured 21,605 characters (5,401 tokens at chars/4).

## Accomplishments

- Added a bounded little-endian halfword scanner and gate before the pinned model input executes. The complete mono control is 128 bytes: one DC plus 63 AC halfwords. Truncated and malformed controls remain covered.
- Kept `expected[2][16]`, including both original hardware arrays and their cross-mode relation, unchanged. The separate regression baseline records all 64 8-bit implementation bytes; its provenance states that the earlier exit-8 CTest output supplied 12 differing words and the other four were exact matches retained from the old hardware array.
- Replaced `mdec.guest_input` with `mdec.guest_8bit_regression`, retaining exact 4-bit coverage. The new model case checks DMA0 consumption, readiness/count, all raw FIFO bytes, DMA1 RAM bytes and final MADR, fresh whole/split runs against the fixed baseline, and a one-byte mutation.
- Added an `8bit-regression` reference mode using the pinned 8-bit executable and log identities. It still parses and digest-checks the original hardware log but compares emulated DMA1 RAM to the separate regression baseline. No compressed reference guest ran in this plan.

## TDD Evidence

- **Input coverage RED:** `ctest --test-dir build/ci -R '^mdec[.]input_coverage$' --no-tests=error --output-failure --output-junit build/ci/mdec-41-task1-red.xml` exited 8 on the intended incomplete coverage assertions; the corrected 128-byte complete fixture and two-byte truncated fixture are recorded in `build/ci/mdec-41-task1-red-evidence.json`. `gsd_run check tdd-red-evidence build/ci/mdec-41-task1-red-evidence.json --raw` returned `RED_EVIDENCE_OK` (exit 0). The first draft used 64 bytes; after review caught the halfword/byte mismatch, the fixture was corrected and the accepted RED run used 128 bytes.
- **8-bit selector RED:** `ctest --test-dir build/ci -R '^mdec[.]guest_8bit_regression$' --no-tests=error --output-junit "$PWD/build/ci/06-41-task1-selector-red.xml" --output-on-failure` exited 8 on 12 output-word assertions against the original hardware array. This corresponds to 16 differing pixels: 15 by one 8-bit level and one by two. `gsd_run check tdd-red-evidence build/ci/06-41-task1-selector-red-evidence.json --raw` returned `RED_EVIDENCE_OK` (exit 0). The original report remains in ignored build storage.
- **Reference identity RED:** `ctest --test-dir build/mdec-reference -R '^mdec[.]reference[.]controls$' --no-tests=error --output-junit "$PWD/build/mdec-reference/06-41-task2-red.xml" --output-on-failure` exited 8 because the not-yet-added mode resolved to status identities. The registered control ran without executing a compressed guest. `gsd_run check tdd-red-evidence build/mdec-reference/06-41-task2-red-evidence.json --raw` returned `RED_EVIDENCE_OK` (exit 0).

## Verification Evidence

| Command | Exit | Runtime | Result |
|---|---:|---:|---|
| `LC_ALL=C sort -c tests/mdec/inventory.txt && git diff --check origin/main -- tests/mdec/input_coverage.h tests/mdec/model.c tests/mdec/tests.cmake tests/mdec/inventory.txt` | 0 | under 0.1s | Sorted inventory and scoped whitespace preflight passed. |
| `cmake --preset ci` | 0 | 0.70s | Refreshed the local CI build. |
| `cmake --build --preset ci --target schwaystation-test-mdec` | 0 | 0.25s | MDEC model target compiled and linked. |
| `ctest --test-dir build/ci -R '^(mdec[.]guest_8bit_regression|mdec[.]guest_4bit|mdec[.]input_coverage)$' --no-tests=error --output-on-failure` | 0 | 0.02s | All three focused model tests passed. |
| `ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure` | 0 | 0.08s | All 12 main-CI MDEC model tests passed. |
| `cmake -S . -B build/mdec-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec` | 0 | 1.17s | Configured the isolated pinned-reference build. |
| `cmake --build build/mdec-reference --target schwaystation-test-mdec-reference` | 0 | 0.08s | Compiled the reference comparator after the source change. |
| `ctest --test-dir build/mdec-reference -R '^mdec[.]reference[.]controls$' --no-tests=error --output-on-failure` | 0 | 0.01s | The no-emulation comparator/log mutation control passed 1/1. |
| `git diff --check origin/main -- tests/mdec/reference.c` | 0 | under 0.1s | Scoped reference source whitespace check passed. |

No hosted run URL applies. The pinned compressed guests were not run: Plan 06-42 adds the in-process source guard, and Plan 06-43 registers the selector. The accepted regression is local model/reference comparator coverage, not external hardware conformance.

## Task Commits

1. `b272413` — `test(06-41): add failing MDEC input coverage test`.
2. `50b942f` — `test(06-41): implement bounded MDEC coverage scan`.
3. `5d6cbcc` — `test(06-41): register 8-bit regression selector`.
4. `6eeae26` — `test(06-41): freeze deterministic 8-bit MDEC output`.
5. `030c955` — `test(06-41): require 8-bit regression identity mapping`.
6. `8a0f457` — `test(06-41): add pinned 8-bit regression comparator`.

The measured plan range also contains `e45242e` (`docs(06-41): add reviewed phase continuation plans`), which added the reviewed 06-41 through 06-45 plans before the task slices. The seven-commit count is measured from the saved execution base through the final task commit; it excludes this summary/state closeout commit.

## Decisions Made

- Keep hardware output records and the deterministic implementation baseline separate. Do not modify hardware bytes or reinterpret the regression as hardware conformance.
- Gate every compressed guest execution on the next plan's in-process source guard; leave nightly registration to 06-43.
- Keep DISC-03 and the prior 06-23/24 acceptance pending; this plan closes only its model/comparator work.

## Deviations from Plan

None. A review correction changed the complete scanner fixture from 64 bytes to 128 bytes because MDEC coefficients are 16-bit halfwords; the accepted RED report and GREEN checks use the corrected fixture.

## Auth Gates

None.

## Next Phase Readiness

Plan 06-42 is next and owns the source guard needed before any pinned compressed reference guest runs. Plan 06-43 owns registration; Plans 06-44/06-45 own local and hosted integration/acceptance. DISC-03, the earlier 06-23/24 acceptance, and ship checks remain pending. No owner hardware capture or action is needed.

## Self-Check: PASSED

- The summary file exists.
- Task commits `b272413`, `50b942f`, `5d6cbcc`, `6eeae26`, `030c955`, and `8a0f457` are ancestors of the current branch head.
- The full local MDEC model label and no-emulation reference controls pass; no compressed reference guest was run.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-17*
