---
phase: 06-discs-and-fmv
plan: "25"
subsystem: testing
tags:
  - C
  - ABI
  - API v5
  - disc callbacks
requires:
  - phase: "06-02"
    provides:
      - API v5 disc callback structs and functions
provides:
  - Public API v5 disc consumer linked to the production library
  - Gated API v5 CTest registration and exact ABI inventory row
affects:
  - Phase 6 disc API consumers
  - ABI compatibility checks
actuals:
  tokens: 7240
  tasks: 2
  commits: 2
  plan_head_before: e5ef4d23f76bdbe630997bf0f08ae1710f72071d
  plan_head_after: 4b7815054f04c0b674d9877165f582f352226313
tech-stack:
  added: []
  patterns:
    - Public-only, versioned API consumer linked to the production library
key-files:
  created:
    - tests/api/v5.c
  modified:
    - tests/abi/tests.cmake
    - tests/abi/inventory.txt
key-decisions:
  - Keep internal disc attachment invariants in core.disc_api; the v5 consumer only uses the installed public header.
patterns-established:
  - New API names and append-only status values are exercised by a public versioned consumer.
requirements-completed:
  - DISC-02
  - DISC-04
coverage:
  - id: D1
    description: The API v5 consumer references the new disc callback, TOC, track, status, and function names.
    requirement: DISC-02
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-api-v5 && ctest --test-dir build/ci -R '^abi\\.api_v5$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Released v1-v4 callers remain unchanged and pass alongside v5 size-tag, layout, and status checks.
    requirement: DISC-04
    verification:
      - kind: integration
        ref: "cmake --preset ci && cmake --build --preset ci --target schwaystation-test-api-v1 schwaystation-test-api-v2 schwaystation-test-api-v3 schwaystation-test-api-v4 schwaystation-test-api-v5 schwaystation-test-disc-api && ctest --test-dir build/ci -R '^abi\\.api_v[1-5]$|^core\\.disc_api$' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^abi\\.api_version$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
duration: 14min
completed: 2026-11-15
status: complete
---

# Phase 6 Plan 25: Public Disc API Consumers Summary

**A public-only v5 consumer now gates disc callbacks, size tags, append-only statuses, and compatibility with the released v1–v4 callers.**

## Performance

- **Duration:** 14 minutes
- **Started:** 2026-11-15T17:36:23Z
- **Completed:** 2026-11-15T17:50:35Z
- **Tasks:** 2
- **Files modified:** 3
- **Runtime/model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna

## Accomplishments

- Added a public-header-only API v5 consumer that uses all new disc callback, TOC, track, status, and function names.
- Registered `schwaystation-test-api-v5` and `abi.api_v5` in the sorted ABI inventory.
- Covered malformed and zero-extended input sizes, callback TOC output sizes, track layout, and retained status values without editing the frozen v1-v4 sources.

## Task Commits

1. **Task 1: Add the v5 disc consumer and its exact registration** — `816095e` (`test`)
2. **Task 2: Preserve prior consumers and size-tagged behavior** — `4b78150` (`test`)

## Files Created/Modified

- `tests/api/v5.c` — Public API consumer and size/status compatibility assertions.
- `tests/abi/tests.cmake` — Registers the v5 executable and CTest case; updates the current API coverage note.
- `tests/abi/inventory.txt` — Adds the sorted `abi.api_v5` entry.

## Decisions Made

Kept the v5 consumer on the installed public header. Internal attachment-state checks remain in the separate `core.disc_api` test, because the public API deliberately exposes no host-attachment getter.

## Verification Evidence

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-api-v5 && ctest --test-dir build/ci -R '^abi\.api_v5$' --no-tests=error --output-on-failure` — exit 0; the v5 consumer passed (1/1).
- `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-api-v1 schwaystation-test-api-v2 schwaystation-test-api-v3 schwaystation-test-api-v4 schwaystation-test-api-v5 schwaystation-test-disc-api && ctest --test-dir build/ci -R '^abi\.api_v[1-5]$|^core\.disc_api$' --no-tests=error --output-on-failure` — exit 0; all six selected compatibility and disc API tests passed.
- `ctest --test-dir build/ci -R '^abi\.api_version$' --no-tests=error --output-on-failure` — exit 0; API header-name coverage passed (1/1).
- `scripts/hygiene.sh --staged` — exit 0 before both task commits.
- These are local checks; no hosted run URL applies to this consumer slice.

## Deviations from Plan

The task-level `tdd="true"` tags did not yield a meaningful RED phase: the API behavior and its core behavior test were already delivered by Plan 06-02, while this plan adds only an external consumer and gate registration. A failing setup or an intentionally incorrect status expectation would be invalid RED evidence, so the consumer was implemented and verified against the existing production behavior.

### Auto-fixed Issues

**1. [Rule 1 - Bug] Corrected the malformed-size test snapshot order**
- **Found during:** Task 2 (Preserve prior consumers and size-tagged behavior)
- **Issue:** The first run compared a valid-size snapshot against the intentionally undersized input, so the test failed before checking whether the API mutated it.
- **Fix:** Captured the snapshot after preparing the malformed size tag.
- **Files modified:** `tests/api/v5.c`
- **Verification:** The first v1-v5 compatibility command exited 8 on the faulty test assertion; the same command passed after the correction.
- **Committed in:** `4b78150`.

**Total deviations:** 1 auto-fixed test bug and 1 TDD applicability adjustment.
**Impact on plan:** No production behavior or scope changed; both task verifications passed.

## Issues Encountered

The sandbox initially denied writing the GSD commit ledger under `.git`; repository write access was granted for the task commits. The generic plan counter advanced the sequential pointer after this out-of-order plan, so it was restored to plan 4 with 06-04 still next. No user action remains.

## User Setup Required

None.

## Next Phase Readiness

The public API compatibility slice is complete. Broader disc parsing and libretro disk-control behavior remain with their owning Phase 6 plans.

## Self-Check: PASSED

- `tests/api/v5.c` exists and is registered in `tests/abi/tests.cmake` and `tests/abi/inventory.txt`.
- Task commits `816095e` and `4b78150` are ancestors of the plan head.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-15*
