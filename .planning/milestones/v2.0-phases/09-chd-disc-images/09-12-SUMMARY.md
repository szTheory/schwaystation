---
phase: 09-chd-disc-images
plan: 12
subsystem: content
tags: [CHD, runner, libretro, media, CMake, CTest]
requires:
  - phase: 09-06
    provides: CUE and CHD sector and subcode parity for the owned fixture
  - phase: 09-11
    provides: CHD-enabled CI fixture path and media/feature preflights
provides:
  - Runner CHD probing through the shared bounded ISO reader and disc callbacks
  - Runner provider and CLI parity checks for CHD and equivalent CUE content
  - Feature-gated libretro CHD loading, extension reporting and replacement safety
affects: [Phase 9 final verification, Plan 09-14]
actuals:
  tokens: 8280
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 858e8f24a5e75308a83c3c1c933354dcf43f75de
plan_head_after: 8a0637ded697a8784dd6cf5a826b7ec0ba1b2e7d
tech-stack:
  added: []
  patterns:
    - Read CHD probe metadata and first-track identity through the existing disc-sector callback.
    - Compare provider TOC, raw sectors, subcode and host-visible behavior with equivalent CUE media.
key-files:
  created:
    - .planning/phases/09-chd-disc-images/09-12-SUMMARY.md
  modified:
    - host/disc_probe.c
    - libretro/CMakeLists.txt
    - libretro/libretro.c
    - tests/libretro/host.c
    - tests/libretro/inventory.txt
    - tests/libretro/tests.cmake
    - tests/runner/disc.c
    - tests/runner/inventory.txt
    - tests/runner/tests.cmake
key-decisions:
  - Keep libretro CHD path acceptance and extension reporting gated by SCHWAYSTATION_CHD.
  - Use the shared provider for CHD probe reads; keep CUE and raw ISO paths unchanged.
  - Do not add CHD M3U membership in this plan because the existing provider rejects CHD playlist children.
patterns-established:
  - Give parallel CTest cases separate scratch directories when they write guest card images.
requirements-completed:
  - MEDIA-01
  - MEDIA-02
  - CHD-01
  - COMPAT-02
coverage:
  - id: D1
    description: Runner CHD callbacks expose the same TOC, raw sectors and subcode as equivalent CUE input, and failed operations leave the selected disc intact.
    requirement: MEDIA-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^runner[.]disc[.](api[.]chd|chd)$' --output-on-failure; CHD ON and OFF focused cases passed."
        status: pass
    human_judgment: false
  - id: D2
    description: The runner probes CHD ISO metadata and first-track identity through shared bounded sector reads without changing existing CUE/ISO behavior.
    requirement: MEDIA-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^runner[.]disc[.](chd|load|probe)$' --output-on-failure; CUE/CHD probe output, guest hash and card identity matched."
        status: pass
    human_judgment: false
  - id: D3
    description: Libretro accepts CHD only in CHD-enabled builds, loads it through the shared provider, and keeps active memory, card data and selection after a rejected replacement.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^libretro[.]content[.]chd$' --output-on-failure; CHD ON and OFF cases passed, including CUE parity."
        status: pass
    human_judgment: false
  - id: D4
    description: The enabled and disabled runner/libretro feature paths retain explicit, inventory-backed compatibility checks.
    requirement: COMPAT-02
    verification:
      - kind: other
        ref: "Sorted runner/libretro inventories and CHD ON/OFF focused CTest selectors passed; both modes configured and built their affected targets."
        status: pass
    human_judgment: false
duration: 38m37s
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 12: Runner and libretro CHD integration Summary

**Runner and libretro now open CHD through the shared provider, with CUE parity, stable runner identity and failure-safe frontend behavior.**

## Performance

- **Duration:** 38m37s wall time; active time was not measured.
- **Started:** 2026-11-20T19:53:15Z.
- **Completed:** 2026-11-20T20:31:52Z.
- **Tasks:** 2.
- **Files modified:** 9 implementation and test files.
- **Actuals:** 8,280 chars/4 tokens over the realized implementation diff; 2 task commits measured from the plan baseline.

## Accomplishments

- Extended the runner's existing bounded ISO reader to read CHD sectors through `schway_disc` callbacks. Direct CHD probe output now matches equivalent CUE output, and the existing first-track SHA-1 identity is computed from provider sectors.
- Added runner API coverage for TOC fields, every track's raw sectors and subcode, closed-lid swap rejection, malformed CHD rejection, guest boot/hash parity, card identity and explicit CHD-off behavior.
- Enabled libretro's `.chd` path and runtime extension only when `SCHWAYSTATION_CHD` is on. Its in-process test checks TOC and guest reads, CUE parity, memory-card behavior, selected-media callbacks and preservation after a rejected replacement.
- Isolated the runner CHD test's scratch directory after the first parallel CTest run exposed a collision with `runner.disc.load` card files.

## Task Commits

1. **Task 1: Load and probe CHD through the runner** — `69c00c9` (`feat`).
2. **Task 2: Use CHD in the in-process frontend contract** — `8a0637d` (`feat`).

## Verification

- Cheap preflight exited 0: `LC_ALL=C sort -cu tests/runner/inventory.txt && LC_ALL=C sort -cu tests/libretro/inventory.txt && git diff --check`; the owned CHD fixture and its manifest were also checked before the enabled build and guest run.
- CHD ON configure exited 0: `cmake --preset ci -DSCHWAYSTATION_CHD=ON`. Target build exited 0 in 0.77s: `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-chd-fixtures schwaystation-test-disc-media schwaystation-test-runner-disc schwaystation-test-libretro-host`. The generated fixture was 5,552 bytes (64 data + 158 audio sectors). Focused suite exited 0, 14/14 in 0.91s: `ctest --preset ci --no-tests=error -R '^runner[.]disc[.](api|api[.]chd|chd|load|probe|unsupported)$|^libretro[.](content[.](chd|lifecycle|no_bios|psexe)|host[.](disc_control|pal))$' --output-on-failure`.
- CHD OFF configure exited 0: `cmake --preset ci -DSCHWAYSTATION_CHD=OFF`. Target build exited 0 in 0.73s: `cmake --build --preset ci --target schwaystation-test-disc-media schwaystation-test-runner-disc schwaystation-run schwaystation-run-test schwaystation-test-libretro-host`. Focused suite exited 0, 6/6 in 0.36s: `ctest --preset ci --no-tests=error -R '^runner[.]disc[.](api[.]chd|chd|unsupported)$|^libretro[.]content[.]chd$' --output-on-failure`. The final build tree was restored to CHD ON for Plan 09-14.
- The final focused CHD ON selector included runner API, CHD probe/load, existing runner probe/unsupported controls, libretro CHD/lifecycle, existing disc control, PS-EXE content and PAL host baseline. No broad suite, hosted CI or Act run was started; Plan 09-14 and code review own the remaining exact-head verification.

## Iteration Notes

- The initial runner CHD test shared `build/ci/tests` with the existing runner load case; the failure was a parallel scratch-file collision. The CHD case now writes under `build/ci/tests/runner-disc-chd`.
- The provider currently rejects CHD files as M3U children. Plan 09-12 only calls for playlist membership coverage when the provider permits it, so this plan leaves that behavior unchanged and covers direct CHD content instead.
- The libretro CHD test did not get a usable serialized-state snapshot from the test path. It verifies failure preservation through the public live system-RAM and save-RAM pointers and bytes, plus the disk-control image count, selection and path. The test does not claim state-save coverage for disc content.

## Deviations from Plan

The plan explicitly allows a small production repair if the host integration test exposes a gap. The runner's probe rejected valid CHD after the shared provider opened it, so `host/disc_probe.c` now reads the ISO user sectors and hashes track 1 through existing disc callbacks. The libretro `.info` already advertised CHD in enabled builds, but its runtime extension list and path gate still rejected `.chd`; those are now feature-gated consistently. Both repairs stayed within the existing host paths and have focused regressions.

## Next Phase Readiness

Plan 09-12 is ready for the Plan 09-14 owned-guest/evidence handoff. The working tree's implementation commits are `69c00c9` and `8a0637d`; the summary, STATE.md and ROADMAP.md closeout remain for the parent to commit. Final source review and broad/hosted verification remain deferred to Plan 09-14.

## Self-Check: PASSED

The summary exists, both task commits are ancestors of HEAD, and the implementation index is clean.
