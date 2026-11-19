---
phase: 08-per-game-card-saves
plan: "04"
subsystem: memory-card
tags: [C, PSX-SPX, PocketStation, serial, save-state]
requires:
  - phase: 08-01
    provides: Card serial protocol, per-slot state visitor, and card test inventory.
provides:
  - Pinned PocketStation 58h–5Fh transcript and bounded storage address notes.
  - Per-slot PocketStation metadata command handling and validated state restoration.
  - State format and behavior revision 18 with reserved 128-byte storage transfer fields.
affects: [08-06, save-state compatibility]
actuals:
  tokens: 9075
  tasks: 3
  commits: 5
  plan_head_before: 72c2ef6d52f1ca1f4bb2dd33b661c89a5ec46630
  plan_head_after: 68ed0a6fe37d5a471337ee16c01dca28374f75a2
tech-stack:
  added: []
  patterns:
    - Per-slot protocol metadata is serialized separately from host-owned card bytes and generation.
    - Reserved transfer state has fixed bounds and remains zero until its first storage caller.
key-files:
  created:
    - .planning/phases/08-per-game-card-saves/08-04-SUMMARY.md
  modified:
    - .planning/preparation/DECISIONS.md
    - .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md
    - docs/embedding.md
    - src/card.c
    - src/card.h
    - src/internal.h
    - tests/card/card_tests.c
    - tests/card/tests.cmake
    - tests/card/inventory.txt
    - tests/core/test_state.c
key-decisions:
  - Directory entry size occupies bytes 4–7 in the pinned source, but its byte order is unspecified; preserve a focused probe for the first 08-06 storage caller.
  - Keep existing DEC.60 ACK scheduling where PSX-SPX gives no command-specific delay; fixed BCD and unsupported storage commands are deterministic bounds, not RTC or PocketStation CPU emulation.
patterns-established:
  - State loads restore bounded protocol metadata while the receiving instance keeps its card bytes and generation.
requirements-completed: [SAVE-05]
coverage:
  - id: D1
    description: PocketStation metadata transcripts run per slot, resume from a validated snapshot, and preserve receiver media.
    requirement: SAVE-05
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^card[.]protocol[.]' --output-on-failure"
        status: pass
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(card[.]state[.]|core[.]state[.])' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Pinned PSX-SPX source fixes command framing and documented flash and directory bounds for the first storage caller.
    requirement: SAVE-05
    verification:
      - kind: other
        ref: "gh api repos/psx-spx/psx-spx.github.io/commits/6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f --jq .sha"
        status: pass
    human_judgment: false
duration: 38 min
completed: 2026-11-19
status: complete
commits: 5
plan_head_before: 72c2ef6d52f1ca1f4bb2dd33b661c89a5ec46630
plan_head_after: 68ed0a6fe37d5a471337ee16c01dca28374f75a2
runtime: Codex/OpenGSD CLI 1.20.0
model: gpt-6-luna
---

# Phase 8 Plan 04: Pin PocketStation transcripts and implement card metadata Summary

**Pinned PocketStation command framing and added per-slot metadata commands with validated format-18 state that resumes without replacing card media.**

## Performance

- **Duration:** 38 minutes (plan start 2026-11-19T13:50:00Z)
- **Completed:** 2026-11-19
- **Tasks:** 3
- **Files modified:** 10
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; executor gpt-6-luna.

## Accomplishments

- Verified and paraphrased the pinned PSX-SPX serial transcripts and documented direct flash, mirror, selected-file, directory-block, and overflow bounds. Exact directory size byte order remains unspecified in the source and is reserved for a focused probe when 08-06 adds its first storage caller.
- Implemented per-slot 58h, 59h, 5Ah, 5Dh, 5Eh, and 5Fh metadata handling with deterministic refusal for unsupported 5Bh/5Ch storage commands. Existing 52h/53h/57h card behavior remains covered.
- Serialized selection, flags, and in-flight metadata command bytes alongside bounded 128-byte storage transfer fields. Format-17 states now return `SCHWAY_STATUS_ERR_STATE_VERSION`; loading validated state does not restore the saved card image or generation.

## Task Commits

1. **Task 1: Establish the exact bounded serial-storage contract** — `8ae1c4b` (docs)
2. **Task 2 RED: Add failing PocketStation metadata transcript test** — `dac8073` (test)
3. **Task 2 GREEN: Add PocketStation metadata commands** — `79aa7b8` (feat)
4. **Task 3 RED: Verify PocketStation state resume and format gate** — `a268b5f` (test)
5. **Task 3 GREEN: Serialize PocketStation metadata and bounded transport** — `68ed0a6` (feat)

## Verification Evidence

- Pinned source commit lookup returned the requested public SHA, exit 0: `gh api repos/psx-spx/psx-spx.github.io/commits/6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f --jq .sha`. Read the pinned [PocketStation transcript](https://github.com/psx-spx/psx-spx.github.io/blob/6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f/docs/pocketstation.md) and [controller/memory-card directory description](https://github.com/psx-spx/psx-spx.github.io/blob/6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f/docs/controllersandmemorycards.md).
- Task 2 RED command `ctest --test-dir build/ci --output-junit /private/tmp/schwaystation-card-red.xml -R '^card[.]protocol[.]pocketstation_metadata$' --no-tests=error --output-on-failure` failed on the absent 58h reply and ACK, exit 8; GSD classified its actual JUnit report as `RED_EVIDENCE_OK`. `cmake --build --preset ci --target schwaystation-test-card && ctest --preset ci --no-tests=error -R '^card[.]protocol[.]' --output-on-failure` then passed 5/5, exit 0.
- Task 3 RED commands `ctest --preset ci --no-tests=error --output-junit /private/tmp/schwaystation-08-04-task3-card-red.xml -R '^card[.]state[.]pocketstation_metadata$' --output-on-failure` and `ctest --preset ci --no-tests=error --output-junit /private/tmp/schwaystation-08-04-task3-state-red.xml -R '^core[.]state[.]rejects$' --output-on-failure` each failed on the missing reserved state fields and acceptance of format 17, exit 8; both actual JUnit reports passed `gsd_run check tdd-red-evidence` with `RED_EVIDENCE_OK`.
- `sort -c tests/card/inventory.txt && git diff --check && cmake --build --preset ci --target schwaystation-test-card schwaystation-test-state && ctest --preset ci --no-tests=error -R '^(card[.]state[.]|core[.]state[.])' --output-on-failure` passed, exit 0: 11/11 focused state tests in 11.36 seconds.
- No hosted run applies; no broad CI or full test suite was started.

## Files Created/Modified

- `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` — source-backed command and storage bounds.
- `.planning/preparation/DECISIONS.md` — reconciled PocketStation scope and ACK timing policy.
- `src/card.c`, `src/card.h` — per-slot metadata and bounded saved transport fields.
- `src/internal.h`, `docs/embedding.md` — format and behavior revision 18 compatibility.
- `tests/card/card_tests.c`, `tests/card/tests.cmake`, `tests/card/inventory.txt`, `tests/core/test_state.c` — transcript, state, and version regressions.

## Decisions Made

- The source provides entry size bytes at offsets 4–7 but does not specify their byte order. Keep a focused probe for the first relevant storage caller in 08-06.
- Source-described byte framing is kept distinct from local policy: use the existing DEC.60 ACK scheduler where no exact delay is published, a fixed BCD metadata reply, and deterministic unsupported outcomes for 5Bh/5Ch. No PocketStation CPU, BIOS, RTC, LCD, or IrDA emulation is claimed.
- Preserve host-owned card bytes and generation counters across state loads; state format 18 records only validated protocol metadata and bounded pending transport fields.

## Deviations from Plan

None. The first Task 3 test run revealed two incorrect test expectations (59h returns the previous index and its first parameter is staged after the two index bytes); these were corrected before the valid RED evidence runs.

## Issues Encountered

The sandbox blocks direct Git index writes. Task commits succeeded through the required GSD commit handler with hooks. No other blocker remains.

## Known Stubs

None. 5Bh/5Ch return the documented deterministic unsupported result for this plan; storage transfer behavior remains assigned to 08-06.

## Threat Flags

None. The changes add bounded in-instance state and serial handling within the plan's existing card trust boundary.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 08-06 has the pinned address map and 128-byte transfer bound. It should probe directory size byte order when it first reads an entry, then implement the documented storage subset. The probe does not require CPU or peripheral emulation.

## Self-Check: PASSED

The summary exists, all five task commits are ancestors of HEAD, and the coverage classifier reported no errors.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*
