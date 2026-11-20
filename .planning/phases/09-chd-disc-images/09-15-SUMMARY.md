---
phase: 09-chd-disc-images
plan: 15
subsystem: build
tags: [CMake, CHD, BE32, WASI, CTest]
requires:
  - phase: 09-04
    provides: Pinned CHD fixture producer and owned source media
  - phase: 09-06
    provides: CHD provider parity checks
  - phase: 09-13
    provides: ON/OFF package validation before distributed wiring
provides:
  - Cross presets that consume a checked native CHD generator without adding a CHD generator target
  - Cross-lane fixture hashes that include CHD bytes but exclude platform-specific generator provenance
  - CHD-off cleanup and negative-control coverage for the relocated manifest
affects: [Phase 9 Plan 09-11, CI parity]
actuals:
  tokens: 1800
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Share pinned tool identity parsing between the native builder and fixture-generation script.
    - Keep comparable media bytes in fixtures and host-specific provenance in the test output tree.
key-files:
  created:
    - cmake/CHDToolPins.cmake
    - .planning/phases/09-chd-disc-images/09-15-SUMMARY.md
  modified:
    - CMakeLists.txt
    - CMakePresets.json
    - cmake/CHDTool.cmake
    - cmake/CHDFixtures.cmake
    - cmake/DiscFixtures.cmake
    - tests/disc/tests.cmake
    - tests/disc/media.c
key-decisions:
  - Keep `build/be32` and `build/wasm32` as the cross-preset binary directories to preserve existing workflow, emulator and module-size paths.
  - Preserve the existing C++17 public-header ABI smoke; external CHD mode adds no generator target or generator C++ source build.
  - Defer actual cross-compiler and hosted eight-lane validation to Plan 09-11 because the local cross compilers are absent.
patterns-established:
  - The fixture hash set contains reproducible media bytes only; generator provenance is validated separately.
requirements-completed: [CHD-01, MEDIA-02, COMPAT-02]
coverage:
  - id: D1
    description: External CMake mode selects a supplied native generator, verifies pinned provenance, separates it from parity-hashed CHD bytes and cleans both outputs when disabled.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "Plan 09-15 Task 2 command: external CHD configuration/build, fixture generation, enabled media selectors (7/7), manifest path checks, CHD-off reconfigure and disabled selector (2/2); exit 0. The BE32 and WASI presets parse; their actual target builds are assigned to Plan 09-11."
        status: pass
    human_judgment: false
duration: 23m
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 15: Native CHD generator handoff Summary

**Cross builds now accept a checked native CHD generator while keeping its host-specific provenance out of byte-for-byte fixture comparison.**

## Performance

- **Duration:** About 23 minutes wall time from executor start to closeout; active time was not measured.
- **Started:** 2026-11-20 18:47:57 UTC
- **Completed:** 2026-11-20 19:10:32 UTC
- **Tasks:** 2
- **Files modified:** 8 implementation files, plus plan and state artifacts.
- **Iteration cost:** Cheap preset, inventory and line-budget checks passed in 0.01 seconds. The fresh external-mode fixture/test build took 16 seconds; no equivalent build was repeated. Focused CTest runs took 0.09–0.14 seconds.

## Accomplishments

- Added `SCHWAYSTATION_CHD_TOOL_EXECUTABLE` and explicit `be32-chd` / `wasm32-chd` presets. They retain the established build directories so the workflow artifact paths, WASI test emulator and module-size check still line up.
- Extracted the audited commit/archive identity into `cmake/CHDToolPins.cmake`; the fixture script checks supplied metadata against that pin and writes complete provenance for external generation.
- Kept `disc_boot.chd` under `fixtures/` and moved `disc_boot.chd.manifest` under `tests/`. A one-off recursive-glob check confirmed CHD is included in cross-lane hashes and the host-specific manifest is excluded.
- Updated the existing CHD-off control to detect a stale manifest in its new directory. Reconfiguring OFF removed both artifacts before the disabled selector ran.
- Kept the public-header C++17 ABI smoke test unchanged. The external CHD build graph contains no `schwaystation-chd-tool` target.

## Verification

| Check | Exit | Result |
|---|---:|---|
| Preset listings, sorted disc inventory and 3901-line budget | 0 | Cross configure/build/test presets parse; inventory sorted; gate remains 3900/3901. |
| Initial in-tree CHD selector attempt | 8 | The command had not built `schwaystation-test-disc-chd`; two selectors used its stale CHD-OFF executable. This was test setup, not a provider failure. |
| Rebuild `schwaystation-test-disc-chd`; rerun focused CHD selector | 0 | 7/7 passed in 0.09 seconds, including the automatically required compatibility preflight. |
| External-tool configuration and target check | 0 | Configure passed in about 8.8 seconds; build target help contained no `schwaystation-chd-tool`. Configure also enabled the pre-existing ABI C++17 smoke language, which was not changed or suppressed. |
| External fixture generation and enabled CHD selectors | 0 | Build took about 16 seconds; generator settings, pinned commit/archive, tool digest and media provenance passed preflight; selectors passed 7/7 in 0.14 seconds. |
| CHD-OFF reconfigure, artifact cleanup and disabled selector | 0 | Configure took about 1.6 seconds; both CHD and manifest were absent immediately; disabled media controls passed 2/2 in 0.03 seconds. |
| `git diff --check`, sorted inventory and line budget | 0 | Clean; 3900/3901 lines. |
| `command -v powerpc-linux-gnu-gcc qemu-ppc clang-21 clang++-21` | 1 | All four local cross-toolchain commands are unavailable; actual BE32/WASI execution remains for Plan 09-11 and hosted CI. |

No workflow changed in this plan, so no `act` or hosted CI run applied. Phase 9 CI routing and cost review stay in Plan 09-11.

## Task Commits

1. **Task 1: Accept an external native generator without a CHD generator target** — `9a35d55`.
2. **Task 2: Separate provenance from comparable fixture bytes** — `4bdeece`.

The plan was added in `dee515e`. The plan checker corrected the dependency wave and expanded external-mode/cleanup verification before implementation.

## Issues Encountered

- The first focused selector run used a stale CHD-OFF test binary because its target was omitted from the build list. Building that one target and rerunning only the same selectors passed; no production code changed for this issue.
- CMake reported C++ during the external configure because the existing ABI tests enable C++17 for the public header. The CHD external mode added no generator C++ source or target, and the ABI smoke remained intact.
- Cross compilers are absent on this host. Plan 09-11 owns the actual BE32/WASI and eight-lane checks.

## Next Plan Readiness

Plan 09-15 is complete. Plan 09-11 is next by dependency order; it owns the eight-lane workflow/fuzz wiring, local `act --dryrun` routing, module-size measurement and the hosted CI/CD efficiency review. No owner action is needed.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*
