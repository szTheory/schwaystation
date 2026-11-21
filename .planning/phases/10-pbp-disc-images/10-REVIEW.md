---
phase: 10-pbp-disc-images
reviewed: 2026-11-21T15:29:09Z
depth: standard
files_reviewed: 56
files_reviewed_list:
  - .github/workflows/nightly.yml
  - ASSET_POLICY.md
  - CMakePresets.json
  - PROVENANCE.md
  - THIRD-PARTY-NOTICES.md
  - cmake/DiscFixtures.cmake
  - cmake/DiscTargets.cmake
  - cmake/Miniz.cmake
  - cmake/PBPFixtures.cmake
  - docs/compatibility.md
  - docs/discs.md
  - docs/libretro.md
  - docs/maintaining.md
  - docs/media-testing.md
  - docs/runner.md
  - host/disc_image.c
  - host/disc_pbp.c
  - host/disc_pbp.h
  - host/disc_pbp_metadata.c
  - host/disc_pbp_metadata.h
  - host/disc_probe.c
  - host/miniz/miniz_common.h
  - host/miniz/miniz_tinfl.c
  - host/miniz/miniz_tinfl.h
  - host/miniz_compat/miniz.h
  - host/pbp_inflate.c
  - host/pbp_inflate.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - scripts/classify-changes.sh
  - tests/cmake/ci_route_controls.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/quality_controls.cmake
  - tests/cmake/triage_controls.cmake
  - tests/cmake/vendored.cmake
  - tests/cmake/workflows.cmake
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/disc/fixture.c
  - tests/disc/image.c
  - tests/disc/inventory.txt
  - tests/disc/pbp.c
  - tests/disc/tests.cmake
  - tests/fixtures/disc_boot_pbp.xml
  - tests/fixtures/manifest.txt
  - tests/fixtures/src/disc_boot.asm
  - tests/fuzz/inventory.txt
  - tests/fuzz/pbp.c
  - tests/fuzz/seeds.h
  - tests/fuzz/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tools/gen_pbp.py
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
  necessity: 0
status: clean
---
# Phase 10: Code Review Report

**Reviewed:** 2026-11-21T15:29:09Z  
**Depth:** standard  
**Files Reviewed:** 56  
**Status:** clean after resolving one warning before final verification.

## Summary

The PBP provider, inflater integration, shared image routing, runner/libretro consumers, generated fixtures, notices, and Nightly routing were reviewed. One CI reliability warning was resolved before final verification. No source-level blockers remain.

## Resolved warning

### WR-01: PBP-only Nightly omitted required fixture dependencies

**Severity:** WARNING  
**File:** `.github/workflows/nightly.yml:194-197`  
**Issue:** When `pbp_only` is true, the dependency step installs only `libclang-rt-21-dev`. The PBP-only build still builds `schwaystation-pbp-fixtures`; that target depends on `schwaystation-disc-fixtures` and on the generated `xa.xa` and `movie.str` media (`cmake/PBPFixtures.cmake:27-30`). Without `SCHWAYSTATION_DISC_MEDIA_DIR`, the fixture chain generates those assets through `psxavenc`, whose configure step requires the FFmpeg development packages (`cmake/DiscFixtures.cmake:88-105`, `cmake/Psxavenc.cmake:105`). On a runner image missing those packages, the route fails during fixture construction before the PBP campaign runs.
**Resolution:** Commit `76bdc68` removes the unnecessary PBP-only dependency branch. Both routes install the media encoder's full FFmpeg development closure, which the shared fixture build requires. The workflow wiring assertion now requires the complete package array, so this dependency cannot silently disappear on the PBP route.

## Resolution verification (2026-11-21)

- `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` — exit 0; job conditions, dependency package list and campaign wiring pass.
- Repository line-budget check — exit 0; 3,900/3,901 lines.
- `act workflow_dispatch --dryrun -W .github/workflows/nightly.yml -j fuzz -P ubuntu-26.04=ubuntu:latest --action-cache-path /private/tmp/schwaystation-act-cache` — exit 0; the Nightly fuzz job and its dependency/preflight steps parse and route. The PBP-only pull-request classification is separately covered by the existing classifier controls and this plan's static wiring check.
- Local actionlint passes after ignoring its stale runner-label catalog for the existing `ubuntu-26.04` labels; no workflow syntax errors remain.

## Follow-up test review (2026-11-21)

The first full local workflow exposed a stale compatibility assertion: the malformed sentinel `.pbp` was expected
to be classified as unsupported even though Phase 10 adds PBP parsing. The assertion now requires the existing
generic `PBP metadata is invalid` diagnostic while continuing to require empty stdout and no private-path leak.
An independent focused review found no actionable issue; `compat.probe.malformed` passed with its compatibility
preflight. The corrected final full local workflow also passed. This was a test expectation correction; no PBP
production code or acceptance bound changed.

---

_Reviewed: 2026-11-21T15:29:09Z_  
_Reviewer: the agent (gsd-code-reviewer)_  
_Depth: standard_
