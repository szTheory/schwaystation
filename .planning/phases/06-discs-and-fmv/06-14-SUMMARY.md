---
phase: 06-discs-and-fmv
plan: "14"
subsystem: disc-fixtures
tags: [disc, fixtures, cmake, ci]
actuals:
  tasks: 2
requirements-completed: []
coverage:
  - id: DISC-02-OWNED-DISC-ASSETS
    description: "Pinned tools generated the owned cue/bin image with boot metadata, XA, STR, CD-DA and SBI data; the runner probe and fixture budget passed."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake --preset ci (exit 0); cmake --build build/ci --target schwaystation-disc-fixtures --parallel 2 (exit 0); ctest --test-dir build/ci -R '^fixtures\\..*pin.*$|^fixtures\\.disc\\.probe$|^policy\\.fixture_budget$' --no-tests=error --output-on-failure --parallel 2 (exit 0, 5/5)."
      status: partial
    human_judgment: false
  - id: DISC-02-PINNED-MEDIA-PRODUCER
    description: "The pinned psxavenc producer path builds, and deliberately stale extraction markers are refreshed from the verified archives."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake -DOUTPUT=/private/tmp/schway-disc-media-producer/psxavenc -DBUILD_ROOT=/private/tmp/schway-psxavenc-producer -P cmake/Psxavenc.cmake (exit 0); wrote stale digest markers, ran cmake --preset ci (exit 0), then confirmed all three source markers match pins.txt."
      status: pass
    human_judgment: false
  - id: DISC-02-REPOSITORY-GATES
    description: "Fixture source hashes and inventories are consistent, and all repository hygiene checks pass under the approved 2700-line ceiling."
    requirement: DISC-02
    verification:
      kind: other
      ref: "Python source/hash/order preflight passed for 13 manifest rows; cmake --workflow --preset hygiene (exit 0, 24/24); gate count 2698/2700; git diff --check passed."
      status: pass
    human_judgment: false
metrics:
  duration: "Elapsed wall time spanned a 22-hour pause/resume; active runtime is unavailable."
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); review gpt-6-luna xhigh."
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 14: Pinned disc media and owned fixtures

This plan adds pinned build-only media tools and authored disc assets to the fixture graph. Local Linux producer and consumer paths pass, the fixture probe succeeds, and media hashes are checked before fixture builds. The full DISC-02 runtime acceptance remains pending for the downstream boot/playback plan and hosted platform matrix.

## Work completed

- Added pinned mkpsxiso and psxavenc build paths, authored cue/ISO recipes, short generated XA, STR and CD-DA media, and SBI callback markers.
- The CI producer builds media once and publishes its run-local digests; every fixture-building matrix, ASan, BE32 and WASM32 lane downloads and verifies the same media before independently building the disc.
- Fixed the Ninja Windows executable path, selected MSVC-compatible floating-point/linker options, and made both pinned source extractors invalidate stale caches when their archive digest changes.
- Wired the disc probe through the configured cross-compiling emulator and kept fixture source digests, image size and gate budget checked.
- Code review findings were corrected before final verification. Local Windows/MSVC and hosted cross-platform jobs were not available in this run.

## Verification

- `cmake --preset ci` — exit 0.
- `cmake --build build/ci --target schwaystation-disc-fixtures --parallel 2` — exit 0.
- Focused fixture pin, media-corruption, disc-probe and budget CTests — exit 0, 5/5.
- `cmake --workflow --preset hygiene` — exit 0, 24/24.
- Pinned psxavenc script-mode producer build — exit 0.
- Stale-source marker negative setup followed by configure and digest comparison — exit 0.
- Source syntax, inventory ordering, 13 fixture manifest hashes and `git diff --check` — pass.
- `actionlint -no-color -ignore 'label "ubuntu-26.04(-arm)?" is unknown' .github/workflows/ci.yml` — exit 0; the repository's custom hosted runner labels were excluded from its label catalogue check.
- Repository checks total 2698 lines against the approved 2700 ceiling. No hosted run URL applies; local platform checks only.

## Remaining acceptance

The local disc builds and probes successfully, but no hosted matrix run has yet demonstrated the Windows/MSVC, ASan, BE32 and WASM32 fixture builds. The downstream plan exercises boot and playback behavior. DISC-02 remains open until those runtime and per-leg checks pass. The owner does not need to run the PS1 now; public hardware records remain partial and no owner capture is scheduled.

## Next

Continue Wave 15 with Plan 06-21, then the dependent disc boot and playback plans. Do not ship Phase 6 until the final phase verification closes DISC-01 through DISC-04.

No hosted run URL applies.
