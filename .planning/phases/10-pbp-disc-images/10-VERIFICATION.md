---
phase: 10-pbp-disc-images
verified: 2026-11-25T02:01:24Z
status: passed
score: 29/29 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .planning/phases/10-pbp-disc-images/10-01-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-01-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-02-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-02-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-03-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-03-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-04-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-04-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-05-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-05-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-06-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-06-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-07-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-07-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-08-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-08-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-09-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-09-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-10-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-10-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-11-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-11-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-12-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-12-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-13-PLAN.md
  - .planning/phases/10-pbp-disc-images/10-13-SUMMARY.md
  - .planning/phases/10-pbp-disc-images/10-CI-EFFICIENCY.md
  - .planning/phases/10-pbp-disc-images/10-PACKAGE-EVIDENCE.md
  - .planning/phases/10-pbp-disc-images/10-REVIEW.md
  - .planning/phases/10-pbp-disc-images/10-VERIFICATION-EVIDENCE.md
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
  - libretro/schwaystation_libretro.info.in
  - quality.json
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
  - tests/hygiene/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tools/gen_pbp.py
covered_digest: "v4:sha256:5edf52885f8360bee9560c5dc762567b9535fd820c881c0ed501bdfe3300b47f"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 12
  total: 12
  not_honored: []
---

# Phase 10: PBP Disc Images Verification Report

**Phase Goal:** Players can open documented unencrypted PlayStation PBP images and get a clear result for other variants.

**Verified:** 2026-11-25T02:01:24Z
**Status:** passed
**Re-verification:** No prior gaps frontmatter existed; this is a fresh goal-backward pass over the current released source.

## Current Source and Hosted Evidence

The released source is v0.14.0, commit f08fc08d40a1ebcb42708e0a31be3111e3eed711. The final test head was 75beb849f4b9e8a9a3bd023cc5c80632e4c74d56; comparing that head to the release commit shows only .release-please-manifest.json, CHANGELOG.md, and version.txt changed. The PBP implementation under review is therefore the code exercised by the final CI.

- Main CI passed on the tested source head: [run 45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076).
- Full Nightly, including the bounded fuzz campaigns, passed on that same head: [run 45442543657](https://github.com/szTheory/schwaystation/actions/runs/45442543657).
- Post-merge Main CI and Release passed on the released commit: [Main CI run 45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354), [Release run 45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400).

No suite was rerun for this report refresh.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | The runner and libretro host read the verified unencrypted PSISO layouts and track variants with matching sectors and deterministic hashes. The provider works with CHD disabled and without system zlib, and the bundled inflater has a permissive notice. | ✓ VERIFIED | The shared host provider dispatches .pbp to host/disc_pbp.c; the runner and libretro use that provider. PBP/CUE parity and host tests are registered in CTest. The standalone inflater is a private tinfl target, linked without system zlib, with its MIT notice. Final Main CI passed across supported build/hash lanes. |
| 2 | Bad PSISO offsets, track tables, sizes, and truncated blocks fail at the media boundary. Encrypted PGD and undocumented layouts return a clear unsupported result before guest execution. The ledger records exercised variants. | ✓ VERIFIED | host/disc_pbp_metadata.c checks PBP offsets, TOC records, track limits, block extents, and exact file end. Provider, runner, libretro, and guest tests cover malformed and unsupported inputs before guest output; the ledger records only the fixture-proven one-track case. Main CI and Nightly passed on the final source head. |
| 3 | Every original control-driver assertion passes and at least 35 counted lines of repeated driver code are removed. | ✓ VERIFIED | The quality and triage control drivers retain their assertions; Plan 10-01 records the measured reduction, and the exact released source passed Main CI. |
| 4 | The host-only provider builds with CHD disabled and has bounded rejection and cleanup behavior. | ✓ VERIFIED | cmake/DiscTargets.cmake registers the provider in the host image target. The CHD-OFF configure/build evidence and the final Main CI support the source configuration; open failure clears the output handle and closes resources. |
| 5 | Every registered provider skeleton case exercises the production boundary and the inventory remains exact and sorted. | ✓ VERIFIED | tests/disc/tests.cmake registers the PBP selectors against the production target, with tests/disc/inventory.txt listing them. The final Main CI passed. |
| 6 | The three admitted inflater units match verified hashes, and the shim/export header and private aliases build without the excluded header. | ✓ VERIFIED | 10-PACKAGE-EVIDENCE.md records the three source SHA-256 values and symbol audit; cmake/Miniz.cmake uses the project shim and private compile-time aliases. The package and vendored checks passed. |
| 7 | The host uses the independent verified inflater, notices identify admitted files, and digest coverage rejects both intended mutations. | ✓ VERIFIED | cmake/Miniz.cmake, PROVENANCE.md, and THIRD-PARTY-NOTICES.md show the standalone MIT closure. The existing vendored digest and notice controls are registered and passed in the Phase 10 package evidence. |
| 8 | Known raw streams decode through the actual private wrapper, and output/stream-bound failures leave caller output unchanged. | ✓ VERIFIED | host/pbp_inflate.c decodes into scratch storage and copies only after exact raw-stream completion and size checks. disc.image.pbp_inflate exercises the private wrapper; Main CI passed. |
| 9 | Both host link configurations pass exact-sized inflater controls without symbol coupling or a system zlib dependency. | ✓ VERIFIED | The CHD-ON/OFF package evidence records the private archive symbols and link closure; tests/disc/pbp.c registers the inflater controls. Both package configurations passed their focused checks. |
| 10 | The same owned source creates byte-identical PBP output twice, with a matching single-track CUE and bounded source/output manifest. | ✓ VERIFIED | tools/gen_pbp.py generates the owned PBP and source CUE, writes source/output digests and geometry, and uses deterministic fixed-Huffman encoding. disc.image.pbp_preflight and fixture controls are wired before consumers. |
| 11 | Fixture identity and complete geometry are checked before decode/guest consumers, and wrong-input controls fail for their intended faults. | ✓ VERIFIED | Fixture preflight validates source identity, sector/block counts, and outputs before dependent selectors. tests/disc/tests.cmake wires those prerequisites to parsing, parity, and guest cases; final Main CI passed. |
| 12 | The supported fixture maps exactly to the single-track source TOC; checked bounds reject malformed ranges before allocation or indexing. | ✓ VERIFIED | host/disc_pbp_metadata.c performs checked offset arithmetic, validates BCD/MSF and A0/A1/A2 records, and bounds block-table allocation. The metadata and bounds selectors passed in final Main CI. |
| 13 | Valid TOC mapping and malformed/unsupported categories pass; rejection precedes oversized allocation and never replaces an active reader. | ✓ VERIFIED | Metadata validation publishes its map only on success; provider tests cover offsets, records, extents, and failure-atomic output. The final Main CI passed the registered PBP selectors. |
| 14 | A successful PBP open validates every stream before guest use and serves shared callback sectors with bounded cache memory. | ✓ VERIFIED | schway__disc_pbp_open scans every block before returning a reader; reads use fixed-size buffers and one cached block. The open, boundary, and parity selectors passed. |
| 15 | Every source sector, normalized TOC, boundary, and identity matches; malformed compressed input is rejected before a reader is returned. | ✓ VERIFIED | disc.image.pbp_parity, pbp_boundaries, and pbp_open compare the PBP against the independently generated CUE source and exercise rejection. Main CI passed on the tested source head. |
| 16 | Every committed replay selector drives the shipping parser with checked run-local seeds and deterministic assertions. | ✓ VERIFIED | tests/fuzz/pbp.c links the shipping schwaystation_disc_image target; fuzz.regress.pbp is registered with the owned fixture preflight. Main CI and Nightly passed. |
| 17 | PBP replay and a bounded sanitizer campaign pass, while Nightly retains existing cases and adds the verified PBP campaign. | ✓ VERIFIED | The replay selector is in the fuzz inventory, and .github/workflows/nightly.yml builds the PBP fixture and validates seeds before campaigns. Full Nightly passed on the tested source head. |
| 18 | Runner load/probe reads supported PBP like its source and reports malformed/unsupported failures clearly. | ✓ VERIFIED | The runner routes through the shared image provider; runner.disc.pbp compares PBP/CUE probe and bounded guest output and asserts PGD, unknown, corrupt-table, and truncated-block results. Final Main CI passed. |
| 19 | User-facing PBP claims match provider and runner evidence; unsupported layouts are not advertised. | ✓ VERIFIED | docs/discs.md and docs/runner.md specify the single-data-track PSISO subset and explicit refusal cases. The generated libretro description also restricts claims to that subset. |
| 20 | The normal libretro path loads supported PBP and preserves active media/card state when replacement media is rejected. | ✓ VERIFIED | libretro/libretro.c uses the shared image reader; libretro.content.pbp asserts parity and preserved state after rejected inputs. The selector passed in final Main CI. |
| 21 | Both feature configurations advertise PBP with a gating host test and accurate linked-source license claims. | ✓ VERIFIED | CHD-ON/OFF .info generation and license metadata are covered by tests/cmake/info_claims.cmake; the registered host-content test exercises PBP. Package evidence confirms both configurations. |
| 22 | CHD-OFF packages contain correct notices and metadata and link independently without system zlib. | ✓ VERIFIED | 10-PACKAGE-EVIDENCE.md records exact archive/notices checks, private inflater symbols, and no -lz/runtime zlib dependency for CHD-OFF. |
| 23 | CHD-ON and CHD-OFF both load PBP, retain exact notices/metadata, and have no inflater symbol collision or runtime zlib dependency. | ✓ VERIFIED | Both configurations ran PBP parity, runner, libretro, metadata, archive, and notice selectors. The separately prefixed standalone inflater remains distinct from libchdr’s copy. |
| 24 | Existing CI routing assertions and narrow/fallback Nightly cases pass, with full selection when routing is uncertain. | ✓ VERIFIED | scripts/classify-changes.sh, tests/cmake/ci_route_controls.cmake, and the Nightly workflow implement and test exact PBP, docs-only, mixed, unknown, rename, and failure cases. |
| 25 | Act routing and fast controls pass; scheduled/manual workloads stay complete, fuzz remains relevant, and the repository-check budget fits. | ✓ VERIFIED | Phase evidence records successful CI/schedule Act dry-runs and classifier controls. The workflow still has full scheduled/manual paths, PBP-only routing retains fuzz, and the Main CI line-budget check passed. |
| 26 | The bounded guest predicates run only after full source parity; the original audio and two-track tests remain intact. | ✓ VERIFIED | tests/disc/tests.cmake requires fixture preflight and parity before PBP guest boot/read/FMV tests. The existing CUE/CHD two-track/audio test inventory remains present; Main CI passed. |
| 27 | The protected ledger retains prior CHD evidence and records only the exercised PBP subset with attributable tests. | ✓ VERIFIED | tests/compat/features.csv records the one-track NTSC-U PBP pass and its provider, runner, libretro, package, and guest selectors; its protected expected snapshot is separately authored and gated. |
| 28 | The frozen source review is clean and cheap consistency preflights pass. | ✓ VERIFIED | 10-REVIEW.md records a clean review after resolved findings; 10-VERIFICATION-EVIDENCE.md records sorted inventory, workflow closure, quality, line-budget, and diff checks passing. |
| 29 | The reviewed head has green local and hosted lane/hash/fuzz evidence, a measured WASI bound, and no unsupported format claim; it proceeds through formal verification and ship. | ✓ VERIFIED | Exact-head Main CI and Nightly passed on 75beb849; the hosted WASI check passed the recorded 947,969-byte bound on that head. Post-merge Main/Release passed, and v0.14.0 was published. |

**Score:** 29/29 must-have truths verified (0 present-but-behavior-unverified).

### Inversion Checks

- **Could offsets still wrap before validation?** The metadata parser uses checked 64-bit addition/multiplication for PBP sections, table entries, and block extents before use.
- **Could a corrupt replacement expose a partially initialized reader?** Both metadata and public reader outputs start empty and are published only on successful validation; the libretro integration test checks rejected replacement state.
- **Could generated fixture parity be circular?** The fixture generator writes a separate CUE/BIN source and validates its deterministic PBP bytes with an independent zlib decoder; it does not call the shipping PBP provider. Host tests compare decoded PBP sectors with that source.

## Required Artifacts

All 60 declared artifacts across the 13 plans passed the artifact query (existence and substantive-pattern checks); no artifact is missing or stubbed. Representative goal artifacts:

| Artifact | Expected | Status | Evidence |
|---|---|---|---|
| host/disc_pbp_metadata.c | Bounded PSISO directory, TOC, and block parsing | ✓ VERIFIED | Checked extents, media limits, track records, BCD/MSF, contiguous blocks, exact file end. |
| host/disc_pbp.c | Validated reader and sector callbacks | ✓ VERIFIED | Opens only after metadata validation, validates every stream before publishing a reader, fixed buffers and one-block cache. |
| host/pbp_inflate.c and cmake/Miniz.cmake | Bounded private raw inflater | ✓ VERIFIED | Exact input/output completion into scratch storage; private aliases, no system zlib link. |
| host/disc_image.c | Shared provider dispatch | ✓ VERIFIED | .pbp opens through the PBP provider and forwards TOC/sectors through the common image interface. |
| runner/probe.c and host/disc_probe.c | Runner load/probe path | ✓ VERIFIED | The runner consumes shared TOC and sector callbacks and reports invalid/unsupported formats before guest execution. |
| libretro/libretro.c | Libretro content loader | ✓ VERIFIED | Uses the shared media provider and retains active media/card state on rejected replacements. |
| tools/gen_pbp.py and cmake/PBPFixtures.cmake | Owned deterministic fixture producer | ✓ VERIFIED | Emits PBP, source CUE/BIN, and bounded manifest; fixture preflight guards downstream use. |
| tests/disc/pbp.c and tests/disc/tests.cmake | Provider/parity/boundary tests | ✓ VERIFIED | Registered PBP metadata, bounds, raw, inflate, open, boundary, and parity selectors. |
| tests/runner/tests.cmake and tests/libretro/host.c | Shipping host consumers | ✓ VERIFIED | Runner and libretro integration tests exercise the shipping provider and failure behavior. |
| PROVENANCE.md and THIRD-PARTY-NOTICES.md | Permissive inflater notice/provenance | ✓ VERIFIED | Pinned MIT source, exact source digests, and packaged notice evidence. |

## Key Link Verification

All 13 plans’ key-link queries passed.

| From | To | Via | Status |
|---|---|---|---|
| cmake/DiscTargets.cmake | host/disc_pbp.c | Host-only source registration | ✓ WIRED |
| cmake/Miniz.cmake | host/miniz/miniz_tinfl.c | Private inflater target | ✓ WIRED |
| host/pbp_inflate.c | cmake/Miniz.cmake | Wrapper consumes prefixed inflater | ✓ WIRED |
| cmake/DiscFixtures.cmake | cmake/PBPFixtures.cmake | Owned PBP fixture producer | ✓ WIRED |
| host/disc_pbp.c | host/disc_pbp_metadata.c | Reader consumes validated map | ✓ WIRED |
| host/disc_image.c | host/disc_pbp.c | .pbp shared-provider dispatch | ✓ WIRED |
| tests/fuzz/pbp.c | host/disc_image.c | Replay enters production parser | ✓ WIRED |
| runner/main.c | host/disc_image.c | Runner uses shared disc provider | ✓ WIRED |
| libretro/libretro.c | host/disc_image.c | Libretro uses shared provider | ✓ WIRED |
| cmake/packaging.cmake | THIRD-PARTY-NOTICES.md | Archive notice check | ✓ WIRED |
| .github/workflows/nightly.yml | scripts/classify-changes.sh | Conservative Nightly routing | ✓ WIRED |
| tests/disc/fixture.c | host/disc_image.c | Bounded PBP guest consumes callbacks | ✓ WIRED |
| .github/workflows/ci.yml | tests/inventory/ci.txt | Hosted CI includes required inventories | ✓ WIRED |

## Data-Flow Trace

| Artifact | Data | Source | Real data reaches consumer | Status |
|---|---|---|---|---|
| host/disc_pbp_metadata.c → host/disc_pbp.c | Offsets, TOC, and block map | Selected PBP bytes | Validated map controls bounded block reads and sector output | ✓ FLOWING |
| Shared provider → runner/libretro | Sector bytes and TOC | Decoded raw or DEFLATE PBP blocks | Both hosts consume the real provider callbacks | ✓ FLOWING |
| PBP parity tests | Sector bytes and TOC | Independently generated source CUE/BIN | All selected PBP sectors are compared against source media | ✓ FLOWING |

## Behavioral Spot-Checks

No tests were rerun for this refresh. The listed exact-head and release runs exercise the current implementation; the release commit changes only generated release metadata from the tested source head.

| Behavior | Check | Result | Status |
|---|---|---|---|
| PBP parse, sector parity, deterministic host behavior and cross-platform hashes | cmake --workflow --preset ci in Main CI on 75beb849 | Passed; [run 45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076) | ✓ PASS |
| Malformed/unsupported rejection before runner guest output | Registered runner.disc.pbp and PBP provider selectors in the same Main CI | Passed on tested source head | ✓ PASS |
| Libretro PBP loading and rejected-replacement state preservation | Registered libretro.content.pbp in the same Main CI | Passed on tested source head | ✓ PASS |
| PBP mutation replay and bounded sanitizer fuzz route | Full Nightly on the same source head | Passed; [run 45442543657](https://github.com/szTheory/schwaystation/actions/runs/45442543657) | ✓ PASS |
| Shipped package/release integrity | Post-merge Main CI and Release on f08fc08 | Both passed; [Main CI](https://github.com/szTheory/schwaystation/actions/runs/45455115354), [Release](https://github.com/szTheory/schwaystation/actions/runs/45455115400) | ✓ PASS |

The phase declares no shell probe. The user-visible runner --probe behavior is covered by runner.disc.pbp.

## Test Quality Audit

| Test File | Linked Requirement | Active | Circular | Strongest Assertion | Verdict |
|---|---|---|---|---|---|
| tests/disc/pbp.c, tests/disc/image.c | PBP-01 | Yes | No | Exact bytes, TOC, status, bounds, full sector parity | VALID |
| tests/runner/tests.cmake | PBP-01 | Yes | No | Exit/status, clear diagnostic, no guest frame on refusal | VALID |
| tests/libretro/host.c | PBP-01 | Yes | No | Content parity, state preservation, frame/audio/RAM comparisons | VALID |
| tests/compat/tests.cmake, tests/compat/features.csv | PBP-01 | Yes | No | Protected evidence snapshot and feature-key expectations | VALID |
| tests/fuzz/pbp.c | PBP-01 | Yes | No | Production parser mutation replay; full Nightly fuzz passes | VALID |

No disabled or skipped requirement test was found. The PBP generator does not invoke the parser under test; source-media parity uses a separate CUE/BIN fixture.

## Requirements Coverage

| Requirement | Source | Description | Status | Evidence |
|---|---|---|---|---|
| PBP-01 | Phase 10 plans 01–13 | Bounded fixture-proven unencrypted PSISO support, deterministic sector output, clear PGD/unknown rejection, permissive in-tree inflater without system zlib | ✓ SATISFIED | Provider, runner, libretro, guest, package, notice, parity, and fuzz checks passed on the tested source head; released as v0.14.0. |

No orphaned requirements are mapped to Phase 10. The decision-coverage query reports 12/12 context decisions honored.

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| tests/compat/features.csv | 39 | hosted_ci.not_run | ⚠️ WARNING | The PBP row correctly records the exercised one-track subset and its tests, but this hosted-evidence token is stale: final Main CI and Nightly passed afterward. It does not invalidate the tested feature behavior or block the phase goal. |
| .planning/todos/pending/051-nightly-pr-job-routing.md | 34 | Pending hosted-run wording | ⚠️ WARNING | It still says to keep the todo pending until an exact-head hosted run, although the Phase 10 Main CI/Nightly runs and timing were subsequently recorded. Planning bookkeeping is stale, not a missing implementation behavior. |

No TBD, FIXME, or XXX debt marker was found in the phase implementation files.

## Human Verification Required

None. The CLI/refusal behavior and host integrations are automated; no visual or owner-hardware acceptance is part of PBP-01.

## Gaps Summary

No substantive implementation or roadmap-criterion gap was found. The two warnings are stale evidence bookkeeping: the feature ledger still says hosted CI was not run, and TODO 051 still requests the now-recorded exact-head hosted run. The Phase 10 Main CI and Nightly evidence is green on the PBP source; the release and post-merge gates are green on v0.14.0. No plan or source changes are required to establish the Phase 10 goal.

---

_Verified: 2026-11-25T02:01:24Z_
_Verifier: the agent (gsd-verifier)_
