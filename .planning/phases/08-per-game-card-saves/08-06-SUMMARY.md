---
phase: 08-per-game-card-saves
plan: "06"
subsystem: card
tags: [memory-card, PocketStation, SIO, save-state]

requires:
  - phase: 08-per-game-card-saves
    provides:
      - Pinned PocketStation command framing, storage bounds, and state contract from plan 08-04
provides:
  - Bounded 5Bh/5Ch FUNC 01 transfers to physical card bytes and the selected file view
  - Staged writes that publish once after the final byte and validate against receiver media on state restore
  - Real JOY-register guest coverage for physical and selected-file reads and writes
affects: [memory-card, PocketStation, save-state]

actuals:
  tokens: 9871
  tasks: 2
  commits: 3
  plan_head_before: 08f0f724447eef0cfd157cb9d4b00a862091e9d8
  plan_head_after: 855061a0a9671de6ef84768cbddeb810692b233a

tech-stack:
  added: []
  patterns:
    - Validate the complete bounded card mapping before reading or staging a transfer
    - Keep pending write bytes in per-slot state and publish them only at the accepted terminator

key-files:
  created: []
  modified:
    - src/card.c
    - tests/card/card_tests.c
    - tests/fixtures/manifest.txt
    - tests/fixtures/src/pocketstation_storage.asm

key-decisions:
  - Treat directory size and link fields as little-endian; the focused probe makes byte reversal invalid.
  - Preserve receiver-owned card bytes and generation on state load, and check the pending mapping against that receiver's current directory chain.

requirements-completed: [SAVE-05]
coverage:
  - id: D1
    description: Bounded physical and selected-file FUNC 01 reads and writes use the PocketStation wire transcript.
    requirement: SAVE-05
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^card[.](protocol|state)[.]' --output-on-failure"
        status: pass
      - kind: e2e
        ref: "ctest --preset ci --no-tests=error -R '^fixtures[.]pocketstation_storage$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Invalid, aborted, and malformed writes leave card bytes and generation unchanged; a completed nonempty write publishes once.
    requirement: SAVE-05
    verification:
      - kind: unit
        ref: "tests/card/card_tests.c#protocol_pocketstation_storage"
        status: pass
      - kind: unit
        ref: "tests/card/card_tests.c#state_pocketstation_storage_restore"
        status: pass
    human_judgment: false
  - id: D3
    description: Directory-chain validation checks states, checksums, links, cycles, and the focused size/link byte-order probe.
    requirement: SAVE-05
    verification:
      - kind: unit
        ref: "tests/card/card_tests.c#protocol_pocketstation_storage"
        status: pass
      - kind: e2e
        ref: "tests/fixtures/src/pocketstation_storage.asm#directory_first"
        status: pass
    human_judgment: false

duration: 41 min
completed: 2026-11-19
status: complete
---

# Phase 08 Plan 06: PocketStation storage transfers Summary

**Bounded 5Bh/5Ch physical and selected-file transfers, staged writes, and a real JOY-register guest transcript**

## Performance

- **Duration:** 40 min 52 sec
- **Started:** 2026-11-19T15:24:16Z
- **Completed:** 2026-11-19T16:05:08Z
- **Runtime/model:** Codex, GPT-6 (the exact model identifier was not exposed)
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Implemented the pinned 5Bh/5Ch FUNC 01 transcript with little-endian address parameters, the fixed 05h parameter count, LEN2 clipped to 128 bytes, scheduled intermediate ACKs, and a final FFh/no-ACK.
- Whitelisted the physical 128 KiB card window and its mirrors, and the selected-file view after validating the complete directory chain and transfer range. Writes stage per slot, publish only after the final dummy byte, and bump generation once when nonempty.
- Verified aborts, invalid ranges, malformed terminals, state restore against a receiver with a different valid block chain, and rejection when the receiver's chain is invalid. The guest fixture performs physical and virtual round trips, pre-seeds directory frames through ordinary 57h writes, checks an invalid request and an aborted write, and retains exit sentinel 51h.

## Task Commits

1. **Task 1 RED:** Assert PocketStation storage command entry — `5f0f50f` (`test(08-06)`)
2. **Task 1 GREEN:** Add bounded PocketStation flash transfers — `fa275a2` (`feat(08-06)`)
3. **Task 2:** Exercise PocketStation storage through the guest — `855061a` (`test(08-06)`)

The plan summary is committed separately from these task commits.

## Verification Evidence

- Pinned source lookup: `gh api repos/psx-spx/psx-spx.github.io/commits/6d7d1bc106a7e0b616b0330fe58401ab1ba57f0f --jq .sha` — exit 0; it returned the pinned commit. The transcript analysis was read from that revision.
- Native CI configure: `cmake --preset ci` — exit 0, 6.07 sec.
- Preflight: fixture inventory was sorted; the PocketStation fixture SHA-256 matched its manifest entry; the existing assembler/runner test registration and CI workflow wiring were present; the exact selected names were 8 card protocol/state tests and 1 fixture test.
- Repository gate: `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0, 3,898 of 3,901 lines.
- RetroArch budget: `cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake` — exit 0, 300 of 300 lines.
- Task 1 RED: `ctest --preset ci --no-tests=error -R '^card[.]protocol[.]pocketstation_metadata$' --output-on-failure --output-junit 08-06.QNyuz0/task1-red-target.xml` — exit 8, target assertion failed because 5Bh was unsupported; the GSD TDD evidence check returned `RED_EVIDENCE_OK`.
- Task 1 GREEN: `cmake --build --preset ci --target schwaystation-test-card` — exit 0. `ctest --preset ci --no-tests=error -R '^card[.](protocol|state)[.]' --output-on-failure` — exit 0, 8/8 passed in 0.61 sec.
- Task 2 RED: the prior card core built successfully; `ctest --preset ci --no-tests=error -R '^fixtures[.]pocketstation_storage$' --output-on-failure --output-junit 08-06.QNyuz0/task2-red-validated.xml` — exit 8, guest exited 0x53 waiting for 5Ch command ACK; GSD returned `RED_EVIDENCE_OK` (0.028 sec). The committed Task 1 source was restored immediately after the baseline run.
- Task 2 GREEN: `LC_ALL=C sort -cu tests/fixtures/inventory.txt && cmake --build --preset ci --target schwaystation-fixtures schwaystation-run && test -s build/ci/fixtures/pocketstation_storage.exe && ctest --preset ci --no-tests=error -R '^fixtures[.]pocketstation_storage$' --output-on-failure` — exit 0, guest fixture passed 1/1 in 0.02 sec.
- No hosted run was requested.

## Files Created/Modified

- `src/card.c` — bounded physical and selected-file mapping, storage transfer transcript, staged publish, and current-media validation during state restore.
- `tests/card/card_tests.c` — wire bounds, payload, generation, byte-order, malformed/abort, slot isolation, and snapshot restoration checks.
- `tests/fixtures/src/pocketstation_storage.asm` — actual JOY-register guest for 5Bh/5Ch reads and writes, directory seeding, invalid/unsupported controls, and abort handling.
- `tests/fixtures/manifest.txt` — updated exact guest-source digest.

## Decisions Made

- The pinned source leaves directory size and link byte order unspecified. The focused test uses little-endian 0x4000 size and 0x0006 link bytes; reversing either value makes the valid 1-to-7 chain invalid.
- A pending snapshot carries protocol and staged payload state only. It is accepted only if its transfer still maps through the receiving instance's current card image; receiving card bytes and generation remain in place.
- `tests/fixtures/tests.cmake` already registered this fixture with the required 51h success sentinel, so no caller registration change was needed.

## Deviations from Plan

None - the plan was executed as written.

## Issues Encountered

- The first abort-control assertion targeted card byte 0x200, which is a directory frame initialized to A0h. The guest was corrected to use untouched file data at 0x2000; the final real-host fixture then passed.
- No remaining issues or blockers.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 08-06 is complete. The orchestrator can merge this worktree and continue the phase workflow; shared state and phase verification remain with the orchestrator.

## Self-Check: PASSED

- All three task commits are ancestors of the plan head recorded above.
- All four modified files exist, the fixture digest matches its manifest, and the focused card and guest selections passed.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*
