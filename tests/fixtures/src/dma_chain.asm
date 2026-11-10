; dma_chain.exe: runs a DMA transfer on the real CPU and records what the registers and RAM show.
; It judges nothing: tests/diag/expected.csv does, one row per record in record order.
;
;   armips -strequ IMAGE <file> dma_chain.asm
;
; Fault prevented: a DMA engine whose transfers are right when the host calls it directly but wrong behind
; the CPU's loads and stores to the DMA registers, or a recorded value that no row supports.
.include "header.inc"
.include "mailbox.inc"

.definelabel IO, 0xBF801000             ; the I/O ports, uncached
.definelabel OT, 0x80100000             ; the ordering table the transfer builds: 4096 words
.definelabel OT_LAST_WORD, 0x00103FFC   ; where the transfer starts, the physical address of the table's last word

.org MB_BASE
mailbox_header 8, 0

.org PC0
  mtc0 zero, sr                         ; kernel mode, interrupts off, no coprocessor
  nop
  nop
  li s5, IO
  li s2, MB_BASE + MB_RECORDS           ; the next record's address
  li s3, 0x00010001                     ; its id word: id 1, flags valid

  sw zero, 0x70(s5)                     ; I_STAT: acknowledge everything
  li t0, 0x0F654321                     ; DPCR: channel 6 enabled
  sw t0, 0xF0(s5)
  li t0, 0x00C00000                     ; DICR: master enable and channel 6 enable
  sw t0, 0xF4(s5)
  li t0, OT_LAST_WORD
  sw t0, 0xE0(s5)                       ; D6_MADR
  li t0, 4096
  sw t0, 0xE4(s5)                       ; D6_BCR
  li t0, 0x11000002                     ; D6_CHCR: start, manual trigger
  sw t0, 0xE8(s5)
  lw t1, 0xE8(s5)                       ; a register read waits for the transfer
  nop
  li a0, OT
  lw t3, 0(a0)                          ; the table's first address holds the end marker
  nop
  sw s3, 0(s2)                          ; record 1: ot_last
  sw t3, 4(s2)
  addiu s2, s2, 8

  li t0, MB_BASE
  li t1, 1
  sb t1, MB_COUNT(t0)
  mailbox_seal
hang:
  j hang
  nop

.align 0x800
SW_END:
.close
