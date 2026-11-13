# Phase 04 Plan 47 Checkpoint

Status: **incomplete**. Do not mark plan 47 complete or advance phase metadata. No SUMMARY, STATE, or ROADMAP update has been made.

Runtime: Codex/OpenGSD 1.20.0, `gpt-6-luna` xhigh. The phase branch is `phase/04-games-play-in-retroarch`.

## Completed at this checkpoint

- Generalized `cmake/GPUReferences.cmake` to select and stage the nine requested build-158 GPU cases from the already verified archive. The selected cases and CTest rows are byte-sorted; the existing PNG adapter and full-word RGB555 judge are reused. `tests/gpu/allow.txt` has no source-supported variants; no observed pixels were admitted.
- Verified fetched executables, PNGs, and inspected caller sources belong to release tag `build-158`, commit `5f73d61b27a8d51269b54dde1c456d2d2f7dc865`. All nine case-source blobs match the earlier source pin. This identity check is required before using source progress or PCs to justify capture bounds.
- Fixed GP0(01h) cache invalidation: PSX-SPX documents that it invalidates both texture and CLUT caches. The focused `gpu.transfer.cache_contract` test first failed the new CLUT refresh assertions against the old code, then passed after the minimal fix. The original `clut-cache` hardware image now passes at zero tolerance.
- Kept the original captured pixels, fixed per-case bounds, and raw RGB decoding unchanged. The transparency source audit found that build-158 clears all VRAM and the current fill operation follows the documented fill model; it did not establish a fill defect.

## Capture outcomes

The focused nine-case nightly run completed at the initial source-derived post-draw bounds with no faults. `texture-overflow` passed. Six first-draw states differ from their original captures; the `rectangles` result is already known to require a fixed-point capture state before it can be classified:

| Case | Fixed advances | Result |
|---|---:|---|
| lines | 1 | 135 differences; first index 21678, expected `0000`, actual `7fff` |
| rectangles | 2 | 2,288 differences; first index 40, expected `2751`, actual `5398`; this first-pass state is not comparable to the capture |
| transparency | 1 | 447,488 differences, exactly all pixels outside 320×240; expected white, actual black |
| quad | 1 | 352 differences; first index 318, expected `3fef`, actual `7def` |
| triangle | 1 | 30,664 differences; first index 17567, expected `7fff`, actual `7c00` |
| uv-interpolation | 2 | 28,725 differences; first index 2049, expected `03e0`, actual `001f` |
| texture-overflow | 2 | exact match |
| vram-to-vram-overlap | 8 | **completion not established**; 34,390 observed differences cannot be classified |
| clut-cache | 2 | exact match after the GP0(01h) fix |

For overlap, no-fault tick progress is insufficient evidence of completion. Its first IRQ is VBlank IRQ0 (`I_MASK` bit 0 enabled; `I_STAT` bit 0 is the source); the first handler entry records Cause `0x400`, EPC `0x8001103c`, which is inside the guest VRAM upload helper. The pinned PSn00bSDK installs a registered `_global_isr` hook with B0(19h), acknowledges IRQ0 in the callback dispatcher, and returns with B0(17h). The default CPU-only stub does not dispatch that hook. The authorized bounded first-caller repair is material (estimated 45–60 minutes and ~120–180 changed lines across the stub assembly/generated image, fixture tests/registration, digests, and documentation), so it was not started here; it requires its focused prerequisite plan before overlap can be judged.

The coordinator's independent read-only audit of `rectangles` found that build-158 clears to white once and then repeats its draw/VSync loop. At `(40,0)` the raw foreground is `2751`; the first mode-0 semi-transparent blend over white is `5398`, matching this core's observed first pass. The capture's mode anchors (`0=foreground`, `1=7fff`, `2=0000`, `3=7fff`) reflect converged repeated draws. This is a capture-state mismatch, not a proven renderer defect; establish a source-derived fixed repeat count or fixed-point endpoint after IRQ/VSync delivery, without frame hunting.

The retained executable is `build/nightly/gpu-reference/gpu/rectangles/rectangles.exe`; expected raw words are `build/nightly/gpu-reference/rectangles-vram.rgb555le`. The first fixed run completed two advances (618,346 ticks) without a fault, but that bound stops before the repeated-draw fixed point represented by the capture.

## Verification commands

- `cmake --preset nightly` — exit 0.
- `cmake --build --preset nightly --target schwaystation-run schwaystation-test-gpu-reference` — exit 0.
- `ctest --test-dir build/nightly -R '^gpu\.reference\.(lines|rectangles|transparency|quad|triangle|uv-interpolation|texture-overflow|vram-to-vram-overlap|clut-cache)$' --no-tests=error --output-on-failure --parallel 4` — exit 8; one match, eight initial mismatches before the CLUT fix.
- `ctest --test-dir build/ci -R '^gpu\.transfer\.cache_contract$' --output-on-failure` — exit 8 before the fix (the two new GP0(01h) CLUT assertions failed), then exit 0 after the fix.
- `cmake --build --preset nightly --target schwaystation-test-gpu-reference` — exit 0 after the fix.
- `ctest --test-dir build/nightly -R '^gpu\.reference\.clut-cache$' --output-on-failure` — exit 0 after the fix; exact captured image.
- `git diff --check` — exit 0.

No broad CI or hygiene workflow was run at this checkpoint. Those remain required after the final plan 47 source changes.
