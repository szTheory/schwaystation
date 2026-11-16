# MDEC

The MDEC at `1F801820h` and `1F801824h` decodes coefficient streams and exposes
decoded blocks to DMA1. The implementation supports monochrome 4/8-bit and
color 15/24-bit output. Dense coefficient blocks resume across bounded input
FIFO refills. Hardware facts and open questions are collected in
[the preparation note, section 4](../../.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md#4-mdec),
with the underlying psx-spx and ps1-tests sources listed there.

## Commands and input

Command 1 decodes blocks. Its fields select output depth, signed output, bit 15,
and the number of following input words. Command 2 loads luma and optional
chroma quantization tables; command 3 loads 64 signed scale-table halfwords.
Command 1 uses the luma table for mono and Y blocks, and the chroma table for
Cr/Cb blocks. DMA0 writes input words; CPU writes to the data port use the same
FIFO path. DMA0 requests a new transfer block only when the input FIFO is empty,
so a 32-word DMA block fits in full (SCM.10).
Command bits 28-25 are mirrored into STAT bits 26-23 for all commands. MDEC(0)
and invalid commands 4-7 also copy command bits 15-0 directly into STAT bits
15-0 without applying the decode command's remaining-word minus-one rule
([SCM.04]).

A block starts with a signed 10-bit DC value and a six-bit quantization scale.
When the scale is nonzero, run-length words carry a six-bit zero run and
signed 10-bit AC value in zigzag order; the decoder applies the quantization
table and clips coefficients to 11 bits. At scale zero, DC and AC values are
doubled, bypass the quantization table, and AC coefficients use natural order
instead of zigzag. The pinned color-frame stream exercises this special mode
with nonzero AC values (SCM.04, SCM.10). Blocks normally end with `FE00h`;
`FE00h` before a DC value is padding. One bounded first-caller exception is recorded from the
pinned ps1-tests `mdec/4bit` guest: its 32-word decode command ends after
coefficient 63 with no EOB, and the completed 64-coefficient block is published
at that command boundary ([SCM.12], guest pin and member digest in `pins.txt`).
This is an observed caller case, not general acceptance of missing EOBs.
Parsing remains bounded by the 64-coefficient block and command input count;
malformed runs or an exhausted command with an incomplete block abort. An RLE
run that reaches or passes coefficient 64 closes the block, as does explicit
`FE00h` EOB.
When EOB is in a word's low half, the high half remains queued for the next
block, including across save and load.

## Decode and output

The inverse transform uses two fixed point matrix passes with the signed
Q14 scale table and 64-bit intermediate sums. Each pass rounds with `+8000h`
before division by `10000h`; intermediate samples are clipped to the supported
range. This is the deterministic implementation rule. The hardware source
describes the two-pass transform but does not settle its exact rounding.

Mono samples are clipped to 8 bits. Unsigned output flips bit 7 of each sample;
signed output keeps the signed byte. For unsigned 4-bit output, the pinned
`mdec/4bit` and `mdec/8bit` hardware logs show each 4-bit pixel equals the
nearest 4-bit level of the corresponding 8-bit pixel, using `min(15, (value +
8) >> 4)` ([SCM.12], log member digests in `pins.txt`). This is an observed
cross-mode hardware result; psx-spx does not specify the rounding rule. Signed
4-bit output retains the existing high-nibble conversion. Four-bit output
stores the least-significant pixel first, with eight pixels in each 32-bit
FIFO word. Eight-bit output stores four pixels per word in little-endian order.
DMA1 drains the FIFO into consecutive RAM words and advances MADR by four bytes
per word.

Color commands decode macroblock components in Cr, Cb, Y1, Y2, Y3, Y4 order.
Each chroma sample covers a 2x2 group of luma pixels. Integer Q16 conversion
uses 91881, 22524, 46811 and 116130 for the red, green and blue terms; channels
clip to signed 8-bit range, then unsigned output flips bit 7. Fifteen-bit
output packs RGB555, with command bit 25 copied to pixel bit 15; DMA1 exposes
the four 8x8 luma blocks in Y1-Y4 order, and the caller swizzles them to a
16x16 image. Twenty-four-bit output packs RGB bytes and interleaves Y1/Y2 and
Y3/Y4 rows into a 16-pixel-wide strip, ready for DMA2 GPU upload.

The registered `mdec.words` control hashes raw mono FIFO words before DMA1 and
separately checks the RAM words and final MADR. `mdec.color` hashes raw 15-bit
FIFO words before DMA1, then checks DMA1 bytes, caller reordering and displayed
pixels. `mdec.display` checks the 24-bit DMA0/DMA1/DMA2 path through native
scanout.

The final IDCT pass applies a signed 9-bit wrap before saturating to signed 8-bit,
matching the pinned ares implementation [SCM.10]. The monochrome output stage
also limits samples to signed 9-bit before signed 8-bit saturation [SCM.04].

The `mdec.vectors` control adds nonuniform AC output words to detect pixel
ordering changes, checks positive and negative signed clipping endpoints, and
compares a block preceded by `FE00h` padding with its unpadded equivalent.

## Readiness and timing

Status bit 31 reports an empty output FIFO, bit 30 a full input FIFO, bit 29 a
command or block in progress, bit 28 DMA0 readiness, and bit 27 DMA1 readiness.
The decoder makes output available at a selected 1,000-cycle block deadline;
the model test checks status immediately before and at that boundary. The
pinned ps1-tests step log records status ordering but contains no cycle stamps,
so 1,000 cycles is an implementation baseline informed by the cited ares
comment, not a measured hardware timing result.

The input FIFO holds 32 words; parser state preserves the current coefficient
scan and block across refills, even when one block is denser than the FIFO. The
output FIFO holds 192 words. A completed block waits with no scheduled
retry if the whole block does not fit; DMA1 reads resume it as soon as enough
space opens. This prevents dropping a block and avoids a same-tick scheduler
loop. State format 16 stores the input halfword cursor and partial coefficient
scan, pending-block state, decoded Cr/Cb planes and the staged 24-bit luma
block so save/load preserves stream and backpressure progress.

## Pinned first-caller references

The `mdec/4bit` and `mdec/8bit` guests and their `psx.log` output dumps are
pinned to ps1-tests commit `f727802fead11f1daa7549285548392ef87749cb`.
Their expected bytes come from those independent logs. The pinned callers pass
their stack buffers to `mdec_readDecodedDma`. In the pinned helper, that
function writes the `data` pointer directly to the MDECout DMA1 MADR and has no
CPU copy step. The log's address is from its hardware run, and the caller
buffer is a stack local; the emulator run can place that buffer at a different
address. The comparator parses and reports the pinned log target, captures and
reports the emulated DMA1 start separately, and compares the independent log
bytes at the emulated start. The pinned helper's direct-pointer path establishes
that this DMA1 start is the emulated caller buffer; no CPU copy is involved.
In the 4-bit guest, the bytes at that start match the log. The 8-bit arithmetic
mismatch remains unresolved, so its reference selector stays red. The guest
TTY emits literal format placeholders for byte values, so that text is not
used as the byte oracle. `[SCM.12]`

The pinned `mdec/step-by-step-log` guest is checked against the corresponding
status log's ordered status, input and output events. Its TTY also leaves
formatted values as placeholders, and the log has no cycle stamps. The model
tests verify status bits directly; neither this ordering check nor the
unstamped log measures the 1,000-cycle scheduling baseline. `[SCM.12]`
