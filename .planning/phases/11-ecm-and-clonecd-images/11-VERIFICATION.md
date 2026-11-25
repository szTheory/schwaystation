---
phase: 11-ecm-and-clonecd-images
verified: 2026-11-25T02:03:04Z
status: passed
score: 45/45 must-haves verified
re_verification:
  previous_status: passed
  previous_score: 45/45
  gaps_closed:
    - "Nightly fuzz closure now builds schwaystation-test-disc-legacy before its closure CTests."
    - "The CCD regression test declares symlink() under strict POSIX C17 builds."
    - "Nightly writes the designated zero-byte CHD seed before timed fuzz campaigns; runner timeout and WASI size gate are corrected from measured evidence."
  gaps_remaining: []
  regressions: []
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .planning/phases/11-ecm-and-clonecd-images/11-01-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-01-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-02-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-02-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-03-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-03-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-04-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-04-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-05-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-05-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-06-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-06-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-07-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-07-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-08-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-08-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-09-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-09-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-10-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-10-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-11-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-11-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-12-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-12-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-13-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-13-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-14-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-14-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-15-PLAN.md
  - .planning/phases/11-ecm-and-clonecd-images/11-15-SUMMARY.md
  - .planning/phases/11-ecm-and-clonecd-images/11-CONTEXT.md
  - .planning/phases/11-ecm-and-clonecd-images/11-DISCUSSION-LOG.md
  - .planning/phases/11-ecm-and-clonecd-images/11-PLAN-CHECK.md
  - .planning/phases/11-ecm-and-clonecd-images/11-PLAN-COVERAGE.md
  - .planning/phases/11-ecm-and-clonecd-images/11-RESEARCH.md
  - .planning/phases/11-ecm-and-clonecd-images/11-REVIEW.md
  - .planning/phases/11-ecm-and-clonecd-images/11-UAT.md
  - .planning/phases/11-ecm-and-clonecd-images/deferred-items.md
  - CMakePresets.json
  - README.md
  - cmake/DiscFixtures.cmake
  - cmake/DiscTargets.cmake
  - cmake/LegacyDiscFixtures.cmake
  - docs/compatibility.md
  - docs/discs.md
  - docs/libretro.md
  - docs/maintaining.md
  - docs/media-testing.md
  - docs/runner.md
  - docs/testing.md
  - examples/sdl2/CMakeLists.txt
  - host/disc_ccd.c
  - host/disc_ccd.h
  - host/disc_ecm.c
  - host/disc_ecm.h
  - host/disc_image.c
  - host/disc_probe.c
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - tests/cmake/info_claims.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/workflows.cmake
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/disc/ccd.c
  - tests/disc/ecm.c
  - tests/disc/fixture.c
  - tests/disc/image.c
  - tests/disc/inventory.txt
  - tests/disc/legacy.c
  - tests/disc/tests.cmake
  - tests/fixtures/manifest.txt
  - tests/fuzz/ccd.c
  - tests/fuzz/ecm.c
  - tests/fuzz/inventory.txt
  - tests/fuzz/replay_main.h
  - tests/fuzz/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tools/gen_ccd.py
  - tools/gen_ecm.py
covered_digest: "v4:sha256:f9ae5eeaa5266186a4c819536b019fb9bb6e98726b65d3207c8220b9538a2e2e"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 11: ECM and CloneCD Images Verification Report

**Phase Goal:** Players can open ECM and CloneCD images while retaining their documented track, sector and subcode behavior.
**Verified:** 2026-11-25T02:03:04Z
**Status:** passed
**Re-verification:** Yes; current descendants were checked after later released phases changed covered files.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Supported ECM files produce the expected sector bytes on repeated and out-of-order reads; source extents and sector counts are checked before guest runs, and malformed tables, offsets and truncated streams fail within documented bounds. | ✓ VERIFIED | `host/disc_ecm.c` validates full encoded extent, decoded length, terminator and EDC before publishing; bounded checkpoint reads are exercised by the ECM tests and the full CI run. `tests/disc/legacy.c` and runner preflights compare exact source and requested/produced counts before guests. |
| 2 | Supported CloneCD fixtures preserve modes, indexes, pregaps, subcode and first and last sector reads through CCD and IMG/SUB companions; invalid companion paths are rejected before an external file is opened. | ✓ VERIFIED | `host/disc_ccd.c` reconciles metadata and exact image/subcode sizes, canonicalizes and confines companion paths before opening, and transposes SUB data. `tests/disc/ccd.c` and `tests/fuzz/ccd.c` exercise boundary sectors, metadata, known-answer transpose, escapes and replacement behavior. |
| 3 | The ledger records fixture results for both providers against the existing disc contract. | ✓ VERIFIED | `tests/compat/features.csv` retains the fixture-backed ECM and CCD results and accurately keeps its phase-time hosted fields as `unknown`; later descendant Main, Nightly and Release runs are recorded below. |
| 4 | Malformed ECM and CCD inputs fail through private host APIs before a reader is published. | ✓ VERIFIED | Private `schway__disc_ecm_open`/`schway__disc_ccd_open` validation paths and malformed regression cases in `tests/disc/ecm.c` and `tests/disc/ccd.c`; full workflow passed. |
| 5 | CMake and exact test inventories include both private readers without changing the core ABI. | ✓ VERIFIED | `cmake/DiscTargets.cmake`, `tests/disc/tests.cmake`, `tests/disc/inventory.txt`, fuzz/runner/libretro registrations and private host headers; no public ABI change is present in the changed implementation. |
| 6 | Existing gates remain within the fixed 3,901-line ceiling. | ✓ VERIFIED | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` passed at 3,901/3,901. |
| 7 | The measured metadata-only Nightly shortcut is explicitly deferred. | ✓ VERIFIED | Phase context D-12 and `.planning/todos/pending/051-nightly-pr-job-routing.md` record the measured line-budget reason and retain full Nightly routing. |
| 8 | Full Nightly fallback, existing cases and required lanes remain in effect. | ✓ VERIFIED | `.github/workflows/nightly.yml` retains the full route; `tests/cmake/workflows.cmake` checks route and campaign wiring. Nightly run 45442543657 passed on the released Phase 14 code descendant. |
| 9 | ECM-01 and CCD-01 stay open until phase product evidence is complete. | ✓ VERIFIED | Both requirements are complete after Phase 11 acceptance and release. Later descendant Main CI, Nightly and Release checks passed; the feature ledger preserves its original phase-time hosted fields. |
| 10 | Aligned literal ECM returns identical source sectors on forward, repeated, reverse and random reads. | ✓ VERIFIED | `tests/disc/ecm.c` compares decoded bytes to source across access orders; `host/disc_ecm.c` resolves reads through indexed checkpoints. Full CI passed. |
| 11 | Each ECM open validates encoded extent, decoded count, terminator and output EDC. | ✓ VERIFIED | `schway__disc_ecm_validate` checks all four before `schway__disc_ecm_open` returns a reader; malformed EDC, terminator and truncation cases are registered. |
| 12 | ECM reader memory and random-read work are bounded independently of total image size. | ✓ VERIFIED | 1 GiB source/output limits, bounded checkpoint allocation and read-forward limits are enforced in `host/disc_ecm.c`; metric assertions are in `tests/disc/ecm.c` and `tests/fuzz/ecm.c`. |
| 13 | A fixture-proven Version 3 single-session CCD returns exact first/last IMG sectors and TOC. | ✓ VERIFIED | `host/disc_ccd.c` validates version/session/TOC and IMG extent; `tests/disc/ccd.c` and legacy parity checks compare boundary sectors and projected TOC. |
| 14 | Malformed and unsupported CCD metadata fails closed without inferred geometry. | ✓ VERIFIED | Metadata parser and validator reject unsupported/conflicting fields and require explicit proven indexes/entries; CCD regression cases cover invalid variants. |
| 15 | Supported ECM records reconstruct the exact original stream independently of sector boundaries. | ✓ VERIFIED | `host/disc_ecm.c` maps byte-literal and modeled records into output; `tests/disc/ecm.c` and source-parity tests compare bytes across mixed record boundaries. |
| 16 | ECM types 2/3 do not invent the omitted 16-byte raw header. | ✓ VERIFIED | Expansion code handles only proven stored fields; ECM fixture and negative cases compare exact source representation. |
| 17 | ECM-backed CUE geometry and bare-image fallback are explicit and exact. | ✓ VERIFIED | `host/disc_image.c` maps ECM CUE sources through validated sector counts and permits only aligned single-track fallback; CUE and direct-image tests cover both paths. |
| 18 | Supported CCD tracks preserve explicit indexes, pregaps and first/last extents without invented geometry. | ✓ VERIFIED | CCD TOC projection uses validated INDEX 0/1 and PLBA values; tests assert indexes, pregaps, leadout and boundary sectors. |
| 19 | Channel-major CloneCD SUB converts to callback bit-interleaved P-W exactly. | ✓ VERIFIED | The reader's transpose loop is checked by an independent known-answer vector in `tests/disc/ccd.c`, plus generated-fixture parity. |
| 20 | Missing/empty SUB follows no-subcode behavior; partial or excess SUB fails. | ✓ VERIFIED | Open validates an optional SUB against `96 * sector_count`; read zero-fills when absent. CCD tests cover absent, empty, short and oversized cases. |
| 21 | Owned ECM/CCD integration fixtures are deterministic derivatives of the same pinned/generated uncompressed sources. | ✓ VERIFIED | `tools/gen_ecm.py`, `tools/gen_ccd.py`, fixture manifests and `cmake/LegacyDiscFixtures.cmake` define deterministic generation and recorded source hashes/counts. |
| 22 | Exact source bytes, decoded sector counts and requested/produced counts are checked before guest runs. | ✓ VERIFIED | `tests/disc/legacy.c`, `tests/disc/tests.cmake` and `tests/runner/tests.cmake` check extents and counts before guest selectors; full workflow passed. |
| 23 | Clean build graphs declare each generated output and consumer dependency. | ✓ VERIFIED | `cmake/LegacyDiscFixtures.cmake` declares generated fixture outputs/byproducts and target dependencies; CMake configure/build checks passed. |
| 24 | Runner load/probe accepts supported ECM and CCD paths through the shared reader. | ✓ VERIFIED | `runner/probe.c` recognizes both suffixes; runner load/probe routes to `schway_disc_image_open`; runner legacy tests cover direct, CUE/M3U and malformed inputs. |
| 25 | Equivalent source media preserve game/save identity and guest output. | ✓ VERIFIED | `tests/disc/legacy.c` compares source and provider identity; runner guest checks compare output from original, ECM and CCD paths. |
| 26 | Malformed direct/nested media fails clearly before guest work. | ✓ VERIFIED | Shared open/probe errors propagate before guest creation; runner tests cover malformed direct and nested inputs. |
| 27 | Libretro loads and replaces ECM/CCD via the same reader and disc identity. | ✓ VERIFIED | `libretro/libretro.c` and disk-control paths use `schway_disc_image_open`; `tests/libretro/host.c` covers legacy content and replacement. |
| 28 | Failed libretro replacements retain the active disc and callbacks. | ✓ VERIFIED | The replacement path opens and validates a candidate before swapping active state; host integration regression checks exercise failure and retained callbacks. |
| 29 | Advertised extensions are tied to registered host evidence. | ✓ VERIFIED | `.info` claims checker maps true claims to inventory tests; `info_claims_self_test.cmake` rejects missing CloneCD description and unsupported claims. Focused selector passed 2/2 after the fix. |
| 30 | Malformed ECM/CCD corpora exercise production parser/read/close behavior on portable replay lanes. | ✓ VERIFIED | `tests/fuzz/ecm.c`, `tests/fuzz/ccd.c`, replay registrations and portable presets target the production image API; registered CI workflow passed. Campaign execution remains unknown. |
| 31 | Replay verifies bounded work, companion confinement and failure-atomic cleanup. | ✓ VERIFIED | ECM/CCD replay code checks read metrics, scoped companion paths, first/last output and baseline preservation after malformed candidates; replay tests are part of the registered checks. |
| 32 | Sanitizer campaigns use the same corpus and validate seed closure before timing. | ✓ VERIFIED | Nightly workflow verifies fixture/replay closure and runs regression replay before bounded campaigns; `tests/cmake/workflows.cmake` checks this configured ordering. No hosted campaign is claimed. |
| 33 | Owned boot/read/audio/FMV cases reach ECM/CCD callbacks with existing outputs and bounds. | ✓ VERIFIED | `tests/disc/tests.cmake` registers provider boot/read/audio/FMV fixture cases; source counts and expected guest results are checked by the fixture harness; full CI passed. |
| 34 | Both feature rows report only observed source/sector/TOC/subcode/host evidence. | ✓ VERIFIED | ECM and CCD rows in `tests/compat/features.csv` identify observed evidence and do not convert hosted unknowns into passes. |
| 35 | Source/count preflights run before long guest cases. | ✓ VERIFIED | Disc and runner CMake preflights validate fixture source sizes/counts before guest tests; full CI passed. |
| 36 | Nightly validates fixture/seed closure before timed campaigns and preserves routes/workflows. | ✓ VERIFIED | Workflow steps and `tests/cmake/workflows.cmake` enforce closure/replay-before-campaign command wiring and conservative route conditions. Hosted Nightly remains unknown. |
| 37 | CCD IMG/SUB resolve by same-stem convention inside the canonical descriptor directory. | ✓ VERIFIED | `schway__disc_ccd_paths` derives same-stem names; canonical path checks enforce containment before opening. |
| 38 | Missing/empty SUB uses no-subcode behavior; unsupported non-empty SUB is rejected. | ✓ VERIFIED | Reader accepts absent/empty SUB and checks exact non-empty size before use; regression cases cover each condition. |
| 39 | Companion escapes fail before outside bytes open, and failed replacement preserves the active image. | ✓ VERIFIED | CCD open resolves/confines companions before `fopen`; CCD fuzz/regression tests cover symlink escapes and active-image preservation. |
| 40 | CHD-OFF shipping products preserve ECM/CCD behavior and the existing package/notice contract. | ✓ VERIFIED | Local CHD-OFF package/notice checks are recorded in Phase 11 evidence; full changed-head workflow passed its release checks. Hosted release checks remain unknown. |
| 41 | Feature evidence separates local package results from Main/Nightly/Release results. | ✓ VERIFIED | Both provider rows distinguish local archive/notice evidence and explicitly leave hosted workflow fields unknown. |
| 42 | Counted-line cap and existing repository checks remain intact. | ✓ VERIFIED | The declared 3,901-line inventory passed at exactly 3,901; full workflow passed all 670 CI checks. |
| 43 | Optional installed-library SDL2 example compiles with ECM/CCD providers and is not launched, tested or packaged. | ✓ VERIFIED | `examples/sdl2/CMakeLists.txt` includes both private providers; the recorded staged-package compile succeeded and the example was not run or packaged, as required. |
| 44 | CHD-ON runner/libretro/provider targets build and focused provider, extension and installed-consumer checks pass. | ✓ VERIFIED | CHD-ON target and focused selector evidence is recorded for Phase 11; the changed-head workflow passed. |
| 45 | Feature evidence distinguishes local CHD-ON/example results from hosted results. | ✓ VERIFIED | `tests/compat/features.csv` records local build/example evidence while hosted Main/Nightly/Release values remain unknown. |

**Score:** 45/45 truths verified (0 present, behavior-unverified).

## Required Artifacts

All declared plan artifacts passed the artifact verifier across the 15 plans. Source review confirms substantive implementations and test registrations.

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `host/disc_ecm.c`, `host/disc_ecm.h` | Bounded ECM validation and read callbacks | ✓ VERIFIED | Full-stream validation, EDC/count/terminator checks, checked limits and sparse checkpoint reads. |
| `host/disc_ccd.c`, `host/disc_ccd.h` | Fixture-proven CCD metadata, companion confinement and callbacks | ✓ VERIFIED | Explicit TOC mapping, canonical same-stem paths, exact extents and SUB transpose. |
| `host/disc_image.c`, `host/disc_probe.c` | Shared provider dispatch and probe | ✓ VERIFIED | ECM/CCD dispatch through the existing disc contract and probe path. |
| Runner and libretro integrations | Consumer load/probe/replacement and metadata | ✓ VERIFIED | Shared reader is used by runner and libretro; `.info` descriptions and claims are checked. |
| Fixture, legacy, fuzz and CMake test files | Deterministic evidence and hostile-input regression | ✓ VERIFIED | Exact inventories and production-reader tests are registered in CI and Nightly workflow checks. |
| Feature ledger and docs | Honest compatibility and documented format subset | ✓ VERIFIED | Rows show local evidence and retain hosted unknowns; docs describe supported bounds and evidence. |
| CHD-OFF/ON package and optional SDL2 example wiring | Preserve product consumers and notices | ✓ VERIFIED | Local evidence is recorded; example is compiled only, not launched/tested/packaged. |

## Key Link Verification

The generic key-link query could not resolve plan links whose `from:`/`to:` values are descriptive labels rather than file paths, so those tool negatives are not treated as product failures. I manually traced the actual connections:

| From | To | Via | Status | Details |
|---|---|---|---|---|
| ECM/CCD private open APIs | shared image object and TOC | `host/disc_image.c` dispatch and callback projection | ✓ WIRED | Both providers populate the existing callback contract and are closed on failed open. |
| ECM/CCD shared image API | runner | `runner/probe.c` and `runner/main.c` | ✓ WIRED | Direct load/probe supports provider suffixes; integration tests compare identity and guest results. |
| ECM/CCD shared image API | libretro | `libretro/libretro.c` load and disk replacement | ✓ WIRED | Content and disk-control paths use the same reader and preserve active state when candidates fail. |
| provider fixtures | guest and feature evidence | fixture preflight, runner/libretro tests, `tests/compat/features.csv` | ✓ WIRED | Exact source counts precede guest tests; the ledger records provider-specific local evidence. |
| provider corpus | Nightly sanitizer route | `tests/fuzz/*`, `.github/workflows/nightly.yml`, workflow test | ✓ WIRED | Fixture closure/replay checks precede timed campaigns in the configured workflow. |

## Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| ECM reader | decoded sector bytes | validated encoded ECM file and checkpointed record decoder | Yes | ✓ FLOWING |
| CCD reader | sector bytes | confined, exact-length same-stem IMG file | Yes | ✓ FLOWING |
| CCD reader | callback subcode | confined, exact-length SUB file transposed from channel-major groups | Yes | ✓ FLOWING |
| Feature ledger | provider evidence | checked-in fixture and consumer check results | Yes; hosted evidence remains explicitly unknown | ✓ FLOWING |

## Behavioral Spot-Checks

At the prior verification point, the source was commit `32fc115`; that report's full local workflow preceded the POSIX feature-test macro and used targeted checks for the final change. The final ship follow-up below reran the full workflow on the repaired source.

| Behavior/check | Command | Result | Status |
|---|---|---|---|
| Full CI workflow, after Nightly target fix and before POSIX feature macro | `cmake --workflow --preset ci` | Exit 0; 670/670 in 82.38s; separate Release checks 7/7 in 1.44s | ✓ PASS (not final source head) |
| Final CCD test source | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^disc[.]image[.]ccd$'` | Exit 0; CCD plus legacy preflight/parity 3/3 | ✓ PASS |
| Strict-C17 Linux CCD test compile | GCC 15.3 in the named auto-removed `schwaystation-phase11-gcc15-ccd-check` container, using hosted warning/error flags | Exit 0; `tests/disc/ccd.c` compiled | ✓ PASS |
| Nightly fuzz closure | Fresh task-scoped fuzz configure/build of `schwaystation-test-disc-legacy` and ECM/CCD replay targets; exact four closure CTests | Build exit 0; CTests 4/4 passed | ✓ PASS |
| Workflow routing preflight | `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | Exit 0; now requires the legacy test executable before closure CTests | ✓ PASS |
| Repository-check budget | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | Exit 0 | ✓ PASS |
| Changed-head workflow/diff preflight | `git diff --check`; `ctest --test-dir build/ci --no-tests=error -R '^docs[.]budgets$'` | Exit 0; README and libretro docs retain the limited-compatibility caveat | ✓ PASS |
| Nightly route dry-run | `act schedule --dryrun` with the fuzz job selected | Exit 0; route included fixture build, closure and campaign steps; mapped to Ubuntu 24.04/arm64, dry-run only | ℹ️ Routing evidence only |
| Final released descendant Main CI | [Run 45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354) | Success on `f08fc08d40a1ebcb42708e0a31be3111e3eed711`; Phase 11 code and registered tests are included | ✓ PASS |
| Nightly and Release workflows | [Nightly 45442543657](https://github.com/szTheory/schwaystation/actions/runs/45442543657); [Release 45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400) | Both succeeded; Nightly ran on `75beb849f4b9e8a9a3bd023cc5c80632e4c74d56`, and Release on `f08fc08d40a1ebcb42708e0a31be3111e3eed711` | ✓ PASS |

### Final local Phase 11 ship follow-up (2026-11-22)

The reviewed follow-up changes passed the final local workflow. The complete run and the focused regressions are recorded here; the hosted PR runs shown below predate these changes and cannot count as post-fix evidence.

| Behavior/check | Command | Result | Status |
|---|---|---|---|
| Full CI workflow on the repaired worktree | `cmake --workflow --preset ci` | Exit 0; 671/671 tests in 111.59s, package step completed, Release checks 7/7 in 1.91s | ✓ PASS |
| CHD replay and seed corpus | `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^fuzz[.]regress[.]chd($|[.]seed_corpus$)'` | Exit 0; fixture setup and both replay tests passed 5/5; 12 corpus files, `empty.exe` is 0 bytes | ✓ PASS |
| CHD-OFF corpus writer | `cmake --preset library`; build `schwaystation-fuzz-chd-replay`; CTest `fuzz.regress.chd.seed_corpus` | Exit 0; CHD-off replay target built and the zero-byte seed corpus test passed 1/1 without the CHD fixture | ✓ PASS |
| Legacy runner timeout under full-suite load | CTest's `runner.disc.legacy` in `cmake --workflow --preset ci` | Passed in 29.05s with the new 60s limit; isolated focused run passed in 19.98s | ✓ PASS |
| Workflow, inventory and budget preflight | `LC_ALL=C sort -cu tests/fuzz/inventory.txt`; `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake`; line-budget command | Exit 0; CTest inventory sorted, CI/Nightly wiring matches, 3,901/3,901 lines | ✓ PASS |
| Nightly route dry-run after the selector update | `act --action-cache-path /private/tmp/schwaystation-phase11-act-actions-v2 --cache-server-path /private/tmp/schwaystation-phase11-act-server-v2 --no-cache-server --platform ubuntu-26.04=catthehacker/ubuntu:act-24.04 --container-architecture linux/arm64 schedule --dryrun --workflows .github/workflows/nightly.yml --job fuzz` | Exit 0; route reached source preflight, fixture setup, closure and campaign step; dry-run only | ℹ️ Routing evidence only |

The CI efficiency correction measures `schwaystation-run` before building the complete WASI portable-test target set. A failed size bound therefore avoids unrelated test-target compilation; when the bound passes, the complete build and portable suite still run. The prior failed run had built 331 Ninja actions before discovering the stale size bound, so this change moves the comparison earlier without removing coverage. The 906,778-byte module measurement came from the exact hosted producer job in [run 45073362654](https://github.com/szTheory/schwaystation/actions/runs/45073362654/job/137365283519).

### Test Quality Audit

The requirement-linked ECM/CCD tests have no disabled cases. Their value assertions compare decoded sectors and reader output against independently generated raw inputs; the SUB path also has a literal known-answer transpose vector. The fixture generators write deterministic source-derived media and do not invoke the production readers. Final descendant Main CI and Nightly runs passed the registered test and replay lanes.

## Probe Execution

No phase-declared `probe-*.sh` was found in the Phase 11 plans/summaries. No separate probe was required beyond the registered CTest/fuzz replay checks.

## Requirements Coverage

| Requirement | Source | Description | Status | Evidence |
|---|---|---|---|---|
| ECM-01 | Phase 11 plans 03, 05, 07, 08, 10, 11 | Bounded ECM decoding, preserved sectors and pre-guest source/count checks | ✓ SATISFIED | ECM source validation/read tests, source parity, runner/libretro integration, Nightly and released-descendant Main CI passed. |
| CCD-01 | Phase 11 plans 04, 06, 07, 08, 10, 11, 13 | CCD/IMG/SUB subset, exact TOC/subcode/boundary behavior, confined companion paths | ✓ SATISFIED | CCD metadata, path, SUB and integration tests, Nightly and released-descendant Main CI passed. |

No Phase 11 requirement is orphaned. ECM-01 and CCD-01 are marked complete after local implementation evidence and verification; later descendant Main, Nightly and Release runs are green.

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `docs/maintaining.md` | 73 | “TODO” substring in the path of a completed artifact-reuse review | ℹ️ Info | Not a debt marker or unfinished implementation. |
| `docs/testing.md` | 75 | “placeholder” describing test registration behavior | ℹ️ Info | Explanatory documentation, not a product stub. |
| `README.md`, `docs/libretro.md` | — | Current user-facing format list and scope caveat | ✓ VERIFIED | The edits list ECM/CloneCD, supported PBP, and CHD when enabled, while retaining the limited-compatibility caveat. Parent-reported `git diff --check` and `docs.budgets` checks passed. |

No unreferenced `TBD`, `FIXME` or `XXX` markers were found in changed implementation files. No blocker anti-pattern was found.

## Hosted Evidence

The original Phase 11 PR attempts at pre-fix head [`45070735988`](https://github.com/szTheory/schwaystation/actions/runs/45070735988) and Nightly [`45070735998`](https://github.com/szTheory/schwaystation/actions/runs/45070735998) exposed the strict-C17 `symlink()` declaration and missing legacy-test target. Those defects were fixed and reviewed. The current Phase 11 implementation is contained in later released descendants: Nightly [45442543657](https://github.com/szTheory/schwaystation/actions/runs/45442543657) succeeded on `75beb849f4b9e8a9a3bd023cc5c80632e4c74d56`; post-merge Main CI [45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354) and Release [45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400) succeeded on `f08fc08d40a1ebcb42708e0a31be3111e3eed711`. Both descendants include the ECM/CCD providers, fixture generators, consumers and registered tests.

The feature ledger keeps its hosted fields as `unknown` because they describe the recorded Phase 11 provider-specific evidence; the later green workflow runs are recorded here without rewriting that historical row. No owner hardware or manual UAT is required.

## Gaps Summary

The Phase 11 code goal remains achieved and all 45 deduplicated roadmap/plan truths are verified. The stale fingerprint was refreshed after checking the current consumers and CI wiring; the original two CI defects remain fixed and covered by regressions. No current implementation gap or human-verification item remains, and the final released descendant Main, Nightly and Release checks are green.

---

_Verified: 2026-11-25T02:03:04Z_
_Verifier: the agent (gsd-verifier)_
