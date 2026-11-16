---
phase: 06-discs-and-fmv
plan: "13"
subsystem: MDEC
tags: [mdec, color, dma, frame-references]
actuals:
  tasks: 2
requirements-completed: []
coverage:
  - id: MDEC-COLOR-DISPLAY
    description: "Synthetic 15/24-bit macroblocks traverse DMA0, DMA1, DMA2 and native scanout; FIFO, color, display, vectors and bounds checks pass."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "cmake --build build/ci --target schwaystation-test-mdec --parallel 2 (exit 0); ctest --test-dir build/ci -R '^mdec\\.(color|display|vectors|bounds)$' --no-tests=error --output-on-failure --parallel 2 (exit 0, 4/4)."
      status: pass
    human_judgment: false
  - id: MDEC-ALL-TESTS
    description: "The complete MDEC model inventory remains gated; the known exact 8-bit guest-input vector still differs in 12 words."
    requirement: DISC-03
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci -R '^mdec\\.' --no-tests=error --output-on-failure --parallel 2 (exit 8, 10/11 passed; mdec.guest_input has 12 mismatched words)."
      status: fail
    human_judgment: false
  - id: MDEC-COLOR-REFERENCE-CONTROLS
    description: "Pinned asset identity and pixel-tolerance mutation controls pass, but the four frame/movie guests exhaust incomplete streams before completion."
    requirement: DISC-03
    verification:
      kind: integration
      ref: "cmake --build build/mdec-reference --target schwaystation-test-mdec-reference --parallel 2 (exit 0); ctest --test-dir build/mdec-reference -R '^mdec\\.reference\\.(frame15|frame24|movie15|movie24)$' --no-tests=error --output-on-failure --parallel 4 (exit 8; all four fail fast on exhausted input; 0.64 seconds total including asset setup). Assets/controls passed 2/2."
      status: partial
    human_judgment: false
  - id: MDEC-FULL-IMAGE-ORACLE
    description: "No complete pinned frame/movie input and hardware-reference pair is available; public records cover compact color vectors and only 19 of 20 frame strips."
    requirement: DISC-03
    verification:
      kind: other
      ref: ".planning/phases/06-discs-and-fmv/06-13-REVIEW.md; public SCPH-5501 records SCM.29/SCM.30; source-coverage counts are 296/300 frame macroblocks and 162/240 movie-frame macroblocks."
      status: blocked
    human_judgment: false
metrics:
  duration: "Exact active time not tracked; bounded follow-up finished 2026-11-16T18:57:07Z."
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable)."
completed: 2026-11-16
status: complete
---

# Phase 6 Plan 13: Color MDEC output and reference coverage

The bounded Plan 13 slice now has synthetic color-to-display checks and pinned reference controls. DISC-03 remains pending: the exact 8-bit guest vector is still red, and the pinned frame/movie assets do not contain enough MDEC blocks for the callers to finish.

## Work completed

- Added instance-owned color decode staging, 15/24-bit packing, and macroblock FIFO streaming/save-state coverage.
- Connected MDEC DMA output through the existing DMA2 and native scanout paths.
- Added pinned frame/movie guest asset checks, reference-image comparisons, and tolerance mutation controls.
- Kept the real guest tests fail-fast when their MDEC input is exhausted; they do not compare a partial display as a pass.
- Recorded the public SCPH-5501 color records and the later-strip capture in the preparation notes. The first exploratory color probe had the wrong qscale and tables; a corrected setup was tried briefly. No speculative arithmetic or probe code was retained. Public web records are not commit-pinned fixture inputs.

## Remaining acceptance

- The reference frame stream covers 296 of 300 requested macroblocks; the movie's first frame covers 162 of 240. Asset checks pass, but all four bounded guest callers fail at input exhaustion before `Done`.
- The full model inventory passes 10 of 11 checks. `mdec.guest_input` still reports 12 exact 8-bit word differences that reproduce from phase-start source and remain assigned to the existing MDEC repair chain.
- The source review retains two warnings: no complete image pair and no raw hash from the real guest's FIFO before DMA1.
- A setup-matched public hardware probe confirmed several compact samples but did not resolve all rounding points. Direct fractional 15-bit rounding fixed one sample and broke the synthetic color expectations, so it was removed.

## Next

Continue the authorized Phase 6 plan chain with Plan 14's pinned disc-media tool and recipe work. The Plan 13 image gate and DISC-03 remain pending; do not mark the phase verified or ship until a complete, independently sourced reference pair passes.

No hosted URL applies. The owner does not need to run the PS1 now; the real console and capture card are noted as a later bounded fallback if public source work does not close the gap.
