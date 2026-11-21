---
phase: 10-pbp-disc-images
plan: "10"
subsystem: packaging
tags: [c, cmake, ctest, pbp, chd, miniz, release-packages]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - Pinned and digest-checked standalone MIT inflater
      - PBP runner and libretro consumers with ON/OFF metadata
provides:
  - CHD-OFF and CHD-ON package identities and exact notice evidence
  - Native link and independent PBP inflater symbol-closure evidence
affects: [host-media, libretro, release-packaging]
actuals:
  tokens: 2206
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 09d430d70b465cdd195a650f235eedfb8646016e
plan_head_after: 32b4104ab529c082fdbca881db667c8a7b4fbd8b
tech-stack:
  added: []
  patterns:
    - Compare exact archive identities and embedded notice bytes in both feature configurations.
    - Inspect native link commands and symbol tables for private inflater closure and runtime dependencies.
key-files:
  created:
    - .planning/phases/10-pbp-disc-images/10-PACKAGE-EVIDENCE.md
  modified: []
key-decisions:
  - Use the existing ci-release preset for release-labeled archive checks, retaining every existing case.
  - Rebuild configuration-sensitive host/card test helpers when changing CHD mode in a reused build tree.
patterns-established:
  - Run digest, notice, inventory, budget and selector checks before build/package work.
  - Treat native module size as native evidence only; do not infer a WASI bound.
requirements-completed: []
coverage:
  - id: pbp_chd_off_package_closure
    description: CHD-OFF component archives preserve exact notices, advertise tested PBP metadata, and contain no system zlib or unprefixed standalone tinfl exports.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "CHD-OFF libretro.vendored/info.claims, PBP parity/runner/libretro selectors, and release archive_contents/notices; exit 0, 7/7 focused and 4/4 archive cases"
        status: pass
    human_judgment: false
  - id: pbp_chd_on_package_closure
    description: CHD-ON preserves PBP behavior and original CHD parity, exact notices and metadata, with separate private inflater symbols and no runtime zlib.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "CHD-ON PBP/CHD parity, runner/libretro selectors, and release archive_contents/notices; exit 0, 9/9 focused and 4/4 archive cases"
        status: pass
    human_judgment: false
duration: 19m
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 10: PBP ON/OFF Package and Link Evidence Summary

**CHD-OFF and CHD-ON PBP packages retain exact notices and metadata while the private inflater links without system zlib.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.21.0; primary model ID not exposed.
- **Elapsed:** About 19 minutes from the preceding saved state checkpoint at 14:09:41Z through evidence and verification; active time was not separately measured.
- **Tasks and files:** 2 tasks, 2 task commits, 1 plan artifact file.
- **Measured bottleneck:** The focused PBP/CHD integration selector took 5.79–6.07 seconds; archive and notice checks took 0.12–0.16 seconds. The content parity tests dominate these targeted checks. Cheap digest, notice, sorted-inventory, budget and selector checks ran before configure/build/package work.
- The existing `build/ci` tree and packages were reused where valid. Configuration-sensitive host/card helpers were rebuilt after switching from ON back to OFF. No broad suite, hosted CI, or `act` run applied because this plan changed no workflow.

## Accomplishments

- Recorded exact CHD-OFF and CHD-ON identities for all three component packages, plus matching SHA-256 values for the packaged notices file.
- Confirmed both generated `.info` files accurately advertise PBP, retain the experimental claim, and reflect their respective linked-source license and extension closures.
- Verified the standalone inflater archive and private symbol aliases remain separate from libchdr's own miniz copy, with no `-lz` link or runtime zlib dependency.
- Preserved the 3,901-line repository-check ceiling and the complete PBP and CHD parity/package cases. PBP-01 remains phase-wide pending for bounded guest and feature-ledger evidence in Plan 10-12.
- Recorded the actual macOS arm64 libretro module sizes for both configurations. These native sizes do not set or infer a WASI size bound.

## Task Commits

1. **Task 1: Package and inspect the default CHD-OFF PBP deliverables** — `d54349b`.
2. **Task 2: Confirm the same PBP behavior and separate symbols with CHD enabled** — `32b4104`.

**Plan metadata:** `24d7a31` (docs: complete plan).

## Commands and Results

The exact package identities, configure/build/test commands, notice digest, metadata claims, symbol findings and measured module sizes are in [10-PACKAGE-EVIDENCE.md](10-PACKAGE-EVIDENCE.md).

- Cheap pin and notice checks, three sorted inventories, gate budget and selector listing — exit 0; final gate count 3,900/3,901.
- CHD-OFF vendored and metadata selectors — exit 0, 4/4; focused PBP parity/runner/libretro selector — exit 0, 7/7 in 6.07 seconds; component archive and notice selectors — exit 0, 4/4 in 0.12 seconds.
- CHD-ON metadata selector — exit 0, 2/2 in 0.27 seconds; PBP and CHD parity plus runner/libretro selector — exit 0, 9/9 in 5.79 seconds; component archive and notice selectors — exit 0, 4/4 in 0.16 seconds.
- `git diff --check` and the final 3,901-line budget check — exit 0.
- No hosted CI run URL applies; no workflow changed.

## Decisions Made

- Use the existing `ci-release` preset for release-labeled CTests because `ci` excludes those selectors; all archive and notice cases remained enabled.
- Rebuild host/card test helpers when toggling CHD mode in a reused build directory so generated configuration matches the selected provider mode.
- Keep the deferred generic SPDX scanner out of scope and leave the quality cap unchanged.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Selected the release-aware CTest preset**
- **Found during:** Task 1 (CHD-OFF package validation)
- **Issue:** The `ci` preset excluded release-labeled archive tests, so the initial release selector found no tests and exited 8.
- **Fix:** Used the existing `ci-release` preset and verified all four archive and notice cases.
- **Files modified:** `.planning/phases/10-pbp-disc-images/10-PACKAGE-EVIDENCE.md`
- **Verification:** `ci-release` archive and notice selector passed 4/4 for both configurations.
- **Committed in:** `d54349b`.

**2. [Rule 3 - Blocking] Rebuilt stale CHD-OFF host/card helpers**
- **Found during:** Task 1 (CHD-OFF PBP host integration)
- **Issue:** A helper built under CHD-ON reported `chd_enabled 0 != 1` after reconfiguration to OFF.
- **Fix:** Rebuilt the host/card test helpers and affected PBP fixtures under the exact OFF configuration, then reran the focused selector.
- **Files modified:** Build-tree outputs only; no tracked source files.
- **Verification:** Final CHD-OFF focused selector passed 7/7.
- **Committed in:** No source change; the evidence is recorded in `d54349b`.

**Total deviations:** 2 blocking issues resolved. Both were local preset/build-tree configuration mismatches; no test cases or assertions were weakened.

## Issues Encountered

- `ctest --preset ci` does not include `release`-labeled archive tests. The existing `ci-release` preset runs them and passed.
- The shared build tree retained stale configuration-sensitive host/card helper binaries after CHD mode changed. Rebuilding those targets made the OFF test reflect the current configuration.

## User Setup Required

None. No human verification is required for package contents or link closure.

## Next Phase Readiness

- Plan 10-10 package and link evidence is complete. Plan 10-12 is the next incomplete dependency-ready plan; it owns bounded PBP guest and feature-ledger evidence.
- PBP-01 stays phase-wide pending until the remaining PBP acceptance evidence is complete.

## Self-Check: PASSED

- The plan evidence artifact and this summary exist.
- Task commits `d54349b` and `32b4104` are ancestors of the recorded plan head.
- The recorded package hashes, test outcomes, and native sizes match the captured verification results.
