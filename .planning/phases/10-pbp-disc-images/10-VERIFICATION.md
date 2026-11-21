---
phase: 10-pbp-disc-images
verified: 2026-11-21T16:08:38Z
status: passed
score: 2/2 must-haves verified
covered_files:
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
covered_digest: "v4:sha256:d6adc872bfa475f5bd3b52ddbab6bf0a390187f773064fb9852d26d79588bcd4"
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

### Ship Gates Pending

The hosted evidence is a **required next ship gate**, not a failed implementation truth. `.planning/AUTONOMOUS.md` and AGENTS rules 9–10 require formal phase verification before `$gsd-ship`, then place exact-head PR CI, Nightly and release-pipeline inspection at ship. The PR does not exist yet, so no hosted run is claimed here. Before release or Phase 11, the ship step must:

1. Run and inspect one exact-head PR CI and full Nightly attempt, including required platform/package jobs, sanitizer, no-float, BE32, WASI, eight-way hashes, RetroArch, release rehearsal and the full fuzz route.
2. Extract the actual linked WASI module size from the hosted `wasm32` job. Keep the previous 859,379-byte bound unchanged until this producer measurement is available; do not substitute the native macOS module size.
3. Record exact run URLs, lane critical path and runner-time signals. Historical data identifies BE32/QEMU as the critical-path candidate; current Phase 10 Actions dollars are not yet available. No coverage reduction is justified from historical runs alone.

The mixed code/workflow head must use the full Nightly route. Docker is available locally, but this Apple Silicon host would need x86_64 emulation for the workflow's pinned x86_64 Wasmtime runtime; Act dry-run already checked route wiring. The hosted WASI job provides the required artifact handoffs and matching architecture, so another local Act run would not establish those hosted properties.

## Decision Coverage

All 12 trackable `10-CONTEXT.md` decisions are honored by the phase plans and implementation; the non-blocking decision-coverage query reported 12/12.

## Gaps Summary

No Phase 10 implementation gap was found. The phase goal is verified on the local host; release and milestone advancement remain gated on the exact-head hosted work listed above. The generic SPDX scanner remains deferred by the owner's explicit choice, and the 3,901-line ceiling remains unchanged.

---
*Verified: 2026-11-21T16:08:38Z*
*Verifier: the agent (gsd-verifier)*
