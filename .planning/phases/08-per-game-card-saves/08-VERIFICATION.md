---
phase: 08-per-game-card-saves
verified: 2026-11-25T01:56:17Z
status: passed
score: 11/11 truths verified
covered_files:
  - .github/workflows/ci.yml
  - .planning/phases/08-per-game-card-saves/08-01-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-01-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-02-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-02-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-03-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-03-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-04-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-04-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-05-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-05-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-06-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-06-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-07-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-07-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-08-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-08-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-09-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-09-SUMMARY.md
  - .planning/phases/08-per-game-card-saves/08-10-PLAN.md
  - .planning/phases/08-per-game-card-saves/08-10-SUMMARY.md
  - AGENTS.md
  - CMakePresets.json
  - cmake/DiscFixtures.cmake
  - docs/embedding.md
  - docs/runner.md
  - docs/testing.md
  - host/cards.c
  - host/cards.h
  - host/save_game_identity.c
  - host/save_game_identity.h
  - libretro/libretro.c
  - libretro/schwaystation_libretro.info.in
  - quality.json
  - runner/CMakeLists.txt
  - runner/main.c
  - src/card.c
  - src/card.h
  - src/internal.h
  - tests/card/card_tests.c
  - tests/card/files.c
  - tests/card/inventory.txt
  - tests/card/tests.cmake
  - tests/cmake/info_claims.cmake
  - tests/cmake/info_claims_self_test.cmake
  - tests/cmake/workflows.cmake
  - tests/core/test_state.c
  - tests/fixtures/disc_boot_playlist.xml
  - tests/fixtures/inventory.txt
  - tests/fixtures/manifest.txt
  - tests/fixtures/src/pocketstation_storage.asm
  - tests/fixtures/tests.cmake
  - tests/libretro/host.c
  - tests/libretro/tests.cmake
  - tests/retroarch/run_retroarch.cmake
  - tests/retroarch/test.cfg.in
  - tests/retroarch/verify.py
  - tests/runner/tests.cmake
covered_digest: "v4:sha256:f9cfbef170ec873a35a414ae21a3b745ae04c25f0bc5cddc7a47001d46e33683"
behavior_unverified: 0
overrides_applied: 0
decision_coverage:
  honored: 10
  total: 10
  not_honored: []
human_verification: []
---

# Phase 8: Per-game card saves Verification Report

**Phase Goal:** Players can use independent memory cards per game, import and export supported card files, and recover their progress after reopening the emulator.
**Verified:** 2026-11-25T01:56:17Z
**Status:** passed
**Re-verification:** No — the prior report had no `gaps:` section, so the verifier followed initial-mode rules.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Normalized serials give stable save names; a declared multi-disc set shares the first valid serial identity; invalid or absent serials use a versioned content digest. | ✓ VERIFIED | `host/save_game_identity.c` normalizes serials and resolves executable or full-disc-set digests; the disc digest covers ordered TOC fields, sectors and subcode. `runner/main.c` resolves once before applying the same identity to card paths. The registered runner identity and playlist checks cover spelling, first-member precedence, fallback changes, moved media and disc swaps. |
| 2 | Raw and supported GME, MEM and PSX card images round trip their 131,072 card bytes, while malformed imports and failed exports preserve prior data. | ✓ VERIFIED | `host/cards.c` validates lengths and wrapper signatures into a temporary image before changing caller memory, then publishes complete card images through a temporary file and atomic replace. `tests/card/files.c` checks byte equality for raw and wrapper formats, malformed input, unchanged output and prior-file preservation after flush/replace failures. `docs/runner.md` documents the supported formats. |
| 3 | Runner saves reload in a new process, keep games and card slots separate, and preserve the prior complete file after a failed write. | ✓ VERIFIED | `runner/main.c` loads each socket using the resolved game identity and only flushes changed card generations. Runner and guest integration tests check fresh-process reload, two-game/two-slot isolation and injected save failure against the existing file. |
| 4 | The libretro host exposes card 1 through `RETRO_MEMORY_SAVE_RAM`, persists other slots separately, resumes both cards in-process and across clean RetroArch processes, and grounds its save claim in tests. | ✓ VERIFIED | `retro_get_memory_data/size` maps SAVE_RAM to the live card 1 memory. Other sockets use content-derived paths in the frontend save directory and atomic card persistence. The metadata template sets `libretro_saves = "true"`; `tests/cmake/info_claims.cmake` requires both registered persistence and controls tests. The current required RetroArch job exercises two clean processes and asserts both 131,072-byte card files and advancing guest counters. |
| 5 | A real guest can read and write card storage through the documented PocketStation commands from `58h` onward. | ✓ VERIFIED | `src/card.c` handles bounded `58h`–`5Fh` protocol commands and stages validated storage writes before commit. The guest fixture transfers and checks card data through emulated JOY transactions; its registered integration selector is part of the current CI inventory. |
| 6 | The owned PocketStation guest fixture is built and included in the portable test inventory. | ✓ VERIFIED | `tests/fixtures/tests.cmake` builds `pocketstation_storage.exe` and registers `fixtures.pocketstation_storage`; `tests/fixtures/inventory.txt` and the fixture manifest include the required inputs. The required CI RetroArch job also carries the memory-card guest as an artifact. |
| 7 | Pinned protocol evidence sets the storage bounds and byte direction before the first storage implementation. | ✓ VERIFIED | The source transcript and byte-order probe are recorded in `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` and `DECISIONS.md`. Git history places the Phase 8 transcript pin at `8ae1c4b` before the first metadata implementation/test sequence, with the focused directory-order probe preceding the storage implementation. |
| 8 | Loading a state restores validated in-flight card transport fields without replacing card bytes or their generation. | ✓ VERIFIED | `src/card.c` serializes pending protocol state only; the card images and generation are instance-owned persistent data outside ordinary states. `card.state.pocketstation_metadata` and related state tests compare the receiving image and generation before and after state application, including rejected fields. |
| 9 | Phase acceptance checks preflight inputs and exact CTest registrations before expensive work. | ✓ VERIFIED | `08-09-PLAN.md` defines the required ordered checks. The current build’s CTest inventory contains the card, runner, libretro, PocketStation and persistence selectors; fixture registrations and artifact wiring were traced in source. The exact-head required CI run completed successfully. |
| 10 | Card protocol state transitions have deterministic tests before final verification. | ✓ VERIFIED | Registered card tests cover completed versus aborted writes, bad checksums, bounded PocketStation reads/writes, slot selection, state restore and card/generation preservation. Runner and libretro tests add fresh-process persistence and failed-publication cases. The current exact-head Main CI passed. |
| 11 | Integrated source review happened before verification. | ✓ VERIFIED | `08-REVIEW.md` records the reviewed implementation, test and CI files, resolved warnings, and no open findings at 2026-11-19T22:38:51Z. This report was verified later; source behavior and wiring were inspected again for this pass. |

**Score:** 11/11 truths verified (0 present, behavior-unverified)

### Required Artifacts

All **40/40 plan-declared artifacts** pass `gsd_run query verify.artifacts`: every file exists, has substantive content and has no reported stub/pattern issue.

| Plan | Declared artifacts | Status |
|---|---|---|
| 08-01 | `tests/cmake/info_claims.cmake`, `tests/cmake/info_claims_self_test.cmake`, fixture inventory, manifest, PocketStation guest source | ✓ VERIFIED (5/5) |
| 08-02 | `host/cards.c`, `host/cards.h`, `tests/card/files.c` | ✓ VERIFIED (3/3) |
| 08-03 | `docs/runner.md`, identity source/header, runner CMake and startup | ✓ VERIFIED (5/5) |
| 08-04 | Decisions and protocol source docs, embedding docs, card source/header | ✓ VERIFIED (5/5) |
| 08-05 | Libretro host source and its test harness/registration | ✓ VERIFIED (3/3) |
| 08-06 | Card source/header/tests and manifested PocketStation fixture/source | ✓ VERIFIED (5/5) |
| 08-07 | CI workflow and RetroArch script/config/verifier | ✓ VERIFIED (4/4) |
| 08-08 | Runner guide, disc probe, identity source and runner startup | ✓ VERIFIED (5/5) |
| 08-09 | Plan evidence summary, testing docs and fixture test registration | ✓ VERIFIED (3/3) |
| 08-10 | Claim evidence summary and libretro metadata template | ✓ VERIFIED (2/2) |

### Key Link Verification

The path-only key-link query directly verified 2/16 plan links. It reported lexical false negatives for the other 14 because CMake target lists, fixture macros and API calls do not contain the target path as a literal. I traced those links through the source, include/target wiring and registered selectors:

| # | From | To | Status | Evidence |
|---|---|---|---|---|
| 1 | `tests/fixtures/tests.cmake` | `tests/fixtures/src/pocketstation_storage.asm` | ✓ WIRED | Fixture macro assembles the source; `fixtures.pocketstation_storage` runs the produced executable. |
| 2 | `tests/cmake/info_claims.cmake` | `tests/libretro/inventory.txt` | ✓ WIRED | The registered claim test supplies the libretro inventory to the checker. |
| 3 | `host/cards.c` | `tests/card/files.c` | ✓ WIRED | The test target links `schwaystation-host`; file tests call the import/export API. |
| 4 | `runner/main.c` | `host/save_game_identity.c` | ✓ WIRED | Runner source list includes the resolver; startup calls it once before loading card paths. |
| 5 | `src/card.c` | `tests/card/card_tests.c` | ✓ WIRED | Card test executable links the core library and exercises JOY protocol/state behavior. |
| 6 | `libretro/libretro.c` | `host/cards.h` | ✓ WIRED | Libretro includes the host card API and imports/atomically persists card images. |
| 7 | `tests/libretro/host.c` | `libretro/libretro.c` | ✓ WIRED | Host test dynamically loads the libretro module and exercises SAVE_RAM and secondary slots. |
| 8 | `tests/fixtures/src/pocketstation_storage.asm` | `src/card.c` | ✓ WIRED | The assembled guest communicates through CPU/JOY register transactions and the emulated card protocol. |
| 9 | `src/card.c` | `tests/card/card_tests.c` | ✓ WIRED | Protocol and state selectors cover bounded fields, commit behavior and state application. |
| 10 | `.github/workflows/ci.yml` | `tests/retroarch/run_retroarch.cmake` | ✓ WIRED | Required hosted RetroArch step invokes the script in `MODE=cards`. |
| 11 | `runner/main.c` | `host/save_game_identity.c` | ✓ WIRED | Runner resolves identity before either card slot is loaded or paths are frozen. |
| 12 | `host/save_game_identity.c` | `host/disc_image.h` | ✓ WIRED | Resolver walks selected playlist members and their ordered TOC/sector providers. |
| 13 | `tests/fixtures/tests.cmake` | `src/card.c` | ✓ WIRED | The guest fixture runs against the runner/core target containing the card implementation. |
| 14 | `.github/workflows/ci.yml` | `tests/fixtures/src/memcard_roundtrip.asm` | ✓ WIRED | CI builds/uploads the guest, downloads it in the RetroArch job and checks that it exists before use. |
| 15 | `libretro/schwaystation_libretro.info.in` | `tests/cmake/info_claims.cmake` | ✓ WIRED | Generated metadata is checked; a true saves claim requires both card persistence and controls tests. |
| 16 | `.github/workflows/ci.yml` | `tests/retroarch/run_retroarch.cmake` | ✓ WIRED | Exact-head CI uses the same required two-process card path as Plan 08-07. |

### Data-Flow Trace (Level 4)

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| Card import/export | 131,072-byte card image | Validated disk image or live card memory; atomic file publisher | Yes | ✓ FLOWING |
| Runner identity and paths | Serial/digest component | Executable bytes or ordered provider TOC, sectors and subcode | Yes | ✓ FLOWING |
| Runner reload/flush | Socket image and generation | Save file import; changed instance card memory | Yes | ✓ FLOWING |
| Libretro card 1 | SAVE_RAM span | Live instance-owned card 1 memory exposed to frontend | Yes | ✓ FLOWING |
| Libretro other slots | Per-content save paths | Frontend save directory and live instance card memory | Yes | ✓ FLOWING |
| PocketStation guest | Command bytes and card storage | Guest JOY transactions through the emulator's card implementation | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command/evidence | Result | Status |
|---|---|---|---|
| Current-source integrated CI, including registered card, runner, libretro, guest and portability checks | [Main CI run 45442543076](https://github.com/szTheory/schwaystation/actions/runs/45442543076), source head `75beb849f4b9e8a9a3bd023cc5c80632e4c74d56) | Passed | ✓ PASS |
| Released-tree regression check on the current repository HEAD | [Post-merge Main CI run 45455115354](https://github.com/szTheory/schwaystation/actions/runs/45455115354), head `f08fc08d40a1ebcb42708e0a31be3111e3eed711` | Passed | ✓ PASS |
| Required RetroArch save continuation and release workflow on the released head | [Release run 45455115400](https://github.com/szTheory/schwaystation/actions/runs/45455115400), head `f08fc08d40a1ebcb42708e0a31be3111e3eed711` | Passed | ✓ PASS |
| Relevant current CTest registrations | `ctest --test-dir build/ci -N` with card, runner, libretro, PocketStation and memory-card fixture selectors | All required selectors are registered; this enumerated tests without rerunning them | ✓ PASS |

No full suite was rerun for this report. The supplied exact-head Main/Release results cover the current code; the current working tree is at the released `f08fc08d40a1ebcb42708e0a31be3111e3eed711` head.

### Probe Execution

No probe scripts are declared or implied by the Phase 8 plans.

### Requirements Coverage

| Requirement | Source plans | Status | Evidence |
|---|---|---|---|
| SAVE-01 | 08-01, 08-02, 08-03, 08-08, 08-09 | ✓ SATISFIED | Normalized serials and executable/disc-set fallback identities are wired to card paths and covered by runner/card tests. |
| SAVE-02 | 08-02, 08-09 | ✓ SATISFIED | Raw and supported wrapper validation, byte-preserving import/export, atomic publication and failure preservation are implemented and tested. |
| SAVE-03 | 08-01, 08-02, 08-03, 08-08, 08-09 | ✓ SATISFIED | Runner reloads both slots, keeps game identities separate and preserves old files after a failed publish. |
| SAVE-04 | 08-01, 08-05, 08-07, 08-09, 08-10 | ✓ SATISFIED | SAVE_RAM mapping, other-slot persistence, claim checks and fresh-process RetroArch checks are wired and passed. |
| SAVE-05 | 08-01, 08-04, 08-06, 08-09 | ✓ SATISFIED | Documented bounded PocketStation commands, state behavior, guest fixture and deterministic tests are present and passed. |

**Coverage:** 5/5 Phase 8 requirements satisfied; no orphaned Phase 8 requirements.

### Test Quality Audit

| Test files | Linked requirement | Active/disabled | Circular | Assertion strength | Verdict |
|---|---|---|---|---|---|
| `tests/card/files.c` and `tests/card/tests.cmake` | SAVE-02 | Active; no disabled linked tests | No circular expected-value writer found | Byte-for-byte equality and unchanged-state assertions | ✓ PASS |
| `tests/runner/tests.cmake` and registered runner selectors | SAVE-01, SAVE-03 | Active; no disabled linked tests | Generated input fixtures are checked against fixed expected paths, hashes and bytes | Multi-run value and failure-preservation assertions | ✓ PASS |
| `tests/libretro/host.c`, `tests/libretro/tests.cmake` | SAVE-04 | Active; no disabled linked tests | No circular expected-value writer found | SAVE_RAM size/data and per-slot contents checked | ✓ PASS |
| `tests/fixtures/src/pocketstation_storage.asm`, `tests/fixtures/tests.cmake` | SAVE-05 | Active; no disabled linked tests | Guest checks its bounded transactions and return status | Byte-level guest result and expected exit status | ✓ PASS |
| `tests/cmake/info_claims.cmake`, `tests/cmake/info_claims_self_test.cmake` | SAVE-04 | Active; no disabled linked tests | Negative controls mutate only the checker input | Required inventory rows and invalid claims asserted | ✓ PASS |

Disabled-test patterns were not found in Phase 8 requirement-linked test paths. The card, runner and guest tests compare against fixed independent byte values, path expectations and guest-visible results; no system-generated expected fixture was identified.

### Decision Coverage

All 10 trackable decisions in `08-CONTEXT.md` are honored by the shipped artifacts. The decision-coverage check was non-blocking and reported no unhonored decisions.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|

No implementation stubs, unreferenced `TBD`/`FIXME`/`XXX` debt markers, empty production paths or user-visible placeholder outputs were found in the Phase 8 implementation and test files. A documentation reference to placeholder registrations describes a configuration case and does not represent an empty implementation.

### Human Verification Required

None. The user-facing runner and host save flows have deterministic integration coverage, and the real RetroArch continuation path is covered by required hosted CI. No hardware-only result is part of the Phase 8 goal.

### Gaps Summary

No gaps found. All five roadmap success criteria, the six distinct plan-specific truths, all 40 declared artifacts, all 16 key links and all five SAVE requirements are verified on the released source tree.

---

_Verified: 2026-11-25T01:56:17Z_
_Verifier: the gsd-verifier agent_
