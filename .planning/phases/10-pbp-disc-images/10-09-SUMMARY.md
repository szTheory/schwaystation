---
phase: 10-pbp-disc-images
plan: "09"
subsystem: libretro
tags: [c, pbp, libretro, ctest, metadata]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - shared bounded PBP provider and owned generated fixture
      - single-track CUE playback path for PBP parity
provides:
  - libretro loads supported PBP through the shared host disc provider
  - PBP and source CUE match in TOC, sector data, bounded callbacks, RAM and cards
  - generated frontend metadata advertises PBP under both CHD configurations
affects: [libretro, host-media, media-testing, release-metadata]
actuals:
  tokens: 5479
  tasks: 2
  commits: 2
commits: 2
plan_head_before: ea941f6bc4c1001c6c609ad6a0e01c8ca1fc07c1
plan_head_after: d354bfe4b73406f4d65f24a11646916cc3134f6b
tech-stack:
  added: []
  patterns:
    - Exercise libretro PBP content through the same host disc provider used by the runner.
    - Compare bounded frame and audio traces plus exact guest RAM and card bytes against the source CUE.
    - Gate generated extension claims on the registered PBP host integration test.
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/tests.cmake
    - tests/libretro/inventory.txt
    - libretro/CMakeLists.txt
    - tests/cmake/info_claims.cmake
    - docs/libretro.md
key-decisions:
  - Keep PBP available with CHD disabled and retain CHD license derivation from the linked provider closure.
  - Keep the public metadata experimental and describe only the tested unencrypted single-data-track PSISO subset.
requirements-completed: []
coverage:
  - id: libretro_pbp_content
    description: The libretro host loads supported PBP through the shared provider and matches its source CUE's TOC, sectors, bounded frame/audio behavior, guest RAM and cards; malformed and PGD replacements preserve active media and cards.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro schwaystation-libretro-card-test schwaystation-pbp-fixtures; ctest --preset ci --no-tests=error -R '^libretro[.]content[.]pbp$' --output-on-failure; exit 0, 3/3 including PBP and compatibility preflights"
        status: pass
    human_judgment: false
  - id: generated_pbp_metadata
    description: CHD-OFF and CHD-ON generated metadata advertise PBP, keep the experimental claim, and preserve the linked-source license closure with a registered PBP host selector.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF; ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure; cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON; same CTest selector; exit 0 in both configurations"
        status: pass
    human_judgment: false
duration: 25m
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 09: Libretro PBP Support

**The libretro core now loads the tested PBP subset through the shared provider and advertises it accurately with CHD enabled or disabled.**

## Performance

- **Runtime/model:** Codex/OpenGSD 1.21.0; executor `gpt-6-luna`, xhigh reasoning.
- **Elapsed:** About 25 minutes from 13:45:05Z through closeout; active time was not separately measured.
- **Tasks and files:** 2 tasks, 2 task commits, 7 files changed.
- **Measured bottleneck:** The focused PBP host test ran 1,200 frames each for PBP and its source CUE and took 5.97 seconds; the PBP and compatibility preflights together took 0.03 seconds. The guest comparison dominates this local check. Its provisional timeout is 60 seconds, about ten times the measured run.
- The existing `build/ci` CHD-OFF products were reused. Only the affected host/module targets were rebuilt. CHD-OFF and CHD-ON configurations each ran the focused metadata checks; no broad suite, hosted CI, or `act` run applied because this plan changed no workflow.

## Accomplishments

- Routed `.pbp` through libretro's normal supported-path check and reported it from both CHD-OFF and CHD-ON runtime extension lists.
- Added `libretro.content.pbp`. It compares the generated PBP and owned single-track CUE TOCs, identity availability, every sector and subcode, 1,200-frame video/audio traces, final RAM and both cards. Malformed and encrypted PGD replacements are rejected without changing the active path, RAM or cards.
- Updated generated `.info` metadata and user documentation to name only the supported unencrypted single-data-track PSISO subset. Existing license-closure derivation, the experimental flag and the install destination remain intact.
- Kept the repository check at 3,900/3,901 lines. PBP-01 remains phase-wide pending; Plan 10-10 still owns package and independent-link evidence.

## Task Commits

1. **Task 1: Exercise PBP content load and failure-atomic replacement** — `47d233e`.
2. **Task 2: Bind PBP extension metadata to its registered host evidence** — `d354bfe`.

## Commands and Results

- The sorted libretro inventory check — exit 0:
  ```sh
  LC_ALL=C sort -cu tests/libretro/inventory.txt
  ```
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0; 3,900/3,901 lines.
- `git diff --check` and `git diff ea941f6..HEAD --check` — exit 0.
- `ctest --preset ci --no-tests=error -R '^disc[.]image[.]pbp_preflight$' --output-on-failure` — exit 0; fixture digest, size and source parity preflight passed in 0.02s.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0; reused the existing CI build tree.
- `ctest --preset ci --no-tests=error -N -R '^libretro[.]content[.]pbp$'` — exit 0; PBP selector and both fixture prerequisites were registered.
- `cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation_libretro schwaystation-libretro-card-test schwaystation-pbp-fixtures` — exit 0.
- Final `ctest --preset ci --no-tests=error -R '^libretro[.]content[.]pbp$' --output-on-failure` — exit 0; 3/3 including fixture and compatibility preflights, total 6.00s.
- CHD-OFF configure plus `ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure` — exit 0; 2/2 in 0.47s.
- CHD-ON configure plus the same metadata selector — exit 0; 2/2 in 0.46s. The generated file reports `exe|psexe|cue|iso|m3u|pbp|chd`, the CHD linked-source license closure and `is_experimental = "true"`; the registered PBP selector remained present.
- No hosted run URL applies.

## Decisions Made

- Leave PBP in the always-built host path and append optional CHD afterward, so extension ordering and license claims reflect the actual link closure.
- Compare full guest RAM and card bytes because the shared disc contract provides no state identity for this image descriptor and save-state serialization is unavailable with attached media of unknown identity. The public `.info` savestate claim remains false.

## Deviations from Plan

None. The test exercises the exact shared-provider subset and rejection behavior specified by the plan.

## Issues Encountered

- An initial fixture probe passed unsupported `--check` to `tools/gen_pbp.py` and exited 2; the existing `disc.image.pbp_preflight` CTest is the canonical fixture check and passed.
- Early test attempts exposed incorrect harness assumptions (a fixed 320-pixel output width and save-state serialization with media lacking an identity). The host test now accepts the core's reported bounded geometry and compares exact RAM/card state with the per-frame video/audio trace; the final focused run passed.

## User Setup Required

None.

## Self-Check: PASSED

- The summary, both task commits and all seven modified source/documentation files exist.
- Both task commits are ancestors of the recorded plan head. No generated PBP, BIOS, executable, card or state bytes were added to the repository.
