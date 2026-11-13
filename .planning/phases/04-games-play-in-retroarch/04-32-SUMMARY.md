---
phase: 04-games-play-in-retroarch
plan: "32"
subsystem: api
tags: [public-api, ctest, documentation, compatibility]
requires:
  - phase: "04-28"
    provides: v4 input, rumble and save behaviors consumed by the API caller
  - phase: "04-48"
    provides: current-head required GPU lane and phase evidence used in closeout
provides:
  - Registered API v4 caller coverage with matching public documentation
  - Retained byte-identical frozen API v1-v3 callers
affects: [phase-04-verification]
actuals:
  tokens: 15073
  tasks: 2
  commits: 2
plan_head_before: f0583877a3b39bc6608dc1712e07f85a6ae00e54
plan_head_after: 2f839fd29819d3d32a7ade8f0348db8fc6a1b1c5
tech-stack:
  added: []
  patterns:
    - Test public API additions through registered strict callers
    - Keep BIOS identity, card naming and frontend ownership prose tied to actual consumers
key-files:
  created: []
  modified:
    - tests/api/v4.c
    - docs/embedding.md
    - docs/runner.md
    - .planning/preparation/DECISIONS.md
    - .planning/preparation/HOSTS-AND-INTEGRATION.md
    - .planning/preparation/PSX-GPU-AND-GTE.md
    - .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md
key-decisions:
  - "Use the built MIT OpenBIOS image only as a CI stand-in; retail Sony BIOS remains user supplied and is not placed in CI or shipped."
  - "Serial memory-card names belong to the runner; libretro uses the frontend content basename and provides no card naming/sharing option."
patterns-established:
  - "Public API behavior and documentation are checked against a real registered caller, with old API callers left byte-identical."
requirements-completed: []
coverage:
  - id: D1
    description: API v4 additions are exercised by a strict registered caller and the public prose matches their real consumers.
    requirement: GAME-01
    verification:
      - kind: unit
        ref: "Registered API v1-v4 callers: 4/4 passed locally; frozen v1-v3 callers remained byte-identical."
        status: pass
      - kind: integration
        ref: https://github.com/szTheory/schwaystation/actions/runs/43404686338
        status: pass
    human_judgment: false
duration: "not measured; candidate and acceptance were checkpointed separately"
completed: 2026-11-13
status: complete
---

# Phase 4 Plan 32: API v4 Caller and Documentation Summary

**The partial API v4 slice now has a registered strict caller and public prose aligned to the shipped consumers.**

## Performance

- **Duration:** Not measured; the candidate and final acceptance were separated by other checkpointed plan work.
- **Started:** Original active start was not retained.
- **Completed:** 2026-11-13
- **Tasks:** 2
- **Files modified:** 7 for this plan's candidate.
- **Runtime/model:** Codex/OpenGSD 1.20.0; gpt-6-luna xhigh.

## Accomplishments

- Added a registered API v4 caller that exercises copied BIOS identity, memory spans, input latching, lag/rumble, card import/generation and bounded calls with side-effect-free peeks.
- Updated embedding, runner and preparation prose to match actual public contracts, including BIOS identity and native video/hash wording.
- Clarified that retail Sony BIOS bytes are user supplied and absent from CI; the built MIT OpenBIOS image is a CI stand-in and is not shipped.
- Corrected memory-card naming prose: the runner accepts serial names, while libretro uses the frontend content basename and has no naming or sharing option; span prose names CARD1 or CARD2.
- Preserved frozen API v1-v3 caller files byte-for-byte.

## Task Commits

1. **Task 1: Exercise API v4 additions and align public docs** — `0b13b66` (`test`)
2. **Task 2: Cross-check API and documentation claims against consumers** — included in `0b13b66` (`test`)

The measured plan32 ledger interval contains two commits: the API/docs candidate and the later plan48 GPU portability repair. The repair is plan48 work; `0b13b66` is plan32's sole implementation commit.

## Files Created/Modified

- `tests/api/v4.c` — strict registered caller for the v4 additions.
- `docs/embedding.md`, `docs/runner.md` — public API and runner behavior.
- `.planning/preparation/DECISIONS.md`, `HOSTS-AND-INTEGRATION.md`, `PSX-GPU-AND-GTE.md`, `PSX-PERIPHERALS-AND-SAVES.md` — reconciled hardware and host details.

## Decisions Made

- Kept revision 4; no new public API or behavior-version bump was needed for this documentation/caller slice.
- Distinguished the user-supplied retail BIOS from the MIT OpenBIOS CI stand-in and retained the non-distribution policy.
- Documented runner serial card naming separately from libretro frontend-basename behavior.

## Deviations from Plan

None in implementation. The reviewed candidate was integrated as a separate partial plan32 commit before the shared exact-head acceptance run, with the required BIOS and card-naming wording corrections applied.

## Evidence

- Registered API callers v1-v4: 4/4 passed locally. Frozen v1-v3 caller files remained byte-identical.
- Exact-head hosted run [43404686338](https://github.com/szTheory/schwaystation/actions/runs/43404686338) completed successfully and included the API caller and current documentation/source matrix.
- Root reviewed the API/public-documentation delta and confirmed the BIOS, card naming and CARD1/CARD2 corrections.

## User Setup Required

None.

## Next Phase Readiness

This partial API/docs slice is complete. The phase-wide verifier owns the remaining goal-backward report. Stop after phase verification before phase 4 ship or phase 5.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-13*

## Self-Check: PASSED

The summary file exists, and its API candidate and recorded plan head are present in the current history.
