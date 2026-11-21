---
phase: 09-chd-disc-images
verified: 2026-11-21T01:21:40Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .planning/phases/09-chd-disc-images/09-01-PLAN.md
  - .planning/phases/09-chd-disc-images/09-01-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-02-PLAN.md
  - .planning/phases/09-chd-disc-images/09-02-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-03-PLAN.md
  - .planning/phases/09-chd-disc-images/09-03-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-04-PLAN.md
  - .planning/phases/09-chd-disc-images/09-04-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-05-PLAN.md
  - .planning/phases/09-chd-disc-images/09-05-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-06-PLAN.md
  - .planning/phases/09-chd-disc-images/09-06-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-07-PLAN.md
  - .planning/phases/09-chd-disc-images/09-07-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-08-PLAN.md
  - .planning/phases/09-chd-disc-images/09-08-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-09-PLAN.md
  - .planning/phases/09-chd-disc-images/09-09-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-10-PLAN.md
  - .planning/phases/09-chd-disc-images/09-10-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-11-PLAN.md
  - .planning/phases/09-chd-disc-images/09-11-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-12-PLAN.md
  - .planning/phases/09-chd-disc-images/09-12-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-13-PLAN.md
  - .planning/phases/09-chd-disc-images/09-13-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-14-PLAN.md
  - .planning/phases/09-chd-disc-images/09-14-SUMMARY.md
  - .planning/phases/09-chd-disc-images/09-15-PLAN.md
  - .planning/phases/09-chd-disc-images/09-15-SUMMARY.md
  - CMakeLists.txt
  - CMakePresets.json
  - PROVENANCE.md
  - THIRD-PARTY-NOTICES.md
  - cmake/CHDFixtures.cmake
  - cmake/CHDTool.cmake
  - cmake/CHDToolPins.cmake
  - cmake/DiscFixtures.cmake
  - cmake/DiscTargets.cmake
  - cmake/Libchdr.cmake
  - docs/compatibility.md
  - docs/discs.md
  - docs/maintaining.md
  - docs/media-testing.md
  - docs/testing.md
  - examples/sdl2/CMakeLists.txt
  - host/disc_chd.c
  - host/disc_chd.h
  - host/disc_chd_metadata.c
  - host/disc_chd_metadata.h
  - host/disc_image.c
  - host/disc_image.h
  - host/disc_probe.c
  - host/media_limits.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - pins.txt
  - quality.json
  - scripts/compat-controls.sh
  - scripts/compat-doc.sh
  - scripts/compat-ratchet.sh
  - tests/cmake/compat-controls.cmake
  - tests/cmake/compat-ratchet.cmake
  - tests/cmake/date_days.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/perf_compare.cmake
  - tests/cmake/workflows.cmake
  - tests/compat/compat-control-cases.tsv
  - tests/compat/features.csv
  - tests/compat/inventory.txt
  - tests/compat/tests.cmake
  - tests/disc/chd.c
  - tests/disc/fixture.c
  - tests/disc/image.c
  - tests/disc/inventory.txt
  - tests/disc/media.c
  - tests/disc/tests.cmake
  - tests/fuzz/chd.c
  - tests/fuzz/inventory.txt
  - tests/fuzz/seeds.h
  - tests/fuzz/tests.cmake
  - tests/inventory/wasm32.txt
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/release/inventory.txt
  - tests/release/tests.cmake
  - tests/runner/disc.c
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tests/tests.cmake
  - tools/chd-tool/main.cpp
covered_digest: "v4:sha256:c8d13054d5c31b7f5d051488ebe7bdd6bc3e2fbb1d20bb171dd15f2460b096a7"
behavior_unverified: 0
overrides_applied: 0
deferred:
  - truth: "The shared media contract and malformed-input coverage must also be applied by future disc providers."
    addressed_in: "Phases 10–12"
    evidence: "ROADMAP.md Phase 10 PBP, Phase 11 ECM/CloneCD, and Phase 12 SBI goals require the later providers and their regression cases."
---

# Phase 9: CHD disc images Verification Report

**Phase Goal:** Players can open the supported CHD subset with the same behavior as the corresponding uncompressed discs.
**Verified:** 2026-11-21T01:21:40Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | The documented CHD subset opens through the common provider and matches the equivalent CUE identity, TOC, sectors, subcode behavior and bounded guest results. | ✓ VERIFIED | `docs/discs.md` states the supported v3–v5 CD metadata subset and v1–v2/parent exclusions. `disc.image.chd_parity` compares all 222 logical sectors in both read orders, TOC and identity; `disc.fixture.chd.boot/read/audio/fmv`, `runner.disc.chd` and `libretro.content.chd` exercise the shipped consumers. Current exact-head CI passed: [run 44843623296](https://github.com/szTheory/schwaystation/actions/runs/44843623296). |
| 2 | CHD parity and hashes pass on all eight applicable hosted lanes, and CHD stays disabled by default until that parity is established. | ✓ VERIFIED | Exact-head CI passed all six package lanes, BE32, WASI and the cross-lane `hash-equality` gate on `16dc50920b793316ad1295d47168bdb39d2cc87b`. `CMakeLists.txt` defaults `SCHWAYSTATION_CHD` to `OFF`; the measured WASI module is exactly 859,379 bytes, matching both the quality floor and ceiling, and its current size gate passed. |
| 3 | CHD-enabled and disabled builds publish accurate libretro extension claims and required archive notices from the pinned permissive reader closure. | ✓ VERIFIED | `libretro/schwaystation_libretro.info.in`, `tests/cmake/info_claims.cmake`, `tests/release/tests.cmake`, `THIRD-PARTY-NOTICES.md` and `PROVENANCE.md` cover conditional `.info`, both package configurations and the BSD-3/libchdr plus codec notices. The exact-head `CI required` and release `rehearsal` passed in run 44843623296. |
| 4 | Bounded parsing, media preflights, malformed-input fuzzing and open-failure isolation protect the CHD path before guest execution. | ✓ VERIFIED | `host/disc_chd_metadata.c`, `host/disc_chd.c` and `host/media_limits.h` enforce the documented bounds; `tests/disc/media.c`, `tests/disc/chd.c`, `tests/disc/image.c`, `tests/runner/disc.c` and `tests/libretro/host.c` cover rejection, preflight, parity, and failure isolation. Hosted CHD fuzz passed on [run 44830486029](https://github.com/szTheory/schwaystation/actions/runs/44830486029); the successful [Nightly run 44842317071](https://github.com/szTheory/schwaystation/actions/runs/44842317071) also passed fuzz on `f4221bd`, whose only relevant delta to this head is the libretro host test. |
| 5 | The feature ledger records only demonstrated CHD capability and its preflight/ratchet preserve key completeness and previous passes. | ✓ VERIFIED | `tests/compat/features.csv`, `scripts/compat-controls.sh`, `scripts/compat-ratchet.sh`, and `tests/cmake/compat-ratchet.cmake` implement the row, missing/duplicate-key rejection and protected pass ratchet. The full exact-head test workflow and current review passed; the CHD row claims only exercised provider/host/package/guest evidence. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Deferred Items

| # | Item | Addressed In | Evidence |
| --- | --- | --- | --- |
| 1 | Apply the shared media contract and add provider-specific malformed-input regression cases for PBP. | Phase 10 | The Phase 10 goal and success criteria cover PBP layouts, bounds, rejection and ledger evidence. |
| 2 | Apply the shared media contract and provider-specific regression cases for ECM and CloneCD. | Phase 11 | The Phase 11 goal and success criteria cover ECM decoding and CloneCD track/companion behavior. |
| 3 | Apply the shared media contract to SBI and protected-disc handling. | Phase 12 | The Phase 12 goal and success criteria cover SBI identity/LBA binding, malformed input and fuzz replay. |

### Required Artifacts

| Artifact | Expected | Status | Details |
| --- | --- | --- | --- |
| `host/disc_chd.c`, `host/disc_chd_metadata.c` | Bounded CHD provider and metadata/TOC projection | PASS | Present, wired by `host/disc_image.c`, and covered by CHD open, metadata, TOC, boundary, parity and malformed-input tests. |
| `tests/disc/*`, `tests/fuzz/chd.c` | Owned fixture preflight, parity, regressions and bounded fuzz | PASS | CHD guest tests require media, parity and compatibility setup fixtures; hosted fuzz and exact-head test gates passed. |
| `tests/runner/disc.c`, `tests/libretro/host.c` | Both shipped hosts select and preserve CHD behavior | PASS | Runner and libretro CHD content paths are in CTest; current exact-head CI passed. |
| `cmake/Libchdr.cmake`, `pins.txt`, provenance and notices | Pinned permissive closure, disabled default and distributable notices | PASS | Pinned archive digest and source closure are checked; package notice controls passed. |
| `libretro/schwaystation_libretro.info.in`, package claims | Accurate conditional extension metadata in ON/OFF packages | PASS | ON/OFF info and release archive selectors passed. |
| `tests/compat/features.csv` and compatibility controls | Auditable CHD row with protected prior passes | PASS | Preflight and ratchet are tested; row scope matches the evidence. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| CMake option and pinned archive | `host/disc_chd.c` | `cmake/Libchdr.cmake`, target wiring and public headers | WIRED | CHD dependency is host-only, optional and built only when enabled. |
| `host/disc_image.c` | Runner and libretro | Common open/select/read callbacks | WIRED | Both consumers use the same provider contract and report failed image opens before replacing the active selection. |
| Owned CUE/CHD fixture and manifest | Preflight, parity and guest tests | CTest fixtures in `tests/disc/tests.cmake` | WIRED | Structural, parity and feature-key checks are prerequisites for CHD guest execution. |
| CHD build option | `.info` extension and package notices | CMake substitutions plus release archive tests | WIRED | `chd` is advertised only in the enabled build; both package configurations are checked. |
| Compatibility ledger | Generated report and protected ratchet | `scripts/compat-doc.sh`, `scripts/compat-ratchet.sh` | WIRED | Ledger keys and existing passes are checked before accepting updated feature evidence. |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| --- | --- | --- | --- | --- |
| CHD sector provider | Decoded raw sector bytes and TOC | Pinned libchdr stream, parsed track records and checked media extents | Yes | VERIFIED: parity compares all 222 logical sectors to the CUE source in both orders. |
| Runner/libretro disc boundary | Selected image path and sector callbacks | `schway_disc_image_open` and the common image descriptor | Yes | VERIFIED: runner and libretro CHD cases consume the provider; libretro checks enabled and rejected-image paths. |
| CHD compatibility row | Provider, host, package and guest predicates | `tests/compat/features.csv` and test-produced predicates | Yes | VERIFIED: feature preflight and protected ratchet passed; unsupported/unmeasured combinations remain unknown. |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| --- | --- | --- | --- |
| CHD/CUE sector, TOC and identity parity | `disc.image.chd_parity` under the Phase 9 focused selector; repeated in the current full CI workflow | 222/222 sectors matched in both orders; guard, TOC, subcode and identity checks passed | PASS |
| CHD guest and host integration | `disc.fixture.chd.*`, `runner.disc.chd`, `libretro.content.chd` through CTest | Boot, read, audio, FMV, runner and libretro paths passed with bounded completion | PASS |
| Bounds, malformed-input and failure isolation | CHD metadata/header/TOC/boundary selectors, regression controls and hosted fuzz | Exact-head CI passed; hosted malformed CHD campaign passed on run 44830486029 | PASS |
| Package and notice behavior | Release `release.archive_contents` and `release.notices`, plus CI rehearsal | Enabled and disabled package checks and rehearsal passed on run 44843623296 | PASS |

### Probe Execution

| Probe | Command | Result | Status |
| --- | --- | --- | --- |
| Cheap routing and consistency preflight | Existing `changes` job in `.github/workflows/ci.yml` | Exact-head workflow routed required gates; inventory, selectors and budgets passed before long jobs | PASS |
| Eight-lane parity and packaging | Main CI `CI required` with all lane dependencies | All six package lanes, BE32, WASI, hash equality, RetroArch and release rehearsal succeeded | PASS |
| Nightly fuzz duplication check | Compare current head to the last successful Nightly inputs | A same-head Nightly was canceled as redundant; it is not a required Phase 9 gate and adds no missing coverage | PASS (reuse justified) |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| --- | --- | --- | --- | --- |
| MEDIA-01 | 09-05, 09-06, 09-12, 09-14 | CHD identity, TOC, track geometry, raw sectors and applicable subcode match the corresponding CUE behavior. | PASS for CHD; remaining providers deferred | 222-sector parity, runner/libretro integration and exact-head CI. |
| MEDIA-02 | 09-04, 09-05, 09-07 | CHD counts, offsets, allocations and malformed inputs are bounded and tested before guest use. | PASS for CHD; remaining providers deferred | Media preflight, malformed-input regressions and hosted CHD fuzz. |
| CHD-01 | 09-02, 09-07, 09-09, 09-11, 09-13, 09-15 | Optional pinned BSD-3 reader, deterministic parity/hash across applicable lanes, fuzz, notices and conditional `.info`. | PASS | Exact-head eight-lane CI, WASI measurement, fuzz and package controls. |
| COMPAT-02 | 09-08, 09-09, 09-14 | Compatibility evidence is keyed, truthful and protected by a dated ratchet. | PASS | Ledger preflight, ratchet controls and exact-head CI. |

### CI Efficiency Review

The exact-head run [44843623296](https://github.com/szTheory/schwaystation/actions/runs/44843623296) completed successfully. `CI required` measured a 2,419-second critical path and the workflow used 6,571 runner-seconds (109.52 runner-minutes). The timing fingerprint differed from the stored baseline, so this is recorded as an unbaselined measurement, not a speedup claim. BE32 was the longest job at 2,375 seconds. The redundant same-head Nightly run was canceled after its noncritical work; required Main CI and all quality gates remain intact. Existing `changes` checks perform cheap selector/inventory/budget preflights before expensive jobs, and `docs/maintaining.md` documents `act --dryrun` routing limits and hosted-only checks.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| --- | --- | --- | --- | --- |
| — | — | None found in Phase 9 implementation or review scope. | — | The current source review is clean; no stub, unwired provider or unsupported package claim was found. |

### Human Verification Required

None. All Phase 9 acceptance checks are automated; no owner hardware or identity-bound action is needed.

### Gaps Summary

No Phase 9 gaps remain. The CHD subset, shared-provider integration, package claims, compatibility evidence and hosted lane coverage are verified. Full cross-provider MEDIA-01/MEDIA-02 coverage is deliberately deferred to the later provider phases named in the roadmap.

---
Verified: 2026-11-21T01:21:40Z  
Verifier: Codex inline goal-backward verification (model ID not exposed)
