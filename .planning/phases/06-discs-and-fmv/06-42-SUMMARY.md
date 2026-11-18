---
phase: 06-discs-and-fmv
plan: "42"
subsystem: testing
tags: [mdec, input-coverage, source-preflight, status-budget]
requires: ["06-41"]
provides:
  - "A bounded structural scanner proves pinned source-span completeness independently of decoder output."
  - "Guest entry points guard pinned mono inputs; reference preflight covers complete frame/movie sources and status budget."
affects: ["DISC-03 evidence", "Phase 6 plans 06-43 through 06-45"]
tech-stack:
  added: []
  patterns:
    - "Separate compressed-source coverage from decoder output progress and guest completion."
    - "Run a cheap, no-emulation preflight before reference guests."
key-files:
  created:
    - tests/mdec/input_coverage.h
    - .planning/phases/06-discs-and-fmv/06-42-SUMMARY.md
  modified:
    - tests/mdec/model.c
    - tests/mdec/reference.c
    - tests/mdec/color_reference.c
    - .planning/phases/06-discs-and-fmv/06-42-PLAN.md
key-decisions:
  - "The pinned frame and movie first-frame spans are complete at 300 and 240 macroblocks; earlier 296/162 values were output progress, not source coverage."
  - "Keep source guards and coverage checks, but do not infer guest completion or hardware fidelity from them."
requirements-completed: []
coverage:
  - id: MDEC-SOURCE-SPAN-COVERAGE
    description: "The bounded scanner verifies pinned mono, frame and movie source spans, including unique executable mappings, structural block counts, and six-block color macroblocks."
    requirement: DISC-03
    verification:
      kind: unit
      ref: "build/ci/tests/schwaystation-test-mdec input_coverage; build/mdec-reference/tests/schwaystation-test-mdec-reference input-preflight build/mdec-reference/mdec-reference (exit 0); ctest --test-dir build/mdec-reference -R '^mdec[.]reference[.]coverage_(frame15|frame24|movie15|movie24)$' --no-tests=error --output-on-failure (exit 0; 4/4)"
      status: pass
    human_judgment: false
  - id: MDEC-PREFLIGHT-AND-BUDGET
    description: "The no-emulation preflight verifies pinned spans and classifies the status guest's requested cycles against the unchanged cap; negative controls cover malformed and mutated coverage."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "build/mdec-reference/tests/schwaystation-test-mdec-reference coverage-controls; ctest --test-dir build/mdec-reference -R '^mdec[.]reference[.](input_preflight|status_budget|coverage_controls)$' --no-tests=error --output-on-failure (exit 0; 3/3)"
      status: pass
    human_judgment: false
  - id: MDEC-GUEST-OUTPUT-LIMIT
    description: "Full frame/movie guest output is not accepted from source coverage: guarded guests consume their inputs but fail to establish full reference output, and frame pixels differ from the pinned image."
    requirement: DISC-03
    verification:
      kind: other
      ref: "ctest --test-dir build/mdec-reference -R '^mdec[.]reference[.](frame15|frame24|movie15|movie24)$' --no-tests=error --output-on-failure --output-junit build/mdec-reference/06-42-color-guest-diagnostic.xml (exit 8; 4 failures under 0.7s); a temporary frame display probe also exited 8 at the existing visible-pixel comparison and was reverted"
      status: pending
    human_judgment: false
---

# Phase 6 Plan 06-42: Guard compressed inputs and measure coverage

**The pinned compressed inputs are now counted before emulation. Frame and movie source spans are complete, but their full guest output remains unresolved.**

## Performance and runtime

- **Duration:** Active time not tracked; execution and evidence review resumed on 2026-11-17.
- **Runtime/model:** Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID not exposed).
- **Commits:** 5 implementation/documentation commits after `34182e2`.
- **Hosted run:** None applies.

## Work completed

- Added a bounded structural scan that checks source identity and mapped spans, parses complete RLE blocks, tracks partial coefficients, and groups color output in six-block macroblocks.
- Added mutation controls for malformed/truncated inputs and macroblock counts.
- Added no-emulation reference preflight for two mono and four color assets, plus the pinned status-budget calculation. The source spans classify as 300/300 frame macroblocks and 240/240 movie first-frame macroblocks.
- Added in-process guards before mono reference guests so direct invocation cannot bypass source validation.
- Preserved pins, expected bytes, production arithmetic, and the 4,915,200-cycle cap.

## Verification evidence

| Command | Exit | Result |
|---|---:|---|
| `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-mdec && build/ci/tests/schwaystation-test-mdec input_coverage` | 0 | MDEC model target built and structural coverage controls passed. |
| `cmake -S . -B build/mdec-reference -G Ninja -DSCHWAYSTATION_INVENTORY=nightly -DSCHWAYSTATION_MEDIA_REFERENCE_ONLY=mdec && cmake --build build/mdec-reference --target schwaystation-test-mdec-reference` | 0 | Reference runner configured, built and linked with source guards. |
| `build/mdec-reference/tests/schwaystation-test-mdec-reference coverage-controls` | 0 | Coverage mutation controls passed. |
| `build/mdec-reference/tests/schwaystation-test-mdec-reference input-preflight build/mdec-reference/mdec-reference` | 0 | Six pinned spans and status budget passed without guest execution. |
| `ctest --test-dir build/mdec-reference -L mdec --no-tests=error --output-on-failure` | 0 | After 06-43 registration, all 12 surviving nightly MDEC selectors passed. |
| `ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure` | 0 | All 12 main-CI MDEC model selectors passed. |
| `ctest --test-dir build/mdec-reference -R '^mdec[.]reference[.](frame15|frame24|movie15|movie24)$' --no-tests=error --output-on-failure --output-junit build/mdec-reference/06-42-color-guest-diagnostic.xml` | 8 | All four consume their complete source but do not establish output/completion. This is diagnostic evidence, not an acceptance pass. |

## Evidence correction

The former `296/300` and `162/240` values measured decoder output progress. The source scanner verifies full source spans: 56,960 bytes/1,800 blocks/300 macroblocks for frame, and 6,784 bytes/1,440 blocks/240 macroblocks for the movie's first frame. A temporary, reverted frame probe continued through input exhaustion and still failed the existing visible-pixel comparison at `(2,0)`. The public SCPH-5501 PIO capture supports a final-read stall in its adapted harness, not matching emulator pixels. Movie DMA1 is a separate unresolved path without a matching capture. Plans 06-43 onward replace full external frame/movie conformance selectors with gating source-coverage selectors under the approved v1.0 scope.

## Next

Plan 06-43 synchronizes selector names and inventories. Plan 06-44 owns source review and complete local acceptance; Plan 06-45 owns exact-head hosted acceptance and the ship handoff. DISC-03, prior 06-23/24 acceptance, and release checks remain pending.
