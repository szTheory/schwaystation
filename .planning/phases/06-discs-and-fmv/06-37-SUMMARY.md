---
phase: 06-discs-and-fmv
plan: "37"
subsystem: MDEC
tags: [mdec, status, ps1-tests, partial]
requires:
  - phase: 06-discs-and-fmv
    provides: "Pinned MDEC guests and exact output comparator"
provides:
  - "MDEC command status mirrors and bounded read-only guest diagnostics"
  - "Evidence-based follow-up scope for unresolved exact arithmetic and status completion"
affects: [06-12, 06-13, 06-14, 06-38, DISC-03]
actuals:
  tokens: 1480
  tasks: 1
  commits: 5
commits: 5
plan_head_before: ad565eb0c018fb1c3e45071fcc3a1b3074e42238
plan_head_after: d1f5f1ad84de8afe3eca1acf371af199ca01e0e9
tech-stack:
  added: []
  patterns:
    - "MDEC command words update documented STAT mirrors"
    - "Failure diagnostics inspect CPU and event counters without reading or advancing device state"
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-38-PLAN.md
  modified:
    - src/mdec.c
    - tests/mdec/model.c
    - tests/mdec/reference.c
    - docs/hardware/mdec.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
key-decisions:
  - "Do not guess a rounding rule: the primary hardware description calls the two-pass form approximate, while the pinned permissive implementation matches the baseline but not the independent guest bytes."
  - "Keep DISC-03 incomplete and preserve all byte/status selectors and the existing 4,915,200-cycle cap."
requirements-completed: []
coverage:
  - id: MDEC-STATUS-MODEL
    description: "MDEC(0) and invalid-command status mirrors are checked in the model."
    requirement: DISC-03
    verification:
      - kind: unit
        ref: "cmake --preset ci; cmake --build --preset ci --target schwaystation-test-mdec; ctest --test-dir build/ci -R '^mdec\.status$' --no-tests=error --output-on-failure; exit 0"
        status: pass
    human_judgment: false
  - id: MDEC-EXACT-OUTPUT
    description: "Pinned 4-bit/8-bit model and guest output bytes remain exact and gating."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "mdec.guest_input exit 8; mdec.reference.4bit exit 8 (byte 4 expected fd, actual ec); mdec.reference.8bit exit 8 (byte 6 expected 04, actual 05)"
        status: fail
    human_judgment: false
  - id: MDEC-STATUS-GUEST
    description: "Ordered status guest reaches Done without changing its run limit."
    requirement: DISC-03
    verification:
      - kind: integration
        ref: "mdec.reference.status exit 8 at 19,200 x 256-cycle steps; 82 status lines, 82 input writes, 0 output reads; final PC 800104bc, a0=0x6fa, MDEC/DMA idle and empty"
        status: fail
    human_judgment: false
  - id: MDEC-CONTROLS-AND-FULL-GATE
    description: "Comparator controls and the complete MDEC/DMA/state/reference gate pass after the status fix."
    requirement: DISC-03
    verification:
      - kind: other
        ref: "The full Plan 06-37 gate was not rerun after the bounded model status fix; selectors remain registered and gating."
        status: partial
    human_judgment: false
duration: "at least 30m12s wall; active time unavailable"
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 37: MDEC status mirrors and unresolved guest output

MDEC command words now update the documented status mirrors; exact output arithmetic and the status caller remain unresolved and gating.

## Performance

- Duration: At least 30m12s wall from 2026-11-16T11:16:31Z; active time unavailable.
- Runtime/model: Codex/OpenGSD 1.20.0; gpt-6-luna xhigh.
- Tasks: One bounded implementation slice; overall plan acceptance remains partial.
- Commits: Five measured from the plan ledger.

## Accomplishments

- Added model coverage for MDEC(0)'s low-word status echo and command mirror fields, including invalid opcodes.
- Updated src/mdec.c to mirror command fields into STAT and documented the behavior against the hardware reference.
- Made failure diagnostics read-only and bounded; they no longer call a device read that could advance emulation or read guest instruction memory outside RAM.
- Recorded the exact arithmetic/status gap in dependent Plan 06-38. Packing was ruled out; neither documented arithmetic form justifies a production rounding change.

## Test Evidence

| Check | Result |
|---|---|
| cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec && ctest --test-dir build/ci -R '^mdec\.status$' --no-tests=error --output-on-failure | exit 0 |
| ctest --test-dir build/ci -R '^mdec\.guest_input$' --no-tests=error --output-on-failure | exit 8; independent exact output vector mismatch |
| mdec.reference.4bit | exit 8; byte 4 expected fd, actual ec |
| mdec.reference.8bit | exit 8; byte 6 expected 04, actual 05 |
| mdec.reference.status | exit 8; no Done within unchanged 19,200 × 256-cycle budget |

The output mismatch remains despite matching packed nibble ordering. The primary hardware description calls its two-pass IDCT rounding approximate and says exact rounding is uncertain; the pinned ares implementation agrees with the selected baseline but not these pinned bytes. No arithmetic experiment was retained. The status guest still spends the remaining run budget in its 10,000-count delay; final state was MDEC/DMA idle and empty.

## Deviations

Plan 06-37 crossed its 30-minute bound without a source-backed exact arithmetic rule or status completion repair. It closes the completed status-mirror model slice only. Plan 06-38 owns the unresolved arithmetic and status guest work. DISC-03 remains incomplete; no selector, expected byte, or run bound was relaxed.

## Next

Run Plan 06-38, then rerun Plan 06-12's unchanged full MDEC gate before downstream consumers.

## Self-Check: PASSED

- Five Plan 06-37 commits are present from ledger base ad565eb0c018fb1c3e45071fcc3a1b3074e42238 through d1f5f1ad84de8afe3eca1acf371af199ca01e0e9.
- Plan 06-38 exists and remains the next gating plan.
