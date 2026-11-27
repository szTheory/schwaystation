---
phase: 08-per-game-card-saves
plan: "05"
subsystem: libretro
tags: [libretro, card-saves, memory-cards, ctest]
requires:
  - phase: 08-01
    provides:
      - "Per-game card slot and generation tracking"
  - phase: 08-02
    provides:
      - "Typed card import outcomes and atomic generation-aware saves"
provides:
  - "Safe card 2 persistence for EXE, plain-disc, and playlist content"
  - "Fresh-instance frontend save and reinjection coverage for both cards"
affects: [phase-08, libretro]
actuals:
  tokens: 6791
  tasks: 2
  commits: 3
  plan_head_before: 08f0f724447eef0cfd157cb9d4b00a862091e9d8
  plan_head_after: 33640e6a3c7011948213a2bf2b1695bb2addc46a
tech-stack:
  added: []
  patterns:
    - "Frontend owns card 1 through RETRO_MEMORY_SAVE_RAM; the adapter owns card 2 sidecar persistence"
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
key-decisions:
  - "Arm card 2 only after content load succeeds and the existing card is either genuinely missing or fully imported."
  - "Keep the playlist's original frontend basename for card 2 across disk swaps."
requirements-completed: [SAVE-04]
coverage:
  - id: D1
    description: "Supported EXE, disc, and playlist content retains a stable frontend-derived card 2 path."
    requirement: SAVE-04
    verification:
      - kind: integration
        ref: "libretro.host.disc_control"
        status: pass
      - kind: integration
        ref: "libretro.content.lifecycle"
        status: pass
    human_judgment: false
  - id: D2
    description: "Rejected card imports and failed publishes preserve existing card 2 bytes."
    requirement: SAVE-04
    verification:
      - kind: integration
        ref: "libretro.cards.controls"
        status: pass
    human_judgment: false
  - id: D3
    description: "A fresh instance resumes both cards after frontend card 1 reinjection; the no-injection control detects a fresh card 1."
    requirement: SAVE-04
    verification:
      - kind: integration
        ref: "libretro.cards.persistence"
        status: pass
    human_judgment: false
duration: 30min
completed: 2026-11-19
status: complete
---

# Phase 08 Plan 05: Per-game card saves Summary

**Libretro keeps card 1 frontend-owned and persists card 2 for EXE, disc, playlist, and fresh-instance reloads.**

## Performance

- **Duration:** 30 min
- **Executor wall time:** 35 min 49 sec, including startup and summary checks.
- **Started:** 2026-11-19T15:21:06Z
- **Completed:** 2026-11-19T15:51:10Z
- **Tasks:** 2
- **Files modified:** 3
- **Runtime/model:** Codex, configured executor model `gpt-6-luna`

## Accomplishments

- Card 2 arms only after successful content loading and a missing or fully accepted import; its path uses the original content basename and stays fixed across playlist swaps.
- Rejected imports remain byte-for-byte intact after guest writes and unload, and failed atomic publishes preserve the previous save.
- The in-process frontend saves the complete card 1 image, reinjects it into a fresh instance, and verifies independent counter advances for both cards. An isolated no-injection control confirms card 1 starts fresh while card 2 resumes.

## Task Commits

1. **Task 1 RED: preserve rejected card imports** - `75c1269` (`test(08-05): preserve rejected card 2 imports`)
2. **Task 1: safely arm card 2 for libretro content** - `20e49df` (`feat(08-05): safely arm card 2 for libretro content`)
3. **Task 2: simulate frontend card reinjection** - `33640e6` (`test(08-05): simulate frontend card reinjection`)

**Plan metadata:** this summary commit.

## Files Created/Modified

- `libretro/libretro.c` - Gate card 2 imports and saves on successful content load and safe import outcomes; derive disc and playlist sidecar paths; expose test-only card 2 inspection accessors.
- `tests/libretro/host.c` - Verify stable content paths, rejected import preservation, fresh-instance reinjection, and the missing-injection control.
- `tests/libretro/tests.cmake` - Pass an isolated save directory to the disc-control host test. Build the card-inspection module only for tests.

## Decisions Made

- Preserve frontend ownership of card 1 through `RETRO_MEMORY_SAVE_RAM`; card 2 remains an adapter-managed sidecar.
- Preserve the playlist basename across member swaps, and do not arm card 2 after a rejected import.

## Verification Evidence

- Source preflight checked sorted inventory membership, exact selectors and registration, fixture producer/consumer wiring, fixture SHA256 `9c8e9f07c963e3cb8c56a90fc56df8046ecb36f80f71e244d73c3da0a22ee8c0`, CI preset wiring, and `libretro_saves = "false"` — exit 0, 1 sec.
- `cmake --preset ci` — exit 0; configured the native `build/ci` graph before exact CTest name checks.
- `cmake -DROOT="$PWD" '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0, 0 sec; 3898 lines.
- `cmake -DROOT="$PWD" -DPATTERNS=tests/retroarch/'*' -DLIMIT=300 -P tests/cmake/line_budget.cmake` — exit 0, 0 sec; 300 lines.
- Task 1 TDD RED: `ctest --preset ci --no-tests=error -R '^libretro[.]cards[.]controls$' --output-on-failure --output-junit /private/tmp/schwaystation-08-05.tHCjvsuU/task1-red.xml` — exit 8 as expected, 3 sec. The target assertion observed the rejected four-byte wrapper being replaced by a 131072-byte card image.
- `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence build/ci/task1-red-evidence.json --raw` — exit 0, 1 sec; returned `RED_EVIDENCE_OK` for the intended assertion.
- `cmake --build --preset ci --target schwaystation-test-libretro-host` — Task 1 fixture build exit 0, 8 sec; after implementation, exit 0, 1 sec; Task 2 rebuild exit 0, 1 sec.
- `ctest --test-dir build/ci --no-tests=error -R '^libretro[.](cards[.]controls|host[.]disc_control|content[.]lifecycle)$' --output-on-failure --output-junit /private/tmp/schwaystation-08-05.tHCjvsuU/task1-green.xml` — exit 0, 3 sec; all 3 tests passed.
- `ctest --test-dir build/ci --no-tests=error -R '^libretro[.]cards[.](controls|persistence)$' --output-on-failure --output-junit /private/tmp/schwaystation-08-05.tHCjvsuU/task2-green.xml` — exit 0, 5 sec; both tests passed.
- `libretro/schwaystation_libretro.info.in` remains `libretro_saves = "false"`. No hosted run or phase verification was requested or run.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

- The first compile reported missing prior prototypes for the test-only card 2 accessors. Guarded declarations were added, and the focused build then passed.
- The sandbox initially denied writes to the linked worktree Git index. The authorized narrow escalation let the GSD commit handler stage the explicit task paths and run the configured hooks; all three task commits succeeded.

## Wave Integration

The executor build exposed the two card-inspection accessors on the installable module. The disc-control and persistence tests now use a separate, uninstalled build of the same source. Other host tests use the installable module and fail if it exposes either accessor.

- `cmake --build --preset ci --target schwaystation-test-libretro-host` passed, exit 0 in 0.47 sec.
- `ctest --preset ci --no-tests=error -R '^libretro[.](cards[.](controls|persistence)|host[.]disc_control|content[.]lifecycle)$' --output-on-failure` passed, exit 0 in 2.90 sec.
- `nm -gU build/ci/libretro/schwaystation_libretro.dylib` and the same command on the test module both exited 0. The installable build had no card-inspection exports and the test module had both.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

Plan 08-05 is complete. The orchestrator owns wave merge, shared state updates, and phase gates. Keep `libretro_saves = "false"` until Plan 08-10 provides its hosted and in-process proof.

## Self-Check: PASSED

- All three modified key files exist, and all three task commits are ancestors of the current plan head.
- The exact Task 1 and Task 2 CTest selectors passed; coverage entries reference those passing cases.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*
