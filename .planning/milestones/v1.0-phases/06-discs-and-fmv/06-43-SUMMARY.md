---
phase: 06-discs-and-fmv
plan: "43"
subsystem: testing
tags: [mdec, ctest, nightly, selector-inventory]
requires: ["06-42"]
provides:
  - "The same 12 surviving MDEC nightly selectors and labels are listed in normal nightly and both media-only configurations."
  - "The main-CI MDEC inventory includes bounded source coverage while preserving the exact 4-bit oracle and separately named 8-bit regression."
affects: ["06-44 local acceptance", "06-45 hosted acceptance"]
tech-stack:
  added: []
  patterns:
    - "Compare complete CTest name and label sets with inventories before broad runs."
    - "Use placeholders in an unrelated media-only configuration to preserve inventory parity without fetching unused assets."
key-files:
  created:
    - .planning/phases/06-discs-and-fmv/06-43-SUMMARY.md
  modified:
    - cmake/MDECReferences.cmake
    - tests/mdec/tests.cmake
    - tests/mdec/inventory.txt
    - tests/nightly/tests.cmake
    - tests/nightly/inventory.txt
    - tests/cmake/workflows.cmake
    - docs/testing.md
    - .planning/phases/06-discs-and-fmv/06-REVIEW.md
key-decisions:
  - "Only the approved unproved full-conformance selectors were replaced by source-coverage selectors; all surviving checks remain gating."
  - "CD-ROM-only CTest listings retain the same MDEC names and labels using placeholders, without preparing MDEC assets."
requirements-completed: []
coverage:
  - id: MDEC-SELECTOR-CONSISTENCY
    description: "Normal nightly and MDEC-only configurations register and pass the same 12 surviving MDEC reference selectors; CTest names and labels match the inventory."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "cmake --preset nightly; CTest JSON inventory comparison (exit 0, 12 exact selectors); cmake -S . -B build/mdec-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec; cmake --build build/mdec-reference --target schwaystation-test-mdec-reference; ctest --test-dir build/mdec-reference -L mdec --no-tests=error --output-on-failure (exit 0, 12/12); post-review repeat of the MDEC-only CTest JSON comparison (exit 0)."
      status: pass
    human_judgment: false
  - id: MEDIA-ONLY-INVENTORY-PARITY
    description: "CD-ROM-only configuration exposes the same 12 MDEC selector names and labels without preparing or fetching the unrelated MDEC reference corpus; the three CD-ROM reference checks pass."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "cmake -S . -B build/cdrom-registration -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=cdrom; CTest JSON comparison (exit 0, 12 exact MDEC selectors); test ! -e build/cdrom-registration/mdec-reference (exit 0); cmake --build build/cdrom-reference --target schwaystation-test-cdrom-reference; ctest --test-dir build/cdrom-reference -R '^cdrom[.]reference[.](timing|getloc|controls)$' --no-tests=error --output-on-failure (exit 0, 3/3); check_junit.cmake (exit 0)."
      status: pass
    human_judgment: false
  - id: MAIN-CI-MDEC-COVERAGE
    description: "Main-CI MDEC tests retain the exact 4-bit oracle, add bounded source-coverage controls and keep the 8-bit implementation regression distinct."
    requirement: DISC-03
    verification:
      kind: unit
      ref: "cmake --preset ci; cmake --build --preset ci --target schwaystation-test-mdec; ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure (exit 0, 12/12); cmake -DROOT=. -DCASE=disc_registration -P tests/cmake/workflows.cmake (exit 0); LC_ALL=C sort -c tests/mdec/inventory.txt and tests/nightly/inventory.txt (exit 0)."
      status: pass
    human_judgment: false
  - id: REVIEW-WARNINGS-WR09-WR10
    description: "The testing guide describes the landed selector set and evidence accurately, and the CD-ROM-only setup no longer prepares unused MDEC assets."
    requirement: DISC-03
    verification:
      kind: other
      ref: "06-REVIEW.md WR-09/WR-10 re-review: both warnings dispositioned resolved by inspection; no tests were run by the reviewer. Separate configure, CTest JSON, and CD-ROM checks recorded above validate the behavior."
      status: pass
    human_judgment: false
---

# Phase 6 Plan 06-43: Register surviving MDEC selectors

**The surviving MDEC checks now agree across main CI, normal nightly and both media-only configurations. The selector changes passed focused and complete MDEC runs.**

## Performance and runtime

- **Duration:** Active time not tracked; execution and evidence review resumed on 2026-11-17.
- **Runtime/model:** Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID not exposed).
- **Commits:** Task-level GSD commits pending in the shared phase branch.
- **Hosted run:** None; hosted acceptance is assigned to Plan 06-45.

## Work completed

- Synchronized MDEC selector names and labels in main CI, normal nightly, MDEC-only and CD-ROM-only CTest listings.
- Kept the exact 4-bit oracle and distinct 8-bit regression; renamed only the approved unproved full-conformance selectors to source-coverage checks.
- Added a cheap workflow registration consistency check and sorted the inventories.
- Kept CD-ROM-only configuration independent from MDEC assets by using matching placeholder registrations.
- Corrected the testing guide and recorded the code-review disposition for WR-09 and WR-10.

## Acceptance boundary

All surviving MDEC selectors passed. These registrations and source-coverage checks do not establish exact 8-bit hardware output or full frame/movie guest conformance. DISC-03 and Phase 6 acceptance remain pending Plan 06-44 local acceptance and Plan 06-45 hosted acceptance.

## Next

Plan 06-44 reviews the final source/contracts and runs the complete local acceptance set before hosted dispatch.
