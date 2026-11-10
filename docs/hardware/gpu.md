# GPU

The GPU stores a 1024 by 512 array of native RGB555 words. GP0 packets update that array immediately; texture reads and
VRAM readback therefore share one source of truth. This page describes the implemented rectangle pixel path. Polygon,
line, and beam behavior is added by the plans that first implement those paths.

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

## Source

- [PSX-SPX Graphics Processing Unit chapter](https://psx-spx.consoledev.net/graphicsprocessingunitgpu/) documents GP0 rectangles, E1/E2/E6, texture decoding, modulation, dithering, and blending.
- The source and oracle inventory for this implementation is in `.planning/preparation/PSX-GPU-AND-GTE.md`, sections two and three. Its pinned GPU images do not cover every arithmetic branch or the raw textured semi-transparent dither case.
