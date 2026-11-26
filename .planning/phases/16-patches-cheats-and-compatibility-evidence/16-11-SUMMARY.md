---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "11"
subsystem: testing
tags: [c, cmake, libfuzzer, replay, nightly]
requires:
  - phase: 16-03
    provides: Host-side disc patch parser and replay conventions.
  - phase: 16-09
    provides: Host cheat replacement, core RAM application, and fuzz workflow conventions.
provides:
  - Deterministic PPF/BPS and cheat replay selectors plus libFuzzer entry points.
  - Exact Nightly PPF and cheat campaign metadata, seed counts, and documentation.
affects: [testing, fuzz, nightly]
actuals:
  tokens: 9162
  tasks: 2
  commits: 2
  plan_head_before: 3d19212fec1794284941793b9102a8b6c3db6c6e
  plan_head_after: ec1cf4064101466a9a5d286a596698d042b9d014
tech-stack:
  added: []
  patterns:
    - Runtime-generated seeds exercise production parsers without committed binary fixtures.
    - Portable replay targets remain available outside the optional libFuzzer build.
key-files:
  created:
    - tests/fuzz/ppf.c
    - tests/fuzz/cheat.c
  modified:
    - tests/fuzz/tests.cmake
    - .github/workflows/nightly.yml
    - tests/cmake/workflows.cmake
    - docs/testing.md
key-decisions:
  - Use generated immutable source bytes for PPF/BPS tests and a generated RAM-backed instance for cheat tests.
  - Keep the full Nightly campaigns at 60 seconds per target, preserve existing counts, and leave the PBP-only route unchanged.
patterns-established:
  - Replay seed emitters declare exact seed names, counts, caps, and fixture identity before campaigns run.
requirements-completed: [PATCH-01, CHEAT-01]
coverage:
  - id: D1
    description: Deterministic patch parser replay and bounded fuzz entry point for PPF1/2/3 and BPS inputs.
    requirement: PATCH-01
    verification:
      - kind: unit
        ref: fuzz.regress.ppf
        status: pass
    human_judgment: false
  - id: D2
    description: Deterministic cheat replay for transactional replacement, caps, canonical bytes, and bounded RAM writes.
    requirement: CHEAT-01
    verification:
      - kind: unit
        ref: fuzz.regress.cheat
        status: pass
    human_judgment: false
  - id: D3
    description: Nightly routing preserves prior campaigns and runs PPF/cheat campaigns with exact seed metadata.
    requirement: PATCH-01
    verification:
      - kind: integration
        ref: "act --dryrun --container-architecture linux/arm64 --action-cache-path build/ci/fuzz-act-cache -P ubuntu-26.04=ubuntu:latest push -W .github/workflows/nightly.yml -j fuzz"
        status: pass
    human_judgment: false
metrics:
  duration: 31m 21s
  completed: 2026-11-25
status: complete
---

# Phase 16 Plan 11: Replay and fuzz patch and cheat hostile input Summary

**Generated-seed replay and libFuzzer coverage now exercise production patch and cheat parsers, with exact PPF and cheat seeds in Nightly.**

## Performance

- **Duration:** 31m 21s
- **Started:** 2026-11-26T03:28:47Z
- **Completed:** 2026-11-26T04:00:08Z
- **Executor model:** gpt-6-luna
- **Tasks:** 2
- **Files modified:** 6

## Accomplishments

- Added deterministic replay and libFuzzer entry points linked to the production PPF/BPS and cheat code. PPF replay covers valid PPF1/2/3 and BPS plus malformed inputs, validates parse determinism, and protects the generated source and prior candidate on failure. Cheat replay covers table replacement transactions, range/line/global caps, canonical bytes, and ordered writes to a real core instance.
- Added 14 PPF and 10 cheat runtime-generated seeds with declared names, byte caps, and fixture metadata. Nightly runs both on the full route for 60 seconds per target while retaining all existing campaign counts and the PBP-only route.
- Added workflow wiring assertions and documented replay, campaign, fixture, and cleanup behavior. Repository-check line budget remains 3,901/3,901.

## Verification Evidence

Checks were run from the repository root. The actual execution order was: create and run the two portable replays; commit the tracer; rerun its end-to-end verification; preflight inventories, workflow pins/timeouts, exact selectors, seed counts/names/caps and fixture metadata; run the Nightly routing dry-run; finish replay, hygiene, and workflow checks; remove task-owned outputs.

- `cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(fuzz[.]regress[.]ppf|fuzz[.]regress[.]cheat)$'` plus a Python selector assertion requiring exactly those two unique tests — **exit 0**.
- `cmake --build --preset ci --target schwaystation-fuzz-ppf-replay schwaystation-fuzz-cheat-replay && ctest --preset ci --no-tests=error -R '^(fuzz[.]regress[.]ppf|fuzz[.]regress[.]cheat)$' --output-on-failure` — **exit 0**, 2/2 tests passed. The task-1 tracer verification repeated this chain after its commit and also passed.
- Seed emitters: `build/ci/tests/schwaystation-fuzz-ppf-replay --write build/ci/tests/fuzz-work/plan16-11-20261126/ppf` — **exit 0**, 14 seeds; `build/ci/tests/schwaystation-fuzz-cheat-replay --write build/ci/tests/fuzz-work/plan16-11-20261126/cheat` — **exit 0**, 10 seeds. Names matched the workflow lists; largest emitted files were 1,099/65,536 bytes (PPF) and 1,026/1,026 bytes (cheat). Simulated workflow metadata/count/cap preflight passed.
- `cmake -DMODE=pins -DDIR=.github/workflows -P tests/cmake/workflows.cmake` — **exit 0**, 61 action pins; `cmake -DMODE=timeouts -DDIR=.github/workflows -P tests/cmake/workflows.cmake` — **exit 0**, 26 timeout comments.
- `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` — **exit 0**. Configured sorted-inventory checks passed. Repository-check line count — **3,901/3,901**, with no ceiling change. Documentation remained within its 61,440-byte limit at 61,200 bytes.
- `cmake --preset hygiene && ctest --preset hygiene --show-only=json-v1 -R '^(hygiene[.]timeout_comments|policy[.]action_pins)$'` with an exact selector assertion, then `ctest --preset hygiene --no-tests=error -R '^(hygiene[.]timeout_comments|policy[.]action_pins)$' --output-on-failure` — **exit 0**, 2/2 checks passed.
- `act --dryrun --container-architecture linux/arm64 --action-cache-path build/ci/fuzz-act-cache -P ubuntu-26.04=ubuntu:latest push -W .github/workflows/nightly.yml -j fuzz` — **exit 0**; the `changes` and `fuzz` routes were evaluated synchronously, with no Docker container execution. An initial dry-run using Act's default cache path exited 1 because that path was outside the writable workspace; rerunning with the project-local cache succeeded.
- Bounded local sanitizer campaigns were not run: `clang-21` was unavailable, and the installed Apple clang did not match the pinned Linux toolchain. Replay commands completed synchronously; no fuzzing child process was launched. Task-owned seed and Act cache directories were removed. The host process-list service was unavailable (`sysmond service not found`), so no independent OS process-table inspection was possible.

## Task Commits

1. **Task 1: Add bounded replay and libFuzzer entry points** — `5c33507` (`feat`).
2. **Task 2: Extend exact Nightly seed and campaign wiring** — `ec1cf40` (`test`).

## Files Created/Modified

- `tests/fuzz/ppf.c` — generated PPF/BPS seeds, replay assertions, and fuzz entry point.
- `tests/fuzz/cheat.c` — generated cheat seeds, replay assertions, bounded instance writes, and fuzz entry point.
- `tests/fuzz/tests.cmake` — portable replay registration and optional libFuzzer targets.
- `.github/workflows/nightly.yml` — PPF/cheat campaign routing with exact seed lists and caps.
- `tests/cmake/workflows.cmake` — guards for the workflow's route, count, names, fixture metadata, and limits.
- `docs/testing.md` — replay and Nightly campaign coverage and limits.

## Decisions Made

- Generate parser inputs in memory, avoiding committed patch payloads and fuzz-controlled paths.
- Keep 14 PPF seeds and 10 cheat seeds and assert their metadata at the workflow boundary.
- Preserve all existing Nightly campaigns and 60-second per-target campaign bounds.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected replay build and generated patch seeds**
- **Found during:** Task 1 (Add bounded replay and libFuzzer entry points)
- **Issue:** Strict compilation required public fuzz entry-point prototypes; initial generated PPF3/BPS seeds had invalid framing/CRC, and a seed buffer was too small.
- **Fix:** Added prototypes, corrected PPF3/BPS construction and CRC data, and sized the seed buffer for its encoded input.
- **Files modified:** `tests/fuzz/ppf.c`, `tests/fuzz/cheat.c`
- **Verification:** Both exact replay selectors passed 2/2, including the tracer rerun.
- **Committed in:** `5c33507`.

**2. [Rule 1 - Bug] Connected new seed fixtures to Nightly metadata validation**
- **Found during:** Task 2 (Extend exact Nightly seed and campaign wiring)
- **Issue:** Adding fixture declarations alone left the existing metadata assertion without the generated PPF/cheat count and fixture fields.
- **Fix:** Append emitted counts and fixture identifiers before metadata validation and assert the exact seed names/caps for both campaigns.
- **Files modified:** `.github/workflows/nightly.yml`, `tests/cmake/workflows.cmake`
- **Verification:** Generated seed metadata preflight, workflow checks, and `act --dryrun` passed.
- **Committed in:** `ec1cf40`.

**Total deviations:** 2 auto-fixed (both Rule 1 bugs).
**Impact on plan:** Corrections were required for valid seed replay and working Nightly metadata validation; no scope was added.

## Issues Encountered

- Act's default cache path was not writable in the sandbox. A second synchronous dry-run used `build/ci/fuzz-act-cache` and passed; that cache was removed afterward.
- No matching pinned local sanitizer compiler was available, so sanitizer campaigns remain for the hosted Nightly route.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

PATCH-01 and CHEAT-01 replay and Nightly wiring are ready for phase-level review and hosted verification. Hosted runners and sanitizer campaigns were not established locally; the orchestrator owns the reviewed-head workflow and hosted gates.

## Self-Check: PASSED

- This summary exists at the required phase path.
- Task commits `5c33507` and `ec1cf40` are ancestors of the current plan head.
- Summary metadata records the measured two task commits from the plan-head ledger.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-25*
