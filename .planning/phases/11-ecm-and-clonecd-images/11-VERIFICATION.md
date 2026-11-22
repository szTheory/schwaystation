---
phase: 11-ecm-and-clonecd-images
verified: 2026-11-22T20:52:25Z
status: passed
score: 45/45 must-haves verified
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
  - .planning/phases/11-ecm-and-clonecd-images/deferred-items.md
  - CMakePresets.json
  - cmake/DiscFixtures.cmake
  - cmake/DiscTargets.cmake
  - cmake/LegacyDiscFixtures.cmake
  - docs/compatibility.md
  - docs/discs.md
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
covered_digest: "v4:sha256:7b0baf4081d2f170e1c2b00109312dedc57d092d918f0cb5c6f3438f192a6ef7"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 11: ECM and CloneCD Images Verification Report

**Phase Goal:** Players can open ECM and CloneCD images while retaining their documented track, sector and subcode behavior.
**Verified:** 2026-11-22T20:52:25Z
**Status:** passed
**Re-verification:** No; no prior verification report existed.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Supported ECM files produce the expected sector bytes on repeated and out-of-order reads; source extents and sector counts are checked before guest runs, and malformed tables, offsets and truncated streams fail within documented bounds. | ✓ VERIFIED | `host/disc_ecm.c` validates full encoded extent, decoded length, terminator and EDC before publishing; bounded checkpoint reads are exercised by the ECM tests and the full CI run. `tests/disc/legacy.c` and runner preflights compare exact source and requested/produced counts before guests. |
| 2 | Supported CloneCD fixtures preserve modes, indexes, pregaps, subcode and first and last sector reads through CCD and IMG/SUB companions; invalid companion paths are rejected before an external file is opened. | ✓ VERIFIED | `host/disc_ccd.c` reconciles metadata and exact image/subcode sizes, canonicalizes and confines companion paths before opening, and transposes SUB data. `tests/disc/ccd.c` and `tests/fuzz/ccd.c` exercise boundary sectors, metadata, known-answer transpose, escapes and replacement behavior. |
| 3 | The ledger records fixture results for both providers against the existing disc contract. | ✓ VERIFIED | `tests/compat/features.csv` has ECM and CCD pass rows for the observed local provider, guest, package and consumer evidence; Main, Nightly, Release and fuzz hosted/campaign evidence stays explicitly `unknown`. |
| 4 | Malformed ECM and CCD inputs fail through private host APIs before a reader is published. | ✓ VERIFIED | Private `schway__disc_ecm_open`/`schway__disc_ccd_open` validation paths and malformed regression cases in `tests/disc/ecm.c` and `tests/disc/ccd.c`; full workflow passed. |
| 5 | CMake and exact test inventories include both private readers without changing the core ABI. | ✓ VERIFIED | `cmake/DiscTargets.cmake`, `tests/disc/tests.cmake`, `tests/disc/inventory.txt`, fuzz/runner/libretro registrations and private host headers; no public ABI change is present in the changed implementation. |
| 6 | Existing gates remain within the fixed 3,901-line ceiling. | ✓ VERIFIED | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` passed at 3,901/3,901. |
| 7 | The measured metadata-only Nightly shortcut is explicitly deferred. | ✓ VERIFIED | Phase context D-12 and `.planning/todos/pending/051-nightly-pr-job-routing.md` record the measured line-budget reason and retain full Nightly routing. |
| 8 | Full Nightly fallback, existing cases and required lanes remain in effect. | ✓ VERIFIED | `.github/workflows/nightly.yml` retains the full route; `tests/cmake/workflows.cmake` checks route and campaign wiring. Actual hosted Nightly execution remains unknown. |
| 9 | ECM-01 and CCD-01 stay open until phase product evidence is complete. | ✓ VERIFIED | At verification time, both requirement rows were unchecked while local and hosted evidence were separated. The subsequent Phase 11 completion transition marked both complete from the local acceptance evidence; hosted Main/Nightly/Release gates remain unknown until ship. |
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

No new commands were run for this verification. The following exact-head results were supplied from the completed Phase 11 run and checked against current source history; no implementation source changed after `961964a` (current HEAD `44ba420` adds planning-only CI-efficiency seed documentation).

| Behavior/check | Command | Result | Status |
|---|---|---|---|
| Full exact-code-head CI workflow | `cmake --workflow --preset ci` at `961964a` | Exit 0; 670/670 CI tests in 105.02s and 7/7 release checks in 1.46s | ✓ PASS |
| Repository-check budget | `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | Exit 0; 3,901/3,901 lines | ✓ PASS |
| CI configuration | `cmake --preset ci` | Exit 0 | ✓ PASS |
| Changed libretro metadata guard and negative self-test | `ctest --test-dir build/ci --output-on-failure -R '^libretro[.]info[.]claims([.]self_test)?$'` | Exit 0; 2/2 passed | ✓ PASS |
| Local Nightly route simulation | `act schedule --dryrun` from the earlier Phase 12 routing task | Passed as recorded; it is not a hosted campaign and was not rerun at the final code head | ℹ️ Informational only |

## Probe Execution

No phase-declared `probe-*.sh` was found in the Phase 11 plans/summaries. No separate probe was required beyond the registered CTest/fuzz replay checks.

## Requirements Coverage

| Requirement | Source | Description | Status | Evidence |
|---|---|---|---|---|
| ECM-01 | Phase 11 plans 03, 05, 07, 08, 10, 11 | Bounded ECM decoding, preserved sectors and pre-guest source/count checks | ✓ SATISFIED | ECM source validation/read tests, source parity, runner/libretro integration and full CI pass. Hosted gates are a separate ship-time check. |
| CCD-01 | Phase 11 plans 04, 06, 07, 08, 10, 11, 13 | CCD/IMG/SUB subset, exact TOC/subcode/boundary behavior, confined companion paths | ✓ SATISFIED | CCD metadata, path, SUB and integration tests; full CI pass. Hosted gates are a separate ship-time check. |

No Phase 11 requirement is orphaned. The verification was recorded before the GSD completion transition, which then marked ECM-01 and CCD-01 complete. Their hosted Main/Nightly/Release evidence remains pending the ship workflow.

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| `docs/maintaining.md` | 73 | “TODO” substring in the path of a completed artifact-reuse review | ℹ️ Info | Not a debt marker or unfinished implementation. |
| `docs/testing.md` | 75 | “placeholder” describing test registration behavior | ℹ️ Info | Explanatory documentation, not a product stub. |

No unreferenced `TBD`, `FIXME` or `XXX` markers were found in changed implementation files. No blocker anti-pattern was found.

## Hosted Evidence and Next Action

Hosted Main, Nightly and Release results are **unknown**. The exact changed-head local workflow and local release checks passed, but they do not establish GitHub runner, artifact, permission or integration behavior. The feature ledger preserves those fields as unknown. No actual `act` container/campaign was run for this final code head; the earlier `act schedule --dryrun` only established routing syntax.

The next action is `$gsd-ship 11`, then confirm the PR's required Main/Nightly/Release checks and automatic release on the merged head. Do not mark hosted fields complete until those runs exist and are green. No owner hardware or manual UAT is required.

## Gaps Summary

The Phase 11 code goal is achieved and all 45 deduplicated roadmap/plan truths are verified. No implementation gaps or human-verification items remain. Hosted checks are intentionally pending the release flow; there is no hosted run URL to report yet.

---

_Verified: 2026-11-22T20:52:25Z_
_Verifier: the agent (gsd-verifier)_
