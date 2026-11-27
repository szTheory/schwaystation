---
status: complete
phase: 16-patches-cheats-and-compatibility-evidence
source: [16-01-SUMMARY.md, 16-02-SUMMARY.md, 16-03-SUMMARY.md, 16-04-SUMMARY.md, 16-05-SUMMARY.md, 16-06-SUMMARY.md, 16-07-SUMMARY.md, 16-08-SUMMARY.md, 16-09-SUMMARY.md, 16-10-SUMMARY.md, 16-11-SUMMARY.md, 16-12-SUMMARY.md, 16-13-SUMMARY.md, 16-14-SUMMARY.md, 16-15-SUMMARY.md, 16-16-SUMMARY.md, 16-17-SUMMARY.md, 16-18-SUMMARY.md]
started: 2026-11-26T18:47:18.789Z
updated: 2026-11-26T18:48:36.542Z
---

## Current Test

[testing complete]

## Tests

### 1. The released API v10 caller reads memory and survives a real frame.
expected: The released API v10 caller reads memory and survives a real frame.
result: pass
source: automated
coverage_id: D1
summary: 16-01-SUMMARY.md
evidence: integration: ctest --preset ci --no-tests=error -R '^(abi[.]api_v10)$' --output-on-failure

### 2. Patch and fuzz regression selectors are registered exactly once in sorted inventories.
expected: Patch and fuzz regression selectors are registered exactly once in sorted inventories.
result: pass
source: automated
coverage_id: D2
summary: 16-01-SUMMARY.md
evidence: other: Phase 16 Plan 01 Task 3 CTest selector enumeration and six-inventory sort preflight

### 3. Runner cheat selectors expose the released memory-read baseline and leave future parser cases visibly failing.
expected: Runner cheat selectors expose the released memory-read baseline and leave future parser cases visibly failing.
result: pass
source: automated
coverage_id: D3
summary: 16-01-SUMMARY.md
evidence: integration: ctest --preset ci --no-tests=error -R '^runner[.]cheats[.]memory_read$' --output-on-failure; other: Phase 16 Plan 01 Task 2 exact selector enumeration

### 4. DAT inventory accounting selectors enumerate exactly once in the compatibility inventory.
expected: DAT inventory accounting selectors enumerate exactly once in the compatibility inventory.
result: pass
source: automated
coverage_id: D4
summary: 16-01-SUMMARY.md
evidence: other: Phase 16 Plan 01 Task 3 CTest selector enumeration and six-inventory sort preflight

### 5. Required-capability evaluation selectors enumerate exactly once in the compatibility inventory.
expected: Required-capability evaluation selectors enumerate exactly once in the compatibility inventory.
result: pass
source: automated
coverage_id: D5
summary: 16-01-SUMMARY.md
evidence: other: Phase 16 Plan 01 Task 3 CTest selector enumeration and six-inventory sort preflight

### 6. Hosts can write checked little-endian byte and halfword values to physical main RAM without advancing ticks.
expected: Hosts can write checked little-endian byte and halfword values to physical main RAM without advancing ticks.
result: pass
source: automated
coverage_id: D1
summary: 16-02-SUMMARY.md
evidence: unit: tests/api/v11.c check_ram_write_api via abi.api_v11

### 7. Released API v9 and v10 input callers preserve their layouts and behavior under API v11.
expected: Released API v9 and v10 input callers preserve their layouts and behavior under API v11.
result: pass
source: automated
coverage_id: D2
summary: 16-02-SUMMARY.md
evidence: unit: abi.api_v9; unit: abi.api_v10; unit: abi.api_v11

### 8. The shared parser accepts only bounded 30 byte and 80 halfword writes and applies enabled entries through the checked RAM API.
expected: The shared parser accepts only bounded 30 byte and 80 halfword writes and applies enabled entries through the checked RAM API.
result: pass
source: automated
coverage_id: cheat-parser-checked-write
summary: 16-03-SUMMARY.md
evidence: unit: runner.cheats.parser, runner.cheats.bounds, runner.cheats.order (final consolidated CTest selector run); unit: abi.api_v11 (final consolidated CTest selector run)

### 9. Canonical cheat bytes are stable across equivalent text, preserve order and enabled state, and reject malformed envelopes without publishing them.
expected: Canonical cheat bytes are stable across equivalent text, preserve order and enabled state, and reject malformed envelopes without publishing them.
result: pass
source: automated
coverage_id: cheat-canonical-state
summary: 16-03-SUMMARY.md
evidence: unit: runner.cheats.order canonical encode/decode assertions (final consolidated CTest selector run); unit: docs.budgets (final consolidated CTest selector run)

### 10. Libretro callbacks apply supported byte and halfword writes at the frame boundary.
expected: Libretro callbacks apply supported byte and halfword writes at the frame boundary.
result: pass
source: automated
coverage_id: callback-application
summary: 16-04-SUMMARY.md
evidence: integration: libretro.cheats.apply focused CTest selector

### 11. Rejection, ordering, disabled entries, aggregate limits, reset, and content lifecycle preserve the documented contract.
expected: Rejection, ordering, disabled entries, aggregate limits, reset, and content lifecycle preserve the documented contract.
result: pass
source: automated
coverage_id: callback-controls
summary: 16-04-SUMMARY.md
evidence: integration: libretro.cheats.controls focused CTest selector

### 12. Libretro state preserves normalized cheat definitions, order, and enable state across continuation and rejects malformed states atomically.
expected: Libretro state preserves normalized cheat definitions, order, and enable state across continuation and rejects malformed states atomically.
result: pass
source: automated
coverage_id: D1
summary: 16-05-SUMMARY.md
evidence: integration: cmake --preset ci; build libretro and core state test targets; CTest selectors libretro.cheats.apply, libretro.cheats.controls, libretro.cheats.state, core.state.round_trip, core.state.rejects

### 13. The runner accepts bounded repeated cheat codes, rejects invalid lists before side effects, and applies writes at frame start.
expected: The runner accepts bounded repeated cheat codes, rejects invalid lists before side effects, and applies writes at frame start.
result: pass
source: automated
coverage_id: D1
summary: 16-06-SUMMARY.md
evidence: integration: runner.cheats.cli; unit: runner.cheats.parser and runner.cheats.order

### 14. Configured normalized cheat continuation changes runner state hashes while no-cheat state pins remain unchanged.
expected: Configured normalized cheat continuation changes runner state hashes while no-cheat state pins remain unchanged.
result: pass
source: automated
coverage_id: D2
summary: 16-06-SUMMARY.md
evidence: integration: runner.cheats.hash and runner.hashes_file; integration: fixtures.gpu_motion.state.150 and fixtures.gpu_motion.state.180

### 15. Apply identity-bound PPF1 patches to an exact raw-track mapping without modifying source media.
expected: Apply identity-bound PPF1 patches to an exact raw-track mapping without modifying source media.
result: pass
source: automated
coverage_id: D1
summary: 16-07-SUMMARY.md
evidence: unit: ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]mapping|disc[.]patch[.]ppf1|disc[.]patch[.]atomic)$' --output-on-failure

### 16. PPF2 patches validate source identity, exact source size, native block bytes, and 32-bit record bounds before overlay creation.
expected: PPF2 patches validate source identity, exact source size, native block bytes, and 32-bit record bounds before overlay creation.
result: pass
source: automated
coverage_id: D1
summary: 16-08-SUMMARY.md
evidence: unit: ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]ppf2)$' --output-on-failure

### 17. PPF3 patches validate BIN headers, optional native block bytes, 64-bit record bounds, undo payloads, and bounded FileID trailers without partial application.
expected: PPF3 patches validate BIN headers, optional native block bytes, 64-bit record bounds, undo payloads, and bounded FileID trailers without partial application.
result: pass
source: automated
coverage_id: D2
summary: 16-08-SUMMARY.md
evidence: unit: ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]ppf1|disc[.]patch[.]ppf2|disc[.]patch[.]ppf3|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' --output-on-failure

### 18. All four BPS1 actions produce a complete deterministic target before the image can publish it.
expected: All four BPS1 actions produce a complete deterministic target before the image can publish it.
result: pass
source: automated
coverage_id: D1
summary: 16-09-SUMMARY.md
evidence: unit: ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]bps)$' --output-on-failure

### 19. Malformed BPS sizes, cursors, varints, CRCs, and staging I/O preserve source bytes and the active overlay.
expected: Malformed BPS sizes, cursors, varints, CRCs, and staging I/O preserve source bytes and the active overlay.
result: pass
source: automated
coverage_id: D2
summary: 16-09-SUMMARY.md
evidence: unit: ctest --preset ci --no-tests=error -R '^(disc[.]patch[.]bps|disc[.]patch[.]atomic|disc[.]patch[.]bounds)$' --output-on-failure

### 20. The runner validates and applies one supported PPF or BPS patch before emulation and reports the checks performed.
expected: The runner validates and applies one supported PPF or BPS patch before emulation and reports the checks performed.
result: pass
source: automated
coverage_id: D1
summary: 16-10-SUMMARY.md
evidence: integration: runner.patch.cli

### 21. Patch candidate mapping, format validation, bounds and atomic rejection remain covered by generated provider tests.
expected: Patch candidate mapping, format validation, bounds and atomic rejection remain covered by generated provider tests.
result: pass
source: automated
coverage_id: D2
summary: 16-10-SUMMARY.md
evidence: unit: disc.patch.mapping, disc.patch.ppf1, disc.patch.ppf2, disc.patch.ppf3, disc.patch.bps, disc.patch.atomic, disc.patch.bounds

### 22. CLI usage, source identity policy, disc mapping limits and refusal statuses match the runner implementation.
expected: CLI usage, source identity policy, disc mapping limits and refusal statuses match the runner implementation.
result: pass
source: automated
coverage_id: D3
summary: 16-10-SUMMARY.md
evidence: other: Reviewed docs/runner.md, docs/patches.md and docs/discs.md against runner/main.c and passing CLI coverage.

### 23. Deterministic patch parser replay and bounded fuzz entry point for PPF1/2/3 and BPS inputs.
expected: Deterministic patch parser replay and bounded fuzz entry point for PPF1/2/3 and BPS inputs.
result: pass
source: automated
coverage_id: D1
summary: 16-11-SUMMARY.md
evidence: unit: fuzz.regress.ppf

### 24. Deterministic cheat replay for transactional replacement, caps, canonical bytes, and bounded RAM writes.
expected: Deterministic cheat replay for transactional replacement, caps, canonical bytes, and bounded RAM writes.
result: pass
source: automated
coverage_id: D2
summary: 16-11-SUMMARY.md
evidence: unit: fuzz.regress.cheat

### 25. Nightly routing preserves prior campaigns and runs PPF/cheat campaigns with exact seed metadata.
expected: Nightly routing preserves prior campaigns and runs PPF/cheat campaigns with exact seed metadata.
result: pass
source: automated
coverage_id: D3
summary: 16-11-SUMMARY.md
evidence: integration: act --dryrun --container-architecture linux/arm64 --action-cache-path build/ci/fuzz-act-cache -P ubuntu-26.04=ubuntu:latest push -W .github/workflows/nightly.yml -j fuzz

### 26. The pinned Redump DAT is accounted for by source occurrence, with untested entries remaining unknown.
expected: The pinned Redump DAT is accounted for by source occurrence, with untested entries remaining unknown.
result: pass
source: automated
coverage_id: D1
summary: 16-12-SUMMARY.md
evidence: integration: build/ci/schwaystation-run --library-inventory $DAT_TMP_FILE after the D-06 SHA-256 check

### 27. Synthetic tests cover missing/repeated/encoded serials, occurrence reports, exact evidence joins, malformed inputs, and bounds.
expected: Synthetic tests cover missing/repeated/encoded serials, occurrence reports, exact evidence joins, malformed inputs, and bounds.
result: pass
source: automated
coverage_id: D2
summary: 16-12-SUMMARY.md
evidence: unit: ctest --preset ci --no-tests=error -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls|compat[.]library[.]synthetic_result|compat[.]library[.]controls)$' --output-on-failure

### 28. The reproduction guide states the data boundary and that inventory counts do not establish playability.
expected: The reproduction guide states the data boundary and that inventory counts do not establish playability.
result: pass
source: automated
coverage_id: D3
summary: 16-12-SUMMARY.md
evidence: other: docs/compatibility-inventory.md

### 29. The exact licensed Redump source is pinned and its 13,407 occurrences reconcile to aggregate status and serial counts.
expected: The exact licensed Redump source is pinned and its 13,407 occurrences reconcile to aggregate status and serial counts.
result: pass
source: automated
coverage_id: D1
summary: 16-13-SUMMARY.md
evidence: integration: gh api pinned commit/license/blob checks; shasum -a 256 build/ci/metadata/Sony-PlayStation.dat; build/ci/schwaystation-run --library-inventory build/ci/metadata/Sony-PlayStation.dat

### 30. Normal library processing validates cached data and reports aggregate evidence without publishing entry titles or serials.
expected: Normal library processing validates cached data and reports aggregate evidence without publishing entry titles or serials.
result: pass
source: automated
coverage_id: D2
summary: 16-13-SUMMARY.md
evidence: unit: ctest --preset ci --no-tests=error -R '^(compat[.]inventory[.]accounting|compat[.]inventory[.]controls|compat[.]library[.]synthetic_result|compat[.]library[.]controls)$' --output-on-failure; other: docs/compatibility-inventory.md

### 31. The required capability matrix covers providers, profiles, peripheral classes, card persistence, patches and cheats.
expected: The required capability matrix covers providers, profiles, peripheral classes, card persistence, patches and cheats.
result: pass
source: automated
coverage_id: D1
summary: 16-14-SUMMARY.md
evidence: integration: compat.capabilities.coverage

### 32. The evaluator and staging hook keep the experimental marker when current required evidence is incomplete or fails.
expected: The evaluator and staging hook keep the experimental marker when current required evidence is incomplete or fails.
result: pass
source: automated
coverage_id: D2
summary: 16-14-SUMMARY.md
evidence: integration: compat.capabilities.results and compat.capabilities.claims

### 33. Staged libretro metadata exposes the tested GameShark cheat subset.
expected: Staged libretro metadata exposes the tested GameShark cheat subset.
result: pass
source: automated
coverage_id: D3
summary: 16-14-SUMMARY.md
evidence: integration: libretro.info.claims and libretro.cheats.state

### 34. Main CI supplies protected feature and capability rows to the compatibility check.
expected: Main CI supplies protected feature and capability rows to the compatibility check.
result: pass
source: automated
coverage_id: D1
summary: 16-15-SUMMARY.md
evidence: integration: compat.ratchet.protected_controls and workflow registration

### 35. Historical feature protection and current package evidence use separate inputs.
expected: Historical feature protection and current package evidence use separate inputs.
result: pass
source: automated
coverage_id: D2
summary: 16-15-SUMMARY.md
evidence: integration: compat.features.ratchet and compat.capabilities.claims

### 36. The generated report reconciles the pinned inventory and summarizes current required capability results.
expected: The generated report reconciles the pinned inventory and summarizes current required capability results.
result: pass
source: automated
coverage_id: D1
summary: 16-16-SUMMARY.md
evidence: integration: policy.compat_doc_fresh and build-local compatibility report

### 37. The report leaves boot-to-play gaps explicit and describes supported patch and cheat predicates.
expected: The report leaves boot-to-play gaps explicit and describes supported patch and cheat predicates.
result: pass
source: automated
coverage_id: D2
summary: 16-16-SUMMARY.md
evidence: integration: compat.capabilities.coverage, compat.capabilities.results and compat.features.ratchet

### 38. Public API and host documentation describe the tested patch and cheat behavior.
expected: Public API and host documentation describe the tested patch and cheat behavior.
result: pass
source: automated
coverage_id: D3
summary: 16-16-SUMMARY.md
evidence: integration: abi.api_v11, runner.cheats.cli, runner.cheats.hash and libretro.cheats.state

### 39. Historical patch feature rows name the exercised formats and validation predicates.
expected: Historical patch feature rows name the exercised formats and validation predicates.
result: pass
source: automated
coverage_id: D4
summary: 16-16-SUMMARY.md
evidence: integration: disc.patch.ppf1, disc.patch.ppf2, disc.patch.ppf3, disc.patch.bps and runner.patch.cli

### 40. Both long tests and the existing BE32 job have recorded timings and an exact proposed selector partition.
expected: Both long tests and the existing BE32 job have recorded timings and an exact proposed selector partition.
result: pass
source: automated
coverage_id: D1
summary: 16-17-SUMMARY.md
evidence: other: .planning/todos/pending/052-be32-movie-test-shard.md

### 41. Patch targets and runner choices operate on the affected native and WASI paths.
expected: Patch targets and runner choices operate on the affected native and WASI paths.
result: pass
source: automated
coverage_id: D1
summary: 16-18-SUMMARY.md
evidence: integration: 16-LOCAL-EVIDENCE.md

### 42. Installed C and C++ callers can use API v11 and the affected cheat behavior is exercised.
expected: Installed C and C++ callers can use API v11 and the affected cheat behavior is exercised.
result: pass
source: automated
coverage_id: D2
summary: 16-18-SUMMARY.md
evidence: integration: abi.api_v11, installed consumers and focused runner/libretro checks

### 43. Inventory, generated documentation and metadata retain their declared claims and bounds.
expected: Inventory, generated documentation and metadata retain their declared claims and bounds.
result: pass
source: automated
coverage_id: D3
summary: 16-18-SUMMARY.md
evidence: integration: 16-LOCAL-EVIDENCE.md

### 44. Local measurement and package staging establish current capability evidence and preserve unknown play results.
expected: Local measurement and package staging establish current capability evidence and preserve unknown play results.
result: pass
source: automated
coverage_id: D4
summary: 16-18-SUMMARY.md
evidence: integration: 16-LOCAL-EVIDENCE.md and 16-REVIEW-FIX.md

## Summary

total: 44
passed: 44
issues: 0
pending: 0
skipped: 0

## Gaps

None. All 44 rows were sourced from passing automated verification evidence; no owner UAT is required by project policy.
