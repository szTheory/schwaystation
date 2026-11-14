; SPDX-License-Identifier: MIT
; spu_tone.exe: the stub-booted SPU integration fixture.
; Hardware register and BRR behavior follows docs/hardware/spu.md and
; .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md §§1.1–1.4.
.include "header.inc"
.include "mailbox.inc"
.include "spu_blocks.inc"

.org MB_BASE
mailbox_header 12, 0

.org PC0
  mtc0 zero, sr
  nop
  nop
  mtc0 zero, cause
wait:
  b wait
  nop

.align 0x800
SW_END:
.close
