---
phase: 06-discs-and-fmv
plan: "36"
subsystem: testing
tags: [mdec, ps1-tests, dma1, reference-harness, ctest]
requires:
  - phase: 06-discs-and-fmv
    provides: "Bounded MDEC input parsing and registered pinned MDEC guest harness"
provides:
  - "Reference output comparator reads the guest's actual DMA1 start MADR"
  - "Independent exact 4-bit and 8-bit model vectors reproduce remaining output differences"
  - "Evidence-based split assigning production output arithmetic and status completion to Plan 06-37"
affects: [06-12, 06-13, 06-14, MDEC, DISC-03]
actuals:
  tokens: 14449
  tasks: 1
  commits: 4
commits: 4
plan_head_before: fa5077664f518125e2a3a5606a3530345dae81d9
plan_head_after: a619a49a72011685858f05f87119bff0b8f55084
tech-stack:
  added: []
  patterns:
    - "Derive host-observed DMA output memory from the first guest-written CHCR start record"
    - "Keep exact model expectations as literals traced to independently pinned log-member digests"
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-37-PLAN.md
  modified:
    - tests/mdec/model.c
    - tests/mdec/reference.c
    - .planning/phases/06-discs-and-fmv/06-36-PLAN.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
key-decisions:
  - "Split production arithmetic and ordered-status completion into dependent Plan 06-37 after Plan 06-36 crossed 30 minutes without a source-backed fix."
  - "Use the recorded DMA1 MADR, not guessed hardcoded addresses, as the guest output comparison base."
  - "Keep every 4-bit, 8-bit, status and comparator selector registered and gating while production mismatches remain."
patterns-established:
  - "A bounded trace cursor scans only new trace-ring records while capturing the first DMA1 start."
requirements-completed: []
coverage:
  - id: D1
    description: "Reference harness observes output bytes at each guest's DMA1 destination and reports exact mismatches."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "build/mdec-reference/06-36-reference-red.log; combined CTest exit 8 with all three expected caller diagnostics"
        status: pass
    human_judgment: false
  - id: D2
    description: "Literal model vectors compare all pinned 4-bit and 8-bit output words independently."
    requirement: DISC-03
    verification:
      - kind: unit
        ref: "build/ci/06-36-model-red-evidence.json; tdd-red-evidence returned RED_EVIDENCE_OK for mdec.guest_input, exit 8"
        status: fail
    human_judgment: false
  - id: D3
    description: "Pinned status caller and comparator-control selectors remain gating with no timeout or selector relaxation."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "mdec.reference.status exit 8 with no Done marker; mdec.reference.controls exit 0"
        status: fail
    human_judgment: false
duration: "at least 57m55s wall; active time unavailable"
completed: 2026-11-16
status: complete
---

# Phase 06 Plan 36: MDEC DMA destination and exact RED vectors

**The MDEC reference harness now reads each pinned output from its recorded DMA1 destination and has independent exact model vectors for the still-failing bytes.**

## Performance

- **Duration:** At least 57m55s wall from the 10:15:20Z start marker; active time unavailable.
- **Started:** 2026-11-16T10:15:20Z
- **Completed:** 2026-11-16 (closeout began after the 11:08:44Z evidence checkpoint)
- **Tasks:** 1 bounded harness/control task
- **Files modified:** 20 tracked files across tests, plans, roadmap, state and this summary.
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh.

## Accomplishments

- Replaced guessed DMA output addresses with the MADR captured from the first DMA1 CHCR start write: 4-bit `0x001ffeb8`, 8-bit `0x001ffe98`.
- Added literal 4-bit and 8-bit output-word vectors sourced from pinned `psx.log` member digests in `pins.txt`; the registered `mdec.guest_input` test fails on those exact expected-word comparisons.
- Added an incremental trace-ring cursor so 256-cycle guest stepping does not repeatedly scan the full 4,096-record trace ring.
- Preserved all three reference guest selectors and the comparator controls. Production output arithmetic and status completion are assigned to dependent Plan 06-37.

## Test Evidence

Commands were run from the repository root. The exact model RED was machine-classified, and all expected failures were target assertions rather than setup errors.

| Check | Result |
|---|---|
| `cmake --preset ci` | exit 0 |
| `cmake --build --preset ci --target schwaystation-test-mdec` | exit 0 |
| ctest --test-dir build/ci -R '^mdec\.guest_input$' --no-tests=error --output-on-failure --output-junit build/ci/06-36-model-red.xml | exit 8; `mdec.guest_input` failed 19 exact-word assertions; `RED_EVIDENCE_OK` |
| `cmake -S . -B build/mdec-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec` | exit 0 |
| `cmake --build build/mdec-reference --target schwaystation-test-mdec-reference` | exit 0 |
| `mdec.reference.4bit` | exit 8; mismatch starts at byte 4, expected `fd`, actual `ec` |
| `mdec.reference.8bit` | exit 8; mismatch at byte 6, expected `04`, actual `05` |
| `mdec.reference.status` | exit 8; no Done marker after 19,200 × 256-cycle steps; current command `0x002003dd`, 5,158,613 ticks, empty MDEC/DMA state |
| Combined `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.(4bit|8bit|status)$' --no-tests=error --output-on-failure` | exit 8 with all three expected diagnostics |
| `mdec.reference.controls` | exit 0 |

The fixed cycle budget remains 4,915,200 cycles; the test harness divides it into 256-cycle steps to capture the live MDEC state. The status reference is an ordered event log without cycle timestamps, so this evidence does not infer timing from it. All executables, JUnit reports, test logs and RED evidence remain under `build/` and are not committed.

## Task Commits

- `a25ea0e`, `051e77c`: expected-RED verification plan amendments.
- `17bee24`: split metadata, dependency graph and Wave 11 Plan 06-37.
- `a619a49`: exact-output model controls and DMA1 destination capture.

The summary/STATE/ROADMAP metadata commit is recorded in Git history after this file is verified.

## Decisions Made

- Split before expanding production diagnosis: the observed byte differences and status completion issue are independent production work, and the plan had crossed its 30-minute bound without source-backed corrections.
- The 8-bit one-LSB mismatch is evidence for investigation, not a license to change rounding; Plan 06-37 must cite the hardware arithmetic rule and keep the independent expected bytes.
- The post-06-35 status trace is authoritative for the next investigation: command `0x002003dd`, empty output/DMA state, no Done marker.

## Deviations from Plan

Plan 06-36 was split after crossing the 30-minute limit. It closes only the corrected DMA1 destination comparator and exact model RED controls. Plan 06-37 now owns the unchanged 4/8-bit guest corrections, ordered-status completion, and final MDEC/DMA/state/reference gate. No production behavior was changed in this plan.

## Issues Encountered

- The previous comparator used hardcoded destination addresses that did not match the guest's DMA1 MADR writes. The host trace now supplies the correct destination, revealing the independent exact byte differences.
- The status caller remains incomplete after the bounded parser fix. The current post-fix trace differs from the earlier pre-fix snapshot; Plan 06-37 carries forward the latest trace.

## User Setup Required

None.

## Next Phase Readiness

Plan 06-37 is ready to address the production arithmetic and ordered status caller. After it passes, rerun Plan 06-12's exact full reference gate; Plans 06-13 and 06-14 remain behind Plan 06-37. DISC-03 is partial and is not complete.

## TDD Gate Compliance

- **RED:** `mdec.guest_input` executed and failed on literal expected MDEC FIFO words (not compilation, setup, or test discovery). Exit 8; the persisted CTest JUnit report and console output are in `build/ci/06-36-model-red-evidence.json`.
- **Classifier:** `gsd_run check tdd-red-evidence build/ci/06-36-model-red-evidence.json --raw` returned `RED_EVIDENCE_OK`, target `mdec.guest_input`, one test, one failing target.
- **Semantic assessment:** The failures are actual arithmetic/output differences. The first 8-bit mismatch is `0x0005ffff` versus independently pinned `0x0004ffff`; production changes remain deferred to Plan 06-37.

## Self-Check: PASSED
