# GPU

The GPU stores a 1024 by 512 array of native RGB555 words. GP0 packets update that array immediately; texture reads and
VRAM readback therefore share one source of truth. This page describes the implemented rectangle and polygon pixel
paths. Lines and beam behavior are described by their own hardware sections as those paths are implemented.

## Textured rectangles

A rectangle command encodes raw-texture (TGE), semi-transparency (ABE), texturing (TME), and a size mode. The size mode
selects variable dimensions, 1 by 1, 8 by 8, or 16 by 16. Its position is signed 11-bit X/Y; E5 offset is applied before
clipping against E3/E4, then the resulting coordinates wrap into VRAM. A textured rectangle carries a UV/CLUT word and,
for variable dimensions, a size word. E1 supplies the texture page and pixel depth. E2 applies its five-bit masks and
offsets to the eight-bit U and V coordinates before the page origin is added. Four-bit and eight-bit
texels are unpacked from VRAM words and resolved through the CLUT; fifteen-bit texels are read directly. The reserved
depth value follows the direct fifteen-bit model. X and Y wrap at the VRAM edges, and CLUT indices wrap across the row.
The texture and CLUT caches are per instance and form a deterministic rendering model; their replacement and stale-hit
behavior are not claimed as measured hardware behavior.

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
vertex's UV word and updates only E1/GPUSTAT texture-page bits 0 through 8 from the second vertex's UV word.

Triangles use widened integer edge equations sampled at pixel centers. Winding is normalized without culling, zero-area
triangles draw nothing, and the top and left edges own samples exactly on an edge. Quads draw as triangles (1,2,3) then
(2,3,4); this gives the shared diagonal to one triangle so immediate pixel writes neither leave a crack nor blend a
shared pixel twice. Each triangle interpolates its vertex colors and UV coordinates over the original geometry, before
draw-area clipping. Flat polygons reuse one color at all vertices; textured polygons use affine UV interpolation, and
the pixel pipeline above handles raw/modulated color, dithering, transparency, and write masks.

Color and UV planes use signed 16.16 steps. Setup products and division use 64-bit intermediates; step slopes are
rounded to the nearest 16.16 value with ties away from zero, and each interpolated eight-bit channel or UV coordinate
is truncated to an integer before pixel processing. The stored horizontal and vertical steps are 32-bit. When an
extreme slope cannot fit that representation, the covered pixel is evaluated from the original vertices with the same
widened integer plane arithmetic. This keeps hostile narrow triangles defined without changing the stored-step model.
These precision and rounding choices are a deterministic core model: the GPU specification does not define its
interpolation precision, and no hardware-precision measurement is claimed here. Gouraud or modulated polygon pixels
are dither-eligible; flat untextured polygons are not. Raw textured semi-transparent polygons do not acquire dither.

## Lines and polylines

Flat and Gouraud GP0 lines include both endpoints. A coincident line writes one pixel using its first color. The
renderer adds signed E5 offsets, rejects the whole line when its vertex delta exceeds 1023 horizontally or 511
vertically, then clips each sample against the inclusive E3/E4 draw area before writing through the shared pixel
helper. Lines are dither-eligible, including flat lines, when E1 dithering is enabled; ABE uses the same blend path
as other untextured primitives.

The line walk uses an integer major-axis error accumulator. Coordinates and each 8-bit Gouraud channel use nearest
integer stepping with midpoint ties rounded away from the starting value; both endpoint values are exact. This is a
deterministic model choice. PSX-SPX describes inclusive Bresenham-class lines but does not specify tie handling or
channel precision, and no hardware arithmetic measurement is claimed here.

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

## Source

- [PSX-SPX Graphics Processing Unit chapter](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/) documents GP0 rectangles, E1/E2/E6, texture decoding, modulation, dithering, and blending.
- The source and oracle inventory for this implementation is in `.planning/preparation/PSX-GPU-AND-GTE.md`, sections two and three. Its pinned GPU images do not cover every arithmetic branch or the raw textured semi-transparent dither case.
