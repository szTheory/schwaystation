---
title: "PSX-GPU-AND-GTE: GPU revisions, VRAM, GP0/GP1 commands, rasterisation rules, timing, display, GTE, oracles, output format"
summary: "What gpu.c and gte.c must do: the v0/v2 GPU differences, VRAM addressing, every GPUSTAT bit, the GP0 primitive set with word counts, the exact dither matrix, blend and modulation formulas, mask bits, texture windows and CLUTs, the fill rules and size limits, video clocks and display geometry as rationals, FIFO and DMA behaviour, measured primitive costs, every GTE register, opcode, cycle count and flag, the UNR division, the oracles and their licences, the accuracy model, the v1 frame format, and the hooks later enhancements need."
read_when: "Before writing or changing gpu.c, gte.c, the frame output path, GPU DMA, timers that use dotclock or hblank, or when a game's geometry, colours, blending, interlacing or GTE-driven culling diverges."
updated: 2026-10-19
---

# PSX-GPU-AND-GTE

## Where to look

| Question | Section |
|---|---|
| Which GPU revision does what; VRAM layout; GPUSTAT bits | 1 |
| GP0 command set, word counts, attributes E1 to E6, fills and copies, GP1 commands | 2 |
| Dither matrix, blend equations, modulation, mask bit, texture decode, fill rule, size limits, lines, rectangles | 3 |
| Clocks, line and frame counts, dotclocks, hblank/vblank, FIFO, DMA, primitive cost, display mapping | 4 |
| GTE registers, formats, opcodes and cycles, flags, UNR division, traps, access delays | 5 |
| Test programs, licences, what is bit-exact, what is committed vs fetched | 6 |
| Accuracy model and confidence table | 7 |
| Enhancement seeds and the v1 output frame format | 8 |
| Low-power rasteriser design | 9 |
| Open questions and the phases that settle them | 10 |

## 1. Revisions, VRAM, status
**1.1 GPU revisions** [GPU.01][GPU.10]

| Difference | v0 (160-pin CXD8514Q, early PU-8 and below) | v2 (208-pin CXD8561Q/BQ/CQ, CXD9500Q, late PU-8 and up, all PSone) |
|---|---|---|
| VRAM type | two dual-ported VRAM chips (separate DAC port) | one DRAM chip, single 32-bit bus |
| GPUSTAT after reset | 14800000h | 14802000h (bit 13 = 1 when interlace off) |
| GPUSTAT bit 15 (TBY2) | always 0 | tpage Y base bit 1 (2 MB VRAM, unused on retail) |
| GP1(10h) index 7 | nothing (latch unchanged) | 00000002h |
| GP1(10h) index 3/4 | 19-bit (Y 0..511) | 20-bit (Y 0..1023) |
| GP0(E1h) bits 12/13 | no rectangle X/Y flip | rectangle X/Y flip |
| Modulation | ((colour/8) * texel) / 2, i.e. colour cropped to 5 bits first | (colour * texel) / 16 |
| GP0(02h) fill, xpos bits 0..3 = Fh | every second pixel written to the wrong address | low 4 bits ignored |
| DMA to VRAM | fails with block size > 10h words | works with 8C0h-word blocks |
| GP0(03h) | not stored in FIFO | unknown, unused |
| Large GP0(80h) VRAM moves | no freeze observed | can freeze the GPU (amidog) |

Detection in the field: Perfect Assassin latches index 4 then index 7 and compares with 2; otherwise it writes GP0(E1h) with bit 11 set and tests GPUSTAT bit 15 (v1 arcade only) [GPU.01]. ps1-tests `version-detect` does the same [GPU.07]. Arcade v1 (CXD8538Q) and v2b (CXD8654Q) have different command sets or unknown differences and are out of scope [GPU.01].

**Choice 1.A (revision set).** Recommendation: model v2 as the default (every PSone and every late console); v0 as a machine-profile switch carrying exactly the rows above (reset status, index 7, modulation precision, fill bug, DMA block limit, no flip). Strongest alternative: v2 only. Would change if: the conformance corpus shows a title that detects v0 and changes behaviour (none known beyond the modulation look of Tomb Raider's snow level, which is cosmetic) [GPU.01].

**1.2 VRAM.** 1 MB, 512 lines of 2048 bytes, addressed as 1024 halfwords per line; coordinates (0,0) top left, not CPU-mapped, reachable only by GP0 transfers or DMA2. One halfword holds 4 four-bit texels, 2 eight-bit texels, 1 fifteen-bit texel, or two thirds of a 24-bit display pixel (3 bytes each, 682.66 per line). 2 MB (1024 lines) exists only on arcade boards [GPU.01]. Pixel: bits 0-4 R, 5-9 G, 10-14 B, 15 mask/semi-transparency flag [GPU.01]. ares and MAME both index VRAM as `vram[y & 511][x & 1023]` with wrap and no carry between X and Y [GPU.05][GPU.13].

**1.3 GPUSTAT (1F801814h read)** [GPU.01][GPU.05][GPU.09]

| Bits | Name | Meaning |
|---|---|---|
| 0-3, 4 | TBX, TBY | texture page X base (N*64), Y base (N*256), from GP0(E1h) |
| 5-6 | ABR | semi-transparency mode 0..3 |
| 7-8 | TPF | texture depth 0=4-bit, 1=8-bit, 2=15-bit, 3=reserved (acts as 15-bit) |
| 9 | DTD | dithering enabled |
| 10 | DFE | drawing to display area allowed |
| 11, 12 | PBW, PBC | force mask bit on write; skip masked pixels |
| 13 | ODE2 | interlace field (always 1 when GP1(08h).5 = 0 on v2, always 0 on v0) |
| 14 | REV | v1 screen flip, nonfunctional on v2 |
| 15 | TBY2 | tpage Y base bit 1 (2 MB) |
| 16, 17-18 | HDS2, HDS | 368 select; 256/320/512/640 |
| 19, 20, 21, 22 | VDS, NPB, LBS, IRS | 480 lines; PAL; 24-bit display; interlace |
| 23 | DMSK | display disabled |
| 24 | IRQ | GP0(1Fh) pending, cleared by GP1(02h) |
| 25 | DREQ | per GP1(04h): 0 -> 0; 1 -> write FIFO not full; 2 -> same as bit 28; 3 -> same as bit 27 (ps1-tests `gpustat` confirms 2 and 3 on hardware) |
| 26 | IDLE | ready for a command word; briefly high between primitives of a chain, so not a "chain done" flag |
| 27 | RFFL | VRAM-to-CPU data ready (GP0(C0h)) |
| 28 | WFEP | ready for DMA block; cleared when a command and its parameters are received, but for polygons and lines cleared on the command word itself |
| 29-30 | DMD | DMA direction |
| 31 | ODE | odd line being drawn; changes per frame in 480-line mode, per line in 240-line mode, 0 during vblank |

## 2. Command sets
**2.1 Ports.** GP0 1F801810h write = rendering and VRAM packets, read = GP0(C0h) data or GP1(10h) latch; GP1 1F801814h write = display control, read = GPUSTAT. GP0 has a 16-word FIFO; GP1 commands bypass it. DMA2 is equivalent to writing GP0 [GPU.01].

**2.2 GP0 decode by command bits 31-29** [GPU.01][GPU.05]

| Code | Group | Bits (polygon / line / rectangle) | Words after the command word |
|---|---|---|---|
| 000 | misc | 00h NOP (no FIFO space), 01h clear texture cache, 02h fill, 03h unknown (takes FIFO space), 04h-1Eh NOP mirrors, 1Fh IRQ1 | fill: 2 |
| 001 | polygon | 24 TGE raw texture, 25 ABE semi-transparent, 26 TME textured, 27 VTX quad, 28 IIP gouraud | 3 or 4 vertices × (1 + IIP + TME) minus 1 (first colour is in the command word) |
| 010 | line | 25 ABE, 27 PLL polyline, 28 IIP | 2 vertices × (1 + IIP) minus 1; polyline ends at a word with (w & F000F000h) == 50005000h |
| 011 | rectangle | 24 TGE, 25 ABE, 26 TME, 27-28 SIZ 0 variable, 1 1x1, 2 8x8, 3 16x16 | 1 vertex + TME + (SIZ == 0) |
| 100 | VRAM to VRAM | 80h-9Fh | src, dst, size |
| 101 | CPU to VRAM | A0h-BFh | dst, size, then data words |
| 110 | VRAM to CPU | C0h-DFh | src, size, then data read from GP0 |
| 111 | environment | E1h draw mode, E2h texture window, E3h/E4h draw area, E5h draw offset, E6h mask, E0h/E7h-EFh NOP | 0 (E3h-E5h take no FIFO space and execute immediately) |

Vertex word: X bits 0-10 and Y bits 16-26 signed 11-bit (-1024..+1023), other bits ignored. Textured vertex: `ClutVVUU` on the first vertex, `PageVVUU` on the second (page bits 0-8 and 11 replace the GP0(E1h) copies; bits 9-10 and 12-13 do not). CLUT: X in 16-halfword steps (6 bits), Y 0..511 [GPU.01]. ares applies the page word's bits 16-19 X, 20 Y, 21-22 ABR, 23-24 depth as the new GPUSTAT draw mode, which ps1-tests `gp0-e1` checks [GPU.05][GPU.07].

**2.3 Environment registers** [GPU.01]
- E1h: bits 0-3 TBX, 4 TBY, 5-6 ABR, 7-8 TPF, 9 dither, 10 draw-to-display, 11 TBY2, 12 X flip, 13 Y flip (rectangles only, v2 only).
- E2h: mask X bits 0-4, mask Y 5-9, offset X 10-14, offset Y 15-19, all in 8-texel units: `tex = (tex & ~(mask*8)) | ((offset & mask)*8)`.
- E3h/E4h: X bits 0-9 (0..1023), Y 10-18 on v0, 10-19 on v2. E5h: X bits 0-10, Y 11-21, signed 11-bit.
- E6h: bit 0 force mask bit on every written pixel, bit 1 skip pixels whose bit 15 is set; applies to rendering and to A0h/80h transfers, not to 02h fill.
- Fill 02h: `x &= 3F0h; y &= 1FFh; w = ((w & 3FFh) + 0Fh) & ~0Fh; h &= 1FFh`; no fill when w or h is 0; the 24-bit colour is truncated to 15-bit with bit 15 = 0; width 400h is reachable only through rounding of 3F1h..3FFh; height 512 masks to 0.
- Copies 80h/A0h/C0h: `x &= 3FFh; y &= 1FFh; w = ((w - 1) & 3FFh) + 1; h = ((h - 1) & 1FFh) + 1`; absolute coordinates, no draw-area clip, wrap at the VRAM edge; an odd halfword count is padded to a 32-bit word. ps1-tests `vram-to-vram-overlap` and `mask-bit` give the hardware images for overlapping copies and mask semantics [GPU.07].

**2.4 GP1 display control** [GPU.01][GPU.05]

| Cmd | Function | Parameter |
|---|---|---|
| 00h | reset: FIFO clear, IRQ ack, display off, DMA off, display start 0, X range 200h..200h+2560, Y range 10h..10h+240, mode 0, E1h-E6h = 0; GP1(09h) untouched; GPUSTAT reads 14802000h | none |
| 01h | reset command buffer and CLUT cache | none |
| 02h | acknowledge IRQ1 | none |
| 03h | display enable (0 on, 1 off; off shows black, syncs continue) | bit 0 |
| 04h | DMA direction 0 off, 1 FIFO, 2 CPU to GP0, 3 GPUREAD to CPU | bits 0-1 |
| 05h | display start in VRAM: X bits 0-9 (ares masks bit 0), Y 10-18 | |
| 06h | horizontal range X1 bits 0-11, X2 12-23 in GPU clocks from HSYNC; default 260h..260h+320*8 | |
| 07h | vertical range Y1 bits 0-9, Y2 10-19 in lines from VSYNC; NTSC centre 88h, PAL A3h | |
| 08h | bits 0-1 HDS (256/320/512/640), 2 VDS 480, 3 PAL, 4 24-bit, 5 interlace, 6 HDS2 368, 7 REV | |
| 09h | v2 only: allow Y 512..1023 (2 MB) | bit 0 |
| 10h-1Fh | latch internal register into GPUREAD: 2 texture window, 3 draw area TL, 4 BR, 5 offset, 7 version (v2 = 2), 8 = 0; others leave the latch | index bits 0-3 (v2) |
| 0Bh | unknown; values 274h..7FFh crash the GPU | |

Display width = `(((X2 - X1) / dots_per_pixel) + 2) & ~3`; height = Y2 - Y1 (doubled by the field when interlaced); nothing rounds the height. A far-too-large Y2 stops vblank IRQs. Chrono Cross shakes the screen with 07h; Pandemonium 2 uses an overscan width; PAL titles use erratic centring, so emulators auto-centre PAL [GPU.01].

## 3. Rasterisation rules
**3.1 Pixel pipeline order** (ares, agrees with psx-spx) [GPU.05][GPU.01]: fetch texel (4/8-bit through the CLUT, 15-bit direct); texel 0000h is discarded; raw or modulated; dither; read destination; blend if ABE and (untextured or texel bit 15 set); skip if destination bit 15 set and PBC; write with bit 15 = PBW | texel bit 15 (0 for untextured).

**3.2 Dither** (4x4, added to each 8-bit channel, saturated 0..255, then >> 3) [GPU.01][GPU.05]:
```
-4  +0  -3  +1
+2  -2  +3  -1
-3  +1  -4  +0
+3  -1  +2  -2
```
Applies to gouraud or modulated polygons, to all lines (even flat), never to rectangles, and only when E1h bit 9 is set. Flat untextured polygons are not dithered. (ares dithers textured semi-transparent polygons too, a divergence to test, see §10.) [GPU.01][GPU.05]

**3.3 Blend** (B = destination, F = new; per 5-bit channel, saturated): mode 0 `(B + F) / 2`; 1 `B + F` clamp 31; 2 `B - F` clamp 0; 3 `B + F / 4` clamp 31. ps1-tests `transparency` and `quad` are the hardware images [GPU.01][GPU.07]. ares computes in 8-bit space then truncates; the exact channel width used by hardware for the half and quarter is not documented, see §10.

**3.4 Modulation.** `out = (texel5 * colour8) >> 7` per channel with 80h = unity and 81h..FFh brighter, saturated to 1Fh (v2); v0 crops colour to 5 bits first (§1.1) [GPU.01][GPU.10]. ares: `min(255, (texel5 * colour8) >> 4)` in 8-bit space, equivalent [GPU.05].

**3.5 Texture decode.** `u = (u & ~(maskX*8)) | ((offX & maskX)*8)`, same for v; 4-bit: halfword `vram[(v + TBY) & 511][(u/4 + TBX) & 1023]` nibble `u & 3`; 8-bit: byte `u & 1`; CLUT entry `vram[CLY][(CLX*16 + index) & 1023]`; 15-bit: `vram[(v + TBY) & 511][(u + TBX) & 1023]` [GPU.01][GPU.05]. UV are 8-bit, so a texture is at most 256x256; rectangles larger than 256 repeat the source [GPU.01]. Texture cache: 2 KB, 64x64 (4-bit), 32x64 (8-bit), 32x32 (15-bit) with 256 eight-byte entries per block and one entry per index; the CLUT cache survives between commands and is invalidated by a different CLUT address or GP1(01h). ps1-tests `clut-cache` shows the stale-cache effect [GPU.01][GPU.07].

**3.6 Triangle rules** [GPU.01][GPU.05]
- Quads are two triangles, vertices (1,2,3) then (2,3,4); vertex order does not matter for visibility (no GPU culling).
- Polygons cover pixels up to but excluding the bottom-right edges; ares implements a top-left rule by biasing edge functions whose edge vector has `dx < 0 || (dx == 0 && dy < 0)` by -1 after making the triangle clockwise, and skips colinear (area 0) triangles.
- Size rejection: a polygon or line whose vertices span more than 1023 horizontally or 511 vertically is not drawn at all (a line from Y -300 to +300 is dropped, -100 to +400 is drawn). ares tests `> 1024 || > 512` on the bounding box, a one-off difference to settle (§10). Rectangles are limited by the 10/9-bit size fields instead.
- Clipping to the draw area is per pixel on hardware (whether clipped pixels cost time is unknown); draw offset is added before clipping.
- Interpolation precision for colour and UV is not documented by psx-spx; ares uses floats from barycentric deltas, which is a bit-mismatch risk. ps1-tests `uv-interpolation` draws 256 quads of width 0..255 with a two-texel-wide UV span and `triangle` draws a gouraud triangle; their vram.png files are the arbiter for the stepper precision (§6).

**3.7 Lines.** Bresenham-class: both end pixels are drawn (inclusive); coincident endpoints give a 1x1 pixel in the first vertex colour; always dithered; polylines chain each vertex to the previous one; the terminator sits in the first word of a vertex (the colour word for gouraud) [GPU.01]. ares steps `max(|dx|,|dy|)` times with 16.16 increments and draws no shaded lines yet; ps1-tests `lines` is the hardware image for every mode [GPU.05][GPU.07].

**3.8 Rectangles.** Single pass, no dither, no gouraud, axis-aligned, texture from the E1h page, UV increment per pixel with X/Y flip decrementing instead (v2 only); odd UV or odd width give a one-pixel sampling error every 16 pixels [GPU.01]. ps1-tests `rectangles`, `texture-flip` and `texture-overflow` are the images; `texture-overflow` shows UV running past the VRAM width [GPU.07].

**Choice 3.A (rasteriser).** Recommendation: integer edge-function triangle with the ares top-left bias, 32-bit fixed-point colour and UV steppers whose fraction width is chosen so that `uv-interpolation` and `triangle` match pixel for pixel (start with 16 fractional bits, adjust to the test); lines as integer DDA over the major axis; rectangles as a separate span loop. Strongest alternative: scanline edge walking per DuckStation's documented approach (not read). Would change if: the two ps1-tests images cannot be matched with a per-span integer stepper, which would point at a per-scanline division that must be reproduced.

## 4. Timing, DMA and display
**4.1 Clocks** [GPU.01][GPU.05]

| Item | NTSC console | PAL console |
|---|---|---|
| GPU clock | 53.693175 MHz | 53.203425 MHz (PSone PAL board: CPU 33.8688 MHz × 11/7 = 53.2224 MHz) |
| GPU clocks per line | 3413 (nocash: "or 3413.6") | 3406 ("or 3406.1") |
| Lines per frame | 263 progressive, 262.5 per field interlaced | 314 progressive, 312.5 interlaced |
| Refresh, nominal (psx-spx) | 59.826 Hz progressive, 59.940 interlaced | 49.761 Hz progressive, 50.000 interlaced |
| Refresh, emulated and reported (263 × 3413, 314 × 3406 clocks per frame; ARCHITECTURE 1) | 53,693,175/897,619 = 59.8173 Hz progressive (derived) | 53,203,425/1,069,484 = 49.7468 Hz progressive (derived) |
| Refresh, other mode | PAL mode on NTSC clock 50.219 / 50.460 | NTSC mode on PAL clock 59.280 / 59.393 |
| Dot dividers | 10 (256), 8 (320), 7 (368), 5 (512), 4 (640) | same |
| Dots per line | 341.3, 426.625, 487.5714, 682.6, 853.25 | 340.6, 425.75, 486.5714, 681.2, 851.5 |
| Dotclocks (MHz) | 5.3693175, 6.7116469, 7.6704536, 10.738635, 13.4232938 | 5.3203425, 6.6504281, 7.6004893, 10.640685, 13.3008563 |

A console always runs its region's crystal whatever GP1(08h).3 says, so the other mode drifts [GPU.01]. Timer 0 counts whole dots per line (fractions dropped, 425.75 rounds up); timer 1 counts hblanks, which also occur during vblank; the hblank and vblank of the timers are not exactly the video signal's [GPU.01][GPU.04]. ares models the line as `hblankStart = X2` then `htotal`, raises vblank at `vcounter == Y2` and lowers it at `Y1`, toggles the field at frame end, and reports GPUSTAT bit 31 as `!vblank && (interlace ? !field : vcounter & 1)` [GPU.05].

**4.2 FIFO and ready bits.** 16 words; GP0(00h), the NOP mirrors and E3h-E5h take no space; GP1 bypasses it. Bit 28 falls on the command word for polygons and lines, so DMA mode 2 must deliver a primitive's parameters in the same block [GPU.01]. ps1-tests `gpustat` passes a 12-case matrix (CPU, linked-list DMA, sync DMA with directions 1 and 2, before/during/after rendering) on hardware; its expectations are in its source [GPU.09].

**4.3 DMA2.** Sync mode 1 (blocks, BS up to 10h words for the GPU) for A0h/C0h data, sync mode 2 (linked list; header = 24-bit next pointer plus 8-bit word count, FFFFFFh ends) for command lists, DMA6 builds the reverse-linked ordering table; CHCR 01000200h read, 01000201h write, 01000401h list; 1 clock per word plus about 17 clocks per 16 words of row setup; the CPU keeps running from cache, scratchpad, COP0 and GTE and a 4-entry write queue, and resumes between blocks and list nodes. Nocash observed that continuous (VRAM) DMA outside vblank loses words on a v0 GPU; linked-list DMA works anywhere. Polygon and VRAM transfer times are "unknown" to psx-spx [GPU.03][GPU.01]. ps1-tests `dma/chopping` and `chain-looping` cover chopping and a self-referencing list [GPU.07].

**4.4 Primitive cost (community measured).** ps1-tests `gpu/bandwidth` on a retail console, timed in hblanks [GPU.08]:

| Operation | Throughput | ares-derived GPU clocks per pixel |
|---|---|---|
| Fill GP0(02h) | 779 MB/s | 0.081 |
| VRAM to CPU / CPU to VRAM / VRAM to VRAM | 60 / 77 / 49 MB/s | (transfer, not pixels) |
| Rectangle flat / semi-transparent | 124 / 82 MB/s | 0.530 / 0.797 |
| Rectangle textured / textured semi | 62 / 62 MB/s | 1.188 / 1.497 |
| Quad flat / semi | 122 / 81 MB/s | 0.538 / 0.812 |
| Quad textured / textured semi | 23 / 23 MB/s | 2.980 / 3.463 |
| Quad 1/4 and 1/2 off screen | 162 / 239 MB/s | clipped pixels still cost something |

ares converted these to clocks per pixel but disabled the model with the note that the figures are "off by a factor of three in other games" [GPU.05]. Lines are unmeasured. Marketing peaks (66 Mpix/s flat, 33 Mpix/s gouraud or textured) are (unverified) [GPU.19]. ps1-tests `gpu/benchmark` is an interactive rate meter for any mode and is the tool for filling the gaps [GPU.07].

**Choice 4.A (GPU busy model).** Recommendation: v1 keeps a GPU clock budget per primitive: setup constant plus pixels × a per-class cost table seeded from the ares-derived column, charged after rendering the primitive at once; GPUSTAT bits 26 and 28 read 0 while the budget is unspent, DMA2 stalls on them, and the budget drains in GPU clocks on the master tick. Strongest alternative: instantaneous drawing with bits 26/28 always 1 (what ares ships). Would change if: a corpus title tears or skips frames because it polls bit 26 or chains DMA at a rate that only a cost model reproduces; or if a measured line and clipped-pixel table arrives, which replaces the seed values.

**4.5 Mapping the display area to an image.** Source: GP1(05h) start X,Y in VRAM, width from GP1(06h) with the resolution's divider, height Y2-Y1 (×2 interlaced). ares subtracts a per-width nominal X1 (228h for 256, 260h otherwise) and nominal Y1 (10h NTSC 240, 20h PAL 256) to place the picture in a 640x512 canvas and draws black elsewhere [GPU.05]. Titles that move the window with 06h/07h (Chrono Cross) are reproduced only if the host honours the offset [GPU.01]. The wobble of vertices and the affine "swimming" of textures are real hardware: the GTE emits integer screen coordinates and the GPU interpolates UV linearly in screen space with no Z, so a native-resolution core must keep them [GPU.01][GPU.18].

## 5. GTE (COP2)
**5.1 Registers** (data cop2r0-31, control cop2r32-63; formats s = sign bits, i = integer bits, f = fraction bits) [GPU.02]

| Regs | Name | Format | Notes |
|---|---|---|---|
| 0-5 | VXY0/VZ0, VXY1/VZ1, VXY2/VZ2 | 3 × s16 | VZ sign-extended on read |
| 6 | RGBC | 4 × u8 | R, G, B, CODE |
| 7 | OTZ | u16 | AVSZ result |
| 8 | IR0 | 1.3.12 | interpolation factor, saturates 0..1000h |
| 9-11 | IR1-3 | s16 | saturate -8000h..7FFFh or 0..7FFFh (lm) |
| 12-15 | SXY0-2, SXYP | 2 × s16 | 3-stage FIFO; SXYP write pushes, SXY2 write does not |
| 16-19 | SZ0-3 | u16 | 4-stage FIFO |
| 20-23 | RGB0-2, RES1 | 4 × u8 | 3-stage colour FIFO; RES1 is an inert R/W slot |
| 24-27 | MAC0-3 | s32 | never saturated |
| 28, 29 | IRGB, ORGB | u15 | 5:5:5 to IR1-3 (× 80h) and back (/ 80h saturated, no flags); IRGB readable after 2-3 nops |
| 30, 31 | LZCS, LZCR | s32, 1..32 | leading zeros (positive) or ones (negative) |
| 32-36 | RT11..RT33 | 1.3.12 | RT33 alone in r36, sign-extended |
| 37-39 | TRX,TRY,TRZ | s32 | |
| 40-44, 48-52 | L (light), LC (colour) matrices | 1.3.12 | same packing as RT |
| 45-47 | RBK,GBK,BBK | 1.19.12 | |
| 53-55 | RFC,GFC,BFC | 1.27.4 | |
| 56-57 | OFX,OFY | 1.15.16 | |
| 58 | H | u16 | read sign-extends (bug), maths uses it unsigned |
| 59, 60 | DQA, DQB | 1.7.8, 1.7.24 | |
| 61-62 | ZSF3, ZSF4 | 1.3.12 | |
| 63 | FLAG | u20 at bits 12-31 | §5.3 |

Writing a 32-bit value to a 16-bit register truncates without flags [GPU.02]. ares stores exactly these packings [GPU.06].

**5.2 Command encoding and opcodes.** COP2 imm25: bits 25-31 = 0100101b, 20-24 fake number (ignored), 19 sf (shift MAC by 12), 17-18 MVMVA matrix (RT, L, LC, garbage), 15-16 MVMVA vector (V0, V1, V2, IR), 13-14 MVMVA translation (TR, BK, FC bugged, none), 10 lm, 0-5 real opcode [GPU.02].

| Op | Name | Cycles | Op | Name | Cycles |
|---|---|---|---|---|---|
| 01h | RTPS | 15 | 1Ch | CC | 11 |
| 06h | NCLIP | 8 | 1Eh | NCS | 14 |
| 0Ch | OP | 6 | 20h | NCT | 30 |
| 10h | DPCS | 8 | 28h | SQR | 5 |
| 11h | INTPL | 8 | 29h | DCPL | 8 |
| 12h | MVMVA | 8 | 2Ah | DPCT | 17 |
| 13h | NCDS | 19 | 2Dh | AVSZ3 | 5 |
| 14h | CDP | 13 | 2Eh | AVSZ4 | 6 |
| 16h | NCDT | 44 | 30h | RTPT | 23 |
| 1Bh | NCCS | 17 | 3Dh | GPF | 5 |
| 3Eh | GPL | 5 | 3Fh | NCCT | 39 |

The 42 other opcodes are undocumented; amidog's test exercises all 64 and the sf/lm/MVMVA bits on every one [GPU.02][GPU.11]. Wipeout 2097 issues NCLIP as 0140006h (wrong fake number and bit 18) and must still work [GPU.02].

**5.3 FLAG** bits: 30/29/28 MAC1-3 > 43 bits positive, 27/26/25 negative, 24/23/22 IR1-3 saturated, 21/20/19 colour FIFO R/G/B saturated, 18 SZ3 or OTZ saturated, 17 divide overflow, 16/15 MAC0 > 31 bits positive/negative, 14/13 SX2/SY2 saturated to -400h..3FFh, 12 IR0 saturated, 31 = OR of 30-23 and 18-13. Cleared at the start of every command; bits 12-30 writable [GPU.02]. ares checks MAC after every addition in the matrix sum and clips the accumulator to 44 bits between terms [GPU.06]. amidog observed that once an addition overflows, later additions in the same MVMVA keep setting flags, so a negative overflow followed by a valid result sets both positive and negative bits; his model is an internal sticky overflow using bits 43 and 44 [GPU.10].

**5.4 Core formulas** [GPU.02][GPU.06]
- RTPS/RTPT: `MAC1-3 = (TR << 12 + RT · V) >> (sf*12)`; IR1-3 from MAC with lm; `SZ3 = MAC3 >> ((1-sf)*12)` saturated 0..FFFFh; `n = divide(H, SZ3)`; `MAC0 = n*IR1 + OFX, SX2 = MAC0 >> 16` saturated -400h..3FFh, same for Y; `MAC0 = n*DQA + DQB, IR0 = MAC0 >> 12` saturated 0..1000h (only on the last vertex of RTPT in ares). The IR3 saturation flag is evaluated as if lm = 0 on `MAC3 >> 12` while the stored IR3 obeys lm (and with sf = 0 the flag only reflects the shifted value).
- NCLIP: `MAC0 = SX0*SY1 + SX1*SY2 + SX2*SY0 - SX0*SY2 - SX1*SY0 - SX2*SY1` (sign = winding; 0 = degenerate).
- AVSZ3/4: `MAC0 = ZSF3*(SZ1+SZ2+SZ3)` or `ZSF4*(SZ0+..+SZ3)`, `OTZ = MAC0 >> 12` saturated 0..FFFFh.
- MVMVA: as RTPS without the projection; cv = 2 (FC) is bugged: the first product is dropped (`MAC = (Mx·2*V2 + Mx·3*V3) >> sf`) while flags behave as if full; mx = 3 is a garbage matrix `[-R<<4, R<<4, IR0; RT13 ×3; RT22 ×3]`.
- Colour: NCx = `IR = (L·V) >> sf`, `IR = (BK << 12 + LC·IR) >> sf`; NCCx then `MAC = (RGB << 4) * IR`; NCDx then depth cue `MAC = MAC + (FC - MAC) * IR0` where `(FC << 12 - MAC) >> sf` is first saturated into IR as if lm = 0; CC/CDP skip the L step; DCPL/DPCS/DPCT/INTPL are the cue alone from `RGB*IR << 4`, `RGB << 16`, `IR << 12`; DPCT runs three times reading RGB0 each time; GPF = `IR*IR0 >> sf`, GPL = `(MAC << sf + IR*IR0) >> sf` (44-bit overflow possible); SQR = `IR*IR >> sf`; OP = cross product of IR with (RT11, RT22, RT33). Colour FIFO gets `MAC >> 4` saturated 0..FFh with CODE from RGBC.

**5.5 UNR division** [GPU.02][GPU.06]: if `H >= SZ3*2` then n = 1FFFFh with flags 17 and 31; else `z = clz16(SZ3); n = H << z; d = SZ3 << z; u = table[(d - 7FC0h) >> 7] + 101h; d = (2000080h - d*u) >> 8; d = (80h + d*u) >> 8; n = min(1FFFFh, (n*d + 8000h) >> 16)`. Table: `table[i] = max(0, (40000h / (i + 100h) + 1) / 2 - 101h)` for i = 0..255 and `table[256] = 0` (ares generates it this way; psx-spx prints the 257 bytes). Results of exactly 20000h (FE3Fh/7F20h, F015h/780Bh) saturate without flags. (derived) The generator and the printed table agree entry for entry; the conformance phase should assert that at startup.

**5.6 Access and delays** [GPU.02][GPU.06]: MFC2/CFC2 load the GPR with a one-instruction delay like a memory load (Tekken 2 breaks without it); MTC2/CTC2/LWC2/SWC2 are immediate in ares; psx-spx's "2 instruction" note for loads is marked doubtful by its own author. A GTE register read or a new command while a command is in flight stalls the CPU for the remaining cycles from the table above; the CPU otherwise runs on. Writing FLAG recomputes bit 31. COP2 disabled in SR raises a coprocessor-unusable exception.

**Choice 5.A (GTE implementation).** Recommendation: a straight integer implementation of §5.4 with 64-bit accumulators, 44-bit clipping between terms, the ares flag placement, the UNR table from the generator, the IR3/lm quirk, the FC and mx = 3 bugs, the H sign-extend read bug, a per-command busy counter from §5.2 that stalls the next COP2 access, and the MFC2/CFC2 load-delay slot shared with the CPU's load delay logic. Strongest alternative: a table-free exact division (fails amidog). Would change if: amidog's flag-chaining model (§5.3) is confirmed by his test to differ from per-term checks; then the MVMVA accumulator gains a sticky overflow bit.

## 6. Tests and oracles

| Suite | Pin / date | Licence | GPU/GTE coverage | Exact oracle | Commit or fetch |
|---|---|---|---|---|---|
| JaCzekanski/ps1-tests | f727802fead1 (2023-03-05), release build-158 tests.zip (2021-04-22) | MIT (LICENSE checked) | gpu: clipping, clut-cache, gp0-e1, gpustat, lines, mask-bit, quad, rectangles, texture-flip, texture-overflow, transparency, triangle, uv-interpolation, version-detect, vram-to-vram-overlap, bandwidth, benchmark; gte: test-all (3.8 MB of generated cases), gte-fuzz with a hardware log; dma: chopping, chain-looping, otc-test | vram.png hardware captures for 11 GPU tests, psx.log hardware output for gpustat, bandwidth, gp0-e1, mask-bit, and the GTE register dumps | commit sources, images and logs with notice; `tools/diffvram` is the comparator |
| amidog psxtest_gte, psxtest_cpu, psxtest_cpx, psxtest_gpu | psx.amidog.se, pages last modified 2019-07-02, accessed 2026-10-19 | wiki CC BY-NC-SA 3.0; the zips state no licence | GTE: every register, all 64 opcodes, sf/lm/MVMVA bit combinations against a software model, hours on hardware, stops after N failures; GPU: every primitive and "other aspects", results on screen | on-screen pass/fail; no file dumps | fetch into the untracked test cache, never vendor; record results in CONFORMANCE |
| PeterLemon/PSX | 6d20c132aba0 (2026-01-20) | no LICENSE file, none in README | GPU 16BPP and 24BPP display demos, CPUTest, MDEC, SPU; tested on hardware by the author | none shipped | fetch only |
| ares ps1/gpu and ps1/cpu/interpreter-gte.cpp | 9408cb43d494 (2026-10-06) | ISC (LICENSE checked) | reference implementation, no timing model | n/a | read and seed with notice |
| MAME src/devices/video/psx.cpp | 96df6a708bdd (master, header checked, `license:BSD-3-Clause`, smf) | BSD-3 | reference GPU | n/a | may be read; not read for this file |
| Avocado | d7fcf9060ac7 | **GPL-2.0 at the pin (LICENSE checked), not MIT as the brief says** | n/a | n/a | never open; facts only from its docs |
| Rustation and its guide | rustation 5e650b949d6e, psx-guide a89043ea499a | **rustation LICENSE is GPL-2 or later; psx-guide has no LICENSE file** | n/a | n/a | never open the emulator; the guide text is unlicensed prose, cite only |
| DuckStation, Mednafen | README 124a4b16feda; docs page | PolyForm Strict; GPL-2 | docs and issues only | n/a | never open |

The hardware images in ps1-tests are 1024x512 PNGs of VRAM; a bit-exact comparison needs the core to expose the whole VRAM, not the display window (§8). The GTE `test-all` cases carry 64 input and 64 output words each and run in seconds, so they are the CI oracle; amidog's is the hours-long release gate.

## 7. Accuracy model and confidence

| Behaviour | Status | Settled by |
|---|---|---|
| Command decode, word counts, environment registers, fill and copy masking | specified (psx-spx, ares, ps1-tests) | ps1-tests clipping, rectangles, mask-bit, vram-to-vram-overlap |
| Dither matrix, blend equations, modulation, mask semantics | specified; channel width of blend arithmetic inferred | transparency, quad, triangle images |
| Top-left fill rule, quad split, colinear drop, size rejection 1023/511 | specified in prose; ares bias is one implementation | quad (fill rules), a harness ROM for the 1023/1024 boundary |
| Gouraud and UV stepper precision | inferred (not documented) | uv-interpolation, triangle images |
| Line algorithm | inferred (ares DDA, "Bresenham" per ps1-tests) | lines image |
| Texture and CLUT caches | specified in size and shape; replacement inferred | clut-cache image; cache staleness is a later seed |
| Clocks, dividers, line and frame counts | specified | timer tests in ps1-tests/timers, SYS timing harness |
| hblank/vblank edges relative to X1/X2/Y1/Y2 and GPUSTAT bit 31 | community (ares model) | a harness ROM reading GPUSTAT and timer 1 per line |
| FIFO depth, ready bits | specified; hardware-logged | gpustat log |
| Primitive cost | measured once (bandwidth), lines absent, factor-of-three caveat | benchmark runs on hardware vs core, corpus frame pacing |
| GTE arithmetic, flags, division | specified and hardware-dumped | test-all, gte-fuzz log, amidog |
| GTE cycle counts and stalls | specified by table, not hardware-timed here | amidog timing items if present, SYS instruction timing harness |
| v0 quirks | specified by two sources | a v0 console (the owner has none); profile stays documentation-driven |

**Choice 7.A (accuracy model).** Recommendation: the GPU runs on the master tick with a line-granular state machine (hcounter, vcounter, field, dot accumulator for timer 0, hblank to timer 1, vblank IRQ at Y2 and release at Y1 as ares does), renders primitives immediately into VRAM when their last word arrives, and charges the §4.4 budget to the ready bits. Strongest alternative: deferred rendering at vblank (faster, breaks mid-frame VRAM reads and GP0(C0h)). Would change if: a corpus title reads back VRAM or GPUSTAT mid-frame in a way immediate rendering mishandles (none expected).

## 8. Enhancement seeds and the v1 frame format
| Later feature | What v1 must keep so it needs no redesign |
|---|---|
| Internal resolution scaling | primitives reach the rasteriser as a struct of integer vertices plus attributes; the rasteriser is one function behind a `gpu_backend` pointer; VRAM stays 1024x512x16 as the source of truth for textures and readback |
| True colour (no 15-bit truncation, no dither) | the pixel write path takes 8-bit channels and truncates in one place; the dither table is a flag |
| PGXP geometry precision and perspective-correct texturing | gte.c exposes the pre-truncation MAC0/IR values and the SZ per vertex through a debug sideband keyed by the SXY FIFO; the GP0 vertex parser keeps a slot for sub-pixel data and W that v1 leaves zero; the libretro write-up explains why both halves are needed [GPU.18] |
| Widescreen hack | the GTE OFX/H path is one function; a host-side aspect override needs the native display rectangle metadata, not a core change |
| Texture filtering | the texel fetch is one function receiving fractional UV; v1 passes integers |
| Deinterlacing | the core emits fields with the field bit and line parity in metadata; a host weaves, bobs or line-doubles; v1 never merges fields itself |
| CRT shaders | host side; the core provides native PAR (dot divider and region) in metadata |

**Choice 8.A (v1 output frame).** Recommendation: the core delivers, per vblank, (a) a read-only view of the whole 1024x512 VRAM in native `uint16_t` (hash domain for ps1-tests), and (b) a display frame: width = `(((X2 - X1) / div) + 2) & ~3`, height = Y2 - Y1, one row per displayed line of the current field, pixels as `uint16_t` 0BBBBBGGGGGRRRRR in 15-bit mode and as packed 24-bit RGB rows of 3 bytes per pixel, straight from the VRAM bytes, in 24-bit mode (both lossless and one copy, so the frame hash is stable across hosts; the XRGB8888 conversion is the host's, HOSTS 1), with metadata {field, interlaced, vres 240/480, hres, dot divider, region, X1, Y1, display start, display enabled, 24-bit}. Display-off frames are all zero with the flag set. Interlaced output is field-sequential at the native 240-row height; the host doubles or weaves. The hash covers the display frame plus the metadata tuple, not the black border placement, which is a host decision following the ares nominal offsets (§4.5). Strongest alternative: a fixed 640x480 RGB888 canvas like ares. Would change if: the conformance harness needs sub-frame (mid-frame) captures, which would add a line-range capture API rather than change the format.

## 9. Low-power considerations
- 320x240 at 60 Hz is 4.6 Mpixel/s of display; typical scenes write a few hundred thousand polygon pixels per frame; a span-based integer rasteriser on a 1 GHz in-order ARM core or wasm32 spends well under a core on this, provided there is no per-pixel division or float conversion (derived from §4.4 throughput: the console itself peaks around 60 Mpixel/s on flat quads).
- Keep all steppers in 32-bit fixed point set up once per triangle (edge functions and attribute deltas computed from the 2D cross product with one integer reciprocal per triangle, or per-row divides of at most 512 per frame), per-pixel work = three adds, one compare, texel fetch, table dither, and a 16-bit store; blending reads the destination only when ABE is set.
- Precompute the 4x4x256 dither table, a 65536-entry 15-bit to 8-bit expansion only if needed, and CLUT rows as 16-entry caches; avoid branches per pixel by templating the eight pixel variants (texture, raw, alpha, dither) as the ares template does.
- VRAM as one `uint16_t[512][1024]` array (1 MB) is cache-friendly for spans; textured spans walk VRAM rows, so keep texture page base and window masks hoisted.
- wasm32 has no 64-bit multiply fast path in some engines; keep GPU maths 32-bit, and confine 64-bit to the GTE accumulators where it is required.

## 10. Open questions (settled by phase)
- Blend arithmetic width (5-bit per channel or 8-bit then truncate) and whether textured semi-transparent polygons dither: ps1-tests `transparency`/`quad` images decide. Phase: GPU rasteriser.
- Size rejection boundary 1023/511 (psx-spx) vs 1024/512 (ares): harness ROM with a 1024-wide polygon. Phase: GPU rasteriser.
- Attribute stepper precision and rounding: match `uv-interpolation` and `triangle`. Phase: GPU rasteriser.
- Primitive costs beyond the bandwidth table (lines, clipped pixels, 4-bit vs 15-bit textures, cache misses): run `benchmark` on hardware and the core. Phase: GPU timing.
- hblank/vblank edge positions and GPUSTAT bit 31 parity vs timer 1: harness ROM. Phase: SYS timing.
- GTE flag chaining in MVMVA (amidog model) vs per-term checks: amidog psxtest_gte. Phase: GTE.
- GTE busy stall exactness: no hardware timing beyond the table; defer. Phase: later seed.
- Brief corrections: Avocado and Rustation are GPL at their pins, so PROVENANCE must drop them from the "may open" list; the psx-guide has no licence file.

## Sources

| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| GPU.01 | T2 | psx-spx consoledev edition, GPU chapter (graphicsprocessingunitgpu) | accessed 2026-10-19 | ports, FIFO, commands, attributes, fills, copies, GP1, GPUSTAT, versions, VRAM, caches, clocks, dither, blend, modulation, perspective notes |
| GPU.02 | T2 | psx-spx, GTE chapter (geometrytransformationenginegte) | accessed 2026-10-19 | registers, formats, encoding, opcodes and cycles, flags, formulas, UNR division and table, delays |
| GPU.03 | T2 | psx-spx, DMA chapter (dmachannels) | accessed 2026-10-19 | DMA2 modes, block sizes, linked lists, transfer rates, CPU during DMA |
| GPU.04 | T2 | psx-spx, Timers chapter | accessed 2026-10-19 | dotclock and hblank sources, counter reset behaviour |
| GPU.05 | T3 | ares `ares/ps1/gpu/{gpu,gp0,gp1,io,renderer,blitter}.cpp`, `gpu.hpp` (ISC, LICENSE checked) | 9408cb43d494 | line model, status bits, decode, top-left bias, size test, dither and blend code, display mapping, derived cost table |
| GPU.06 | T3 | ares `ares/ps1/cpu/interpreter-gte.cpp` (ISC) | 9408cb43d494 | register packing, MAC checks, saturation, division, RTP, MVMVA bugs, access instructions |
| GPU.07 | T3 | github.com/JaCzekanski/ps1-tests README, `gpu/*`, `gte/test-all`, `gte-fuzz`, `dma/*` (MIT, LICENSE checked) | f727802fead1; release build-158 | test list, expected images, GTE case format, fuzz log |
| GPU.08 | T1, measured | ps1-tests `gpu/bandwidth/psx.log` (hardware run, hblank-timed) | f727802fead1 | throughput table |
| GPU.09 | T1, measured | ps1-tests `gpu/gpustat/psx.log` | f727802fead1 | DREQ semantics, 12-case ready-bit matrix |
| GPU.10 | T2 | AmiDog's PSX Wiki, pages psx:gpu and psx:gte:flags (CC BY-NC-SA 3.0) | last modified 2019-07-02, accessed 2026-10-19 | v0/v2 table incl. modulation and VRAM move freeze, flag chaining model |
| GPU.11 | T2 | AmiDog's PSX Wiki, psx:download:gte, :gpu, :cpu | accessed 2026-10-19 | test descriptions, run time, no licence stated |
| GPU.12 | T3, facts only | github.com/PeterLemon/PSX README and tree (no LICENSE) | 6d20c132aba0 | demo list, hardware-tested claim |
| GPU.13 | T3 | github.com/mamedev/mame `src/devices/video/psx.cpp` header only | 96df6a708bdd | BSD-3-Clause header, smf copyright |
| GPU.14 | T4 | github.com/stenzek/duckstation README (docs only) | 124a4b16feda | feature list: upscaling, texture filtering, true colour, PGXP, motion adaptive deinterlacing |
| GPU.15 | T2 | mednafen.github.io/documentation/psx.html | accessed 2026-10-19 | psx.slstart/slend, h_overscan, correct_aspect, video.deinterlacer default weave |
| GPU.16 | T3 | github.com/JaCzekanski/Avocado LICENSE | d7fcf9060ac7 | GPL-2.0, not opened |
| GPU.17 | T3 | github.com/simias/rustation LICENSE; github.com/simias/psx-guide tree | 5e650b949d6e; a89043ea499a | GPL-2+; no licence file |
| GPU.18 | T4 | libretro.com/?p=45256 (Beetle PSX PGXP write-up) | accessed 2026-10-19 | wobble = integer vertices, warping = affine UV; PGXP needs both sub-pixel precision and perspective correction |
| GPU.19 | T4 | en.wikipedia.org/wiki/PlayStation_technical_specifications | accessed 2026-10-19 | marketing fill rates (unverified) |
| GPU.20 | T4 | tasvideos.org/Forum/Topics/20751 | accessed 2026-10-19 | Mednafen presents interlaced output as alternating fields in a 480-line buffer |
