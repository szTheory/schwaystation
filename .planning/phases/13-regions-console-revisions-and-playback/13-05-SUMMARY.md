---
phase: 13-regions-console-revisions-and-playback
plan: "05"
subsystem: runner
tags: [machine-profile, region-resolution, disc-probe, ctest]
requires:
  - phase: 13-01
    provides: Board profile API and runner-side region resolver seam.
  - phase: 13-03
    provides: Canonical board defaults and GPU profile pairing.
  - phase: 13-04
    provides: Validated board identity and board-sensitive state behavior.
provides:
  - Runner `--machine` selection for nine documented boards.
  - Automatic region selection from serial, disc licence, and executable hints.
  - Lightweight disc identity probing that avoids full-track SHA-1 during auto selection.
affects: [phase-13-final-verification, runner-cli]
actuals:
  tokens: 13756
  tasks: 2
  commits: 5
plan_head_before: d68151646fdd71b0dda5bdbb14953e98f401e017
plan_head_after: e108691d1d566eaf59eea7ce7422dcbc289d4e62
tech-stack:
  added: []
  patterns:
    - Keep the pure region precedence resolver separate from runner and media I/O.
    - Use lightweight host identity extraction for region choice and preserve full identity hashing for probes.
key-files:
  created:
    - runner/region.c
    - runner/region.h
    - tests/runner/test_region_probe.c
  modified:
    - host/disc_probe.c
    - host/disc_probe.h
    - runner/CMakeLists.txt
    - runner/main.c
    - runner/probe.c
    - runner/probe.h
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - docs/runner.md
key-decisions:
  - "Region auto uses serial, then disc licence, then executable marker; explicit region overrides the result while retaining disagreement diagnostics."
  - "Board selection is independent from title serial and region; PU-7 and PU-8 early use the locked GPU_V0 pairing."
requirements-completed: []
coverage:
  - id: D1
    description: The runner accepts all nine documented machine tokens and runs each with its canonical board/GPU profile pairing.
    requirement: PROFILE-02
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^runner[.]usage[.]machine$' --output-on-failure
        status: pass
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^runner[.]hash[.]machine$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D2
    description: Automatic region choice retains NTSC-U without hints, uses disc-derived PAL over a conflicting executable marker, and honors an explicit override.
    requirement: PROFILE-01
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^runner[.]region[.]auto$' --output-on-failure
        status: pass
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^runner[.]region[.]probe$' --output-on-failure
        status: pass
    human_judgment: false
  - id: D3
    description: Region selection skips full-track hashing while the complete identity probe retains its SHA-1 output and unmatched metadata remains redacted.
    requirement: PROFILE-01
    verification:
      - kind: integration
        ref: ctest --preset ci --no-tests=error -R '^(runner[.]disc[.]probe|compat[.]probe[.]full_identity)$' --output-on-failure
        status: pass
    human_judgment: false
duration: not-tracked
completed: 2026-11-23
status: complete
---

# Phase 13 Plan 13-05: Runner Machine and Auto Region Summary

**The runner now selects documented console boards independently from region and resolves automatic region hints without hashing the full disc track.**

## Performance

- **Duration:** Not tracked; no trustworthy UTC start/end interval was available. Active time was also not measured.
- **Started:** Not recorded.
- **Implementation and final focused acceptance completed:** 2026-11-23T19:05:10Z.
- **Runtime/model:** Codex GSD executor; configured `gpt-6-luna` xhigh; primary runtime model ID unavailable.
- **Tasks:** 2.
- **Implementation/test/documentation files modified:** 11.
- **Measured commits:** 5 commits between `plan_head_before` and `plan_head_after`, including the two task commits and three checked plan corrections.

## Accomplishments

- Added a pure host-side region resolver with serial, disc licence, and executable-marker precedence, conflict diagnostics, and NTSC-U fallback.
- Added an optional lightweight disc probe for region selection. Full `--probe` identity hashing remains intact, and unmatched identity data stays redacted.
- Added `--machine` for all nine documented board tokens and `--region auto`; explicit regions override detected hints.
- Verified no-hint fallback, a disc-derived PAL choice over an NTSC-U executable marker, explicit NTSC-J override, and existing NTSC-J/PAL hash regressions.

## Task Commits

1. **Task 1: Resolve host region hints and lint disagreements** — `e59ff11` (feat).
2. **Task 2: Expose independent runner machine and region options** — `e108691` (feat).

Checked plan corrections during execution: `60723cf` (lightweight region probe), `2181560` (fixture-expanded selector set), and `177b815` (all-nine machine-token checks).

## Files Created/Modified

- `runner/region.c` and `runner/region.h` — pure precedence resolver for region hints.
- `tests/runner/test_region_probe.c` — table-driven resolver tests.
- `host/disc_probe.c` and `host/disc_probe.h` — separate lightweight region probe from full identity hashing.
- `runner/CMakeLists.txt`, `runner/main.c`, `runner/probe.c`, and `runner/probe.h` — register shared resolver and expose board/region behavior.
- `tests/runner/tests.cmake` and `tests/runner/inventory.txt` — focused region, identity, and board tests.
- `docs/runner.md` — document the new runner options and precedence.

## Test Evidence

All checks were local to the CI preset; no hosted run URL applies.

| Command | Exit | Duration | Result |
| --- | ---: | ---: | --- |
| `LC_ALL=C sort -cu tests/runner/inventory.txt` | 0 | 0.00s | Runner inventory sorted. |
| `cmake -DROOT=. -DLIMIT=3901 '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -P tests/cmake/line_budget.cmake` | 0 | 0.02s | Task 1 repository-check gate passed at 3,901/3,901 lines. |
| `cmake --preset ci` | 0 | 0.75s; subsequent configure 0.74s | Task 1 configure and corrected selector registration passed. |
| Task 1 `ctest --show-only=json-v1` exact selector assertion | 0 | 0.03s | Exactly two tests were discovered: `runner.region.probe` and `runner.disc.probe`. |
| `cmake --build --preset ci --target schwaystation-disc-fixtures` | 0 | 0.02s | Fixture target completed. |
| `ctest --preset ci --no-tests=error -R '^policy[.]fixture_budget$' --output-on-failure` | 0 | 0.02s | Fixture extent and budget selector passed 1/1. |
| Task 1 focused target build | 1, then 0 | First attempt 0.27s; successful retry duration not captured | Initial build reported a missing generated include path on the new test target; adding `${PROJECT_BINARY_DIR}/generated` fixed the target, which then built successfully. |
| Task 1 region/disc focused CTest selector | 0 | 0.04s | `runner.region.probe` and `runner.disc.probe` passed 2/2. |
| Task 2 sorted inventory checks and 3,901-line gate | 0 | 0.00s each | Runner and compatibility inventories were sorted; the line gate remained 3,901/3,901. |
| `cmake --preset ci` | 0 | 0.81s | Reused and configured `build/ci`. |
| Task 2 `ctest --show-only=json-v1` exact selector assertion | 0 | 0.01s | Exact 16-test set including `compat.features.preflight` fixture setup. |
| `cmake --build --preset ci --target schwaystation-run schwaystation-test-region-probe schwaystation-test-probe schwaystation-run-test` | 0 | 1.00s | Focused runner and probe targets built after the final machine-token test change. |
| `ctest --preset ci --no-tests=error -R '^(runner[.]disc[.]probe|runner[.](region|machine|usage)[.]|runner[.]hash[.]machine$|runner[.]hash[.](ntsc_j|pal)$|compat[.]probe[.]full_identity$)' --output-on-failure` | 0 | 0.19s | Exact focused selection passed 16/16, including all nine board tokens, canonical profile acceptance, auto fallback/precedence, fixture setup, redaction, and full identity hash regression. |
| `git diff --check` | 0 | Not separately timed | No whitespace errors before the task commit. |

No broad suite, hosted CI, Act, Docker, or hardware run was needed because workflow wiring did not change. The saved evidence supports runner region/profile behavior only; it does not mark either phase-level requirement complete.

## Decisions Made

- Keep board, title serial, and video region as separate runner inputs and profile values.
- Use a lightweight disc identity path for automatic region selection while preserving the full content identity probe.
- Keep `PROFILE-01` and `PROFILE-02` pending until full Phase 13 verification.

## Deviations from Plan

None. The checked plan was updated during execution to account for the fixture-expanded selector graph and to include a bounded positive check for all nine machine tokens before final acceptance.

## Issues Encountered

The first Task 1 focused build failed in 0.27s because the new test target lacked the build-generated include directory required by the public header. Adding `${PROJECT_BINARY_DIR}/generated` to the target fixed it; the focused build and 2-test selection then passed. No product-code workaround was needed.

## User Setup Required

None.

## Next Phase Readiness

Plan 13-05 is complete. Plan 13-06 is next. `PROFILE-01` and `PROFILE-02` remain pending until the remaining Phase 13 evidence and final verification are complete.

---
*Phase: 13-regions-console-revisions-and-playback*
*Completed: 2026-11-23*

## Self-Check: PASSED

The plan's task commits are present; the exact focused selector passed 16/16 after the final test change, and the summary records that evidence and keeps the phase requirements pending.
