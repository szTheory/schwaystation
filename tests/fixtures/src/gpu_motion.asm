; SPDX-License-Identifier: MIT
; gpu_motion.exe: draw one new red pixel through DMA2 on each vertical blank.
; The frame view then exposes the real CPU/GPU/beam path and saved VRAM.
; Hardware references: docs/hardware/dma.md (sync mode 1 and DMA2 GP0),
; docs/hardware/gpu.md (flat variable rectangles), and docs/hardware/timers.md.
.include "header.inc"
.include "mailbox.inc"

.definelabel IO, 0xBF801000
.definelabel VECTOR, 0x80000080
.definelabel PIXEL_PACKET, 0x80011000
.definelabel IRQ_VBLANK, 0x01

.org MB_BASE
mailbox_header 10, 0

.org PC0
  mtc0 zero, sr                         ; kernel mode with interrupts disabled
  nop
  nop
  mtc0 zero, cause
  li t0, vector_jump
  li t1, VECTOR
  lw t2, 0(t0)
  lw t3, 4(t0)
  lw t4, 8(t0)
  lw t5, 12(t0)
  sw t2, 0(t1)
  sw t3, 4(t1)
  sw t4, 8(t1)
  sw t5, 12(t1)

  li s0, IO
  li s1, 0                              ; completed vertical blanks
  sw zero, 0x70(s0)                     ; acknowledge pending I_STAT bits
  li t0, IRQ_VBLANK
  sw t0, 0x74(s0)                       ; enable the vertical blank interrupt

  li t0, 0x04000002                     ; GP1(04h): DMA direction CPU to GP0
  sw t0, 0x814(s0)
  li t0, 0x0F654B21                     ; enable DMA2 (channel 2)
  sw t0, 0x0F0(s0)
  li t0, 0xE3000000                     ; GP0(E3h): draw-area top-left (0,0)
  sw t0, 0x810(s0)
  li t0, 0xE403BCFF                     ; GP0(E4h): draw-area bottom-right (255,239)
  sw t0, 0x810(s0)
  li t0, 0x08000000                     ; GP1(08h): 256x240, dot divider 10
  sw t0, 0x814(s0)
  li t0, 0x03000000                     ; GP1(03h): display on
  sw t0, 0x814(s0)
  li t0, 0x00000401                     ; SR: interrupt mask bit 10 and IEc
  mtc0 t0, sr
idle:
  b idle
  nop

handler:
  sw zero, 0x70(s0)                     ; acknowledge I_STAT bit 0
  addiu s1, s1, 1
  addiu t0, s1, -1                      ; frame 1 draws x=0, then advances once per frame
  li t1, PIXEL_PACKET
  li t2, 0x600000FF                     ; GP0(60h): flat red variable-size rectangle
  sw t2, 0(t1)
  li t2, 0x00640000                     ; y=100, x=completed blank minus one
  or t2, t2, t0
  sw t2, 4(t1)
  li t2, 0x00010001                     ; width=1, height=1
  sw t2, 8(t1)
  sw t1, 0x0A0(s0)                      ; D2_MADR
  li t2, 0x00010003                     ; one block of three GP0 words
  sw t2, 0x0A4(s0)                      ; D2_BCR
  li t2, 0x01000201                     ; RAM to GP0, sync mode 1, start
  sw t2, 0x0A8(s0)                      ; D2_CHCR
  mfc0 k0, epc
  nop
  jr k0
  rfe

vector_jump:
  lui k0, handler >> 16
  ori k0, k0, handler & 0xFFFF
  jr k0
  nop

.align 0x800
SW_END:
.close
