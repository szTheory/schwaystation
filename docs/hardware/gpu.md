# GPU

The GPU stores a 1024 by 512 array of native RGB555 words. GP0 packets update that array immediately; texture reads and
VRAM readback therefore share one source of truth. This page describes the implemented rectangle and polygon pixel
paths. Lines and beam behavior are described by their own hardware sections as those paths are implemented.

## GPU readiness and DMA

DMA2 writes use the same GP0 packet parser as CPU writes, so packet state carries across blocks and linked-list nodes.
DMA2 reads consume the same GPUREAD stream as CPU reads. A transfer reset clears an unfinished packet or VRAM transfer.

GPUSTAT bit 26 is set only when the command parser is idle and the GPU is ready. Bit 28 follows the parser's block-ready
state independently of bit 26: it remains high for a completed CPU-fed GP0 command in DMA direction 0 while that
primitive is still rendering, and is low for busy rendering under a nonzero DMA direction. Partial polygon or line
packets and active VRAM uploads or polyline continuations retain their parser-specific rules. Bit 27 reports unread
VRAM-to-CPU words. Bit 25 and DMA2's request line follow GP1's DMA direction: direction 1 reports FIFO space, even
while rendering is busy; direction 2 follows bit 28, and direction 3 requests while VRAM read words remain. These
rules match the ordered source-backed `gpustat` observations and do not claim a complete physical FIFO model.

The GPU busy deadline is a deterministic model seeded from `.planning/preparation/PSX-GPU-AND-GTE.md`, section four
point four. It charges 8 setup clocks plus the rounded per-pixel cost below after rasterization; it is not a console
timing measurement. DMA2 linked-list nodes wait for this deadline, except when they carry parameters owed by an already
started packet. Upload blocks and list nodes retain their existing atomic DMA cost.

| Operation | GPU clocks per pixel (model) |
|---|---:|
| Fill | 0.081 |
| Flat / semi-transparent rectangle | 0.530 / 0.797 |
| Textured / semi-transparent textured rectangle | 1.188 / 1.497 |
| Flat / semi-transparent polygon | 0.538 / 0.812 |
| Textured / semi-transparent textured polygon | 2.980 / 3.463 |
| Line / VRAM copy | 1.000 / 1.000 |

GPUREAD and busy timing are saved as instance state, so a state load resumes unread transfer data and the remaining
readiness deadline.

## Textured rectangles

A rectangle command encodes raw-texture (TGE), semi-transparency (ABE), texturing (TME), and a size mode. The size mode
selects variable dimensions, 1 by 1, 8 by 8, or 16 by 16. Its position is signed 11-bit X/Y; E5 offset is applied before
clipping against E3/E4, then the resulting coordinates wrap into VRAM. A textured rectangle carries a UV/CLUT word and,
for variable dimensions, a size word. E1 supplies the texture page and pixel depth. E2 applies its five-bit masks and
offsets to the eight-bit U and V coordinates before the page origin is added. Four-bit and eight-bit
texels are unpacked from VRAM words and resolved through the CLUT; fifteen-bit texels are read directly. The reserved
depth value follows the direct fifteen-bit model. X and Y wrap at the VRAM edges, and CLUT indices wrap across the row.
The texture and CLUT caches are per instance and form a deterministic rendering model. GP0(01h) invalidates both
caches, and a different CLUT address invalidates the CLUT cache, following the
[PSX-SPX GPU documentation](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#gpu-texture-caching). Other cache
replacement and stale-hit behavior is not claimed as measured hardware behavior.

## Pixel pipeline

The core applies these steps for each pixel:

1. A decoded texture word of `0000h` leaves the destination unchanged.
2. Raw texture keeps the texel's RGB555 channels. Modulated texture computes
   `min(31, (texel5 * color8) >> 7)` per channel with integer intermediates.
3. Eligible pixels use the E1 dither switch and this signed 4 by 4 matrix. Each offset is added to an eight-bit channel,
   clamped to 0 through 255, then shifted right by three.

   | -4 | 0 | -3 | 1 |
   |---:|--:|---:|--:|
   | 2 | -2 | 3 | -1 |
   | -3 | 1 | -4 | 0 |
   | 3 | -1 | 2 | -2 |

4. Semi-transparent untextured pixels blend. Textured pixels blend only when texel bit 15 is set. The four E1 modes use
   five-bit channels: average, saturated add, clamped subtract, and background plus one quarter of foreground.
5. E6 bit 1 skips a destination whose bit 15 is set. E6 bit 0 forces bit 15 on the written value. Otherwise a textured
   pixel preserves the texel's bit 15 and an untextured pixel clears it.

Dither eligibility is supplied by the primitive that calls the pixel helper. Gouraud or modulated polygon pixels and
lines can opt in; rectangles do not. Raw textured semi-transparent polygons are explicitly kept out of the dither path.
These are the core's documented model rules, not claims backed by a dedicated hardware capture for each arithmetic
branch.

Blend width and truncation remain a doc/derived choice: the equations and the pinned transparency/quad images motivate
the model but do not prove the GPU's internal blend width. The local known-answer tests separately pin every mode,
saturation, subtraction, all sixteen dither positions, the limits, raw texture, STP gating, E6 masks, palette wrapping,
and the raw semi-transparent dither exception.

## Polygons

GP0 supports flat and Gouraud colors, textured and untextured pixels, raw and modulated texture, and semi-transparency.
Coordinates are signed 11-bit values with the signed E5 draw offset added before clipping. E3/E4 edges are inclusive;
an inverted draw area produces no pixels. The renderer rejects the whole polygon when its offset-applied vertex span
exceeds 1023 horizontally or 511 vertically, before clipping to VRAM. A textured polygon takes its CLUT from the first
vertex's UV word and updates E1/GPUSTAT texture-page bits 0 through 8 from the second vertex's UV word. Its TPage bit
11 updates GPUSTAT bit 15 only on v2 when GP1(09h) has enabled the 2 MB texture-Y gate. Clearing the page bit clears
the latched status bit; a set bit is ignored when the gate is disabled. GP0(E1h).11 uses the same gate. Disabling
GP1(09h) alone preserves the last accepted status bit. GPU v0 ignores GP1(09h) and always reads bit 15 as zero. This
status latch does not add a second VRAM bank or change texture sampling.

Triangles use widened integer edge equations sampled at integer pixel coordinates. Winding is normalized without culling, zero-area
triangles draw nothing, and the top and left edges own samples exactly on an edge. Quads draw as triangles (1,2,3) then
(2,3,4); this gives the shared diagonal to one triangle so immediate pixel writes neither leave a crack nor blend a
shared pixel twice. Each triangle interpolates its vertex colors and UV coordinates over the original geometry, before
draw-area clipping. Flat polygons reuse one color at all vertices; textured polygons use affine UV interpolation, and
the pixel pipeline above handles raw/modulated color, dithering, transparency, and write masks.

Color and UV planes use signed 12.12 steps anchored at each triangle's first vertex. Setup products and accumulation
use 64-bit intermediates; signed slope division truncates toward zero. The existing polygon-span limit and eight-bit
attributes keep each stored 32-bit slope in range. At a covered pixel, adding half a 12-bit unit and converting to an
integer rounds the interpolated channel or UV coordinate to nearest; colors then pass through the pixel pipeline.
These integer-origin and 12.12 rules match the selected build-158 triangle and UV captures. They define a deterministic
core model: the GPU specification does not define its interpolation precision, and no physical precision measurement
is claimed here. Gouraud or modulated polygon pixels are dither-eligible; flat untextured polygons are not. Raw textured
semi-transparent polygons do not acquire dither.

## Lines and polylines

Flat and Gouraud GP0 lines include both endpoints. A coincident line writes one pixel using its first color. The
renderer adds signed E5 offsets, rejects the whole line when its vertex delta exceeds 1023 horizontally or 511
vertically, then clips each sample against the inclusive E3/E4 draw area before writing through the shared pixel
helper. Lines are dither-eligible, including flat lines, when E1 dithering is enabled; ABE uses the same blend path
as other untextured primitives.

The line walk uses an integer major-axis error accumulator and emits a sample when its minor-axis error reaches the
major-axis step count. It starts the error at `steps / 2`, except for X-major lines with descending Y and Y-major lines
with ascending X, which start at `(steps - 1) / 2`; this is the selected spatial midpoint rule (X-lower/Y-upper in
screen coordinates). Each 8-bit Gouraud channel uses a 12.12 value initialized to `channel * 4096 + 2048` and advances
by the signed, truncated step `(abs(end - start) * 4096) / steps`. Dropping the fractional bits rounds the stored
12.12 sample to nearest, with ties toward the higher channel value; slope truncation means that sample need not be
the exact rational midpoint. Both endpoints remain exact within the accepted 1023-step maximum. These are
deterministic model choices supported by the pinned line capture, not measurements of physical GPU tie handling or
channel precision. PSX-SPX describes inclusive Bresenham-class lines but leaves those details unspecified.

Polylines retain only their current vertex, color, next Gouraud color, mode, and parser phase. Each received vertex
can emit at most one bounded line segment. The terminator mask is checked only where the stream expects a vertex in
flat mode or a color in Gouraud mode. Those fields are part of the validated save state so a partial polyline can
resume deterministically.

## Rectangle texture flips

Rectangles draw one span per row, with 10-bit width and 9-bit height masked directly from the variable-size word;
zero dimensions draw nothing. Fixed sizes are 1 by 1, 8 by 8, and 16 by 16. E5 is applied before inclusive E3/E4
clipping; rectangles do not dither. Texture coordinates wrap at eight bits and are fetched through the shared
texture/CLUT cache path. E1 flip bits decrement U or V for v2 rectangles only. `SCHWAY_PROFILE_GPU_V0` selects the
v0 profile, where those flip bits have no effect; GPU v2 is the default. PSX-SPX documents the flip revision limit,
the mask widths, and the odd-UV/odd-width sampling anomaly; the current sampler records the latter as an unresolved
hardware arithmetic detail rather than assigning an unsupported pixel pattern.

These rules follow the PlayStation GPU command and raster descriptions in [PSX-SPX](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/)
and the source notes in `.planning/preparation/PSX-GPU-AND-GTE.md`, sections two point two, two point three, and three point six. Known-answer tests pin both
sides of each span limit, edge ownership, clipping, signed offsets, Gouraud/UV values, page/CLUT effects, polygon flags,
negative gradients, and narrow hostile slopes. They also check clipping invariance and state-split determinism. The
directed interpolation tests pin the core model; they do not claim to calibrate GPU arithmetic precision against hardware.

## VRAM transfers

GP0(80h) copies wrap source and destination coordinates independently in VRAM. This implementation reads each row in
forward chunks of up to 16 words into a bounded local buffer, then writes that chunk forward. The overlap capture
validates the supported 2-, 8-, 15-, and 16-word cases; it does not measure a unique physical buffer width. Wider rows
continue in later chunks, and each later row reads the VRAM state left by earlier rows. E6 checks the destination's
existing mask bit before applying the force-mask bit. The command and mask behavior follow PSX-SPX's
[VRAM-to-VRAM blitting description](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#vram-to-vram-blitting-command-4-100),
and coordinate wrapping follows its [transfer wrapping notes](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/#wrapping).
The 16-word read chunk is a capture-backed deterministic model documented with the [GPU reference test](../testing.md#gpu-hardware-captures).

## Beam and timer timing

The beam advances lazily on the CPU tick line. It carries the exact GPU/CPU ratio remainder: 715,909 GPU clocks per
451,584 CPU cycles for NTSC and 709,379 per 451,584 for PAL. A line is 3,413 GPU clocks for NTSC or 3,406 for PAL;
the progressive field contains 263 or 314 lines. Dot clocks divide GPU clocks by 10, 8, 5, or 4, with the 368-dot
mode using 7. Both fractional phases live in each instance and are serialized.

The scheduler derives the next horizontal-blank edge, line end, or enabled Timer 0 dot-target deadline. It advances by
whole spans between those points, so Timer 0 can raise an IRQ in the middle of a line without polling each dot. X2 starts
horizontal blank, and Y2/Y1 comparisons start and end vertical blank when the line value is within the field. A range that
misses those comparisons can suppress an edge; it cannot stop line or field progression. Equal-tick processing catches
up system-clock counters, applies beam and dot edges, then runs DMA events.

Loaded content completes a public frame at the fixed line-counter wrap, independent of Y2 and its VBlank interrupt.
GP1(00h) resets the beam position and partial command state. The public call's maximum-field deadline is fixed before
the guest runs, so repeated resets return `SCHWAY_STOP_BUDGET` instead of extending the call.

These constants and edge positions are the documented model derived from PSX-SPX GPU timing and the settled ratio/line
choices in `.planning/preparation/DECISIONS.md` (DEC.09 and DEC.10), with architecture details in
`.planning/preparation/ARCHITECTURE.md`, section one. They are doc/derived, not measured hardware results.

## GPUSTAT field and line status

GPUSTAT bit 13 reports the saved field phase on v2 and reads as zero on v0. A full GPU reset starts the v2 phase at
one. A transition from progressive mode into interlace selects that same initial phase; while IRS is enabled, the
existing beam edge entering the model's last scanline toggles it. Disabling IRS preserves the phase, and resetting
the GP1 command buffer does not change it. The phase is serialized independently of the current display mode.

Bit 31 is zero outside the programmed vertical display interval, clipped to the modeled field. In effective 480-line
interlace it is the inverse of the saved field phase. In effective 240-line modes, including IRS enabled with VDS off,
it follows beam-line parity. Display disable and GPU IRQ acknowledgement do not gate either status bit. Status
composition and public peeks are pure. A real GP1 read performs the existing scheduled beam catch-up first, so any
transition it observes is caused by elapsed emulated time, not by the read itself.

These rules follow the v0/v2 probe report published in the [PSX-SPX GPU status documentation](
https://psx-spx.consoledev.net/graphicsprocessingunitgpu/) and its [2026-11-03 update, commit 519c8db](
https://github.com/psx-spx/psx-spx.github.io/commit/519c8dbcc025467a6c80b02e61aafe3adcfa497b). The implementation maps those
semantics onto the existing fixed 263/314-line beam. It does not model the reported alternating physical field lengths
or the early-vblank interlace-entry exception, so its phase-entry and edge ticks are deterministic model choices.

## Source

- [PSX-SPX Graphics Processing Unit chapter](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/) documents GP0 rectangles, E1/E2/E6, texture decoding, modulation, dithering, and blending.
- [PSX-SPX GPU status-register update, commit 519c8db](https://github.com/psx-spx/psx-spx.github.io/commit/519c8dbcc025467a6c80b02e61aafe3adcfa497b) reports v0/v2 bit 13 and 31 probes, mode retention, and PAL behavior.
- The source and oracle inventory for this implementation is in `.planning/preparation/PSX-GPU-AND-GTE.md`, sections two and three. Its pinned GPU images do not cover every arithmetic branch or the raw textured semi-transparent dither case.
