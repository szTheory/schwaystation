---
phase: 09-chd-disc-images
plan: 03
subsystem: media
tags: [chd, cmake, license-audit, fixture-tool]
requires: []
provides:
  - A complete permissive source and codec closure audit for the run-local CHD fixture creator.
  - An opt-in native CHD fixture tool that stays outside the default build and install components.
affects: [Phase 9 fixture generation]
actuals:
  tokens: 9507
  tasks: 2
  commits: 2
tech-stack:
  added: [C++20 opt-in build-only CHD fixture tool]
  patterns: [Verify pinned archive bytes before extraction, re-extract before each opt-in configure, exclude fixture tools from default builds]
key-files:
  created:
    - .planning/phases/09-chd-disc-images/09-GENERATOR-AUDIT.md
    - cmake/CHDTool.cmake
    - tools/chd-tool/main.cpp
  modified:
    - CMakeLists.txt
    - .planning/phases/09-chd-disc-images/09-03-PLAN.md
    - PROVENANCE.md
    - pins.txt
key-decisions:
  - The exact pinned libchdman-rs source bundle provides a permissively licensed C/C++ build closure; its MAME gitlink is not fetched or built.
  - Keep C++ and the generator source out of normal builds; enable them only with SCHWAYSTATION_BUILD_CHD_TOOL=ON.
  - Re-extract the verified archive on every opt-in configure so edits to a cached extracted tree cannot bypass the source pin.
patterns-established:
  - Run archive, source-count, license and submodule consistency checks during configure before creating build targets.
requirements-completed: []
coverage:
  - id: D1
    description: The exact fixture-generator source and compiled codec closure has a permissive license disposition.
    requirement: CHD-01
    verification:
      - kind: other
        ref: .planning/phases/09-chd-disc-images/09-GENERATOR-AUDIT.md and pinned archive SHA-256 verification
        status: pass
    human_judgment: false
  - id: D2
    description: The native CHD fixture tool builds only when explicitly enabled and is absent from install components.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON && cmake --build --preset ci --target schwaystation-chd-tool (exit 0)"
        status: pass
      - kind: integration
        ref: "Fresh CHD-off configure: no CMAKE_CXX_COMPILER and no pins/libchdman-rs directory (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: Opt-in configure restores modified extracted source from the hash-verified archive before build generation.
    verification:
      - kind: other
        ref: "Corrupt extracted sys/chd_shim.cpp, reconfigure, confirm marker removed, then build target (exit 0)"
        status: pass
    human_judgment: false
duration: 43min
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-03 Summary

**The pinned CHD fixture creator now has a cleared permissive build closure and an opt-in native CMake target.**

## Performance

- **Duration:** 43 minutes from the preceding saved Phase 9 checkpoint; exact active start was not recorded.
- **Started:** 2026-11-20T04:28:46Z (previous committed state checkpoint)
- **Completed:** 2026-11-20T05:12:09Z
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex/OpenGSD 1.20.0; primary GPT-6 model ID is not exposed.

## Accomplishments

- Audited the pinned `libchdman-rs` archive and the complete reachable C/C++ and codec closure. The audit records each source set, license basis, source archive digest, deterministic codec settings and the bundled MAME gitlink boundary.
- Added the native `schwaystation-chd-tool` target. It is excluded from the default build and install components; normal builds remain C-only unless `SCHWAYSTATION_BUILD_CHD_TOOL=ON` is explicitly set.
- Added configure-time checks for the archive digest, upstream base revision, submodule declaration, licenses, expected source files and source count before creating compiler targets.
- Tightened the preflight after review: every opt-in configure now extracts from the verified archive, so a modified cached source tree is discarded before the build graph is generated.

## Task Commits

1. **Task 1: Audit one complete fixture-generator closure** - `a8a37c0` (`docs(09-03): clear CHD fixture generator closure`)
2. **Task 2: Add the cleared build-only tool recipe** - `153f90b` (`feat(09-03): add opt-in CHD fixture tool`)

Plan metadata: recorded in the following summary/state commit.

## Files Created/Modified

- `.planning/phases/09-chd-disc-images/09-GENERATOR-AUDIT.md` - exact pin, complete source closure, license evidence and deterministic build settings.
- `cmake/CHDTool.cmake` - archive verification, configure preflight and opt-in native target.
- `tools/chd-tool/main.cpp` - project-owned CUE-to-CHD command-line wrapper.
- `CMakeLists.txt`, `pins.txt`, `PROVENANCE.md`, and `09-03-PLAN.md` - opt-in wiring and pin/provenance records.

## Decisions Made

- Use the pinned permissive C/C++ closure and avoid a Rust/Cargo toolchain in the CI matrix.
- Keep the fixture creator outside shipped deliverables and do not enable it in the default C-only build.

## Deviations from Plan

### Auto-fixed Issues

1. The first target build found a missing OSD include path; the opt-in target now carries the audited include set.
2. The initial CLI used `std::filesystem`, which exceeded the project's macOS deployment floor. Replaced it with the platform's stat APIs and kept vendored headers marked as system includes.
3. Review found the extracted-source stamp could trust a locally modified build directory. Configure now re-extracts the hash-verified archive on each opt-in run; a deliberate source-corruption probe confirmed restoration.

**Total deviations:** 3 auto-fixed; all remain within the tool-recipe task and preserve the default build.

## Verification and Efficiency

- `cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON` — exit 0; the initial configure preflight completed in about 2.2 seconds. The final reconfigure after the source-reset change completed in about 0.8 seconds.
- `cmake --build --preset ci --target schwaystation-chd-tool` — exit 0; compiled the pinned native target. The final incremental build reported no work after the archive re-extraction because the pinned source timestamps were unchanged.
- Fresh CHD-off configure — exit 0 in about 0.3 seconds, with no C++ compiler entry in `CMakeCache.txt` and no generator source directory.
- A local corruption probe modified the extracted shim, reconfigured, confirmed the marker was removed, and rebuilt the target — exit 0.
- `git diff --check origin/main...HEAD` and `git diff --check HEAD` — exit 0.
- No hosted CI or `act` run applied: this plan changes no workflow and the focused local build exercises the only new build path. The generator was not run on disc data; fixture production and output checks are Plan 09-04.

## Issues Encountered

The source bundle's `.gitmodules` file initially looked like a build-time dependency. Checking its exact gitlink and the bundled source inventory confirmed that it is only the public base used to regenerate the included source subset; configure does not fetch or build the submodule.

## User Setup Required

None.

## Next Phase Readiness

Plan 09-04 can generate the owned CHD oracle from the checked-in CUE/BIN fixture and add its run-local manifest and structural preflight. The fixture tool's output behavior remains unverified until that plan.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*
