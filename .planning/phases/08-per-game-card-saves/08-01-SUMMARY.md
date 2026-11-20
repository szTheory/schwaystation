---
phase: 08-per-game-card-saves
plan: "01"
subsystem: testing
tags: [runner, memory-cards, libretro, cmake, ctest]

requires: []
provides:
  - Literal serial card naming is verified across fresh runner processes and both card slots.
  - A portable PS-EXE guest verifies the standard card ID exchange through JOY MMIO.
  - True libretro save claims require both registered card persistence tests.
affects: [phase-08-per-game-card-saves, pocketstation]

actuals:
  tokens: 4564
  tasks: 3
  commits: 3
  plan_head_before: f5a75ad29717e16af667270839493a9bfd861984
  plan_head_after: 38ad4bb1c0e3a33e866daffa58eaacf845ccda03

tech-stack:
  added: []
  patterns:
    - Inventory-registered PS-EXE guests test host-visible protocol behavior.
    - Boolean libretro claims are tied to exact test inventory rows.

key-files:
  created:
    - tests/fixtures/src/pocketstation_storage.asm
  modified:
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/fixtures/manifest.txt
    - tests/cmake/info_claims.cmake
    - tests/cmake/info_claims_self_test.cmake

key-decisions:
  - "Preserve the caller-supplied serial spelling for literal runner card filenames."
  - "Map a true libretro_saves claim to both card persistence and controls tests while keeping the template false."
  - "Use the standard card ID guest as the reusable transport base for later PocketStation transfers."

patterns-established:
  - "Bound guest-side JOY waits and report distinct transport failures through the debug exit port."
  - "Require exact per-directory CTest inventory entries for portable guest fixtures."

requirements-completed:
  - SAVE-01
  - SAVE-03
  - SAVE-04
  - SAVE-05

coverage:
  - id: D1
    description: "The runner preserves a literal mixed-case serial for both card filenames across two fresh processes."
    requirement: SAVE-01
    verification:
      - kind: integration
        ref: "ctest -R '^fixtures[.]memcard_roundtrip[.](persistence|controls)$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "Both runner card slots reload and advance their persisted values across fresh processes."
    requirement: SAVE-03
    verification:
      - kind: integration
        ref: "fixtures.memcard_roundtrip.persistence and fixtures.memcard_roundtrip.controls"
        status: pass
    human_judgment: false
  - id: D3
    description: "The manifested portable guest checks the standard card ID response and has an exact CTest selector."
    requirement: SAVE-05
    verification:
      - kind: integration
        ref: "fixtures.pocketstation_storage"
        status: pass
      - kind: other
        ref: "fixtures inventory sort and pocketstation_storage.asm manifest SHA-256 check"
        status: pass
    human_judgment: false
  - id: D4
    description: "A true libretro save claim is accepted only when both card persistence and controls rows exist."
    requirement: SAVE-04
    verification:
      - kind: integration
        ref: "libretro.info.claims and libretro.info.claims.self_test"
        status: pass
      - kind: other
        ref: "cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake"
        status: pass
    human_judgment: false

duration: 74min
completed: 2026-11-19
status: complete
---

# Phase 8 Plan 1: Prove the Existing Card Path and Register Phase 8 Checks Summary

**Fresh runner processes prove literal serial saves, a PS-EXE guest checks card ID over JOY, and libretro save claims require both card tests.**

## Performance

- **Duration:** 1h 14m
- **Started:** 2026-11-19T11:43:23Z
- **Completed:** 2026-11-19T12:56:58Z
- **Runtime/model:** Codex with OpenGSD CLI 1.20.0; executor model gpt-6-luna
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments

- Extended the runner round trip to use the exact literal serial `sLuS_123.45`, verify the two named 131,072-byte card files, check counters after each process, and compare distinct frame-120 hashes.
- Added a manifested PS-EXE guest that verifies the `81h/53h` card ID exchange through JOY MMIO. Receive, ACK, and release waits are bounded; transport failures and success use distinct debug exit bytes.
- Added the exact portable fixture selector and kept its inventory byte-sorted.
- Guarded the libretro `libretro_saves` claim as a boolean and require both `libretro.cards.persistence` and `libretro.cards.controls` before accepting `true`. The `.info` template remains `false`.

## Task Commits

1. **Task 1: Prove literal serial saves through two runner processes** - `a140c81` (`test(08-01): prove literal serial card saves`)
2. **Task 2: Register the owned PocketStation guest and exact inventory** - `42fcbd8` (`test(08-01): register PocketStation storage fixture`)
3. **Task 3: Wire the save claim guard before enabling it** - `38ad4bb` (`test(08-01): guard libretro save claims with card tests`)

## Files Created/Modified

- `tests/fixtures/tests.cmake` - literal serial persistence assertions, fixture build wiring, and exact portable test registration.
- `tests/fixtures/inventory.txt` - sorted `fixtures.pocketstation_storage` selector.
- `tests/fixtures/manifest.txt` - source digest for the new guest.
- `tests/fixtures/src/pocketstation_storage.asm` - bounded standard card ID guest with distinct failure codes.
- `tests/cmake/info_claims.cmake` - boolean validation and both-test mapping for a true save claim.
- `tests/cmake/info_claims_self_test.cmake` - synthetic true claim, malformed value, and one-missing-row controls.

## Decisions Made

- Kept the supplied serial spelling unchanged in the literal override path.
- Left the libretro save claim disabled in the template; the checker now requires both existing card tests before a true claim can pass.
- Used the standard card ID transaction as the test guest’s reusable base for later PocketStation transfer coverage.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test Bug] Fixed the PocketStation guest output matcher.**
- **Found during:** Task 2
- **Issue:** The first CTest run exited 8 because the escaped newline regex did not match, although the runner returned status 6 and printed `exit code=0x51`.
- **Fix:** Matched the stable `exit code=0x51` text directly.
- **Files modified:** `tests/fixtures/tests.cmake`
- **Verification:** The corrected Task 2 command passed with exit 0. The guest already exercised the implemented card ID behavior; no core behavior change was needed.
- **Committed in:** `42fcbd8`

**2. GSD plan-head ledger location was adjusted for the workspace sandbox.**
- **Issue:** Directly creating the required ledger under `.git/` returned `operation not permitted`.
- **Recovery:** Stored the starting HEAD in a temporary `.planning/` ledger, measured the three task commits with `git rev-list`, and left the user’s config edit unstaged. Removed the temporary ledger after recording the measured range here.

## Issues Encountered

- The first Task 2 assertion run was rejected by its regex despite the guest producing the expected exit byte. This was fixed before the task commit; the passing rerun is the final evidence.
- Git metadata writes required the standard sandbox escalation. All three task commits ran the configured `.githooks` hygiene hook with the GitHub noreply identity.
- `state.update-progress` reported no `Progress:` line in this `STATE.md` body and made no change; `state.advance-plan` updated the plan position and frontmatter progress count.

## Verification Evidence

All checks ran locally from the repository root; no hosted run URL applies.

| Check | Result | Evidence |
|---|---|---|
| Task 1 runner round trip and controls | Exit 0; 2/2 tests passed | `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-fixtures && ctest --preset ci --no-tests=error -R '^fixtures[.]memcard_roundtrip[.](persistence|controls)$' --output-on-failure` |
| Task 2 exact selector after configure | Exit 0; one test selected | `cmake --preset ci && ctest --test-dir build/ci --no-tests=error -N -R '^fixtures[.]pocketstation_storage$'` |
| Task 2 first assertion attempt | Exit 8; guest returned expected status and byte, but the regex was too strict | `LC_ALL=C sort -cu tests/fixtures/inventory.txt && cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-fixtures && test -s build/ci/fixtures/pocketstation_storage.exe && ctest --preset ci --no-tests=error -R '^fixtures[.]pocketstation_storage$' --output-on-failure --output-junit build/ci/pocketstation_storage.xml` |
| Task 2 guest and inventory | Exit 0; 1/1 test passed | `LC_ALL=C sort -cu tests/fixtures/inventory.txt && cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-fixtures && test -s build/ci/fixtures/pocketstation_storage.exe && ctest --preset ci --no-tests=error -R '^fixtures[.]pocketstation_storage$' --output-on-failure` |
| Task 3 claim guard and self-test | Exit 0; 2/2 tests passed; line budget 3898/3901 | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake && cmake --preset ci && ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure` |

## User Setup Required

None.

## Next Phase Readiness

The runner path, reusable guest, exact inventory, and libretro claim guard are ready for the remaining Phase 8 plans. The save claim remains false until a later plan enables it after the required runtime acceptance is implemented.

## Self-Check: PASSED

- Summary and created guest fixture exist.
- All three task commits are ancestors of the current phase branch head.
- The measured plan commit range is 3 commits from `f5a75ad29717e16af667270839493a9bfd861984` through `38ad4bb1c0e3a33e866daffa58eaacf845ccda03`.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*
