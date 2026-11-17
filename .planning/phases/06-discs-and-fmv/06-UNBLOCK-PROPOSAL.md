# Phase 6 unblock proposal

**Status:** Owner-approved 2026-11-17; limited acceptance supersession is in progress. The 2026-11-17
source-coverage correction below updates the evidence and selector rationale without expanding the approved
v1.0 MDEC claim or changing any hardware expectation, algorithm or cycle cap.

## Findings

Three independent MDEC acceptance gaps remain:

1. **8-bit arithmetic:** the pinned `heart.mdec` input and tables are verified, but the current integer
   transform differs from the exact hardware bytes at 16 pixels (15 by one level and one by two). The
   permitted ares arithmetic and tested alternatives do not supply a hardware-verified correction. Sony's
   developer note does not specify internal IDCT rounding; the public hardware captures use other inputs or
   publish only partial words/CRCs. No source-backed adjustment or replacement full vector was found.
2. **Ordered-status guest:** its 768 `delay(10000)` calls request 7,680,000 emulated cycles before overhead,
   against the fixed 4,915,200-cycle limit. The pinned guest cannot reach its output-drain/Done path under that
   limit. The cap stays unchanged; the main-CI `mdec.status` model tests cover documented counter behavior.
3. **Full STR/movie references:** the pinned frame source contains 56,960 bytes/1,800 blocks/300 macroblocks;
   the movie's first frame contains 6,784 bytes/1,440 blocks/240 macroblocks. These spans are complete. Earlier
   counts of 296 and 162 were decoder output progress, not source coverage. All four guest paths consume their
   pinned source but do not prove full output or hardware fidelity. The frame path stops with an empty output
   FIFO and fails the existing visible-pixel comparison at (2,0) in a bounded diagnostic. The pinned frame
   helper polls for another word after its final requested read; the public adapted SCPH-5501 PIO capture also
   stalls there after 19 uploaded strips, but this does not prove our output matches that capture. The movie
   uses DMA1 and stalls with output empty; no corresponding public capture was found. The owned `disc_boot` STR
   remains a separate complete self-authored end-to-end DMA/display integration check, not a hardware capture.

The 4-bit guest and direct MDEC controls pass. Current full local CI is 609/610; the only CI failure is the
8-bit exact output. Phase 6 also still needs the complete nightly/hosted acceptance evidence after local gates
are green.

## Recommended route if v1.0 must ship without owner hardware

Keep v1.0's MDEC claim at **deterministic integer decoding with the available verified coverage**, not full
hardware-exact 8-bit or full-frame/movie conformance. Do not pick a corpus-fitted `±2` tolerance. Keep the
current arithmetic and all pinned evidence unchanged. Document the 8-bit mismatch, complete-but-unverified
frame/movie guests, and separate owned STR integration as the known fidelity boundary. Revisit full hardware
accuracy only when exact arithmetic evidence or independently captured output supports it.

If approved, a successor Phase 6 plan would:

- Preserve the exact 4-bit hardware oracle, the passing direct/model checks, the full owned `disc_boot` STR
  path, and all currently passing disc/API/audio integration checks.
- Replace the failing 8-bit hardware selector with a clearly labeled deterministic 8-bit integration/regression
  check. Keep the hardware vector and expected bytes as evidence; do not claim that the replacement proves
  hardware accuracy.
- Remove the impossible full-guest status selector from this phase's required lane while retaining the fixed
  cap and the passing bounded `mdec.status` model checks.
- Keep the frame/movie pins and a cheap source-unit coverage preflight; the inputs are complete, but their
  current guest output/completion is unresolved. Replace the full-image conformance selectors with gating
  source-coverage selectors under the approved limited claim; retain the exact mismatch/stall evidence. Use the
  complete owned STR as the v1.0 end-to-end path. Restore external frame/movie conformance only with a supported
  guest path and independent expected output.
- Revise `DISC-03`, the roadmap/context, the superseding MDEC decision, and verification docs so the released
  claim matches the evidence. Keep every remaining behavior test gating and keep the dependency tree unchanged.
- Re-run code review, affected MDEC preflights/selectors, full CI and hygiene, the surviving nightly MDEC/fuzz
  matrix, hosted per-leg checks and background RetroArch acceptance. Ship only after those pass.

This approved route narrows the v1.0 product promise. It does not increase the cycle cap, request owner hardware,
claim the current MDEC output is fully hardware accurate, or authorize starting v2.0 before v1.0 is released.

## Route if the current accuracy bar is retained

Keep Phase 6 blocked. Resume only when a public primary source supplies either the full hardware output for
the pinned 8-bit input and exact tables, a precise arithmetic rule that predicts it, or complete independently
captured STR/movie inputs and outputs. The status guest still needs a source-compatible way to complete inside
the unchanged cap. Further rounding guesses, changing expected bytes, widening tolerance, synthesizing missing
compressed data from PNGs, or increasing the cap are not supported by the evidence.

## Sources

- [Sony MDEC developer note](https://psx.arthus.net/sdk/Psy-Q/DOCS/TECHNOTE/mdecnote.pdf)
- [psx-spx MDEC specification and its approximate IDCT notes](https://psx-spx.consoledev.net/macroblockdecodermdec/)
- [Public SCPH-5501 MDEC output captures](https://psx.wiki/tests/mdecout/)
- [Public MDEC hand-made stream captures](https://psx.wiki/tests/mdec2/)
- [Pinned ps1-tests MDEC tree](https://github.com/JaCzekanski/ps1-tests/tree/f727802fead11f1daa7549285548392ef87749cb/mdec)
- [Pinned frame guest](https://github.com/JaCzekanski/ps1-tests/blob/f727802fead11f1daa7549285548392ef87749cb/mdec/frame/main.cpp)
- [Pinned movie guest](https://github.com/JaCzekanski/ps1-tests/blob/f727802fead11f1daa7549285548392ef87749cb/mdec/movie/main.cpp)
