---
phase: 08-per-game-card-saves
plan: "03"
subsystem: runner-persistence
tags: [memory-cards, save-identity, ps-exe, sha256]

requires:
  - phase: 08-01
    provides: Runner two-slot generation-based persistence and the round-trip guest.
  - phase: 08-02
    provides: Validated card-file handling and the checked identity path helper.
provides:
  - Opt-in PS-EXE serial and content-digest save identities for the runner.
  - One resolved identity shared by both runner card slots.
  - Regression coverage for literal overrides, full-byte digests, and two-game isolation.
affects: [08-08, runner memory-card persistence]

actuals:
  tokens: 8882
  tasks: 2
  commits: 4
plan_head_before: 564ec69bd26075b9185181b31cedf3d2150c38d3
plan_head_after: a8abb369e3403c6ac0787b0b265a9b05ac0b78e4

tech-stack:
  added: []
  patterns:
    - Domain-separated SHA-256 with version, media-kind, and fixed-endian length framing.
    - Resolve once after executable validation and reuse the result for both slots.
key-files:
  created:
    - host/save_game_identity.c
    - host/save_game_identity.h
  modified:
    - runner/main.c
    - runner/CMakeLists.txt
    - tests/runner/tests.cmake
    - docs/runner.md
key-decisions:
  - "Use a complete PS-EXE filename stem for serial extraction; otherwise hash all validated bytes while excluding the path."
  - "Keep explicit serial overrides literal and preserve the libretro frontend naming policy."
patterns-established:
  - "The runner resolves a save identity once, then derives both slot paths from that same value."
  - "Digest names include the full 64 lowercase hex digits under the versioned sgi1- prefix."
requirements-completed:
  - SAVE-01
  - SAVE-03
coverage:
  - id: SAVE-01-identity
    description: Runner serial naming resolves a complete PS-EXE stem or a full framed executable digest while preserving literal overrides.
    requirement: SAVE-01
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-run-stop schwaystation-fixtures && ctest --preset ci --no-tests=error -R '^(runner[.]cards[.](controls|persistence)|fixtures[.]memcard_roundtrip[.]persistence)$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: SAVE-03-two-slot-isolation
    description: Two automatic identities keep both card slots isolated across fresh runner processes and preserve complete files on refusal or publication failure.
    requirement: SAVE-03
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(runner[.]cards[.](controls|persistence|sigterm)|fixtures[.]memcard_roundtrip[.](persistence|controls))$' --output-on-failure"
        status: pass
    human_judgment: false
metrics:
  duration: 39min
  completed: 2026-11-19
  status: complete
---

# Phase 8 Plan 03: Resolve executable save-game identities and prove two-slot isolation

The runner now derives opt-in save identities from complete PS-EXE serial stems or a versioned SHA-256 of every validated executable byte, and applies one identity consistently to both card slots.

## Performance

- **Duration:** About 39 minutes wall time; active time was not separately measured.
- **Started:** 2026-11-19T14:27:47Z (Phase 8 execution start recorded in STATE.md).
- **Completed:** 2026-11-19.
- **Tasks:** 2.
- **Files modified:** 6.
- **Runtime/model:** Codex / OpenGSD CLI 1.20.0; executor gpt-6-luna.

## Accomplishments

- Added a private host resolver that normalizes a complete serial-shaped PS-EXE filename stem, preserves literal override bytes, and otherwise hashes the full validated image with versioned, fixed-endian framing.
- Changed runner serial mode to require `--save-dir`; automatic naming requires a standalone PS-EXE, while disc or content-free runs continue to accept literal overrides. Default basename behavior and libretro naming are unchanged.
- Proved two different identities map to four separate slot files and that a fresh process reloads and updates only its selected pair. Failure tests preserve the prior complete files.
- Documented serial grammar, identity precedence, digest framing, host wrapper export support, and I/O failure behavior.

## Verification Evidence

| Check | Result | Evidence |
| --- | --- | --- |
| Task 1 build and exact selector | Exit 0; 3/3 tests passed | `cmake --preset ci && cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-run-stop schwaystation-fixtures && ctest --preset ci --no-tests=error -R '^(runner[.]cards[.](controls|persistence)|fixtures[.]memcard_roundtrip[.]persistence)$' --output-on-failure`; local run, no hosted URL. |
| Task 2 integration selector | Exit 0; 5/5 tests passed | `ctest --preset ci --no-tests=error -R '^(runner[.]cards[.](controls|persistence|sigterm)|fixtures[.]memcard_roundtrip[.](persistence|controls))$' --output-on-failure`; local run, no hosted URL. |
| Test-first behavior | Both pre-implementation runner selectors failed with CTest exit 8 on the prior `--serial`-required behavior; `gsd_run check tdd-red-evidence` accepted both RED reports. | No hosted URL. |
| Preflight | Sorted runner and fixture inventories; guest source 7,030 bytes and built PS-EXE 4,096 bytes; repository checks 3,898/3,901 lines and RetroArch E2E 300/300 lines. | Completed before configure/build; no hosted run. |
| Working tree check | `git diff --check` passed; `.planning/config.json` was left unstaged and unchanged by this plan. | Local repository check, no hosted URL. |

No hosted CI or RetroArch run was required by this plan. The configured build found no work to rebuild on the final run, and only the plan's named runner and fixture selectors were executed.

## Task Commits

1. **Task 1: Resolve the two-slot identity for validated PS-EXE content** — `1d9bf95` (test), `100929b` (feat).
2. **Task 2: Prove automatic isolation and document the naming contract** — `929b301` (test), `a8abb36` (docs).

The plan commit range is measured from `564ec69` through `a8abb36` and contains four task commits. The execution metadata commit is recorded separately by GSD.

## Files Created/Modified

- `host/save_game_identity.c` and `.h` — tagged identity resolution, complete-stem serial parsing, and full-byte digest framing.
- `runner/main.c` — post-validation resolution and consistent two-slot path selection.
- `runner/CMakeLists.txt` — links the resolver into the shared runner source list.
- `tests/runner/tests.cmake` — serial, literal override, digest, tail-byte, refusal, and two-game persistence regressions.
- `docs/runner.md` — runner naming and persistence contract.

## Decisions Made

The explicit `--serial` argument remains a literal compatibility override. Automatic serial naming comes only from a complete source filename stem; if that grammar does not match, the full validated PS-EXE bytes determine the namespaced digest. Both slots use the same resolved value.

## Deviations from Plan

### Auto-fixed Issues

**1. Corrected focused test harness setup and repeatability**
- **Found during:** Tasks 1 and 2.
- **Issue:** The runner CMake script initially omitted the fixture path; a case-sensitive filename assertion was invalid on the case-insensitive development volume; and reruns reused temporary identity files.
- **Fix:** Forwarded the fixture path, compared the actual literal filename, and cleared/restored per-run test files so repeated selectors are isolated.
- **Files modified:** `tests/runner/tests.cmake`.
- **Verification:** The final exact Task 1 selector passed 3/3 and Task 2 selector passed 5/5.
- **Committed in:** `100929b`.

**Total deviations:** 1 test-harness correction. No product scope changed.

## Issues Encountered

The first focused persistence retry found state left by a previous invocation; the test now clears its isolated directory. A failure-path assertion also needed to restore its intentionally corrupted second slot before checking the later rename failure. These were test-isolation fixes; the final scoped selectors passed.

## User Setup Required

None.

## Next Phase Readiness

The executable identity seam and runner two-slot behavior are ready for the later plan that adds automatic disc and multi-disc-set identities. No libretro policy was changed.

## Self-Check: PASSED

The summary exists, all four plan commits are ancestors of the current branch head, the six modified files match the plan, and `git diff --check` passed. `.planning/config.json` remains the sole uncommitted user change.
