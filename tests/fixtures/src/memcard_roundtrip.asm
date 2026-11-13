; SPDX-License-Identifier: MIT
; Read and update independent card counters through SIO0, draw both values, then
; publish card bytes and VRAM readbacks at frame 120.
; Card commands follow docs/hardware/sio.md (Memory-card Read and GetID).
; Display and GPUREAD commands follow docs/hardware/gpu.md (Drawing and VRAM readback).
.include "header.inc"
.include "mailbox.inc"

.definelabel IO, 0xBF801000
.definelabel CARD_BUFFER, 0x80011000
.definelabel IRQ_VBLANK, 0x01

.org MB_BASE
mailbox_header 13, 0

.org PC0
  mtc0 zero, sr
  nop
  nop
  mtc0 zero, cause
  li s0, IO
  li t0, 0x000C
  sh t0, 0x048(s0)
  li t0, 0x0088
  sh t0, 0x04E(s0)

  move a0, zero
  jal card_read
  nop
  li t0, 0xA0
  beq v0, t0, card1_fresh
  nop
  li t0, 0xFF
  bne v0, t0, card1_present
  nop
card1_fresh:
  move s2, zero
  b card1_next
  nop
card1_present:
  move s2, v0
card1_next:
  addiu s2, s2, 1

  li a0, 0x2000
  jal card_read
  nop
  li t0, 0xA0
  beq v0, t0, card2_fresh
  nop
  li t0, 0xFF
  bne v0, t0, card2_present
  nop
card2_fresh:
  li s3, 2
  b card2_next
  nop
card2_present:
  addiu s3, v0, 1
card2_next:

  move a0, zero
  move a1, s2
  jal card_write
  nop
  li a0, 0x2000
  move a1, s3
  jal card_write
  nop

  li t0, 0x00000000                     ; GP1(00h): reset GPU command state
  sw t0, 0x814(s0)
  li t0, 0xE3000000                     ; GP0(E3h): draw-area top-left (0,0)
  sw t0, 0x810(s0)
  li t0, 0xE403BCFF                     ; GP0(E4h): draw-area bottom-right (255,239)
  sw t0, 0x810(s0)
  li t0, 0x08000000                     ; GP1(08h): 256x240, dot divider 10
  sw t0, 0x814(s0)
  li t0, 0x03000000                     ; GP1(03h): display on
  sw t0, 0x814(s0)

  li t0, 0x600000FF                     ; GP0(60h): flat red variable rectangle
  sw t0, 0x810(s0)
  li t0, 0x00640000                     ; card 1 counter row at y=100
  sw t0, 0x810(s0)
  li t0, 0x00010000
  or t0, t0, s2                         ; width is the card 1 counter
  sw t0, 0x810(s0)
  li t0, 0x600000FF
  sw t0, 0x810(s0)
  li t0, 0x00650000                     ; card 2 counter row at y=101
  sw t0, 0x810(s0)
  li t0, 0x00010000
  or t0, t0, s3                         ; width is the independent card 2 counter
  sw t0, 0x810(s0)

  li a0, 0x00640000
  jal gpu_read_pixel
  nop
  move s4, v0
  li t0, 0x00640000
  addiu t1, s2, -1
  or a0, t0, t1                         ; card 1's last pixel at y=100
  jal gpu_read_pixel
  nop
  move s5, v0
  li t0, 0x00650000
  addiu t1, s3, -1
  or a0, t0, t1                         ; card 2's last pixel at y=101
  jal gpu_read_pixel
  nop
  move s6, v0

  sw zero, 0x070(s0)                    ; clear pending interrupts
  li s1, 0
frame_wait:
  lw t0, 0x070(s0)
  nop
  andi t0, t0, IRQ_VBLANK
  beq t0, zero, frame_wait
  nop
  sw zero, 0x070(s0)                    ; acknowledge the completed vertical blank
  addiu s1, s1, 1
  li t0, 117
  bne s1, t0, frame_wait
  nop

  li t0, MB_BASE
  li t1, 0x00010001                     ; record 1: card 1's persisted counter
  sw t1, MB_RECORDS(t0)
  sw s2, MB_RECORDS + 4(t0)
  li t1, 0x00010002                     ; record 2: card 2's independent counter
  sw t1, MB_RECORDS + 8(t0)
  sw s3, MB_RECORDS + 12(t0)
  li t1, 0x00010003                     ; record 3: first card 1 pixel
  sw t1, MB_RECORDS + 16(t0)
  sw s4, MB_RECORDS + 20(t0)
  li t1, 0x00010004                     ; record 4: last card 1 pixel
  sw t1, MB_RECORDS + 24(t0)
  sw s5, MB_RECORDS + 28(t0)
  li t1, 0x00010005                     ; record 5: last card 2 pixel
  sw t1, MB_RECORDS + 32(t0)
  sw s6, MB_RECORDS + 36(t0)
  li t1, 5
  sb t1, MB_COUNT(t0)
  mailbox_seal
hang:
  b hang
  nop

; a0 = port-select bit; returns byte zero from sector 1.
card_read:
  addiu sp, sp, -16
  sw ra, 12(sp)
  move s7, a0
  li t0, 0x1003
  or t0, t0, s7
  sh t0, 0x04A(s0)
  li a0, 0x81
  li a1, 1
  jal sio_exchange
  nop
  li a0, 0x52
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
  li a1, 1
  jal sio_exchange
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li a0, 1
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
  li a1, 1
  jal sio_exchange
  nop
  li t8, CARD_BUFFER
  li t9, 128
  li t7, 1
card_read_bytes:
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  sb v0, 0(t8)
  xor t7, t7, v0
  addiu t8, t8, 1
  addiu t9, t9, -1
  bne t9, zero, card_read_bytes
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  bne v0, t7, fixture_fault
  nop
  move a0, zero
  move a1, zero
  jal sio_exchange
  nop
  li t0, 0x47
  bne v0, t0, fixture_fault
  nop
  sh zero, 0x04A(s0)
  li t0, CARD_BUFFER
  lbu v0, 0(t0)
  lw ra, 12(sp)
  addiu sp, sp, 16
  jr ra
  nop

; a0 = port-select bit; a1 = byte to write at byte zero of sector 1.
card_write:
  addiu sp, sp, -16
  sw ra, 12(sp)
  sw a1, 8(sp)
  move s7, a0
  li t8, CARD_BUFFER
  li t9, 128
card_clear_data:
  sb zero, 0(t8)
  addiu t8, t8, 1
  addiu t9, t9, -1
  bne t9, zero, card_clear_data
  nop
  li t0, CARD_BUFFER
  lw t1, 8(sp)
  nop
  sb t1, 0(t0)
  sb t1, 127(t0)                        ; frame XOR byte for the one nonzero counter
  li t7, 1                              ; count XOR count XOR sector 1

  li t0, 0x1003
  or t0, t0, s7
  sh t0, 0x04A(s0)
  li a0, 0x81
  li a1, 1
  jal sio_exchange
  nop
  li a0, 0x57
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
  li a1, 1
  jal sio_exchange
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li a0, 1
  li a1, 1
  jal sio_exchange
  nop
  li t8, CARD_BUFFER
  li t9, 128
card_write_bytes:
  lbu a0, 0(t8)
  li a1, 1
  jal sio_exchange
  nop
  addiu t8, t8, 1
  addiu t9, t9, -1
  bne t9, zero, card_write_bytes
  nop
  move a0, t7
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
  li t0, 0x47
  bne v0, t0, fixture_fault
  nop
  sh zero, 0x04A(s0)
  lw ra, 12(sp)
  addiu sp, sp, 16
  jr ra
  nop

; a0 = VRAM x/y, returns the low RGB555 pixel word from GPUREAD.
gpu_read_pixel:
  li t0, 0xC0000000
  sw t0, 0x810(s0)
  sw a0, 0x810(s0)
  li t0, 0x00010001
  sw t0, 0x810(s0)
  lw v0, 0x810(s0)
  nop
  andi v0, v0, 0xFFFF
  jr ra
  nop

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

fixture_fault:
  li t0, MB_BASE
  li t1, 0xFF
  sb t1, MB_STATUS(t0)
  b hang
  nop

.align 0x800
SW_END:
.close
