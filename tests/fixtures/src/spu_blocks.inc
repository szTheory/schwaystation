; SPDX-License-Identifier: MIT
; Authored PS1 SPU BRR blocks. The table is deliberately deterministic and
; contains no captured audio or executable bytes.
.align 4
spu_brr_blocks:
  .byte 0x0C, 0x03
  .fill 14, 0x17
  .byte 0x0C, 0x03
  .fill 14, 0x71
spu_brr_blocks_end:
