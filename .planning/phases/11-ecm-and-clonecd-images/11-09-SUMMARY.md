---
phase: 11-ecm-and-clonecd-images
plan: "09"
subsystem: libretro
tags: [libretro, ecm, ccd, cmake, ctest]
requires:
  - phase: 11-07
    provides: owned ECM/CCD fixtures, source counts and preflight/parity selectors
provides:
  - Libretro path loading and runtime metadata for ECM and CCD
  - Host integration checks for source parity, failure-atomic replacement and disk replacement
  - Generated frontend extension claims tied to the legacy host inventory
affects: [libretro, frontend-metadata, phase-11]
actuals:
  tokens: 6325
  tasks: 2
  commits: 4
  plan_head_before: 7b268761d36914a256144aa8ccad0e7a1848db6f
  plan_head_after: 7ef84fb75b62ef8bc2cf0e49ac199108079d89c4
tech-stack:
  added: []
  patterns:
    - Use the shared disc-image reader for libretro loading and replacement.
    - Keep runtime and generated frontend extension lists aligned with registered host evidence.
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - libretro/CMakeLists.txt
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - tests/cmake/info_claims.cmake
    - tests/cmake/info_claims_self_test.cmake
key-decisions:
  - Keep CHD support conditional while appending ECM and CCD to both CHD-on and CHD-off extension lists.
  - Preserve phase requirements as incomplete until the remaining provider, runner, fuzz and phase verification evidence is available.
patterns-established:
  - Compare libretro provider output with its source CUE through the existing host reader and deterministic guest traces.
  - Reject invalid replacement content while checking active RAM, save RAM and disk path remain unchanged.
requirements-completed: []
coverage:
  - id: D1
    description: Libretro loads ECM and matches its source CUE sector, TOC, identity, reset and read behavior.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(disc[.]image[.]ccd|libretro[.]content[.]legacy)$' --output-on-failure; exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: Libretro loads CCD, preserves its active image on invalid replacements, and replaces media through disk controls.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(disc[.]image[.]ccd|libretro[.]content[.]legacy)$' --output-on-failure; exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: Generated ECM/CCD extension claims require the registered legacy host test.
    requirement: CCD-01
    verification:
      - kind: other
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure; exit 0"
        status: pass
    human_judgment: false
metrics:
  duration: 28min
  completed: 2026-11-22
status: complete
---

# Phase 11 Plan 09: Libretro legacy content and extension evidence Summary

**Libretro now loads fixture-proven ECM and CCD through the shared disc reader, preserves active content after rejected replacements, and advertises only inventory-backed formats.**

## Performance

- **Runtime/model:** Codex with OpenGSD 1.20.0; executor model configured as `gpt-6-luna`; primary runtime model ID was not exposed.
- **Duration:** 28 minutes, measured from 2026-11-22 16:47:08 UTC to final closeout.
- **Tasks:** 2.
- **Product and checker files modified:** 7.

## Accomplishments

- Added the focused `libretro.content.legacy` host test for ECM/CCD source-CUE parity, identity, reset/read traces, bounded guest mailbox reads, rejected replacement preservation and extended disk replacement.
- Added case-insensitive `.ecm` and `.ccd` routing to the existing libretro provider path, runtime extension metadata, and generated `.info` extensions while preserving the CHD conditional.
- Bound extension claims to `libretro.content.legacy` in the existing CMake claim checker and added a negative inventory-removal control.

## Task Commits

1. **Task 1 RED:** `89172e0` — added the failing legacy content route test.
2. **Task 1 extension RED:** `b253d64` — asserted runtime ECM/CCD extension metadata.
3. **Task 1 GREEN:** `3f18e4d` — enabled shared-reader ECM/CCD routing and runtime metadata.
4. **Task 2:** `7ef84fb` — tied generated extension claims to host inventory evidence.

**Plan metadata commit:** recorded separately after state and roadmap updates.

## Verification Evidence

- RED 1: `ctest --preset ci --no-tests=error -R '^libretro[.]content[.]legacy$' --output-on-failure --output-junit build/ci/plan11-09-task1-full-red.xml` exited 8 on the intended `p_load_game` assertion after the legacy preflight, parity and compatibility fixtures passed. `gsd-tools check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- Task 1 build: `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro schwaystation-libretro-card-test schwaystation-legacy-disc-fixtures` exited 0.
- Task 1 final selector: `ctest --preset ci --no-tests=error -R '^(disc[.]image[.]ccd|libretro[.]content[.]legacy)$' --output-on-failure` exited 0; 5 selected and fixture-setup tests passed in 10.04 seconds. This includes the existing CCD known-answer/GetlocP checks, legacy preflight/parity, and the 1200-frame libretro comparisons.
- RED 2: `ctest --preset ci --no-tests=error -R '^libretro[.]content[.]legacy$' --output-on-failure --output-junit build/ci/plan11-09-task1-ext-red.xml` exited 8 only at the expected runtime extension assertion after the provider behavior checks passed. `gsd-tools check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- Task 2 budget preflight: `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` exited 0 at 3899/3901 lines.
- Task 2 configure: `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` exited 0.
- Task 2 selector: `ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure` exited 0; both claim tests passed.
- Source review and `git diff --check` passed before final verification. No hosted CI, Docker or broad suite was run.

## Deviations from Plan

None. The two TDD failures were assertion-level REDs and were validated by the canonical evidence checker; no test or assertion was removed.

## Issues Encountered

The sandbox denied creation of the required GSD baseline file under `.git`; the narrowly scoped authorized write succeeded. No code or test issue remains.

## User Setup Required

None.

## Next Phase Readiness

Plan 10 is next and can proceed after this plan closes. ECM-01 and CCD-01 remain phase-level requirements; this plan establishes libretro and generated-metadata evidence only. Phase review and final verification remain pending.

---
*Phase: 11-ecm-and-clonecd-images*
*Completed: 2026-11-22*

## Self-Check: PASSED

The summary exists, all four task commits are ancestors of the current plan head, and the measured task-commit count matches `actuals.commits`.
