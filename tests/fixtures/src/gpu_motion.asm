; gpu_motion.exe: the no-draw tracer shell. The behavior test expects a moving pixel
; after the GP0/DMA path is added; this first RED version establishes display geometry.
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
mailbox_header 10, 0

.org PC0
  li s0, 0xBF801000
  li t0, 0x08000000                   ; GP1(08h): 256x240, dot divider 10
  sw t0, 0x814(s0)
  li t0, 0x03000000                   ; GP1(03h): display on
  sw t0, 0x814(s0)
idle:
  b idle
  nop

.align 0x800
SW_END:
.close
