---
phase: 16-patches-cheats-and-compatibility-evidence
verified: 2026-11-27T03:56:24Z
status: passed
score: 4/4 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-01-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-01-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-02-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-02-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-03-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-03-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-04-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-04-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-05-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-05-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-06-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-06-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-07-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-07-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-08-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-08-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-09-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-09-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-10-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-10-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-11-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-11-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-12-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-12-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-13-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-13-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-14-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-14-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-15-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-15-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-16-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-16-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-17-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-17-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-18-PLAN.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-18-SUMMARY.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-LOCAL-EVIDENCE.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-RESEARCH.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-REVIEW-DISPOSITION.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-REVIEW.md
  - .planning/phases/16-patches-cheats-and-compatibility-evidence/16-SOURCE-AUDIT.md
  - CMakeLists.txt
  - CMakePresets.json
  - PROVENANCE.md
  - THIRD-PARTY-NOTICES.md
  - cmake/DiscTargets.cmake
  - cmake/toolchains/wasm32-wasi.cmake
  - docs/cheats.md
  - docs/compatibility-inventory.md
  - docs/compatibility.md
  - docs/discs.md
  - docs/embedding.md
  - docs/libretro.md
  - docs/maintaining.md
  - docs/patches.md
  - docs/runner.md
  - docs/testing.md
  - examples/sdl2/CMakeLists.txt
  - host/cheats.c
  - host/cheats.h
  - host/dat_inventory.c
  - host/dat_inventory.h
  - host/disc_bps.c
  - host/disc_image.c
  - host/disc_image.h
  - host/disc_patch.c
  - host/disc_patch.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - pins.txt
  - quality.json
  - runner/main.c
  - scripts/compat-controls.sh
  - scripts/compat-doc.sh
  - scripts/compat-ratchet.sh
  - scripts/library.sh
  - src/instance.c
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v10.c
  - tests/api/v11.c
  - tests/api/v9.c
  - tests/cmake/Capabilities.cmake
  - tests/cmake/capabilities_controls.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/workflows.cmake
  - tests/compat/dat-summary.txt
  - tests/compat/features.csv
  - tests/compat/inventory.txt
  - tests/compat/library_tests.c
  - tests/compat/required-capabilities.csv
  - tests/compat/tests.cmake
  - tests/disc/inventory.txt
  - tests/disc/patch.c
  - tests/disc/tests.cmake
  - tests/fuzz/cheat.c
  - tests/fuzz/inventory.txt
  - tests/fuzz/ppf.c
  - tests/fuzz/tests.cmake
  - tests/hygiene/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/runner/cheats.c
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
covered_digest: "v4:sha256:a2446cbd5a6ad9cc93d5b0e2273b982b09ec0f7a590c7211c1ac325b636220a6"
behavior_unverified: 0
overrides_applied: 0
re_verification:
  previous_status: gaps_found
  previous_score: 4/4
  gaps_closed:
    - "The Phase 16 repository-check budget mismatch is resolved: REQUIREMENTS.md, the measured total, and active gates all use 4,694 lines."
  gaps_remaining: []
  regressions: []
---

# Phase 16: Patches, Cheats and Compatibility Evidence Verification Report

**Phase Goal:** Players can use supported disc patches and cheat strings, and can see which library claims the shipped evidence supports.
**Verified:** 2026-11-27T03:56:24Z
**Status:** passed
**Re-verification:** Yes — refreshed for the current source head after resolving the repository-check budget discrepancy.

## Goal Achievement

The four roadmap success criteria and all four phase requirements are verified against source head `a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8` and the recorded local/current-head hosted evidence. The implementation supports the documented patch and cheat subsets, exposes the inventory and evidence reports, and keeps unsupported capability claims unknown. The repository-check budget discrepancy is resolved: `REQUIREMENTS.md`, the measured total, the current maintainer instructions and active CI, Nightly, hygiene, and workflow checks use the approved 4,694-line ceiling.

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | Runner patching supports PPF1/2/3 and BPS with source identity and bounds validation, immutable source bytes, deterministic sector overlays, and no partial publication after malformed input. | ✓ VERIFIED | `host/disc_patch.c`, `host/disc_bps.c`, `host/disc_image.c`, and `runner/main.c` implement candidate validation, BPS staging, identity checks, and replacement. `tests/disc/patch.c` covers format checks, malformed records, source binding, and replace/preserve behavior. Final local CI and hosted CI passed; hosted BE32 passed all 694 tests. See `docs/patches.md` and [16-LOCAL-EVIDENCE.md](16-LOCAL-EVIDENCE.md). |
| 2 | Runner and libretro apply the documented bounded GameShark subset through the supported RAM write API; invalid replacement does not mutate the prior configuration, and hashes/state include active cheats. | ✓ VERIFIED | `host/cheats.c` parses and validates a candidate before replacing it, then applies writes through `schway_write_ram` in `src/instance.c`. `runner/main.c` applies cheats at the frame boundary; `libretro/libretro.c` applies them from `retro_run` and validates the serialized cheat envelope before restoring. `tests/runner/cheats.c`, API v11 tests, and `tests/libretro/host.c` cover parsing, order, invalid replacement, hashing, and serialization. Final local CI and hosted CI passed. |
| 3 | Compatibility inventory accounts for the pinned licensed 13,407-entry DAT, points supported outcomes to evidence, and describes untested rows as unknown rather than playability claims. | ✓ VERIFIED | `host/dat_inventory.c`, the runner inventory command, `scripts/compat-doc.sh`, `tests/compat/dat-summary.txt`, and `docs/compatibility.md` carry the pinned source/digest/license and aggregate counts. The current inventory is 13,407 unknown, 0 pass, and 0 fail; all occurrences are accounted for, with 177 missing serials and 988 duplicate-serial entries. Final CI passed both compatibility inventory emulator cases and their dependent hash comparison. |
| 4 | Libretro remains experimental unless every required current capability passes; the matrix covers providers, region/board profiles, peripherals, card persistence, patches, and cheats. | ✓ VERIFIED | `tests/cmake/Capabilities.cmake` validates the 51-row required matrix, imports fresh matching build results, and only clears the experimental marker for a complete all-pass result. `tests/cmake/capabilities_controls.cmake` covers missing, stale, and failed evidence. Current build output has 16 passes and 35 unknowns, including all provider/profile boot-to-play rows; generated `schwaystation_libretro.info` therefore correctly remains `is_experimental = "true"`. |

**Score:** 4/4 roadmap truths verified.

### Advisory (New Scope, Unevidenced)

None.

## Required Artifacts

The GSD artifact verifier checked all 53 `must_haves.artifacts` declared across the 18 plans: all exist and passed their substantive path/pattern checks. Direct review confirmed the production files are not placeholders and the main paths are connected.

| Artifact group | Representative artifacts | Status | Details |
| --- | --- | --- | --- |
| Patch implementation and runner integration | `host/disc_patch.c`, `host/disc_bps.c`, `host/disc_image.c`, `runner/main.c` | ✓ VERIFIED | Candidate validation and transactional replacement feed the runner's disc-sector provider. |
| Cheat API and host integration | `include/schwaystation.h`, `src/instance.c`, `host/cheats.c`, `libretro/libretro.c` | ✓ VERIFIED | Explicit RAM write API, bounded host parser, frame-boundary apply, and versioned serialization are present and wired. |
| Inventory and compatibility claims | `host/dat_inventory.c`, `scripts/compat-doc.sh`, `docs/compatibility.md`, `tests/compat/dat-summary.txt` | ✓ VERIFIED | A pinned source generates aggregate inventory counts; per-claim evidence and unknowns remain visible. |
| Capability gate and regressions | `tests/cmake/Capabilities.cmake`, `tests/cmake/capabilities_controls.cmake`, `libretro/CMakeLists.txt`, `.github/workflows/ci.yml`, `.github/workflows/nightly.yml` | ✓ VERIFIED | Freshness, required-row completeness and the experimental marker have implementation and tests. The libretro install hook imports completed capability results before copying the generated `.info` file into the package. |
| Current example and maintainer guidance | `examples/sdl2/CMakeLists.txt`, `docs/testing.md` | ✓ VERIFIED | The SDL2 example target includes the PBP inflater and PBP reader sources. The testing guide states the approved 4,694-line cap and invokes the line-budget check with `LIMIT=4694`, matching `REQUIREMENTS.md` and the active workflow preflights. |

## Key Link Verification

The generic key-link matcher did not recognize many CMake/test wiring edges, so those were traced manually in source and build definitions. No runtime feature path was found disconnected. One plan edge paired two separate fuzz harness source files as if one called the other; in code they are independent fuzz targets, each wired to its own production parser and Nightly campaign.

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `runner/main.c` patch option | disc image sector reads | load candidate → validate identity → replace overlay → provider read | ✓ WIRED | PPF/BPS candidates are only attached after complete validation; BPS output is staged before publication. |
| runner/libretro cheat inputs | core RAM | parser → bounded ordered table → `schway_write_ram` | ✓ WIRED | Runner applies before frames; libretro applies at `retro_run` frame boundary. |
| libretro cheat configuration | serialize/restore and state hash | canonical cheat table in versioned envelope and hash material | ✓ WIRED | Restore validates header/checksum and a complete candidate before publishing state. |
| pinned DAT source and inventory ledgers | generated compatibility report | runner aggregate inventory → doc generator | ✓ WIRED | Report accounts the source entries and keeps their status separate from boot evidence. |
| required capability matrix | libretro `.info` experimental claim | current build results → matrix evaluation → generated info marker | ✓ WIRED | Missing, stale, or failed rows preserve the experimental marker. |

## Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| Patched sector response | Raw sector bytes plus validated overlay | Host-supplied source image/read callback and validated patch bytes | Yes | ✓ FLOWING |
| Cheat writes | Normalized address/width/value entries | Runner arguments or libretro callback input parsed into the bounded table | Yes | ✓ FLOWING |
| DAT report | Source occurrence counts and aggregate statuses | Pinned licensed DAT parsed by `schwaystation-run --library-inventory` | Yes | ✓ FLOWING |
| Capability marker | Required row results for current source/build | Fresh matching CTest evidence and required-capability matrix | Yes | ✓ FLOWING |

No rendered compatibility value terminates at a static fallback or empty prop. The checked-in no-build summary honestly records unknown evidence; the build-local report uses current run results.

## Behavioral Spot-Checks

No additional tests were run during this verification, as requested. The exact-head Release workflow completed successfully on the current source head, including the current CMake integration files; the existing phase review remains present and current.

| Behavior | Command/evidence | Result | Status |
| --- | --- | --- | --- |
| Full local build, tests, and release checks | `cmake --workflow --preset ci` in `16-LOCAL-EVIDENCE.md` | Exit 0; 742/742 tests and all 8 release checks passed. | ✓ PASS (recorded run) |
| Full Release CI on current exact source head | [CI run 45923155197](https://github.com/szTheory/schwaystation/actions/runs/45923155197) | `Release` completed successfully at `a2f37b46eab24f53e30f99c6b3faff6eedc8f2f8`. | ✓ PASS (hosted run) |
| Cross-platform and big-endian regression, including inventory emulation and downstream hash comparison | [CI run 45902643250](https://github.com/szTheory/schwaystation/actions/runs/45902643250) | Passed; BE32 694/694, both inventory emulator cases, dependent hash comparison, platform jobs, RetroArch, and release rehearsal passed. | ✓ PASS (hosted run) |
| Compatibility vectors, benchmark legs, and bounded fuzz targets | [Nightly run 45902643154](https://github.com/szTheory/schwaystation/actions/runs/45902643154) | Passed; vectors, all four benchmark legs, and all 13 bounded fuzz campaigns passed. | ✓ PASS (hosted run) |

## Probe Execution

Not applicable. No phase plan declares a probe, and no conventional `scripts/*/tests/probe-*.sh` probe is present.

## Requirements Coverage

All requirement identifiers declared in the 18 plan frontmatters were cross-referenced to `.planning/REQUIREMENTS.md`; no declared identifier is missing, and no Phase 16 requirement mapped in `REQUIREMENTS.md` is orphaned from the plans.

| Plan | Declared requirement IDs | Cross-reference |
| --- | --- | --- |
| 16-01 | PATCH-01, CHEAT-01, COMPAT-01, COMPAT-03 | All four exist in `REQUIREMENTS.md`. |
| 16-02 | CHEAT-01 | Exists in `REQUIREMENTS.md`. |
| 16-03 | CHEAT-01 | Exists in `REQUIREMENTS.md`. |
| 16-04 | CHEAT-01 | Exists in `REQUIREMENTS.md`. |
| 16-05 | CHEAT-01 | Exists in `REQUIREMENTS.md`. |
| 16-06 | CHEAT-01 | Exists in `REQUIREMENTS.md`. |
| 16-07 | PATCH-01 | Exists in `REQUIREMENTS.md`. |
| 16-08 | PATCH-01 | Exists in `REQUIREMENTS.md`. |
| 16-09 | PATCH-01 | Exists in `REQUIREMENTS.md`. |
| 16-10 | PATCH-01 | Exists in `REQUIREMENTS.md`. |
| 16-11 | PATCH-01, CHEAT-01 | Both exist in `REQUIREMENTS.md`. |
| 16-12 | COMPAT-01 | Exists in `REQUIREMENTS.md`. |
| 16-13 | COMPAT-01 | Exists in `REQUIREMENTS.md`. |
| 16-14 | COMPAT-03, CHEAT-01 | Both exist in `REQUIREMENTS.md`. |
| 16-15 | COMPAT-03 | Exists in `REQUIREMENTS.md`. |
| 16-16 | PATCH-01, CHEAT-01, COMPAT-01, COMPAT-03 | All four exist in `REQUIREMENTS.md`. |
| 16-17 | COMPAT-03 | Exists in `REQUIREMENTS.md`. |
| 16-18 | PATCH-01, CHEAT-01, COMPAT-01, COMPAT-03 | All four exist in `REQUIREMENTS.md`. |

| Requirement | Description | Status | Evidence |
| --- | --- | --- | --- |
| PATCH-01 | PPF1/2/3/BPS runner overlay with identity/bounds validation, immutable source, deterministic sectors, atomic malformed-input rejection. | ✓ SATISFIED | Production path in `host/disc_patch.c`, `host/disc_bps.c`, `host/disc_image.c`, and `runner/main.c`; regression cases in `tests/disc/patch.c`; final local and hosted suites passed. |
| CHEAT-01 | Documented bounded GameShark subset for runner/libretro, explicit core operation, no mutation on invalid input, and inclusion in relevant hashes/state. | ✓ SATISFIED | `host/cheats.c`, `src/instance.c`, `runner/main.c`, `libretro/libretro.c`; parser/runner/libretro coverage passed in final CI. |
| COMPAT-01 | Pinned licensed Redump inventory accounts for all 13,407 entries with evidence and explicit unknowns; report distinguishes inventory from playability. | ✓ SATISFIED | `tests/compat/dat-summary.txt`, `docs/compatibility.md`, `docs/compatibility-inventory.md`; final CI passed both inventory emulator cases and the dependent hash comparison. |
| COMPAT-03 | Experimental status clears only for a complete all-pass required matrix; incomplete evidence remains experimental; DAT-wide playability stays out of the matrix. | ✓ SATISFIED | 51-row matrix, negative controls, current 16 pass/35 unknown evidence, and generated `is_experimental = "true"`; all regression checks passed. |

## Anti-Patterns Found

No unresolved `TBD`, `FIXME`, `XXX`, `TODO`, `HACK`, or placeholder implementation was found in the phase implementation files. The current SDL2 and libretro CMake wiring was checked against the phase claims and is covered by the successful exact-head Release run. The budget declaration, maintainer instructions, measured total and active checks agree at 4,694 lines.

## Human Verification Required

None. The relevant plan blocks contain no deferred human checks, and `.planning/AUTONOMOUS.md` directs verification to use the recorded automated evidence.

## Gaps Summary

No remaining gaps. The patching, cheat, and compatibility outcomes are implemented and supported by the final local and exact-head hosted evidence; all four requirement IDs are satisfied. The three previously stale inputs reconcile: the guide uses the approved 4,694-line limit, the SDL2 example wires the PBP reader sources, and libretro package staging imports current capability results before installing metadata.

---

_Verified: 2026-11-27T03:56:24Z_
_Verifier: the agent (gsd-verifier)_
