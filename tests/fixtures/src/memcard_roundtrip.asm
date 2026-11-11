; SPDX-License-Identifier: MIT
; Guest card persistence fixture. The mailbox, pixels, and real SIO0 card transactions
; are filled in by the persistence proof implementation.
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
mailbox_header 12, 0

.org PC0
  mtc0 zero, sr
  nop
  nop
hang:
  b hang
  nop

.align 0x800
SW_END:
.close
