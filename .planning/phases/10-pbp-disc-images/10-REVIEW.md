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

## Ship-fix review (2026-11-21)

An independent review of the repaired Phase 10 CI diff found no blockers or warnings. It confirmed the
classifier test still checks the two outputs consumed by the CI job while the dedicated route test covers
`nightly_full` and `pbp_only`; the pointer check is type-safe; and the MSVC opener matches existing host code.
After the hosted matrix exposed a protected-base dependency gap, the `compat-base` sparse checkout added
`tests/cmake/date_days.cmake`, and the workflow wiring assertion now guards that file closure. This follow-up
was also independently reviewed clean. Focused classifier, PBP and protected-ratchet checks passed locally.

## Seed-preflight assertion follow-up (2026-11-21)

An independent review found that the Nightly workflow assertion did not guarantee every replay seed count was
checked before the first timed campaign. The workflow itself already placed the loader campaign after the seed
loop, but the assertion mentioned that loader campaign before `seed_output`, and its route regex treated the
literal shell `||` as regex alternation. That made the regression check weaker than intended.

The assertion now treats `||` literally and requires the loader campaign after the seed validation loop closes.
`cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` passed. A temporary negative fixture
with the loader campaign moved inside the seed loop was rejected by the same command (expected exit 1). The
3,901-line budget remains exactly at its cap. No implementation or workflow coverage was removed.

## Generated-media graph review follow-up (2026-11-21)

The Nightly failure showed the PBP fixture directly depends on `xa.xa` and `movie.str`, which the no-external-
media generator produces alongside `media.sha256`. Those files were not declared as Ninja byproducts. The
`DiscFixtures.cmake` command now declares `cdda.wav`, `movie.str`, and `xa.xa` as byproducts. Independent review
also caught that the first static assertion searched the whole file and could accept byproducts attached to an
unrelated command. The assertion now extracts the `add_custom_command` whose output is `media.sha256` and checks
only that block. A temporary negative fixture attaching byproducts to another command was rejected, while the
actual `disc_registration` check passed. The narrowed assertion was independently re-reviewed clean; no
coverage was dropped, and the line budget remains 3,901/3,901.
