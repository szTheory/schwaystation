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
  li s1, SPU
  sw zero, 0x070(s0)                   ; clear pending interrupt bits
  sw zero, 0x074(s0)                   ; leave all interrupts masked
  li t0, 0x0F6D4321
  sw t0, 0x0F0(s0)                     ; initialize DMA priorities, including channel 4
  li t0, 0x7F000000
  sw t0, 0x0F4(s0)                     ; acknowledge all DMA flags with channel IRQs disabled

  sh zero, 0x1AA(s1)                   ; transfer stop
  li t0, 4
  sh t0, 0x1AC(s1)                     ; one bank, capture rings enabled
  li t0, 0x0200
  sh t0, 0x1A6(s1)                     ; upload at SPU RAM 1000h
  li t0, 0xC000
  sh t0, 0x1AA(s1)                     ; SPU enabled and unmuted
  li s2, spu_brr_blocks
  li t2, 16                            ; two 16-byte authored blocks, through the IO FIFO
upload_io:
  lhu t3, 0(s2)
  nop
  sh t3, 0x1A8(s1)
  addiu s2, s2, 2
  addiu t2, t2, -1
  bne t2, zero, upload_io
  nop
  li t0, 0xC010
  sh t0, 0x1AA(s1)                     ; ManualWrite
  li t2, 256                            ; register writes take effect on the next SPU sample tick
wait_io_tick:
  addiu t2, t2, -1
  bne t2, zero, wait_io_tick
  nop
wait_io:
  lhu t0, 0x1AE(s1)                    ; wait until the FIFO has reached SPU RAM
  nop
  andi t0, t0, 0x0400
  bne t0, zero, wait_io
  nop
  li t0, 0xC000
  sh t0, 0x1AA(s1)                     ; stop transfers before voice setup

  li t0, 0x04000002
  sw t0, 0x814(s0)                     ; GP1(04h): CPU/DMA writes target GP0
  li t0, 0xE3000000
  sw t0, 0x810(s0)                     ; GP0(E3h): draw-area top-left
  li t0, 0xE403BCFF
  sw t0, 0x810(s0)                     ; GP0(E4h): draw-area bottom-right
  li t0, 0x08000000
  sw t0, 0x814(s0)                     ; GP1(08h): 256x240 display mode
  li t0, 0x03000000
  sw t0, 0x814(s0)                     ; GP1(03h): display on
  jal draw_colour_bars
  nop

  sh zero, 0x184(s1)                   ; reverb output volume left
  sh zero, 0x186(s1)                   ; reverb output volume right
  sh zero, 0x198(s1)                   ; disable reverb send for voices 1-16
  sh zero, 0x19A(s1)                   ; disable reverb send for voices 17-24

  li t0, 0x3FFF
  sh t0, 0x000(s1)                     ; voice 1 left volume
  li t0, 0x1FFF
  sh t0, 0x002(s1)                     ; voice 1 right volume, deliberately lower
  li t0, 0x1000
  sh t0, 0x004(s1)                     ; one decoded sample per SPU tick
  li t0, 0x0200
  sh t0, 0x006(s1)                     ; voice 1 start at RAM 1000h
  li t0, 0x000F
  sh t0, 0x008(s1)                     ; fast attack and full sustain level
  sh zero, 0x00A(s1)                   ; linear sustain, no release until key-off

  li t0, 0x0400
  sh t0, 0x1A6(s1)                     ; voice 2 table copy at SPU RAM 2000h
  li t0, 0xC020
  sh t0, 0x1AA(s1)                     ; SPU enabled, unmuted, DMAWrite
  li t2, 256                            ; apply TSA and ATTR on the next sample tick
wait_dma_setup_tick:
  addiu t2, t2, -1
  bne t2, zero, wait_dma_setup_tick
  nop
  li s2, spu_brr_blocks
  sw s2, 0x0C0(s0)                      ; D4_MADR: the same two-block table
  li t0, 0x00010008
  sw t0, 0x0C4(s0)                     ; one block of eight words (32 bytes)
  li t0, 0x01000201
  sw t0, 0x0C8(s0)                     ; D4_CHCR: RAM to SPU, sync mode 1
  li t1, 0x01000000
wait_dma:
  lw t0, 0x0C8(s0)
  nop
  and t0, t0, t1
  bne t0, zero, wait_dma
  nop

  li t0, 0xC000
  sh t0, 0x1AA(s1)                     ; stop transfer mode; reverb remains disabled
  li t0, 0x3FFF
  sh t0, 0x010(s1)                     ; voice 2 left volume
  li t0, 0x1FFF
  sh t0, 0x012(s1)                     ; voice 2 right volume
  li t0, 0x1800
  sh t0, 0x014(s1)                     ; voice 2 pitch: 1.5 samples per tick
  li t0, 0x0402
  sh t0, 0x016(s1)                     ; voice 2 starts at table block two
  li t0, 0x000F
  sh t0, 0x018(s1)                     ; fast attack and full sustain level
  sh zero, 0x01A(s1)
  li t0, 0x3FFF
  sh t0, 0x180(s1)                     ; main left volume
  sh t0, 0x182(s1)                     ; main right volume
  li t0, 3
  sh t0, 0x188(s1)                     ; key on voices 1 and 2 together

  sw zero, 0x070(s0)                   ; begin vblank count from a clean status
  li s3, 0
frame_wait:
  lw t0, 0x070(s0)
  nop
  andi t0, t0, 1
  beq t0, zero, frame_wait
  nop
  sw zero, 0x070(s0)                   ; acknowledge VBlank without a BIOS call
  addiu s3, s3, 1
  li t0, 600
  beq s3, t0, measure_envx
  nop
  li t0, 620
  beq s3, t0, hang
  nop
  b frame_wait
  nop

measure_envx:
  li t0, MB_BASE
  li t1, 0x00010001
  sw t1, MB_RECORDS(t0)
  lhu t2, 0x00C(s1)                    ; voice 1 ENVX at guest frame 600
  nop
  sw t2, MB_RECORDS + 4(t0)
  li t1, 0x00010002
  sw t1, MB_RECORDS + 8(t0)
  lhu t2, 0x01C(s1)                    ; voice 2 ENVX at guest frame 600
  nop
  sw t2, MB_RECORDS + 12(t0)
  li t1, 0x00010003
  sw t1, MB_RECORDS + 16(t0)
  sw s3, MB_RECORDS + 20(t0)           ; record the guest's vblank count
  li t1, 3
  sb t1, MB_COUNT(t0)
  mailbox_seal
after_measure:
  li t0, 620
  beq s3, t0, hang
  nop
  b frame_wait
  nop

draw_colour_bars:
  li s2, 0
draw_tile:
  andi t3, s2, 1
  bne t3, zero, draw_green
  nop
  li t0, 0x600000FF                     ; alternating red and green vertical bars
  b draw_colour
  nop
draw_green:
  li t0, 0x6000FF00
draw_colour:
  sw t0, 0x810(s0)
  srl t1, s2, 3
  sll t1, t1, 3
  addiu t1, t1, 64
  sll t1, t1, 16
  andi t2, s2, 7
  sll t2, t2, 3
  addiu t2, t2, 64
  or t1, t1, t2
  sw t1, 0x810(s0)
  li t1, 0x00080008
  sw t1, 0x810(s0)
  addiu s2, s2, 1
  slti t0, s2, 64
  bne t0, zero, draw_tile
  nop
  jr ra
  nop

hang:
  b hang
  nop

.include "spu_blocks.inc"

.align 0x800
SW_END:
.close
