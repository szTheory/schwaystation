---
phase: 10-pbp-disc-images
plan: "02"
subsystem: host-media
tags: [c, pbp, cmake, licensing, ctest]
requires:
  - phase: 10-pbp-disc-images
    provides:
      - host-only PBP envelope reader boundary
      - PBP header selector and fixture inventory
provides:
  - verified pinned modular MIT inflater source and project-owned compatibility shim
  - private static PIC C target with generated export header and private tinfl symbols
  - digest checks and independent mutation controls for the existing and new vendored files
affects: [host-media, disc-testing, repository-gate-budget, licensing]
actuals:
  tokens: 18113
  tasks: 2
  commits: 2
plan_head_before: de48799783ca7e8a70b0cb3bc0a75664a633a7ba
plan_head_after: 07fd276ae8d5fb355ca32720cb06306200d2c395
commits: 2
tech-stack:
  added: []
  patterns:
    - host-only static C dependency with private symbol aliases
key-files:
  created:
    - cmake/Miniz.cmake
    - host/miniz/miniz_tinfl.c
    - host/miniz/miniz_tinfl.h
    - host/miniz/miniz_common.h
    - host/miniz_compat/miniz.h
  modified:
    - cmake/DiscTargets.cmake
    - tests/cmake/vendored.cmake
    - THIRD-PARTY-NOTICES.md
    - PROVENANCE.md
key-decisions:
  - Use the library preset to prove the host image target with CHD disabled.
  - Confine upstream conversion-warning suppressions to the unchanged inflater source.
patterns-established:
  - Private static C target for modular vendored host code, with hidden prefixed symbols.
requirements-completed: []
coverage:
  - id: D1
    description: The pinned modular inflater is built as an independent host-only target with four private symbols and its exact MIT notice.
    requirement: PBP-01
    verification:
      - kind: integration
        ref: cmake --preset library && cmake --build --preset library --target schwaystation_disc_image; exact source/license and nm symbol checks
        status: pass
    human_judgment: false
  - id: D2
    description: Existing vendored digests remain covered and both the old and new source mutations fail with the digest-mismatch diagnostic.
    requirement: PBP-01
    verification:
      - kind: unit
        ref: ctest --preset ci --no-tests=error -R '^libretro[.]vendored([.]self_test)?$' --output-on-failure
        status: pass
      - kind: integration
        ref: cmake -DROOT=. -DMANIFEST=tests/fixtures/manifest.txt -DNOTICES=THIRD-PARTY-NOTICES.md -P tests/cmake/notices.cmake
        status: pass
    human_judgment: false
duration: 1h10m
completed: 2026-11-21
status: complete
---

# Phase 10 Plan 02: Standalone PBP inflater Summary

**The PBP host path now has a pinned, privately named MIT inflater and a digest self-test that guards both legacy and new vendored sources.**

## Performance

- **Runtime/model:** Codex with OpenGSD 1.20.0; typed executor `gpt-6-luna` (xhigh).
- **Elapsed wall interval:** about 1h05m from the prior STATE checkpoint at 06:16:36Z through closeout; active time was not measured.
- **Tasks:** 2.
- **Task commits:** `8e64f5c`, `07fd276`.
- **Observed bottleneck:** The first compile exposed upstream conversion warnings and undefined portability macros under the project’s strict warning policy. The correction was confined to the unmodified upstream source. A symbol assertion also needed to account for Mach-O underscores and archive member labels.

## Accomplishments

- Added only the three pinned modular miniz files, the project-owned compatibility shim and provenance. The upstream `miniz.h` was excluded.
- Added a private static, PIC, C-only target; all four emitted tinfl globals receive private aliases. `MINIZ_NO_MALLOC`, `MINIZ_NO_STDIO` and `MINIZ_NO_TIME` remain enabled, and CMake generates the export header.
- Linked the inflater independently of libchdr and proved the host image target builds with the separate `library` preset’s CHD and CHD tool options both OFF.
- Extended the existing digest table without removing prior entries. Its self-test separately mutates and restores `rc_hash_disc.c` and `miniz_tinfl.c`; each produces the expected digest-mismatch failure.
- Added the exact pinned MIT grant and binary coverage notice. TODO 006 is moved to the completed directory. The repository gate is 3,869/3,901 lines.

## Decisions and Issues

- The repository CI preset defaults CHD ON, so the required CHD-OFF build used the existing separate `library` preset. A `cmake --preset ci -LAH` inspection reconfigured the CI directory and reset its cache to the preset defaults; the read-only form for future inspection is `cmake -LA -N build/ci`.
- AppleClang 21.0.0.21000101 rejected upstream conversions under `-Werror` and `-Wundef`. The three portability macros use the verified portable value `0`; only `host/miniz/miniz_tinfl.c` receives GNU/Clang-family `-Wno-conversion` and `-Wno-sign-conversion`. Project-owned C files keep the full warning policy.
- The `nm` check initially matched the archive member label rather than an undefined symbol. Parsing symbol fields and normalizing Mach-O’s leading underscore confirmed the four intended definitions and only standard runtime undefineds.

## Next Plan Readiness

Plan 10-11 is the other ready Plan 2 wave item and is next under the configured sequential executor. It changes Nightly routing/workflow, so its verification should use `act --dryrun` for routing and review any hosted run’s critical path and runner minutes while and after it runs. Phase-wide PBP-01 remains pending.

## Verification

- Pin, SHA-256, exact upstream LICENSE text and admitted-file checks: exit 0 (0.76s).
- `cmake -DROOT=. -DMANIFEST=tests/fixtures/manifest.txt -DNOTICES=THIRD-PARTY-NOTICES.md -P tests/cmake/notices.cmake && cmake -DCASE=check -DROOT=. -P tests/cmake/vendored.cmake && cmake -DCASE=self_test -DROOT=. -DWORK=/private/tmp/schwaystation-vendored-self-test -P tests/cmake/vendored.cmake`: exit 0 (0.05s). Both mutation controls produced the expected digest-mismatch result and were restored.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake && git diff --check`: exit 0 (0.07s), 3,869/3,901.
- `cmake --preset library && cmake --build --preset library --target schwaystation_disc_image`: exit 0. The configure cache confirms CHD and the CHD tool are OFF. The caller compile command carries all three MINIZ flags and four aliases; the archive contains exactly the four private definitions and no unresolved tinfl, allocator or zlib symbols.
- `ctest --preset ci --no-tests=error -R '^libretro[.]vendored([.]self_test)?$'`: exit 0, 2/2 in 0.11s. The CI preset does not register `policy.notices`; its direct hygiene script passed above.
- No workflow changed in this plan, so `act` and hosted CI were not applicable.
