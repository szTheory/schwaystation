; SPDX-License-Identifier: MIT
; spu_tone.exe: the stub-booted SPU integration fixture.
; Hardware register and BRR behavior follows docs/hardware/spu.md and
; .planning/preparation/PSX-SPU-CDROM-AND-MDEC.md §§1.1–1.4.
.include "header.inc"
.include "mailbox.inc"

.definelabel IO, 0xBF801000
.definelabel SPU, 0xBF801C00

.org MB_BASE
mailbox_header 12, 0

.org PC0
  mtc0 zero, sr
  nop
  nop
  mtc0 zero, cause
  li s0, IO
  sw zero, 0x070(s0)                   ; clear pending interrupt bits
  sw zero, 0x074(s0)                   ; leave all interrupts masked
  li t0, 0x0F6D4321
  sw t0, 0x0F0(s0)                     ; initialize DMA priorities, including channel 4
  li t0, 0x7FC00000
  sw t0, 0x0F4(s0)                     ; acknowledge DMA flags and keep DMA IRQs disabled

  li s0, SPU
  sh zero, 0x1AA(s0)                   ; transfer stop
  li t0, 4
  sh t0, 0x1AC(s0)                     ; one bank, capture rings enabled
  li t0, 0x0200
  sh t0, 0x1A6(s0)                     ; upload at SPU RAM 1000h
  li t0, 0xC000
  sh t0, 0x1AA(s0)                     ; SPU enabled and unmuted
  li s1, spu_brr_blocks
  li t2, 16                            ; two 16-byte authored blocks, through the IO FIFO
upload_io:
  lhu t3, 0(s1)
  nop
  sh t3, 0x1A8(s0)
  addiu s1, s1, 2
  addiu t2, t2, -1
  bne t2, zero, upload_io
  nop
  li t0, 0xC010
  sh t0, 0x1AA(s0)                     ; ManualWrite
  li t2, 256                            ; register writes take effect on the next SPU sample tick
wait_io_tick:
  addiu t2, t2, -1
  bne t2, zero, wait_io_tick
  nop
wait_io:
  lhu t0, 0x1AE(s0)                    ; wait until the FIFO has reached SPU RAM
  nop
  andi t0, t0, 0x0400
  bne t0, zero, wait_io
  nop
  li t0, 0xC000
  sh t0, 0x1AA(s0)                     ; stop transfers before voice setup

  li t0, 0x3FFF
  sh t0, 0x000(s0)                     ; voice 1 left volume
  li t0, 0x1FFF
  sh t0, 0x002(s0)                     ; voice 1 right volume, deliberately lower
  li t0, 0x1000
  sh t0, 0x004(s0)                     ; one decoded sample per SPU tick
  li t0, 0x0200
  sh t0, 0x006(s0)                     ; voice 1 start at RAM 1000h
  li t0, 0x000F
  sh t0, 0x008(s0)                     ; fast attack and full sustain level
  sh zero, 0x00A(s0)                   ; linear sustain, no release until key-off
  li t0, 0x3FFF
  sh t0, 0x180(s0)                     ; main left volume
  sh t0, 0x182(s0)                     ; main right volume
  li t0, 1
  sh t0, 0x188(s0)                     ; key on voice 1
wait:
  b wait
  nop

.include "spu_blocks.inc"

.align 0x800
SW_END:
.close
