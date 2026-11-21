---
phase: 10-pbp-disc-images
verified: 2026-11-21T16:50:05Z
status: passed
score: 2/2 must-haves verified
covered_files:
  - .github/workflows/ci.yml
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
  - .planning/phases/10-pbp-disc-images/10-REVIEW.md
  - .planning/phases/10-pbp-disc-images/10-VERIFICATION-EVIDENCE.md
  - host/disc_pbp.c
  - quality.json
  - tests/cmake/workflows.cmake
  - tests/hygiene/tests.cmake
  - tests/libretro/host.c
covered_digest: "v4:sha256:602528f9e6b3f1c8f7e1a164122bf19352f6a538c3b3f0c11a6c11ed5e7b07b2"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 12
  total: 12
  not_honored: []
human_verification: []
---

# Phase 10: PBP Disc Images Verification Report

**Phase Goal:** Players can open documented unencrypted PlayStation PBP images and get a clear result for other variants.
**Verified:** 2026-11-21T16:08:38Z
**Status:** passed

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | The runner and libretro host read the fixture-proven unencrypted PSISO layout with matching sectors and deterministic outputs; parsing works without system zlib and the bundled inflater has a permissive notice. | ✓ VERIFIED | `host/disc_pbp_metadata.c` checks PBP/PSAR offsets, TOC records, track limits and contiguous bounded blocks; `host/disc_pbp.c` validates every block before publishing a reader and serves bounded cached sectors. `host/disc_image.c`, `runner/probe.c` and `libretro/libretro.c` route through the shared host provider. The local log records sector/TOC parity, runner, libretro, guest and package tests passing. `cmake/Miniz.cmake` links the private miniz inflater without system zlib; the MIT notice is present in `THIRD-PARTY-NOTICES.md`. |
| 2 | Malformed PSISO structures fail at the media boundary; PGD and undocumented layouts return a clear unsupported result before guest execution; exercised variants are recorded. | ✓ VERIFIED | Provider tests cover bad headers, offsets, TOC/track metadata, block extents, truncation and raw/deflated blocks. Runner probes assert PGD/unknown errors and malformed errors before a frame hash; the libretro host test verifies rejected replacements preserve RAM and card state. The feature ledger records the supported single-track NTSC-U PBP case and explicitly marks hosted CI as not yet run. |

**Score:** 2/2 roadmap truths verified

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| `host/disc_pbp_metadata.c` | Bounded PSISO directory, TOC and block parsing | ✓ EXISTS + SUBSTANTIVE | Checks checked-arithmetic extents, media limits, track tables, BCD MSF, contiguous block offsets, block sizes, and exact file end. |
| `host/disc_pbp.c` and `host/pbp_inflate.c` | Validated sector reader and bounded raw inflater | ✓ EXISTS + SUBSTANTIVE | Validates all compressed blocks before returning a reader, caps each block, requires full input consumption and exact output size, and caches one block. |
| `host/disc_image.c` | Shared provider dispatch and sector/TOC callbacks | ✓ EXISTS + WIRED | `.pbp` dispatch calls the provider, projects TOC, and forwards sector reads through the common media interface. |
| `runner/probe.c`, `libretro/libretro.c` | Normal host consumers | ✓ EXISTS + WIRED | Runner probes PBP through the shared host probe; libretro includes PBP in accepted extensions and uses the shared media loader. |
| `cmake/Miniz.cmake`, `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md` | Private permissive inflater closure | ✓ EXISTS + SUBSTANTIVE | Private symbol aliases and no system zlib dependency; MIT notice and pinned provenance are documented. |
| PBP CTest and fixture inventory entries | Repeatable producer, parity, boundary, host and guest evidence | ✓ EXISTS + WIRED | The 655-test local workflow log shows PBP preflight, parity, headers, inflate, metadata, raw, bounds, fixture controls, open, boundaries, guest boot/read/FMV, runner, libretro and compatibility checks passing. |

**Artifacts:** all phase-goal artifacts verified. The plan's stale `libretro/schwaystation_libretro.c` path was corrected to the actual `libretro/libretro.c` in `10-09-PLAN.md`; the subsequent plan artifact query passed 5/5 and the key-link query passed 1/1.

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| `libretro/libretro.c` | `host/disc_image.c` | Normal content loader uses the shared PBP provider | ✓ WIRED | The libretro content path calls the shared `schway_disc_image_open` provider and consumes the common sector/TOC callbacks. |
| `runner/probe.c` | `host/disc_image.c` | PBP probe opens the shared media image | ✓ WIRED | `schway__pbp_source` opens through the shared provider and derives probe metadata from its TOC and sector interface. |
| `host/disc_image.c` | `host/disc_pbp.c` | `.pbp` dispatch | ✓ WIRED | Extension dispatch opens the PBP reader and forwards TOC/read-sector operations. |
| `host/disc_pbp.c` | `host/pbp_inflate.c` | Compressed block decode | ✓ WIRED | The reader invokes the private bounded raw-DEFLATE wrapper for compressed PSISO blocks. |

**Wiring:** 4/4 key connections verified. GSD artifact and key-link queries passed for all 13 plans after correcting the Plan 10-09 path.

### Data-Flow Trace

| Artifact | Data | Source | Produces real data | Status |
|---|---|---|---|---|
| `host/disc_pbp_metadata.c` → `host/disc_pbp.c` | PBP offsets, TOC and block map | Selected image bytes | Yes; checked metadata becomes a bounded map, then decoded sectors | ✓ FLOWING |
| Shared provider → runner/libretro | Sector bytes, TOC and subcode | Decoded PBP blocks and host callbacks | Yes; both consumers use the actual provider | ✓ FLOWING |
| Fixture parity tests | Expected sector bytes and TOC | Separately generated source CUE | Yes; tests compare every sector in both orders, TOC, subcode and content identity | ✓ FLOWING |

No rendered UI or static runtime fallback exists in this phase.

### Behavioral Spot-Checks

The verifier did not start or repeat tests. It inspected the final workflow output already produced by Plan 10-13:

| Behavior | Command | Result | Status |
|---|---|---|---|
| Complete native build/test/package workflow | `cmake --workflow --preset ci` | exit 0; 655/655 standard tests, 7/7 release checks; 78 seconds total | ✓ PASS |
| PBP provider and every-sector parity | PBP CTest inventory in the saved workflow output | `disc.image.pbp_preflight`, `disc.image.pbp_parity`, `disc.image.pbp_headers`, `disc.image.pbp_inflate`, `disc.image.pbp_metadata`, `disc.image.pbp_raw`, `disc.image.pbp_bounds`, `disc.image.pbp_fixture_controls`, `disc.image.pbp_open`, `disc.image.pbp_boundaries` passed | ✓ PASS |
| Bounded guest and host consumers | PBP CTest inventory in the saved workflow output | PBP boot/read/FMV, `runner.disc.pbp`, and `libretro.content.pbp` passed | ✓ PASS |
| Feature ledger and malformed-input privacy | PBP CTest inventory in the saved workflow output | Compatibility preflight/ratchet and malformed-PBP redaction test passed | ✓ PASS |

The saved uncommitted-output path is `/private/tmp/schwaystation-phase10-ci-final.log`; it contains 100% pass summaries for both test phases. No log file is committed.

### Test Quality Audit

| Test File | Linked Req | Active | Skipped | Circular | Assertion Level | Verdict |
|---|---|---:|---:|---|---|---|
| `tests/disc/pbp.c` and `tests/disc/image.c` | PBP-01 | Yes | 0 found | No | Exact bytes/TOC/status/bounds; all sectors compared with source CUE | VALID |
| `tests/runner/tests.cmake` | PBP-01 | Yes | 0 found | No | Exit status, clear diagnostic, no frame output before guest | VALID |
| `tests/libretro/host.c` | PBP-01 | Yes | 0 found | No | Load, full sector parity, frame/audio/state comparisons and rejection state preservation | VALID |
| `tests/compat/tests.cmake`, `tests/compat/features.csv` | PBP-01 | Yes | 0 found | No | Malformed path redaction and explicit exercised/unknown feature evidence | VALID |
| `tests/fuzz/pbp.c` | PBP-01 | Yes | 0 found | No | Production parser replay over mutation cases; sanitizer campaign is a required Nightly gate | VALID |

No disabled-test patterns or unreferenced `TODO`/`HACK`/`FIXME`/`XXX` markers were found in the requirement-linked PBP tests and production paths. The parity expectations come from the separate source CUE; the fixture generator does not call the provider under test. This is not a circular test.

### Requirements Coverage

| Requirement | Source | Description | Status | Evidence |
|---|---|---|---|---|
| PBP-01 | Phase 10 plans 01–13 | Fixture-proven unencrypted PSISO PBP, bounded metadata/block parsing, deterministic sectors, clear unsupported cases, permissive inflater without system zlib | ✓ SATISFIED in local implementation | Provider/runner/libretro/guest/parity/package tests passed locally. The required exact-head hosted CI/Nightly and linked WASI size remain mandatory ship gates before release or advancing to Phase 11. |

No orphaned Phase 10 requirements were found. Context decision coverage is 12/12 honored.

### Probe Execution

No shell probe was declared for Phase 10. The runner's PBP `--probe` path is covered by the registered `runner.disc.pbp` test.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|
| - | - | None in reviewed PBP implementation and linked test files | - | No stub, unreferenced debt marker, or disabled requirement test was found. |

### Human Verification Required

None. The phase has no visual or hardware-only acceptance criterion; its file parsing, determinism and host seams are exercised by automated tests. No BIOS, disc, or physical-console test is required.

### Initial Ship-Gate Snapshot — 2026-11-21T16:08:38Z

The hosted evidence is a **required ship gate**, not a failed implementation truth. `.planning/AUTONOMOUS.md` and AGENTS rules 9–10 require exact-head PR CI, Nightly and release-pipeline inspection before release or Phase 11. The first attempt ran on pre-repair commit `50efc31217b851de8cbe3b53677b525b9b65447d` and was cancelled after actionable failures; it does not satisfy the gate. Before release or Phase 11, the ship step must:

1. Run and inspect one exact-head PR CI and full Nightly attempt, including required platform/package jobs, sanitizer, no-float, BE32, WASI, eight-way hashes, RetroArch, release rehearsal and the full fuzz route.
2. Re-measure the linked WASI module on the repaired head. The first hosted producer measured 875,971 bytes on [run 44924014258](https://github.com/szTheory/schwaystation/actions/runs/44924014258), [job 136906260832](https://github.com/szTheory/schwaystation/actions/runs/44924014258/job/136906260832); `quality.json` now records that exact measurement. If the repaired producer differs, update the bound from its linked artifact. Do not substitute the native macOS module size.
3. Record exact run URLs, lane critical path and runner-time signals. Historical data identifies BE32/QEMU as the critical-path candidate; current Phase 10 Actions dollars are not yet available. No coverage reduction is justified from historical runs alone.

The mixed code/workflow head must use the full Nightly route. Docker is available locally, but this Apple Silicon host would need x86_64 emulation for the workflow's pinned x86_64 Wasmtime runtime; Act dry-runs checked route wiring only. The hosted WASI job provides the required artifact handoffs and matching architecture, so another local Act run would not establish those hosted properties.

## Follow-Up Verification — 2026-11-21T16:50:05Z

This follow-up preserves the initial report above and checks the Phase 10 ship-fix batch against the two roadmap success criteria. No PBP parsing or acceptance scope changed. The implementation truths remain **2/2 verified locally**; hosted exact-head checks are still a separate required ship gate. No broad suite was rerun by this verifier.

| Repair | Code and wiring evidence | Verification result |
|---|---|---|
| MSVC PBP opener | `host/disc_pbp.c` routes PBP opens through `schway__disc_pbp_open_file`; the `_MSC_VER` branch uses `fopen_s`, while other compilers use `fopen`. | The local PBP module build passed on this host. The MSVC branch has not yet been compiled on the repaired head; the old Windows jobs failed before this fix, so Windows CI remains required. |
| Pointer-safe libretro assertion | `tests/libretro/host.c` compares the current RAM pointer directly with the saved `pbp_ram` pointer after rejected replacements. | The local full workflow recorded 655/655 tests and 7/7 release checks passing. This removes the pointer-to-integer conversion that failed the old BE32 build; repaired BE32 CI remains pending. |
| Classifier workflow assertion | `tests/hygiene/tests.cmake` filters classifier output to the two values that `ci.classify` owns. `tests/cmake/ci_route_controls.cmake` independently checks `nightly_full` and `pbp_only` outputs across route cases. | Recorded focused checks passed: `ci.classify` 1/1 and `ci.classify_policy` plus PBP tests 11/11. |
| Protected-base sparse checkout | `.github/workflows/ci.yml` includes `tests/cmake/date_days.cmake` in `compat-base`; `tests/cmake/workflows.cmake` asserts that required dependency remains in the sparse checkout. `tests/cmake/compat-ratchet.cmake` includes that helper. | The route/closure check passed, as did protected-base ratchet tests against a full base checkout (7/7). GitHub has not yet exercised the repaired sparse checkout. |
| WASI module bound | `quality.json` now records 875,971 bytes, matching the linked module measured by the first PBP-enabled WASI attempt. The local quality comparison passed. | This is a measurement from the failed/cancelled pre-repair run, not a green repaired-head WASI gate. Re-measure the linked module on the repaired head. |

The first exact-head PR run [44924014258](https://github.com/szTheory/schwaystation/actions/runs/44924014258) ended **cancelled** on pre-repair SHA `50efc31217b851de8cbe3b53677b525b9b65447d`; hygiene, WASI, BE32 and several native build jobs had failed. The concurrent [Nightly run 44924014369](https://github.com/szTheory/schwaystation/actions/runs/44924014369) on the same SHA also ended **cancelled**, with its fuzz and report jobs cancelled. Neither is a passing ship gate. At 16:50Z, PR [#34](https://github.com/szTheory/schwaystation/pull/34) still pointed to the pre-repair SHA; the repaired working-tree changes therefore have no exact-head hosted result yet.

Local evidence recorded in `10-VERIFICATION-EVIDENCE.md` includes the successful native workflow, targeted classifier and PBP checks, module build, workflow closure test, and `act --dryrun` routing check. Act establishes routing only; hosted Windows/MSVC, BE32, WASI, platform/package lanes, artifact handoffs, RetroArch and release integrations remain unverified here. Repaired exact-head PR CI, the full Nightly route, linked WASI measurement and release pipeline must be green and inspected before shipping or advancing to Phase 11.

## Decision Coverage

All 12 trackable `10-CONTEXT.md` decisions are honored by the phase plans and implementation; the non-blocking decision-coverage query reported 12/12.

## Gaps Summary

No Phase 10 implementation gap was found. The phase goal is verified on the local host; release and milestone advancement remain gated on the exact-head hosted work listed above. The generic SPDX scanner remains deferred by the owner's explicit choice, and the 3,901-line ceiling remains unchanged.

## Ship repair follow-up (2026-11-21)

The pre-repair exact-head CI attempt failed in five areas: the hygiene classifier-output assertion, the prior WASI
size bound, a pointer-to-integer assertion in the BE32 host test, MSVC's warning-as-error for direct `fopen`, and
the protected-base sparse checkout's missing `date_days.cmake` dependency. Its Nightly attempt's classifier,
vectors, WASI benchmark and three benchmark legs passed; the fuzz job was cancelled. Both attempts were cancelled
after diagnosis. They are recorded in [10-CI-EFFICIENCY.md](10-CI-EFFICIENCY.md) and are not treated as green
ship evidence.

The fixes were independently reviewed clean. The full local workflow passed 655 standard tests and 7 release
checks; focused hygiene, PBP, classifier-policy, protected-ratchet and workflow-closure checks passed. The
protected-base sparse checkout now includes `tests/cmake/date_days.cmake`, guarded by a workflow assertion. The
repository-check budget is exactly 3,901/3,901 lines. These results preserve the two roadmap truths and do not
claim cross-platform hosted success. The repaired exact-head CI, Nightly and release-pipeline runs remain
required before Phase 11.

---
*Verified: 2026-11-21T16:50:05Z*
*Verifier: the agent (gsd-verifier)*
