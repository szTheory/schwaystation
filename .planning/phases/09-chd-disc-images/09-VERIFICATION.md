---
phase: 09-chd-disc-images
verified: 2026-11-25T01:58:43Z
status: passed
score: 50/50 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .github/workflows/release.yml
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
  - docs/testing.md
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
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/release/inventory.txt
  - tests/release/tests.cmake
  - tests/runner/disc.c
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tools/chd-tool/main.cpp
covered_digest: "v4:sha256:7798a9bf7581766e655dd3cffe7604aae1edc7b7cef3e379afc28737b2f38610"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 9
  total: 9
  not_honored: []
---

# Phase 9: CHD disc images Verification Report

**Phase Goal:** Players can open the supported CHD subset with the same behavior as the corresponding uncompressed discs.
**Verified:** 2026-11-25T01:58:43Z
**Status:** passed
**Re-verification:** No — the earlier report had no gaps block, so this ran in initial mode.

## Goal Achievement

### Observable Truths

Roadmap success criteria and each plan's `must_haves.truths` were checked against current code, tests, and the current release evidence. There are four roadmap truths and 46 plan truths; all 50 behavior and implementation truths are verified. The Plan 09-13 metadata behavior is implemented inline in the release test script rather than in the separate source-notices helper named by its plan link; the package behavior is present and verified, so this path difference is recorded without a product gap.

| # | Truth | Status | Evidence |
| --- | --- | --- | --- |
| 1 | The documented CHD subset opens through the common image provider and matches CUE identity, TOC, sectors, applicable subcode and hashes across the applicable lanes; CHD remains opt-in. | ✓ VERIFIED | `docs/discs.md` documents v3–v5 track metadata and explicit v1–v2 exclusions. `host/disc_chd.c` preflights before `chd_read`; `tests/disc/media.c` compares all 222 sectors in both orders, TOC, subcode and content identity. `SCHWAYSTATION_CHD` defaults OFF in `CMakeLists.txt`. Final Main CI and current-release post-merge Main passed. |
| 2 | CHD ON/OFF builds advertise accurate `.info` metadata and carry the required notices from the pinned permissive reader closure. | ✓ VERIFIED | `libretro/CMakeLists.txt` derives CHD claims from the linked provider; `cmake/Libchdr.cmake`, `pins.txt`, `PROVENANCE.md` and `THIRD-PARTY-NOTICES.md` pin and document the closure. `release.notices` checks packaged metadata and `archive_contents.cmake` checks archive members and notice bytes. Current Release passed. |
| 3 | Shared media bounds, malformed-input regression/fuzz checks, fixture preflights and failed-open isolation protect the CHD path; later providers now apply the shared contract. | ✓ VERIFIED | `host/media_limits.h`, `host/disc_chd.c` and `host/disc_image.c` check sizes/extents before publication; tests cover boundary LBAs, malformed inputs and preserving the active image. Current Main and Nightly evidence covers replay and fuzz. Current `docs/discs.md` and provider tests cover the completed PBP/ECM/CloneCD follow-on paths. |
| 4 | The compatibility ledger contains demonstrated CHD evidence, rejects bad key sets and protects previous passes with dated allowances. | ✓ VERIFIED | `tests/compat/features.csv`, `scripts/compat-ratchet.sh`, `scripts/compat-doc.sh` and `tests/cmake/compat-ratchet.cmake` implement the ledger and protected ratchet; the current CI workflow passed. |
| 5–7 | Plan 09-01: the owned CUE preflight runs before guest tests; empty, single-track, touching-boundary and final-LBA cases are asserted; inventories remain tab-separated and bytewise sorted. | ✓ VERIFIED | `disc.media.preflight` is a `FIXTURES_SETUP` dependency for guest cases; `tests/disc/media.c` supplies the independent geometry assertions; CI checks sorted inventories. |
| 8–10 | Plan 09-02: ordinary builds default CHD OFF without fetching it; enabled builds use the pinned static C-only reader and bounded malformed-header checks; the core target stays free of CHD and file I/O. | ✓ VERIFIED | `CMakeLists.txt`, `cmake/DiscTargets.cmake` and `cmake/Libchdr.cmake` show host-only conditional linkage; `src/` has no CHD or host file API references; `tests/disc/chd.c` exercises malformed headers. |
| 11–13 | Plan 09-03: the fixture generator is admitted only after license closure checks; source bytes and build entry points are pin/digest checked; the generator and generated discs stay out of shipped archives and tracked fixtures. | ✓ VERIFIED | `cmake/CHDTool.cmake`, `cmake/CHDToolPins.cmake`, `pins.txt`, package inventories and archive checks implement these boundaries. |
| 14–16 | Plan 09-04: the owned CUE creates a CHD below 512 KiB without changing source coverage; repeated output is byte-identical; consumer lanes reject missing, corrupt or mismatched fixtures before guest execution. | ✓ VERIFIED | `cmake/CHDFixtures.cmake` checks the two source tracks, 222-sector count, size cap and repeat digest; fixture manifests gate the consumer tests. |
| 17–19 | Plan 09-05: valid metadata projects to the ordered CUE TOC; stored/virtual pregaps and four-frame padding map correctly; duplicate/out-of-order tracks, unsupported modes/subcode/postgap/session semantics and overflow reject before decode. | ✓ VERIFIED | `host/disc_chd_metadata.c`, `tests/disc/chd.c`, and `disc.image.chd_toc`/metadata/boundary selectors implement and exercise the mapping and rejection cases. |
| 20–22 | Plan 09-06: common-provider CHD output matches CUE identity, TOC, sectors and subcode; first/last/pregap/track/hunk transitions map to logical LBAs; repeated reads and failed opens preserve deterministic output and active media. | ✓ VERIFIED | `tests/disc/media.c` and `tests/disc/chd.c` compare exact outputs and exercise the failure-atomic open path; `disc.image.chd_parity`, `disc.image.chd_open` and boundary tests ran in current Main CI. |
| 23–25 | Plan 09-07: shared limits precede decode/allocation and keep companion paths confined; malformed CHD replay is required on enabled lanes; bounded sanitizer fuzz exercises the production parser without publishing rejected media. | ✓ VERIFIED | `host/media_limits.h`, `tests/disc/image.c`, `tests/fuzz/chd.c` and the ASan CI workflow cover bounds, unsafe paths, replay and active-image preservation. Nightly fuzz passed on the same relevant source head; the CHD parser and harness have no later source changes. |
| 26–28 | Plan 09-08: the ledger has complete unique keys and sorted rows with negative controls; the ratchet prevents regressions absent matching dated allowances; row counts and date arithmetic are bounded. | ✓ VERIFIED | `tests/compat/features.csv`, `tests/compat/compat-control-cases.tsv`, `scripts/compat-ratchet.sh` and `tests/cmake/compat-ratchet.cmake` provide the controls and protected baseline. |
| 29–31 | Plan 09-09: only enabled provider builds advertise CHD; `.info` license/extension claims match the pinned source closure; package names and required notice bytes are preserved. | ✓ VERIFIED | `libretro/CMakeLists.txt`, the `.info.in` template, notice files and release archive tests verify the claims and packages in both configurations. |
| 32–34 | Plan 09-10: CI reuse decisions use attributable exact-SHA/check/artifact and runner-time evidence; concurrency alone is not claimed as a wall-time saving; Main, Release and archive checks remain required. | ✓ VERIFIED | `.planning/phases/09-chd-disc-images/09-CI-REUSE.md` documents the measurements and disposition; both workflows retain their required guarantees. |
| 35–37 | Plan 09-11: all eight applicable lanes exercise CHD ON with identical fixture hashes; cheap selector/inventory/routing checks precede expensive jobs; distributed CHD enablement follows parity while standalone defaults remain OFF. | ✓ VERIFIED | `.github/workflows/ci.yml`, `.github/workflows/nightly.yml`, `CMakePresets.json` and `tests/cmake/workflows.cmake` wire the checks. Final-head Main CI passed all required jobs and the cross-lane hash gate. |
| 38–40 | Plan 09-12: runner and libretro use the normal shared content path; canonical identity matches CUE; lifecycle, disc-control and save behavior remain intact on accepted/rejected CHD. | ✓ VERIFIED | `tests/runner/disc.c`, `tests/libretro/host.c`, `host/disc_probe.c` and `libretro/libretro.c` connect both hosts to `schway_disc_image`; the current Main CI passed. |
| 41–43 | Plan 09-13: CHD ON then OFF are separately configured, built, packaged and validated; component archives preserve exact contents/notices and metadata; archive names remain stable. | ✓ VERIFIED | `tests/release/tests.cmake` and `tests/cmake/archive_contents.cmake` implement the package checks, with ON/OFF order in the plan's recorded sequence. Current Release passed. The separate declared notices.cmake key link is the single gap below. |
| 44–46 | Plan 09-14: the owned guest has matching boot/read/audio/FMV outputs within existing bounds; preflights finish before guest execution; ledger evidence and docs point to real checks and distinguish hosted from unknown evidence. | ✓ VERIFIED | `tests/disc/fixture.c`, CTest fixture dependencies, `tests/compat/features.csv`, `docs/discs.md` and `docs/compatibility.md` show the checks and evidence scope. |
| 47–50 | Plan 09-15: cross tests use an external native generator without changing defaults or adding a target generator; CHD bytes but not host metadata are cross-hashed; pins/digests reach every manifest; CHD OFF removes both fixture and manifest after external generation. | ✓ VERIFIED | `CMakePresets.json`, `CMakeLists.txt`, `cmake/CHDFixtures.cmake`, `cmake/DiscFixtures.cmake` and `tests/cmake/hashes.cmake` show those flows. |

**Score:** 50/50 truths verified (0 present, behavior-unverified).

### Required Artifacts

All 32 declared PLAN artifacts passed the structured existence and substance check. Source inspection confirmed the relevant providers, tests, workflows and consumers are wired; plan-grouped detail follows.

| Plan | Required artifacts | Status | Details |
| --- | --- | --- | --- |
| 09-01 | `tests/disc/media.c`; `tests/disc/tests.cmake` | ✓ VERIFIED | Preflight executable, registered CTest and fixture dependency. |
| 09-02 | `cmake/Libchdr.cmake`; `host/disc_chd.h` | ✓ VERIFIED | Conditional static reader and private backend interface. |
| 09-03 | `09-GENERATOR-AUDIT.md`; `cmake/CHDTool.cmake` | ✓ VERIFIED | Generator provenance audit and opt-in pinned build. |
| 09-04 | `cmake/CHDFixtures.cmake`; `tests/disc/media.c` | ✓ VERIFIED | Deterministic fixture producer and checked manifest. |
| 09-05 | `host/disc_chd_metadata.c`; `host/disc_chd.c`; `docs/discs.md` | ✓ VERIFIED | Bounded metadata mapping and documented subset. |
| 09-06 | `host/disc_image.c`; `tests/disc/media.c` | ✓ VERIFIED | Shared provider routing and parity checks. |
| 09-07 | `host/media_limits.h`; `tests/fuzz/chd.c` | ✓ VERIFIED | Checked arithmetic and bounded production-parser replay. |
| 09-08 | `tests/compat/features.csv`; `tests/cmake/compat-ratchet.cmake` | ✓ VERIFIED | Evidence ledger, controls and protected ratchet. |
| 09-09 | `libretro/schwaystation_libretro.info.in`; `THIRD-PARTY-NOTICES.md` | ✓ VERIFIED | Config-derived metadata and selected license notices. |
| 09-10 | `09-CI-REUSE.md` | ✓ VERIFIED | Exact-SHA workflow/artifact/time evidence and decision. |
| 09-11 | `.github/workflows/ci.yml`; `.github/workflows/nightly.yml` | ✓ VERIFIED | Lane generation, checks, hashing and fuzz wiring. |
| 09-12 | `tests/runner/disc.c`; `tests/libretro/host.c` | ✓ VERIFIED | Runner and libretro integration coverage. |
| 09-13 | `tests/release/tests.cmake`; `tests/release/inventory.txt` | ✓ VERIFIED | Registered ON/OFF archive, notice and metadata checks. |
| 09-14 | `tests/disc/fixture.c`; `tests/compat/features.csv`; `docs/discs.md` | ✓ VERIFIED | Bounded guest, evidence ledger and format docs. |
| 09-15 | `CMakePresets.json`; `cmake/CHDFixtures.cmake`; `tests/disc/media.c` | ✓ VERIFIED | External-generator presets, fixture validation and shared test target. |

### Key Link Verification

| From | To | Via | Status | Details |
| --- | --- | --- | --- | --- |
| `tests/disc/tests.cmake` | `tests/disc/media.c` | media test target and preflight fixture | WIRED | CMake target compiles `media.c`; the preflight fixture gates guests. |
| `cmake/DiscTargets.cmake` | `cmake/Libchdr.cmake` | CHD option and host-only static link | WIRED | Included only when `SCHWAYSTATION_CHD` is ON. |
| `cmake/CHDTool.cmake` | `pins.txt` | exact source pin and archive digest | WIRED | `CHDToolPins.cmake` reads and validates the single pin row. |
| `cmake/DiscFixtures.cmake` | `cmake/CHDFixtures.cmake` | owned fixture target dependency and manifest | WIRED | Included for enabled fixture builds; test targets depend on its generated output. |
| `host/disc_chd.c` | `include/schwaystation.h` | normalized public TOC fields | WIRED | Private metadata maps to the public `schway_toc` contract through the host image headers. |
| `host/disc_image.c` | `host/disc_chd.c` | optional backend open/close/TOC/read callbacks | WIRED | Extension dispatch and callback lifetime use the CHD backend. |
| `tests/fuzz/chd.c` | `host/disc_image.c` | actual open/read/close on isolated input | WIRED | The replay opens the owned baseline and candidate via production APIs. |
| `scripts/compat-ratchet.sh` | `tests/compat/features.csv` | protected baseline and row comparison | WIRED | Compatibility CTest passes the ledger as the candidate input. |
| `libretro/CMakeLists.txt` | `.info.in` | linked-provider-derived extension and license | WIRED | CMake configures the template from target linkage and feature option. |
| `09-CI-REUSE.md` | `.github/workflows/release.yml` | exact-SHA artifact/check review | WIRED | The evidence document records workflow and artifact identity used for the disposition. |
| `.github/workflows/ci.yml` | `cmake/CHDFixtures.cmake` | run-local fixture producer | WIRED | CI builds a native tool, generates and validates local CHD data before consumers. |
| `.github/workflows/release.yml` | `.github/workflows/ci.yml` | reusable CI and exact-release checks | WIRED | Release calls the shared CI workflow and retains separate archive checks. |
| `tests/runner/disc.c` | `host/disc_image.c` | ordinary runner content operations | WIRED | Test uses the public provider API for CUE/CHD and failed-open checks. |
| `tests/libretro/host.c` | `host/disc_image.c` | frontend load, unload and replacement operations | WIRED | Host tests open both providers and compare their content. |
| `tests/release/tests.cmake` | packaged `.info` metadata and `tests/cmake/archive_contents.cmake` | script-mode metadata comparison plus archive validation | WIRED | `release.notices` invokes `tests/release/tests.cmake` in CMake script mode to compare packaged metadata with the configured provider; `archive_contents.cmake` checks archive inventory and exact notice bytes. The plan names `tests/cmake/notices.cmake`, which is the separate source-manifest hygiene check. |
| `tests/release/tests.cmake` | `tests/cmake/archive_contents.cmake` | archive inventory and notice bytes | WIRED | Release archive test calls this script through its configured path. |
| `tests/disc/tests.cmake` | `tests/disc/fixture.c` | CHD guest cases after required preflights | WIRED | Guest targets require the media/parity/feature fixtures. |
| `tests/compat/features.csv` | `tests/disc/tests.cmake` | evidence references for CHD predicates | WIRED | CHD row names the real parity, runner, libretro and guest selectors. |
| `CMakePresets.json` | `CMakeLists.txt` | external native generator for cross builds | WIRED | Cross presets set the external executable; root CMake avoids building the tool target. |
| `cmake/CHDFixtures.cmake` | `tests/cmake/hashes.cmake` | generated fixture bytes in cross-lane hash set | WIRED | CI generates the CHD into the fixtures directory; hash script recursively hashes its members, not the separately stored manifest. |

### Data-Flow Trace (Level 4)

| Artifact | Data | Source | Real data | Status |
| --- | --- | --- | --- | --- |
| CHD sector provider | raw sectors, TOC and subcode | validated file extents, parsed track records and pinned `libchdr` `chd_read` output | Yes | VERIFIED — the 222-sector owned CHD is compared against the CUE in both orders. |
| Shared host consumers | content identity and sector callbacks | `schway_disc_image` dispatches to the selected provider; runner and libretro consume the descriptor | Yes | VERIFIED — no static or empty fallback supplies user content. |
| Cross-lane fixture hashes | generated CHD bytes | `CHDFixtures.cmake` writes under build fixtures; `hashes.cmake` hashes sorted fixture members, while the manifest lives separately | Yes | VERIFIED — all applicable lane hash files were compared in final Main CI. |
| Compatibility row | CHD predicate and evidence keys | `tests/compat/features.csv` and registered selector names | Yes | VERIFIED — ledger key and ratchet controls are part of CI. |

### Behavioral Spot-Checks

| Behavior | Command/evidence | Result | Status |
| --- | --- | --- | --- |
| CHD/CUE parity, TOC and content identity | `disc.image.chd_parity` in [final Main CI](https://github.com/szTheory/schwaystation/actions/runs/45442543076) on the implementation head | 222/222 sectors matched in forward and reverse reads; TOC, subcode and identity assertions passed. | PASS |
| CHD guest and host integration | `disc.fixture.chd.*`, `runner.disc.chd`, and `libretro.content.chd` in Main CI | Boot/read/audio/FMV guest cases and both consumers passed with existing bounds. | PASS |
| Malformed input, bounded replay and fuzz | ASan/replay in Main CI and [Nightly](https://github.com/szTheory/schwaystation/actions/runs/45442543657) on the same relevant CHD source head | CHD replay passed; the Nightly fuzz campaign passed. `host/disc_chd.c` and `tests/fuzz/chd.c` have not changed since Phase 9. | PASS |
| Packaged metadata, archive contents and notices | [post-merge Release](https://github.com/szTheory/schwaystation/actions/runs/45455115400) on current release `f08fc08` | ON/OFF package validations passed. | PASS |

### Probe Execution

No phase-declared probe paths or conventional `scripts/*/tests/probe-*.sh` files were found; not applicable.

### Requirements Coverage

| Requirement | Source plans | Status | Evidence |
| --- | --- | --- | --- |
| MEDIA-01 | 09-01, 09-02, 09-04–09-07, 09-12, 09-14, 09-15 | SATISFIED | CHD/CUE identity, TOC, sector and applicable subcode parity; shared host selection and subsequent provider checks are present in current docs/tests. |
| MEDIA-02 | 09-01–09-07, 09-11, 09-14, 09-15 | SATISFIED | Checked counts/extents and preflight, malformed replay, active-image rejection isolation and cross-lane input validation. |
| CHD-01 | 09-02–09-07, 09-09–09-15 | SATISFIED | Pinned BSD-3 closure, default-off behavior, parity/hash, fuzz, exact package notices and conditional `.info` claims. |
| COMPAT-02 | 09-08, 09-11, 09-12, 09-14, 09-15 | SATISFIED | Unique feature evidence keys, protected ratchet, generated docs and CI checks. |

No requirements mapped to Phase 9 are orphaned from the plans.

### Decision Coverage

All trackable CONTEXT.md decisions are honored by shipped artifacts: 9/9.

### Test Quality Audit

| Test files | Linked requirements | Active | Disabled | Circular | Strongest assertion | Verdict |
| --- | --- | --- | --- | --- | --- | --- |
| `tests/disc/chd.c`, `tests/disc/media.c`, `tests/disc/image.c` | MEDIA-01, MEDIA-02, CHD-01 | Yes | 0 | 0 | Value and behavioral | PASS — exact sectors/TOC, boundary reads, malformed headers and failure isolation. |
| `tests/fuzz/chd.c` | MEDIA-02, CHD-01 | Yes | 0 | 0 | Behavioral | PASS — invokes the production API on owned baseline and hostile candidate bytes. |
| `tests/runner/disc.c`, `tests/libretro/host.c` | MEDIA-01, CHD-01 | Yes | 0 | 0 | Behavioral | PASS — real host/provider operations and rejected-image behavior. |
| `tests/release/tests.cmake`, `tests/cmake/archive_contents.cmake` | CHD-01 | Yes | 0 | 0 | Value | PASS — package members, notice bytes and configured metadata are compared. |
| `tests/compat/tests.cmake`, `tests/cmake/compat-ratchet.cmake` | COMPAT-02 | Yes | 0 | 0 | Value | PASS — negative controls and protected prior rows. |

**Disabled tests on requirements:** 0. **Circular patterns detected:** 0. **Insufficient assertions:** 0.

### CI Evidence and Efficiency

The current release is `v0.14.0` at `f08fc08d40a1ebcb42708e0a31be3111e3eed711`. Final Main CI [45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076) and Nightly [45442543657](https://github.com/szTheory/schwaystation/actions/runs/45442543657) passed on implementation head `75beb849f4b9e8a9a3bd023cc5c80632e4c74d56`; post-merge Main [45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354) and Release [45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400) passed on the current release commit.

The existing STATE measurements show Main's 3,943-second critical path and Nightly's 814-second duration, including 802 seconds for fuzz; both timing endpoints reported zero billable milliseconds. No speedup or dollar-savings claim is made. No full suite was rerun for this report.

### Anti-Patterns Found

| File | Pattern | Severity | Impact |
| --- | --- | --- | --- |
| — | No unreferenced `TBD`, `FIXME` or `XXX` markers; no placeholder implementation or console-log-only behavior in phase implementation files. | — | The `return NULL` matches are ordinary error/null sentinels with callers checking them; no flow reaches user-visible output as a stub. |

### Human Verification Required

None. This has no visual or external-service flow; the runner, libretro, package and malformed-input behaviors are exercised by automated CI.

### Gaps Summary

No gaps remain in the phase goal. The implementation opens the documented CHD subset through the shared host path, and current tests prove parity, host integration, bounds, package metadata and notice bytes. Plan 09-13's packaged-metadata helper is implemented inline in `tests/release/tests.cmake`; `tests/cmake/notices.cmake` is used for source-manifest hygiene.

---

_Verified: 2026-11-25T01:58:43Z_
_Verifier: the agent (gsd-verifier)_
