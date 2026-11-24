---
phase: 14-multitap-and-special-controllers
plan: "13"
subsystem: libretro-memory-card-persistence
tags: [libretro, memory-card, multitap, atomic-saves]
requires:
  - phase: 14-multitap-and-special-controllers
    plan: "08"
    provides: stable socket-aware card paths and atomic host saves
provides:
  - Seven independently loaded and dirty-flushed libretro sidecars
  - Stable socket paths anchored to the original content basename
  - A focused round-trip and failed-replacement host test
affects: [libretro, memory-card-persistence, phase-14-verification]
actuals:
  tokens: 4603
  tasks: 1
  commits: 2
commits: 2
plan_head_before: 4e0ba28d93ed452c38ad8462fa7e1bf9b5815c17
plan_head_after: 444e8af72d5eda28b85af17eda1644ae4056bc1c
tech-stack:
  added: []
  patterns:
    - per-socket host path and saved-generation tracking
    - frontend ownership excludes port1A from adapter persistence
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
key-decisions:
  - Keep port1A exclusively frontend-owned through the existing SAVE_RAM span.
  - Resolve the seven adapter sidecars once from the initial content basename and preserve those paths across disc swaps.
  - Advance each saved generation only after its atomic replacement succeeds.
patterns-established:
  - Enumerate adapter-owned memory cards by stable one-based physical port/socket coordinates.
  - Skip clean card images and retry failed generations at later lifecycle flush points.
requirements-completed: []
coverage:
  - id: D1
    description: Seven adapter-owned card images load, flush, and round-trip independently at stable socket paths while frontend SAVE_RAM remains the unchanged port1A span.
    requirement: SIO-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.](cards[.](multitap|persistence|controls)|memory[.](aliases|boundaries))$' --output-on-failure"
        status: pass
    human_judgment: false
metrics:
  duration: 20m
  completed_date: 2026-11-24
duration: 20m
completed: 2026-11-24
status: complete
---

# Phase 14 Plan 13: Libretro Eight-Card Persistence Summary

**Seven libretro adapter sidecars now load and flush independently, with port1A remaining frontend-owned SAVE_RAM.**

## Performance

- **Runtime/model:** Codex GSD executor; OpenGSD 1.23.0; configured `gpt-6-luna` xhigh.
- **Duration:** 20 minutes.
- **Started:** 2026-11-24T10:56:13Z.
- **Completed:** 2026-11-24T11:16:01Z (implementation, focused verification, summary, state closeout, and metadata commit).
- **Tasks:** 1.
- **Files modified:** 2.

## Accomplishments

- Loaded the seven adapter-owned cards from stable port/socket sidecars. Port2A remains `<content>.1.mcr`; the other sockets use the existing `.p1b`–`.p2d` names.
- Kept port1A on its unchanged `RETRO_MEMORY_SAVE_RAM` pointer and size contract; the adapter never loads or flushes its `.srm` file.
- Flushes only dirty generations at the existing lifecycle points. A failed atomic replacement leaves its previous generation unsaved so a later flush retries it.
- Added a short private-module host test for seven distinct writes, stable paths, clean-card no-write behavior, failed replacement/retry, and unload/reload persistence without executing a BIOS boot.

## Task Commits

1. **Task 1 RED:** `7686490` — add failing socket persistence assertions and test-only inspection hooks.
2. **Task 1 GREEN:** `444e8af` — persist the seven adapter-owned card sockets independently.

## Files Modified

- `libretro/libretro.c` — per-socket path, load, generation, and atomic flush lifecycle.
- `tests/libretro/host.c` — focused eight-card host persistence assertions.

## Decisions Made

- The shared host helper owns filename construction; the adapter retains the resolved paths from the initial content identity.
- Port1A is excluded from the adapter loop; saved generations advance only after a successful complete-image replacement.

## TDD Gate Compliance

- **RED:** The direct `libretro.cards.multitap` host run exited 1 on the assertion that the six additional socket paths were missing. Its TAP report was accepted as `RED_EVIDENCE_OK` by `gsd-tools check tdd-red-evidence`.
- **GREEN:** The implementation passed the focused five-selector CTest run. No additional refactor was needed.

## Verification

- `LC_ALL=C sort -cu tests/libretro/inventory.txt` — exit 0.
- `ctest --preset ci --show-only=json-v1 -R '^libretro[.](cards[.](multitap|persistence|controls)|memory[.](aliases|boundaries))$' | python3 -c 'import json,sys; actual={t["name"] for t in json.load(sys.stdin)["tests"]}; required=set("libretro.cards.multitap libretro.cards.persistence libretro.cards.controls libretro.memory.aliases".split()); print("missing selectors:", sorted(required-actual)); sys.exit(bool(required-actual))'` — exit 0; all five selectors were present.
- `cmake --preset ci` — exit 0; reused `build/ci`.
- `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro --parallel 4` — exit 0.
- `build/ci/tests/schwaystation-test-libretro-host build/ci/libschwaystation-libretro-card-test.so cards-multitap build/ci/tests/libretro-cards-multitap` — RED exit 1; the TAP report failed the expected sidecar-path assertion. `gsd-tools check tdd-red-evidence .gsd/tdd-evidence/14-13/red.json --raw` returned `RED_EVIDENCE_OK` (exit 0).
- `ctest --preset ci --no-tests=error -R '^libretro[.]cards[.](multitap|persistence|controls)$' --output-on-failure` — exit 0; 3/3 card selectors passed.
- `ctest --preset ci --no-tests=error -R '^libretro[.](cards[.](multitap|persistence|controls)|memory[.](aliases|boundaries))$' --output-on-failure` — exit 0; 5/5 passed in 5.90 seconds.
- `git diff --check` — exit 0 before commits.
- No broad suite, hosted CI, Act, or Docker run; Plan 15 owns the final reviewed-head integrated verification.

## Auto-Fixed Issues

**1. [Rule 3 - Blocking test build issue] Corrected private test-hook portability.**

- **Found during:** Task 1 RED setup.
- **Issue:** Strict warnings rejected a const-dropping memory cast, and the host harness lacked a portable `rmdir` declaration.
- **Fix:** Mutated the test fixture through the instance-owned card slot and included the platform directory-removal declaration.
- **Files modified:** `libretro/libretro.c`, `tests/libretro/host.c`.
- **Verification:** The focused build and all five owned selectors passed.
- **Committed in:** `7686490`.

**Total deviations:** 1 auto-fixed Rule 3 issue. **Impact:** No scope change; the corrections only allowed the planned private test harness to compile portably.

## Issues Encountered

The first build attempt exposed the test-hook cast warning; the next exposed the missing `rmdir` declaration. Both were fixed before accepting RED evidence. A CTest run made after the failed build used the stale host executable and was discarded; the direct TAP test run then produced the valid failing behavior assertion.

The per-plan commit ledger could not be written under the sandbox's read-only `.git` permission. The two task commits were measured from the plan-start HEAD captured in this summary's frontmatter.

## User Setup Required

None.

## Next Phase Readiness

This plan’s persistence slice is complete. Shared requirement SIO-01 remains gated until its other declaring plans and phase verification complete. Plan 15 owns code review and final integrated verification.

---
*Phase: 14-multitap-and-special-controllers*
*Completed: 2026-11-24*

## Self-Check: PASSED

The summary exists, both task commits are ancestors of HEAD, and the GSD coverage classifier accepted the passing integration deliverable.
