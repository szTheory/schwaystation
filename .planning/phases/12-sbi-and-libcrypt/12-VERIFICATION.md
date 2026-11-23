---
phase: 12-sbi-and-libcrypt
verified: 2026-11-23T08:38:26Z
status: passed
score: 2/2 truths verified
covered_files:
  - .github/workflows/nightly.yml
  - .planning/phases/12-sbi-and-libcrypt/12-01-PLAN.md
  - .planning/phases/12-sbi-and-libcrypt/12-01-SUMMARY.md
  - .planning/phases/12-sbi-and-libcrypt/12-02-PLAN.md
  - .planning/phases/12-sbi-and-libcrypt/12-02-SUMMARY.md
  - .planning/phases/12-sbi-and-libcrypt/12-03-PLAN.md
  - .planning/phases/12-sbi-and-libcrypt/12-03-SUMMARY.md
  - .planning/phases/12-sbi-and-libcrypt/12-04-PLAN.md
  - .planning/phases/12-sbi-and-libcrypt/12-04-SUMMARY.md
  - .planning/phases/12-sbi-and-libcrypt/12-05-PLAN.md
  - .planning/phases/12-sbi-and-libcrypt/12-05-SUMMARY.md
  - .planning/phases/12-sbi-and-libcrypt/12-06-PLAN.md
  - .planning/phases/12-sbi-and-libcrypt/12-06-SUMMARY.md
  - .planning/phases/12-sbi-and-libcrypt/12-07-PLAN.md
  - .planning/phases/12-sbi-and-libcrypt/12-07-SUMMARY.md
  - CMakePresets.json
  - cmake/DiscFixtures.cmake
  - cmake/DiscTargets.cmake
  - docs/discs.md
  - host/disc_image.c
  - host/disc_sbi.c
  - host/disc_sbi.h
  - src/cdrom.c
  - src/cdrom_sector.c
  - src/disc.c
  - tests/cdrom/inventory.txt
  - tests/cdrom/sbi.c
  - tests/cdrom/sectors.c
  - tests/cdrom/tests.cmake
  - tests/cmake/workflows.cmake
  - tests/compat/features.csv
  - tests/compat/tests.cmake
  - tests/disc/inventory.txt
  - tests/disc/sbi.c
  - tests/disc/tests.cmake
  - tests/fuzz/inventory.txt
  - tests/fuzz/sbi.c
  - tests/fuzz/seeds.c
  - tests/fuzz/seeds.h
  - tests/fuzz/tests.cmake
  - tests/libretro/tests.cmake
covered_digest: "v4:sha256:6d51059c88cb41257c3f1e5c4283fde7718d605c10e5637243b02f0a571afd2f"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 0
  total: 8
  not_honored: [D-01, D-02, D-03, D-04, D-05, D-06, D-07, D-08]
---

# Phase 12: SBI and LibCrypt Verification Report

**Phase goal:** With a matching SBI sidecar, supported protected discs receive the required subchannel responses.
**Verified:** 2026-11-23T08:38:26Z
**Status:** passed
**Score:** 2/2 roadmap truths verified

## Goal Achievement

| # | Observable truth | Status | Evidence |
|---|---|---|---|
| 1 | SBI records are tied to the selected provider and its addressable LBA map; M3U selection carries the selected child's own bad-Q callback; malformed or mismatched sidecars do not publish an overlay for another disc. | VERIFIED | `host/disc_image.c` stores the parsed overlay per concrete image, discovers the same-stem sidecar, validates it against that image's TOC bounds, and exposes `bad_q` on the descriptor. `tests/disc/sbi.c` and `tests/cdrom/sbi.c` exercise same-LBA child differences, missing/malformed sidecars, and selection/swap behavior. All registered tests passed in the full local workflow. |
| 2 | A bad-Q sector still delivers its data while GetlocP retains the selected disc's previous good Q; malformed parser inputs have deterministic replay and fuzz coverage; compatibility evidence stays within exercised predicates. | VERIFIED | `tests/cdrom/sbi.c` runs the selected-provider command sequence and checks the payload plus previous-good GetlocP; `tests/cdrom/sectors.c` covers read and seek handling; `src/disc.c` clears cached Q on provider replacement. The 7-case SBI seed replay passed in the full suite, and `tests/fuzz/sbi.c` calls the production parser. The Nightly workflow builds and runs the libFuzzer target. `tests/compat/features.csv` records only selected-provider binding, bad-Q data, and previous-good GetlocP. |

## Required Artifacts

| Artifact | Status | Evidence |
|---|---|---|
| `host/disc_sbi.c` / `host/disc_sbi.h` | VERIFIED | Bounded production parser handles format 1/2/3, checked BCD/MSF conversion, LBA and record/file limits, duplicate sorting, and failure-atomic output. `tests/disc/sbi.c` asserts known LBAs and failure results. |
| `host/disc_image.c` | VERIFIED | Opens the sidecar before publishing a concrete image, keeps its overlay on that image object, exposes a sorted-LBA lookup callback, and clears the overlay on close. |
| `src/cdrom.c`, `src/cdrom_sector.c`, `src/disc.c` | VERIFIED | Bad-Q status gates Q/cache position changes in scheduled read and seek paths; valid data still enters the sector FIFO; disc application clears Q cache and current position. |
| `tests/disc/sbi.c`, `tests/cdrom/sbi.c`, `tests/cdrom/sectors.c` | VERIFIED | Parser, provider-selection, command-sequence, data-delivery, seek, and disc-swap assertions are registered and active. |
| `tests/fuzz/sbi.c`, `tests/fuzz/tests.cmake`, `tests/fuzz/seeds.c`, `tests/fuzz/inventory.txt` | VERIFIED | Seed replay and mutation targets both call the production parser; the portable replay selector is included in CI and wasm32 inventories. |
| `.github/workflows/nightly.yml`, `CMakePresets.json`, `tests/cmake/workflows.cmake` | VERIFIED | Nightly SBI corpus count, size, names, build targets, and workflow controls are wired; route and preset preflights pass. |
| `tests/libretro/tests.cmake` | VERIFIED | Three region captures feed six batch/single comparisons; file existence, byte-size/alignment, digest, and exit assertions remain. The full workflow passed `libretro.host`. |
| `tests/compat/features.csv` | VERIFIED | Ledger row is limited to provider binding, bad-Q data availability, and previous-good GetlocP evidence. |

## Key Link Verification

| From | To | Status | Details |
|---|---|---|---|
| `cmake/DiscTargets.cmake` | `host/disc_sbi.c` | VERIFIED | The host-only disc-image target compiles the parser. |
| `tests/disc/tests.cmake` | `tests/disc/inventory.txt` | VERIFIED | Indirect generic wiring: `SchwaystationTests.cmake` includes `tests/cmake/inventory.cmake`, which reads the selected `tests/<dir>/inventory.txt` and fails configure unless registered test names/labels match. |
| `host/disc_image.c` | `host/disc_sbi.c` and `schway_disc.bad_q` | VERIFIED | Sidecar loading invokes the parser, stores its output on the image object, and sets the descriptor callback only for a non-empty overlay. |
| M3U child selection | CD-ROM bad-Q callback | VERIFIED | Selection returns the child's descriptor; `disc_apply` installs its callbacks/TOC and clears Q cache; sector-read/seek code consults that callback. |
| `tests/fuzz/tests.cmake` | `tests/fuzz/sbi.c` / production parser | VERIFIED | Replay and fuzz targets link the parser library; CTest includes the portable replay, and Nightly runs the mutation target. |
| Runner PCM capture | Batch/single callback comparisons | VERIFIED | The same checked PCM file is supplied to both host cases for each of NTSC-J, NTSC-U and PAL. |

The generic key-link checker could not resolve several prose endpoints and the inventory's indirect include. Those links were manually traced through the CMake registration helper and the production call paths above; no wiring gap remains.

## Local Verification Evidence

| Command | Result |
|---|---|
| `for f in tests/*/inventory.txt; do LC_ALL=C sort -cu "$f" || exit 1; done` | Exit 0; all subsystem inventories sorted and unique. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | Exit 0; repository-check gate is 3,901/3,901 lines. |
| `cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake` | Exit 0; hosted selector/inventory wiring is connected. |
| `cmake --workflow --preset ci` | Exit 0; full CTest run passed 674/674 tests, followed by 8/8 Release checks. This was the single full workflow run for this verification. |

The local workflow took about 3m36s by the accumulated command-session wait time. `libretro.input.movies` took 49.84s versus the prior 25.94s local sample in Phase 10 (about 1.9×). This is one unrelated full-suite sample and does not justify a timeout or workflow change. The Plan 12-07 `libretro.host` optimization reduced runner captures from six to three while preserving all six comparisons; no end-to-end speedup is claimed without comparable hosted timing.

The Apple Clang host lacks `libclang_rt.fuzzer_osx.a`, so no local mutation campaign was run. The seed replay passed locally, and the hosted Ubuntu Nightly mutation campaign is wired but has not yet run on this head. It remains a host-side check for ship; this report does not claim a Nightly campaign pass.

## Test Quality Audit

No linked test is disabled. Expected SBI parser statuses, LBAs, Q positions, and data markers are independently specified; test-written files are inputs to the production parser/provider, not expected output generated by it. The CD-ROM command sequence asserts values and data delivery rather than only successful calls. No circular expected-value generation was found.

## Decision Coverage

The decision-coverage heuristic reported 0/8 honored decisions (D-01–D-08). Manual comparison against the implementation, tests, summaries and commits confirms those decisions are represented: provider-owned overlays, checked formats and bounds, selection cache reset, data-preserving bad-Q handling, the guest-visible sequence, narrow ledger, capture reuse, and unchanged required Main/Nightly/Release lanes and repository-check cap. This gate is warning-only; the wording matcher did not recognize the implementation evidence.

## Anti-Patterns and Human Verification

No `TBD`, `FIXME`, `XXX`, `TODO`, `HACK` or placeholder markers were found in changed files, and the relevant tests contain no disabled-test markers. This is an infrastructure/core-library phase with no user-facing manual step; human verification is N/A.

## Probe Execution

N/A — Phase 12 is a subsystem implementation phase and declares no probe scripts.

## Requirements Coverage

| Requirement | Status | Evidence |
|---|---|---|
| CRYPT-01 | SATISFIED | Parser/provider binding, bad-Q data delivery, prior-good GetlocP, malformed seed replay, and narrow compatibility evidence pass the local CI workflow. The hosted Nightly mutation execution remains pending ship. |

---
_Verifier: gsd-verifier_
