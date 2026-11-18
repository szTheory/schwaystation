---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 20
subsystem: docs
tags:
  - requirements
  - roadmap
  - testing-guide
  - hash-state

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the gte, dma and timers test directories, the per-profile judge, the vector-file guard and the --hash-state runner flag (plans 02 to 19)
provides:
  - REQUIREMENTS GTE-01, GTE-02 and GTE-03 worded as the phase built them, and the phase-3 paragraph with the gate ceiling, policy.notices and the --hash-state line
  - ROADMAP success criterion 2 worded as a yield at the guard
  - docs/testing.md sections for the gte, dma and timers labels, the anchored -L regex, the two reference-model tests, the vector guard, the hw/doc/derived rule, the per-profile judge and the state lines in hashes.txt
affects:
  - the phase's requirements step and verification, which read these wordings
  - the todo-ledger plan, which binds the remaining review findings

actuals:
  tokens: 4100
  tasks: 2
  commits: 2

plan_head_before: de2cc44383d93ebaee2fca5fdb082b9b0cb09421
plan_head_after: 84fb91754fcf8f4da28a3023e63ea643b1b2e149
commits: 2

tech-stack:
  added: []
  patterns:
    - "a documented value source is one of hw (copied from a hardware artefact at a public pin, with its path and case named), doc (a published document) or derived (a fitted model, kept as a band and never a judged row)"

key-files:
  created: []
  modified:
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - docs/testing.md

key-decisions:
  - "The phase-3 paragraph in REQUIREMENTS states the --hash-state line as the requirements step's one line for the flag, which the 03-19 executor had left outstanding"
  - "The testing guide names the judge's two accepted sources, doc and hw, and describes derived as a band label that no expected.csv row carries, matching what the judge in tests/fixtures/tests.cmake enforces"

patterns-established:
  - "the testing guide gives evidence commands for a label with an anchored regex, -L '^gte$', because -L matches a substring"

requirements-completed: []

coverage:
  - id: D1
    description: "REQUIREMENTS GTE-01, GTE-02 and GTE-03 carry the corrected wordings: at most 30 gte_vectors records, the CD-ROM word cost selected in phase 6, the guard yielding while the frame still ends, and timer records judged per profile against ranges from the frame constants, every row sourced doc"
    requirement: GTE-02
    verification:
      - kind: other
        ref: "grep -q 'device end in phase 6' .planning/REQUIREMENTS.md and the other plan-1 greps, exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "REQUIREMENTS gains the phase-3 paragraph: the 1,520-line gate ceiling, the policy.notices gate with its fault, and one line for schwaystation-run --hash-state"
    requirement: GTE-01
    verification:
      - kind: other
        ref: "grep -q 'hash-state' .planning/REQUIREMENTS.md && grep -q 'policy.notices' .planning/REQUIREMENTS.md, exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "ROADMAP phase 3 success criterion 2 says the chain-looping list yields at the guard while the frame still ends"
    requirement: GTE-02
    verification:
      - kind: other
        ref: "grep -q 'terminates by the 1000h-node guard' .planning/ROADMAP.md; test $? -eq 1, exit 0"
        status: pass
    human_judgment: false
  - id: D4
    description: "docs/testing.md documents the gte, dma and timers labels and lanes, the anchored -L regex, the 1,150-case run and its controls, the two reference-model tests with seeds and counts, the gte_vectors guard, the value-source rule, the per-profile judge and the state lines in hashes.txt, with no planning identifier"
    requirement: GTE-03
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci, exit 0, then ctest --test-dir build/ci -R '^docs\\.' --no-tests=error, exit 0"
        status: pass
      - kind: other
        ref: "grep -qE 'D-[0-9]{2}|GTE-0[0-9]|PX\\.[0-9]|DEC\\.[0-9]|XFER-' docs/testing.md; test $? -eq 1, exit 0"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 20: Requirements, roadmap and testing guide Summary

**The phase-3 requirements, the roadmap criterion and docs/testing.md now say what the phase built: the guard yields while the frame ends, the CD-ROM cost is chosen in phase 6, timer rows are `doc` ranges from the frame constants, and the guide covers the gte, dma and timers labels, both reference models, the vector guard, the per-profile judge and the state lines.**

## Performance

- **Duration:** 10 min
- **Started:** 2026-11-10T07:54:00Z
- **Completed:** 2026-11-10T08:04:00Z
- **Tasks:** 2
- **Files modified:** 3

## Accomplishments

- GTE-01 says the `gte_vectors` mailbox holds at most 30 records; GTE-02 reads "1, 4, 24, or 40 selected by the CD-ROM device end in phase 6" and "yielding at the guard while the frame still ends"; GTE-03 judges the records per profile against ranges derived from the frame constants (the exact 715909/451584 ratio with the interrupt-path overhead), every row `doc`, and keeps its GAME-02 pointer for the dot-clock and hblank records.
- The phase-3 paragraph follows the phase-2 paragraph's style: the 1,520-line gate ceiling ratcheted at ship, `policy.notices` with the fault it prevents, and the `--hash-state` line. This is the requirements line the 03-19 executor reported outstanding (CONTEXT D-27 and D-39); it is included here.
- ROADMAP success criterion 2 is reworded the same way.
- `docs/testing.md` gains the three labels in the label table, the lane note, the anchored `-L '^gte$'` warning and a new section covering the 1,150-case run with its three controls, the two reference-model tests with their seeds and case counts, `fixtures.gte_vectors.guard`, the `hw`, `doc` and `derived` rule, the per-profile judge and the state lines in `hashes.txt`.

## Task Commits

1. **Task 1: Tracer - the requirement and roadmap corrections** - `62ed83b` (docs)
2. **Task 2: The testing guide for the phase's labels and checks** - `84fb917` (docs)

**Plan metadata:** recorded in the following `docs(03-20)` commit.

The tracer's `<verify>` ran end to end after the commit and passed before the second task began.

## Files Created/Modified

- `.planning/REQUIREMENTS.md` - corrected GTE-01 to GTE-03 and the phase-3 paragraph
- `.planning/ROADMAP.md` - success criterion 2 wording
- `docs/testing.md` - phase-3 labels, lanes and checks

## Decisions Made

- The new section names the judge's two accepted sources, `doc` and `hw`, and presents `derived` as a band label that no `expected.csv` row carries, because the judge in `tests/fixtures/tests.cmake` rejects any other source. The table's `source` and `citation` rows were adjusted to say what an `hw` row must cite.
- Task 2 spells out the evidence commands with the anchored regex so a maintainer cannot select a longer label by accident.

## Deviations from Plan

None - plan executed exactly as written. The plan's `requirements-completed:` is empty by its own output spec, since the three requirements finish in other plans, so no requirement was marked complete here.

## Issues Encountered

None.

## Verification Evidence

Run from the repository root:

- plan-1 greps (hash-state, policy.notices, no old guard wording, CD-ROM deferral wording, no PX.12 and PX.21, GTE-03 pointing at GAME-02): exit 0
- `grep -q 'terminates by the 1000h-node guard' .planning/ROADMAP.md; test $? -eq 1`: exit 0
- `grep -q '24 or 40)' .planning/REQUIREMENTS.md; test $? -eq 1`: exit 0
- `cmake --workflow --preset ci`: exit 0
- `ctest --test-dir build/ci -R '^docs\.' --no-tests=error --output-on-failure`: exit 0, 3 of 3 passed
- the two `docs/testing.md` acceptance greps: exit 0
- `asan`, `be32` and `wasm32` need clang-21 and cross tools this host lacks; hosted CI runs them. This plan changes no code, so those lanes see only the documentation tests.

## Known Stubs

None.

## Next Phase Readiness

Ready for 03-21 and 03-22, which share no file with this plan. The todo ledger is plan 03-25.

## Self-Check: PASSED

- `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md` and `docs/testing.md` exist and are committed.
- Commits `62ed83b` and `84fb917` are ancestors of HEAD.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
