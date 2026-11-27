---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "13"
subsystem: compatibility
tags: [library-inventory, Redump, DAT, SHA-256, CMake, CTest]
requires:
  - phase: 16-12
    provides: Bounded DAT occurrence parser and aggregate inventory output.
provides:
  - Verified provenance and aggregate counts for the pinned PlayStation DAT.
  - Cached and newly fetched library DAT digest validation with aggregate reports.
affects: [compatibility, library-inventory, provenance]
actuals:
  tokens: 4490
  tasks: 2
  commits: 3
  plan_head_before: cdac692b9d9a674d2375a5ca955aa1828ba5abba
  plan_head_after: d3d63ef0540a4c54afa174c607389c0ea6a7b119
tech-stack:
  added: []
  patterns:
    - Validate both cached and fetched public data against its pinned digest before inventory parsing.
    - Publish aggregate inventory evidence while keeping per-entry DAT data build-local.
key-files:
  created:
    - tests/compat/dat-summary.txt
  modified:
    - pins.txt
    - PROVENANCE.md
    - THIRD-PARTY-NOTICES.md
    - scripts/library.sh
    - tests/compat/tests.cmake
    - tests/compat/library_tests.c
    - docs/compatibility-inventory.md
key-decisions:
  - Retain only the source pin, digest, license attribution, and aggregate counts; never commit the raw DAT or a bulk title/serial table.
  - Preserve all 13,407 source occurrences as unknown without matching measured evidence; inventory does not establish playability.
patterns-established:
  - Verify cached DAT bytes with the same pinned SHA-256 requirement as newly fetched bytes.
  - Validate normal library report behavior using synthetic controls confined to test mode.
requirements-completed: [COMPAT-01]
coverage:
  - id: D1
    description: The exact licensed Redump source is pinned and its 13,407 occurrences reconcile to aggregate status and serial counts.
    requirement: COMPAT-01
    verification:
      - kind: integration
        ref: "gh api pinned commit/license/blob checks; shasum -a 256 build/ci/metadata/Sony-PlayStation.dat; build/ci/schwaystation-run --library-inventory build/ci/metadata/Sony-PlayStation.dat"
        status: pass
    human_judgment: false
  - id: D2
    description: Normal library processing validates cached data and reports aggregate evidence without publishing entry titles or serials.
    requirement: COMPAT-01
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls|compat[.]library[.]synthetic_result|compat[.]library[.]controls)$' --output-on-failure"
        status: pass
      - kind: other
        ref: docs/compatibility-inventory.md
        status: pass
    human_judgment: false
metrics:
  duration: 34m
  completed: 2026-11-26
commits: 3
plan_head_before: cdac692b9d9a674d2375a5ca955aa1828ba5abba
plan_head_after: d3d63ef0540a4c54afa174c607389c0ea6a7b119
status: complete
---

# Phase 16 Plan 13: Pinned DAT Provenance and Library Validation Summary

**The library route now validates cached DAT digests and reports pinned-source aggregate counts while keeping the DAT itself out of the repository.**

## Performance

- **Duration:** 34 minutes
- **Started:** 2026-11-26T05:23:59Z
- **Completed:** 2026-11-26
- **Executor model/runtime:** gpt-6-luna / Codex OpenGSD executor
- **Tasks:** 2
- **Plan files modified:** 8

## Accomplishments

- Verified the public source commit `12c21cf79f1e20eff6ac68b80875816c717d0a7a` with `gh api`, inspected the pinned root license and DAT header, fetched the exact DAT blob, and checked its SHA-256 before parsing. The source is CC BY-SA 4.0 and its required attribution is retained.
- Reconciled all 13,407 occurrences: 0 pass, 0 fail, 13,407 unknown, 177 entries with missing serials, and 988 entries in duplicate-serial groups. Unknown remains the result without matching evidence; inventory is not playability evidence.
- Added digest validation for cached and newly fetched DATs in the library route, retaining aggregate reporting without emitting the per-entry title or serial table. Synthetic tests cover wrong cached digests and the report boundary.
- Removed the downloaded DAT from ignored build-private storage after verification. No BIOS, disc collection, or owner library workflow was used.

## Verification Evidence

All commands ran from the repository root. The exact DAT was downloaded only to `build/ci/metadata/Sony-PlayStation.dat`; its digest was checked before the parser ran. The reported counts were captured and asserted against the pinned digest and required total, then the raw DAT was removed. No DAT or per-title table is committed.

- `gh api repos/libretro/libretro-database/commits/12c21cf79f1e20eff6ac68b80875816c717d0a7a --jq .sha` — **exit 0**; returned the requested public commit.
- `gh api 'repos/libretro/libretro-database/contents/LICENSE?ref=12c21cf79f1e20eff6ac68b80875816c717d0a7a' --jq .content | base64 -d | sed -n '1,2p'` and `gh api -H 'Accept: application/vnd.github.raw' repos/libretro/libretro-database/git/blobs/4d198d81a06e11577cc9160358ac1b02d3dc65a0` — **exit 0**; confirmed the pinned license and inspected the DAT header/blob.
- `shasum -a 256 build/ci/metadata/Sony-PlayStation.dat` — **exit 0**; matched `52978215854d307bfb5353052c9ce3c34995310a970d586f0100bb67f7de01a8` before parsing.
- `build/ci/schwaystation-run --library-inventory build/ci/metadata/Sony-PlayStation.dat` with assertions for the source pin/digest, occurrence count, status reconciliation, missing serials, and duplicate-serial entries — **exit 0**; produced the aggregate counts recorded above.
- TDD RED: `ctest --preset ci --no-tests=error -R '^compat[.]library[.]controls$' --output-on-failure --output-junit .gsd/tdd-evidence/16-13/red.xml` — **exit 8**, expected failure at the wrong-cached-digest assertion. `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence .gsd/tdd-evidence/16-13/red.json --raw` — **exit 0**, validated the RED evidence. Evidence files are ignored and no raw DAT is included.
- `cmake --preset ci` — **exit 0**. Exact selector inventory assertion for `compat.inventory.accounting`, `compat.inventory.controls`, `compat.library.synthetic_result`, and `compat.library.controls` — **exit 0**, all unique selectors present.
- `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-library` — **exit 0**.
- `ctest --preset ci --no-tests=error -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls|compat[.]library[.]synthetic_result|compat[.]library[.]controls)$' --output-on-failure` — **exit 0**, all four selectors passed along with the setup fixture (5/5 tests).
- Cheap checks (`sh -n scripts/library.sh`, `git diff --check HEAD`, sorted inventory/fixture checks, source digest and selector preflights) — **exit 0**. Repository-check line budget measured 3,900 of 3,901 allowed lines.
- `test ! -e build/ci/metadata/Sony-PlayStation.dat` — **pass**; the fetched DAT was removed. Every build/test/parse command returned; no command session remained active. A separate process-table query was denied by the sandbox, so no OS-wide process listing is claimed.

All automated checks were local. The orchestrator owns code review and broad local/hosted phase verification on the reviewed head; those are not claimed here and no hosted run URL applies to this plan slice.

## TDD Gate Compliance

- RED was the intended cached digest rejection assertion and returned exit 8 before the integration change. The red-evidence checker passed.
- GREEN was verified by the exact four plan selectors, their setup fixture, and the targeted runner/library build; all returned exit 0.

## Task Commits

1. **Task 1: Verify the exact public DAT and source license** — `f276d8b` (`docs(16-13): record Redump inventory provenance`).
2. **Task 2 RED: Add cached digest and summary controls** — `939ce6d` (`test(16-13): reject wrong cached DAT and validate summaries`).
3. **Task 2 GREEN: Integrate pinned inventory accounting** — `d3d63ef` (`feat(16-13): account for pinned inventory in library reports`).

The measured plan commit range contains 3 task commits, from `cdac692b9d9a674d2375a5ca955aa1828ba5abba` through `d3d63ef0540a4c54afa174c607389c0ea6a7b119`. The summary metadata commit is separate.

## Files Created/Modified

- `tests/compat/dat-summary.txt` — source pin, digest, aggregate count and attribution evidence.
- `pins.txt`, `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md` — public source identity and CC BY-SA 4.0 attribution.
- `scripts/library.sh` — cached/new DAT digest validation and aggregate inventory report integration.
- `tests/compat/tests.cmake`, `tests/compat/library_tests.c` — synthetic cached-digest and result-report controls.
- `docs/compatibility-inventory.md` — inventory data boundary and evidence interpretation.

## Decisions Made

- Applied settled D-06 as specified: verify the pinned data and publish only aggregate counts and attribution.
- Kept the real DAT in ignored build output only and removed it immediately after the source reconciliation.

## Deviations from Plan

None. Two test-harness assumptions found during focused runs were corrected within the planned test files: inventory mode now disables a test-only probe log, and the synthetic snapshot baseline consistently excludes both generated report outputs. The required selectors passed after those corrections.

## Issues Encountered

- The first synthetic library control exposed probe-log contamination in inventory mode; the test was isolated from that test-only log.
- The next control exposed generated sidecars in its before/after snapshot baseline; both generated outputs are now excluded consistently. These were test harness issues and did not require production behavior changes.

## Known Stubs

None.

## Next Phase Readiness

This plan is ready for the orchestrator's code review and reviewed-head phase verification. `STATE.md` and `ROADMAP.md` were intentionally left untouched in this worktree per the worktree task instruction. No owner action is required for this plan.

## Self-Check: PASSED

- All eight planned files exist and are committed in the measured task range; this summary exists at the required path.
- Task commits `f276d8b`, `939ce6d`, and `d3d63ef` are ancestors of the plan head.
- The raw DAT is absent and the worktree was clean before writing this summary.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*
