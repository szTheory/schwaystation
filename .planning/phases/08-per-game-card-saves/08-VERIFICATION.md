---
phase: 08-per-game-card-saves
verified: 2026-11-19T23:04:05Z
status: passed
score: 5/5 must-haves verified
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
covered_digest: "v4:sha256:d53f1b7a14cb27a7df5eb845513bcd472082fbe08a620cd1b591b339d7607cfb"
behavior_unverified: 0
overrides_applied: 0
human_verification: []
ship_evidence_pending:
  - "Phase PR CI and the release PR/main CI remain ship-time gates."
---

# Phase 8: Per-game card saves Verification Report

**Phase Goal:** Players can use independent memory cards per game, import and export supported card files, and recover their progress after reopening the emulator.
**Verified:** 2026-11-19T23:04:05Z
**Status:** passed
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | Normalized serials produce stable save names, declared multi-disc sets share identity, and invalid or missing serials use distinct content-derived fallbacks. | ✓ VERIFIED | Runner identity is resolved before card paths are applied. The executable fallback hashes validated bytes; the disc fallback frames media type and hashes ordered TOC metadata and provider sector/subcode data. Runner tests cover serial precedence, moved media, playlist identity, disc swaps and fallback changes. |
| 2 | Raw, GME, MEM and PSX card files round trip with byte-identical card contents; invalid input leaves active data and stored files unchanged. | ✓ VERIFIED | Host import validates size and wrapper signatures before copying into the output. Export writes a temporary complete image and atomically replaces the destination. The card file tests cover wrappers, byte preservation, malformed input and failed publication. |
| 3 | Runner saves reload across processes, isolate games and both slots, and preserve complete files after write failure. | ✓ VERIFIED | Fresh-process fixture tests exercise both card slots and separate identities; controls cover failed writes and refused input. |
| 4 | Libretro exposes and reloads card 1 through SAVE_RAM, persists additional slots separately, passes in-process and hosted fresh-process continuation, and advertises saves only after verification. | ✓ VERIFIED | The API maps RETRO_MEMORY_SAVE_RAM to card 1; generated metadata says libretro_saves true and the claim checker maps that claim to persistence and controls tests. Local selectors pass. Hosted run 44618451958 passed the full required matrix and RetroArch card step on the claim-true implementation head; its artifact records two processes, 120 frames each, two 131,072-byte card images and advancing counters. |
| 5 | A guest can use documented PocketStation storage commands from 58h onward to read and write card data. | ✓ VERIFIED | The protocol implementation and pinned transcript are exercised by unit tests and the manifested PocketStation guest. The guest fixture selector passed. |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
|---|---|---|---|
| host/cards.c and host/cards.h | Validated card import, exact wrapper handling and atomic full-image export | ✓ VERIFIED | Implementations are used by runner and libretro hosts; card.files round-trip and failure tests passed. |
| host/save_game_identity.c and runner/main.c | Stable executable/disc identity applied before either runner card slot loads | ✓ VERIFIED | Runner source resolves one identity before loading both paths; runner CMake includes the identity implementation and the runner persistence selectors passed. |
| src/card.c | Bounded two-slot card transport and PocketStation storage protocol | ✓ VERIFIED | JOY protocol, state and write-commit tests passed; the guest exercised the public transport. |
| libretro/libretro.c and schwaystation_libretro.info.in | SAVE_RAM mapping, additional-slot persistence and grounded save claim | ✓ VERIFIED | The host tests and metadata claim/refusal self-test passed; the exact-head hosted RetroArch test passed. |
| tests/fixtures/src/pocketstation_storage.asm and fixture registration | Reproducible guest storage transaction | ✓ VERIFIED | The source is manifested, assembled through the fixture helper and registered as fixtures.pocketstation_storage; that test passed. |
| CI RetroArch path | Required fresh-process card continuation proof | ✓ VERIFIED | The required job is wired to the cards mode and artifact; exact-head run 44618451958 passed. |

**Artifacts:** All 40 plan-declared artifacts passed the GSD artifact checks across Plans 08-01 through 08-10.

### Key Link Verification

| From | To | Via | Status | Details |
|---|---|---|---|---|
| runner/main.c | host/save_game_identity.c | runner source list and one-time startup resolution | ✓ WIRED | CMake includes the identity implementation; runner startup calls the resolver before loading either slot. |
| host/cards.c | tests/card/files.c | host library linked into card-file test target | ✓ WIRED | tests/card/tests.cmake links the test executable to schwaystation-host; four card-file selectors passed. |
| tests/cmake/info_claims.cmake | tests/libretro/inventory.txt | inventory passed by the registered CTest | ✓ WIRED | tests/libretro/tests.cmake supplies the inventory path; the claim checker requires both card persistence and controls rows. |
| tests/fixtures/tests.cmake | PocketStation guest source | fixture macro, manifest and registered guest selector | ✓ WIRED | The assembly source is assembled and the guest test is registered; its test passed. |
| libretro/libretro.c | host/cards.h and frontend SAVE_RAM | import/export helpers and memory mapping | ✓ WIRED | The implementation includes the host card API and maps SAVE_RAM to card 1; in-process and hosted persistence checks passed. |
| .github/workflows/ci.yml | tests/retroarch/run_retroarch.cmake | required RetroArch guest artifact and cards mode | ✓ WIRED | Workflow regression checks and the exact-head hosted card step passed. |

The generic key-link checker reported lexical false negatives for several CMake-variable and target-list relationships. Those links were traced through the CMake source lists and test registrations above; the registered behavioral selectors passed.

### Data-Flow Trace

| Artifact | Data variable | Source | Produces real data | Status |
|---|---|---|---|---|
| host card import/export | 128 KiB card image | Input file or caller-provided card bytes | Yes | ✓ FLOWING |
| runner save identity | identity component | Serial metadata or executable/disc content | Yes | ✓ FLOWING |
| libretro card 1 | SAVE_RAM span | Live instance card memory | Yes | ✓ FLOWING |
| libretro card 2 | save image | Live instance card memory, atomically persisted by host | Yes | ✓ FLOWING |
| PocketStation guest | protocol response and card bytes | Guest transactions through the emulated card transport | Yes | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
|---|---|---|---|
| Phase 8 identity, file validation, atomic writes, protocol, runner, libretro and fixture regressions | cmake --preset ci && ctest --preset ci --no-tests=error -R '^(abi[.]api_v4|card[.]|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))' --output-on-failure | Configure exit 0; 24/24 tests passed in 5.13 seconds | ✓ PASS |
| Real RetroArch fresh-process continuation | Required hosted CI run 44618451958 | Full required matrix passed; two independent 120-frame processes resumed both card slots; artifact 14576843021 | ✓ PASS |
| Save capability claim | libretro.info.claims and libretro.info.claims.self_test | Passed in the 24-test selector; claim maps to both required card tests | ✓ PASS |

### Probe Execution

No probes are declared or implied by the Phase 8 plans.

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|---|---|---|---|---|
| SAVE-01 | 08-01, 08-03, 08-08, 08-09 | Stable per-game identity, multi-disc identity and content-derived fallback | ✓ SATISFIED | Runner identity tests and the 24-test selector passed. |
| SAVE-02 | 08-02, 08-09 | Validated raw/wrapped import, byte-preserving export and atomic failure behavior | ✓ SATISFIED | Card file wrapper, round-trip and failure selectors passed. |
| SAVE-03 | 08-01, 08-03, 08-08, 08-09 | Runner reload, two-game/two-slot isolation and complete-file preservation | ✓ SATISFIED | Runner and fresh-process guest selectors passed. |
| SAVE-04 | 08-01, 08-05, 08-07, 08-09, 08-10 | Libretro card APIs, two-slot persistence and verified capability claim | ✓ SATISFIED | Local host selectors and exact-head hosted CI/RetroArch proof passed. |
| SAVE-05 | 08-01, 08-04, 08-06, 08-09 | Documented PocketStation storage protocol and guest round trip | ✓ SATISFIED | Protocol/state tests and the PocketStation fixture passed. |

**Coverage:** 5/5 requirements satisfied; no orphaned Phase 8 requirements.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|---|---:|---|---|---|

No unresolved TODO, FIXME, XXX, TBD, placeholder or implementation-stub markers were found in the scanned changed source, test and workflow files.

### Human Verification Required

None — the Phase 8 behavior is covered by deterministic local tests and automated hosted RetroArch checks. No owner-hardware check is required.

### Gaps Summary

No gaps found. Phase 8 meets all five roadmap success criteria and all five SAVE requirements. Phase PR CI and release PR/main CI remain ship-time gates.

### CI Efficiency Review

The exact-head claim-true run completed the required matrix; the BE32 leg remained the critical path at about 65m39s. The stable-state-size preflight ran before the long Linux build/test job and completed in about nine seconds. The report-only perf job skipped on a manual dispatch without a comparison ref; PR perf gating remains enabled. A duplicate act run was not useful because the local CMake workflow and exact-head hosted matrix already covered the reproducible and hosted-only portions. No additional coverage-preserving speed change was supported by these runs; the measured bottleneck and existing preflight are recorded for future CI work.

## Verification Metadata

**Verification approach:** Goal-backward against the five Phase 8 roadmap success criteria, all plan must-haves, and SAVE-01 through SAVE-05.
**Automated checks:** 24/24 focused tests passed in 5.13 seconds; plan artifact checks passed 40/40; the exact-head required hosted matrix and RetroArch continuation step passed.
**Human checks required:** None.
**Total verification time:** Approximately 12 minutes including source tracing and report generation; the focused local command took 5.13 seconds.

---
Verified: 2026-11-19T23:04:05Z
Verifier: Codex, applying the gsd-verifier workflow inline during recovery
