---
phase: 04-games-play-in-retroarch
plan: "16"
subsystem: serial-io
tags: [C, API-4, SIO0, memory-card, state]
requires:
  - phase: 04-games-play-in-retroarch
    plan: "03"
    provides: API v4 base types and the validated state visitor.
  - phase: 04-games-play-in-retroarch
    plan: "14"
    provides: Scheduled SIO0 byte delivery and digital controller transactions.
  - phase: 04-games-play-in-retroarch
    plan: "15"
    provides: API v4 caller and saved in-flight SIO protocol state.
provides:
  - Two instance-owned formatted 131,072-byte memory-card images.
  - API v4 read-only spans and exact-size pre-execution raw card import.
  - Bounded SIO card Read and GetID transactions on both ports.
  - Saved in-flight card protocol state with persistent card images outside ordinary states.
affects:
  - "04-33 card writes"
  - "04-34 host import and state integration"
  - libretro persistence
actuals:
  tokens: 12080
  tasks: 2
  commits: 4
commits: 4
plan_head_before: 4ef4810983289d0d33c7e819988b0477e2409a51
plan_head_after: 0bf37baf838f1ea58f1a3a7a5e45a3d8a3b93dd0
tech-stack:
  added: []
  patterns:
    - Per-instance card protocol state uses the existing validated state visitor.
    - Persistent card bytes remain stable while protocol progress is serialized.
key-files:
  created:
    - src/card.c
    - src/card.h
    - tests/card/card_tests.c
  modified:
    - CMakeLists.txt
    - docs/embedding.md
    - docs/hardware/sio.md
    - docs/runner.md
    - include/schwaystation.h
    - src/instance.c
    - src/internal.h
    - src/sio.c
    - src/state.c
    - tests/api/v4.c
    - tests/card/inventory.txt
    - tests/card/tests.cmake
    - tests/embed/CMakeLists.txt
    - tests/fixtures/tests.cmake
    - tests/inventory/ci.txt
key-decisions:
  - Keep API, state format, and behavior revision at 4 under the approved unreleased-v4 additive policy; preserve frozen v1-v3 callers.
  - Keep card bytes and generations outside ordinary save states while saving only in-flight protocol state.
  - Model card ACK at 1,500 cycles from transmit start and the 31,000-cycle pause after Read reply byte seven; these are not hardware measurements.
patterns-established:
  - Register card behavior tests in both the CMake target and byte-sorted inventory before configuring.
  - Serve card sectors from stable instance-owned spans without host file access.
requirements-completed: []
coverage:
  - id: D1
    description: API v4 exposes stable RAM, scratchpad, VRAM, BIOS, and card spans with exact-size pre-execution card import.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "abi.api_v4 via ctest --test-dir build/ci -R '^abi\\.api_v4$'"
        status: pass
    human_judgment: false
  - id: D2
    description: Formatted card images return the requested imported bytes through real SIO on both ports, including first and last sectors, with modeled timing and no ACK on the final byte.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "card.protocol.read via ctest --test-dir build/ci -R '^card\\.protocol\\.read$'"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; all tests and release checks passed)"
        status: pass
    human_judgment: false
  - id: D3
    description: Pending protocol state resumes in another instance without restoring or moving its persistent card bytes, and power reset preserves bytes and span addresses.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "card.protocol.read_edges via ctest --test-dir build/ci -R '^card\\.protocol\\.read_edges$'"
        status: pass
      - kind: unit
        ref: "abi.api_v4 state visitor confirms card bytes and generations are excluded"
        status: pass
    human_judgment: false
  - id: D4
    description: Invalid slots and sizes, invalid sectors, unsupported commands, and repeated reads preserve card images and recover without hanging.
    requirement: GAME-04
    verification:
      - kind: unit
        ref: "card.protocol.read_edges and abi.api_v4 focused CTest cases"
        status: pass
    human_judgment: false
duration: 44min
completed: 2026-11-11
status: complete
---

# Phase 4 Plan 16: Memory-card Read and GetID Summary

**API v4 now exposes stable raw memory and card spans, while two instance-owned cards answer bounded Read and GetID commands over timed SIO.**

## Performance

- **Duration:** 44 min
- **Started:** 2026-11-11T06:23:52Z
- **Completed:** 2026-11-11T07:08:18Z
- **Tasks:** 2
- **Files modified:** 18

## Accomplishments

- Added formatted, instance-owned card images and API v4 access to RAM, scratchpad, VRAM, BIOS, and both cards. Raw imports require an exact image size and must happen before execution.
- Implemented bounded Read and GetID exchanges on both SIO ports, with the 1,500-cycle ACK, the 31,000-cycle pause after reply byte seven, and no final-byte ACK.
- Saved in-flight card protocol state while keeping card bytes and generations outside ordinary save states; tested invalid input, state transfer, instance independence, reset stability, and repeat reads.

## Task Commits

1. **Task 1 RED: add card ACK timing test** - `4046f2d` (`test`)
2. **Task 1 GREEN: read cards through SIO spans** - `7e3a4ba` (`feat`)
3. **Task 2: cover read edges and state ownership** - `7c5e5d4` (`test`)
4. **Direct docs integration: refresh the runner state-hash example** - `0bf37ba` (`docs`)

## Files Created/Modified

- `src/card.c`, `src/card.h` — per-instance card storage, formatted blank data, Read/GetID protocol, and protocol-state validation.
- `src/sio.c`, `src/internal.h`, `src/instance.c`, `src/state.c`, `CMakeLists.txt` — dispatch, ownership, stable spans/import API, state visitor, and build linkage.
- `include/schwaystation.h`, `tests/api/v4.c`, `tests/card/card_tests.c`, `tests/card/tests.cmake`, `tests/card/inventory.txt`, `tests/inventory/ci.txt` — additive API and gating coverage.
- `docs/hardware/sio.md`, `docs/embedding.md`, `docs/runner.md` — card timing, raw spans, save-state behavior, and current state-hash example.
- `tests/embed/CMakeLists.txt`, `tests/fixtures/tests.cmake` — update the embedding target assertion and v4 content-state hashes for the newly serialized protocol fields.

## Decisions Made

- Kept the unreleased API, state format, and behavior revision at version 4 as authorized for Phase 04; v1-v3 callers remain frozen.
- Kept card contents outside ordinary states so loading protocol state cannot rewind persistent card bytes.
- Kept timing values as deterministic model values rather than claiming a new measurement from hardware.

## TDD Gate Compliance

- RED was committed as `4046f2d`. The registered `card.protocol.read` test exited 8 while the card ACK implementation was absent; the failed assertions were the expected RED, and `check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- GREEN was committed as `7e3a4ba`. The focused `card.protocol.read` command passed after implementation and again at the tracer feedback gate.
- Task 2 added the required edge cases after the tracer passed. It did not relabel a passing regression as RED.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Place the long Read wait after the seventh reply byte**
- **Found during:** Task 1 source review.
- **Issue:** The first implementation scheduled the 31,000-cycle wait after the fifth reply byte, before the sector address was complete.
- **Fix:** Moved the pause to the seventh Read reply byte and asserted its exact TX-relative deadline in the card test.
- **Files modified:** `src/card.c`, `tests/card/card_tests.c`.
- **Verification:** `card.protocol.read` passed in the focused check, tracer gate, and full CI.
- **Committed in:** `7e3a4ba`.

**2. [Rule 3 - Blocking integration] Synchronize documented API names, embedding targets, and content-state hashes**
- **Found during:** First full CI closeout.
- **Issue:** The public-name check lacked new slot names, the embedding consumer rejected the added card object target, and two expected content-state hashes changed when card protocol fields joined format 4 state.
- **Fix:** Updated the embedding guide and target assertion, then refreshed the two unreleased-v4 state hash expectations and the runner output example. The approved v4 compatibility policy and frozen v1-v3 callers remain intact.
- **Files modified:** `docs/embedding.md`, `docs/runner.md`, `tests/embed/CMakeLists.txt`, `tests/fixtures/tests.cmake`.
- **Verification:** The four affected checks passed; final full CI passed.
- **Committed in:** `7c5e5d4`, `0bf37ba`.

**Total deviations:** 2 auto-fixed (one Rule 1 bug, one Rule 3 integration issue).
**Impact on plan:** Required timing and repository-integration corrections; no new feature scope or gate was added.

## Issues Encountered

The first full CI run exposed the four directly attributable integration expectations described above. After updating them, the affected checks and final full workflow passed. No unresolved issues remain.

## User Setup Required

None.

## Next Phase Readiness

The core read path and raw card import API are ready. Card writes remain assigned to 04-33; host import and state integration remain assigned to 04-34.

## Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-card && ctest --test-dir build/ci -R '^card\.protocol\.read$' --no-tests=error --output-on-failure` — exit 0 after the Task 1 commit and at the tracer gate.
- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-card schwaystation-test-api-v4 && ctest --test-dir build/ci -R '^(card\.protocol\.read_edges|abi\.api_v4)$' --no-tests=error --output-on-failure` — exit 0.
- `ctest --test-dir build/ci -R '^(fixtures\.gpu_motion\.state\.(150|180)|embed\.subdirectory|docs\.api_names)$' --no-tests=error --output-on-failure` — exit 0 after updating directly affected integration expectations.
- `build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 150 --hash-state` — exit 0; output matched the refreshed documentation example.
- `ctest --test-dir build/ci -R '^docs\.budgets$' --no-tests=error --output-on-failure` — exit 0.
- `cmake --workflow --preset ci` — exit 0; all 441 tests and both release checks passed.
- `cmake --workflow --preset hygiene` — exit 0; all 23 hygiene and policy checks passed.

## Self-Check: PASSED

- Confirmed all created source and test files and the SUMMARY exist.
- Confirmed all four measured plan commits are ancestors of HEAD.
- `git diff --check 4ef4810..HEAD` exited 0.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*
