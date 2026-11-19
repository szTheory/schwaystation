---
status: complete
phase: 08-per-game-card-saves
source:
  - 08-01-SUMMARY.md
  - 08-02-SUMMARY.md
  - 08-03-SUMMARY.md
  - 08-04-SUMMARY.md
  - 08-05-SUMMARY.md
  - 08-06-SUMMARY.md
  - 08-07-SUMMARY.md
  - 08-08-SUMMARY.md
  - 08-09-SUMMARY.md
  - 08-10-SUMMARY.md
started: 2026-11-19T23:08:54Z
updated: 2026-11-19T23:08:54Z
---

## Current Test

[testing complete]

## Tests

### 1. Per-game save identity
expected: Normalized serials and declared disc sets resolve stable save names; content-derived fallbacks distinguish otherwise unidentified content.
result: pass
source: automated
evidence: runner.disc.load, runner.cards.controls and runner.cards.persistence passed in the fresh 24-test Phase 8 selector; the runner resolves identity before loading either slot.

### 2. Card import, export and failure safety
expected: Raw and supported wrapped cards preserve the complete card image, while malformed input and failed writes leave active data and the prior complete file intact.
result: pass
source: automated
evidence: card.files.atomic_roundtrip, card.files.atomic_failures, card.files.wrappers and card.files.wrapper_edges passed in the fresh 24-test Phase 8 selector.

### 3. Runner continuation and slot isolation
expected: A later runner process resumes both card slots without crossing games or publishing partial files.
result: pass
source: automated
evidence: runner.cards.persistence, runner.cards.sigterm and fixtures.memcard_roundtrip.persistence passed; the fixture runs separate runner processes and checks both slot values.

### 4. Libretro save continuation
expected: The frontend receives card 1 through SAVE_RAM, card 2 remains separate, and the core advertises save support only with its tested path.
result: pass
source: automated
evidence: libretro.cards.persistence, libretro.cards.controls, libretro.info.claims and libretro.info.claims.self_test passed locally. Required hosted CI run 44618451958 passed the full matrix and two-process RetroArch card continuation; artifact 14576843021 records both 128 KiB slots resuming over two 120-frame processes.

### 5. PocketStation storage guest
expected: A guest can issue the documented storage commands and read back card data through the bounded card transport.
result: pass
source: automated
evidence: card.protocol.pocketstation_metadata, card.protocol.write_commit, card.protocol.write_edges, card.state.pocketstation_metadata and fixtures.pocketstation_storage passed.

## Summary

total: 5
passed: 5
issues: 0
pending: 0
skipped: 0

## Automation Note

No owner-operated UAT was requested or needed under the project's automation-first verification policy. These five phase-level checks record behavior exercised by deterministic tests and the hosted RetroArch run. The GSD coverage classifier accepted the first seven plan summaries as fully automated; for Plans 08-08 through 08-10 it reported verification_not_list because it expects an array, while this repository's AGENTS.md explicitly defines verification as a single object with kind, ref and status. The required test refs and hosted evidence were traced and checked directly instead of creating human checkpoints.
