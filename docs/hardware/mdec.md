# MDEC

The MDEC at `1F801820h` and `1F801824h` decodes coefficient streams and exposes
decoded blocks to DMA1. This page describes the monochrome 4-bit and 8-bit path
implemented in `src/mdec.c`; color conversion and 15-bit/24-bit output remain
outside this path. Hardware facts and open questions are collected in
[the preparation note, section 4](../../.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md#4-mdec),
with the underlying psx-spx and ps1-tests sources listed there.

## Commands and input

Command 1 decodes blocks. Its fields select output depth, signed output, bit 15,
and the number of following input words. Command 2 loads luma and optional
chroma quantization tables; command 3 loads 64 signed scale-table halfwords.
The implemented mono path uses command 1, the luma table, and the scale table.
DMA0 writes input words; CPU writes to the data port use the same FIFO path.

A block starts with a signed 10-bit DC value and a six-bit quantization scale.
The following run-length words carry a six-bit zero run and signed 10-bit AC
value in zigzag order. The decoder applies the quantization table and clips
coefficients to 11 bits. Blocks normally end with `FE00h`; `FE00h` before a DC
value is padding. One bounded first-caller exception is recorded from the
pinned ps1-tests `mdec/4bit` guest: its 32-word decode command ends after
coefficient 63 with no EOB, and the completed 64-coefficient block is published
at that command boundary ([SCM.12], guest pin and member digest in `pins.txt`).
This is an observed caller case, not general acceptance of missing EOBs.
Parsing remains bounded by the 64-coefficient block and command input count;
malformed runs or an exhausted command with an incomplete block abort.
When EOB is in a word's low half, the high half remains queued for the next
block, including across save and load.

## Decode and output

The inverse transform uses two fixed point matrix passes with the signed
Q14 scale table and 64-bit intermediate sums. Each pass rounds with `+8000h`
before division by `10000h`; intermediate samples are clipped to the supported
range. This is the deterministic implementation rule. The hardware source
describes the two-pass transform but does not settle its exact rounding.

Mono samples are clipped to 8 bits. Unsigned output flips bit 7 of each sample;
signed output keeps the signed byte. Four-bit output stores each sample's high
nibble, least-significant pixel first, with eight pixels in each 32-bit FIFO
word. Eight-bit output stores four pixels per word in little-endian order.
DMA1 drains the FIFO into consecutive RAM words and advances MADR by four bytes
per word. The registered `mdec.words` control hashes raw FIFO words before DMA1
and separately checks the RAM words and final MADR afterward.

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

The output FIFO holds 192 words. A completed block waits with no scheduled
retry if the whole block does not fit; DMA1 reads resume it as soon as enough
space opens. This prevents dropping a block and avoids a same-tick scheduler
loop. State format 12 stores the input halfword cursor and this pending-block
state so save/load preserves stream and backpressure progress.

## Pinned first-caller references

The `mdec/4bit` and `mdec/8bit` guests and their `psx.log` output dumps are
pinned to ps1-tests commit `f727802fead11f1daa7549285548392ef87749cb`.
Their expected bytes come from those independent logs; the reference test runs
each actual guest and compares the DMA1 destination RAM against every logged
byte. The guest TTY emits literal format placeholders for these byte values,
so that text is not used as the byte oracle. `[SCM.12]`

The pinned `mdec/step-by-step-log` guest is checked against the corresponding
status log's ordered status, input and output events. Its TTY also leaves
formatted values as placeholders, and the log has no cycle stamps. The model
tests verify status bits directly; neither this ordering check nor the
unstamped log measures the 1,000-cycle scheduling baseline. `[SCM.12]`
