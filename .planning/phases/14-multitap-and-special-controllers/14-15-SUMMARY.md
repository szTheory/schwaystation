---
phase: 14-multitap-and-special-controllers
plan: "15"
subsystem: compatibility-evidence
tags: [compatibility, multitap, mouse, negcon, jogcon, flight]
requires:
  - phase: 14-07
    provides: aggregate state and tap protocol coverage
  - phase: 14-09
    provides: runner card persistence across eight sockets
  - phase: 14-14
    provides: libretro card lifecycle isolation
provides:
  - Six tested controller and card capability rows in the feature ledger
  - Independent expected rows and regenerated compatibility documentation
affects: [phase-14-verification, compatibility-evidence]
actuals:
  tokens: 1610
  tasks: 1
  commits: 1
commits: 1
plan_head_before: 0079f0e796bf061bb8398dbe359b7de64fa1f61a
plan_head_after: a05c6919b70662794a54e6d212f0fcd3bd257050
tech-stack:
  added: []
  patterns:
    - Feature claims cite passing direct selectors and retain the unknown game-compatibility boundary
key-files:
  created: []
  modified:
    - tests/compat/features.csv
    - tests/compat/tests.cmake
    - docs/compatibility.md
key-decisions:
  - Use the existing ntsc-u.pu8-late.v2 profile token for these core feature rows.
  - Keep each compatibility claim to selector evidence; do not infer retail-game support or undocumented Jogcon behavior.
patterns-established:
  - The expected feature ledger independently mirrors every new provider/profile/peripheral/predicate key.
requirements-completed: []
coverage:
  - id: C1
    description: Dual Analog flight button and axis payloads, unsupported command refusal, and libretro mapping are recorded from passing selectors.
    requirement: SIO-04
    verification:
      - kind: integration
        ref: "sio.special.flight and libretro.input.special passed in Plans 14-06 and 14-12; compat.features.preflight, compat.features.ratchet, and compat.features.controls passed in this plan."
        status: pass
    human_judgment: false
  - id: C2
    description: Jogcon's tested native poll payload, supported configuration boundary, and libretro mapping are recorded without claiming dial mechanics or feedback.
    requirement: SIO-03
    verification:
      - kind: integration
        ref: "sio.special.jogcon and libretro.input.special passed in Plans 14-06 and 14-12; compat.features.preflight, compat.features.ratchet, and compat.features.controls passed in this plan."
        status: pass
    human_judgment: false
  - id: C3
    description: Mouse relative movement, frame latch, active-low buttons, and libretro mapping are recorded from passing selectors.
    requirement: SIO-02
    verification:
      - kind: integration
        ref: "sio.special.mouse, sio.tap.poll_methods, and libretro.input.special passed in Plans 14-05 and 14-12; all three compatibility feature selectors passed in this plan."
        status: pass
    human_judgment: false
  - id: C4
    description: Eight card sockets have tested protocol and independent runner/libretro persistence evidence.
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "card.tap.protocol, card.tap.state, runner.cards.multitap, runner.cards.persistence, libretro.cards.multitap, and libretro.cards.persistence passed in Plans 14-04, 14-09, 14-13, and 14-14; all three compatibility feature selectors passed in this plan."
        status: pass
    human_judgment: false
  - id: C5
    description: Multitap controller poll methods and request history, plus the tested libretro topology and descriptors, are recorded.
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "sio.tap.poll_methods and sio.tap.history_edges passed in Plan 14-03; libretro.input.multitap and libretro.input.descriptors passed in Plans 14-10 and 14-11; all three compatibility feature selectors passed in this plan."
        status: pass
    human_judgment: false
  - id: C6
    description: neGcon native poll payload and libretro mapping evidence are recorded.
    requirement: SIO-03
    verification:
      - kind: integration
        ref: "sio.special.negcon and libretro.input.special passed in Plans 14-05 and 14-12; all three compatibility feature selectors passed in this plan."
        status: pass
    human_judgment: false
metrics:
  duration: 12m
  completed: 2026-11-24
duration: 12m
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 15: Tested controller feature ledger Summary

**Six compatibility rows now record selector-backed multitap, eight-card, mouse, neGcon, Jogcon, and flight behavior.**

## Performance

- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Duration:** 12 minutes (started 2026-11-24T12:06:17Z).
- **Completed:** 2026-11-24.
- **Tasks:** 1.
- **Files modified:** 3 task files; this summary is the plan artifact.

## Accomplishments

- Added six narrow feature rows backed by passing SIO, card, runner, and libretro selectors.
- Updated the independent compatibility ratchet fixture to expect those exact rows.
- Regenerated `docs/compatibility.md` with the existing generator and confirmed it is current.
- Kept title-ledger rows unchanged and made no claim about retail-game compatibility, Jogcon dial mechanics, or force feedback.

## Task Commits

1. **Task 1: Record tested protocol, mapping and save predicates** — `a05c691` (docs).

**Plan metadata:** this summary, STATE, and ROADMAP are included in the GSD bookkeeping commit.

## Files Created/Modified

- `tests/compat/features.csv` — six capability rows with passing selector references.
- `tests/compat/tests.cmake` — independent expected feature rows for the compatibility ratchet.
- `docs/compatibility.md` — generated compatibility evidence table.

## Checks

- `git diff --check` — exit 0.
- `tail -n +2 tests/compat/features.csv | LC_ALL=C sort -cu` — exit 0; data rows byte-sorted.
- `awk -F, 'NR > 1 { if (NF != 8) exit 1; key=$1 "!" $2 "!" $3 "!" $4 "!" $5; if (seen[key]++) exit 1; n++ } END { print "feature rows:", n; if (n != 38) exit 1 }' tests/compat/features.csv` — exit 0; 38 rows, eight fields per row, no duplicate keys.
- `cmake -DFEATURES=tests/compat/features.csv -DMODE=features-preflight -P tests/cmake/compat-ratchet.cmake` — exit 0.
- `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md` — exit 0.
- `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` — exit 0.
- `LC_ALL=C sort -cu tests/compat/inventory.txt` — exit 0.
- `cmake --preset ci` — exit 0; reused `build/ci`.
- `ctest --preset ci --show-only=json-v1 -R '^compat[.]features[.](preflight|ratchet|controls)$' | python3 -c 'import json,sys; actual={t["name"] for t in json.load(sys.stdin)["tests"]}; required=set("compat.features.preflight compat.features.ratchet compat.features.controls".split()); print("missing selectors:", sorted(required-actual)); sys.exit(bool(required-actual))'` — exit 0; all three selectors present.
- `cmake --build --preset ci --target schwaystation-test-probe` — exit 0.
- `ctest --preset ci --no-tests=error -R '^compat[.]features[.](preflight|ratchet|controls)$' --output-on-failure` — exit 0; 3/3 passed in 1.30 seconds.
- No broad suite, hosted CI, Act, or Docker run was part of this ledger slice.

The underlying controller and persistence selectors cited by the feature rows passed in their owning prior plans; this plan ran only the three compatibility feature selectors. SIO-01 through SIO-04 remain phase-gated until Plan 14-16 and phase verification finish.

## Decisions Made

- Used the canonical existing core profile token `ntsc-u.pu8-late.v2` for the six rows.
- Kept unsupported retail-game and Jogcon-mechanics claims out of the ledger.

## Deviations from Plan

None — the ledger, independent expected rows, and generated documentation were completed as planned.

## Issues Encountered

The sandbox initially denied the required Git metadata write under `.git`; the task commit succeeded after the necessary escalation. No repository files outside this plan's scope were staged.

## User Setup Required

None.

## Next Phase Readiness

Plan 14-16 is next. Shared SIO requirements remain gated pending that slice and final phase verification; no owner action is needed.

## Self-Check: PASSED

- The three task files and this summary exist.
- Task commit `a05c691` is an ancestor of HEAD.
- The measured task commit count is one, matching the recorded plan ledger.
