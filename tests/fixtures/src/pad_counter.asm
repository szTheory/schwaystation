; SPDX-License-Identifier: MIT
; pad_counter.exe polls a digital controller through scheduled SIO0 bytes once per vblank.
; Cross rising edges add a pixel to a red counter; frame 300 records the count and a GPUREAD
; sample from VRAM. SIO byte order and pad replies follow docs/hardware/sio.md; display and
; rectangle/readback commands follow docs/hardware/gpu.md.
.include "header.inc"
.include "mailbox.inc"

.definelabel IO, 0xBF801000
.definelabel PIXEL_PACKET, 0x80011000
.definelabel IRQ_VBLANK, 0x01

.org MB_BASE
mailbox_header 11, 0

.org PC0
  mtc0 zero, sr
  nop
  nop
  mtc0 zero, cause
  li s0, IO
  li s1, 0                              ; completed vertical blanks
  li s2, 0                              ; Cross rising-edge count
  li s3, 0                              ; previous Cross state
  li t0, 0x000C
  sh t0, 0x048(s0)                     ; JOY_MODE: eight data bits, MUL1
  li t0, 0x0088
  sh t0, 0x04E(s0)                     ; JOY_BAUD
  li t0, 0x0003
  sh t0, 0x04A(s0)                     ; enable TX and select controller port 1

  sw zero, 0x070(s0)                   ; clear pending I_STAT bits
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

frame_wait:
  lw t0, 0x070(s0)
  nop
  andi t0, t0, IRQ_VBLANK
  beq t0, zero, frame_wait
  nop
  sw zero, 0x070(s0)                   ; acknowledge the completed vertical blank
  addiu s1, s1, 1
  li a0, 0x01
  li a1, 1
  jal sio_exchange
  nop
  li a0, 0x42
  li a1, 1
  jal sio_exchange
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  move a0, zero
  move a1, zero
  jal sio_exchange
  nop
  andi t0, v0, 0x40                    ; Cross is active-low bit 6 of the high button byte
  beq t0, zero, cross_pressed
  nop
  move s3, zero
  b counter_done
  nop
cross_pressed:
  bne s3, zero, counter_done
  nop
  li s3, 1
  addiu s2, s2, 1
  li t0, PIXEL_PACKET
  li t1, 0x600000FF                     ; GP0(60h): one flat red pixel
  sw t1, 0(t0)
  li t1, 0x00640000                     ; y=100, x=press count minus one
  addiu t2, s2, -1
  or t1, t1, t2
  sw t1, 4(t0)
  li t1, 0x00010001                     ; width=1, height=1
  sw t1, 8(t0)
  sw t0, 0x0A0(s0)                     ; D2_MADR
  li t1, 0x00010003                     ; one block of three GP0 words
  sw t1, 0x0A4(s0)                     ; D2_BCR
  li t1, 0x01000201                     ; RAM to GP0, sync mode 1, start
  sw t1, 0x0A8(s0)                     ; D2_CHCR
counter_done:
  li t0, 300
  beq s1, t0, mailbox_publish
  nop
  li t0, 298
  bne s1, t0, frame_wait
  nop
  ; Inputs are settled by frame 298. Compute the mailbox CRC now, keep status pending until
  ; frame 300, then publish it so the runner's stop-on-mailbox result includes frame 300.
  li t0, MB_BASE
  li t1, 0x00010001                     ; record 1: SIO-observed Cross edges
  sw t1, MB_RECORDS(t0)
  sw s2, MB_RECORDS + 4(t0)
  li t1, 0xC0000000                     ; GP0(C0h): read one VRAM pixel
  sw t1, 0x810(s0)
  li t1, 0x00640000                     ; x=0, y=100
  sw t1, 0x810(s0)
  li t1, 0x00010001                     ; width=1, height=1
  sw t1, 0x810(s0)
  lw t2, 0x810(s0)                     ; GPUREAD exposes the stored RGB555 word
  nop
  andi t2, t2, 0xFFFF
  li t1, 0x00010002                     ; record 2: pixel at the counter origin
  sw t1, MB_RECORDS + 8(t0)
  sw t2, MB_RECORDS + 12(t0)
  li t1, 2
  sb t1, MB_COUNT(t0)
  mailbox_seal
  li t0, MB_BASE
  li t1, MB_STATUS_RUNNING
  sb t1, MB_STATUS(t0)
  b frame_wait
  nop
mailbox_publish:
  li t0, MB_BASE
  sb zero, MB_STATUS(t0)
  b hang
  nop
; a0 = byte to transmit, a1 = nonzero when the controller acknowledges it; v0 = received byte.
sio_exchange:
  sb a0, 0x040(s0)
sio_rx_wait:
  lhu t0, 0x044(s0)
  nop
  andi t0, t0, 2
  beq t0, zero, sio_rx_wait
  nop
  lbu v0, 0x040(s0)
  beq a1, zero, sio_done
  nop
sio_ack_wait:
  lhu t0, 0x044(s0)
  nop
  andi t0, t0, 0x80
  beq t0, zero, sio_ack_wait
  nop
sio_release_wait:
  lhu t0, 0x044(s0)
  nop
  andi t0, t0, 0x80
  bne t0, zero, sio_release_wait
  nop
sio_done:
  jr ra
  nop

hang:
  b hang
  nop

.align 0x800
SW_END:
.close
