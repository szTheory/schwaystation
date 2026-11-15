; unbuilt.exe: register accesses that the core does not build yet, then either a clean finish or a gap.
;
;   armips -strequ IMAGE <file> -equ MODE <0 or 1> unbuilt.asm
;
;   MODE 0  writes MDEC input (not modeled), then seals the mailbox as done
;   MODE 1  writes 1F801030h, which no device answers: a first fault
; Fault prevented: accesses to unbuilt devices that leave no sign, and a write to a gap that is dropped
; without a fault.
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
mailbox_header 6, 0

.org PC0
.if MODE == 0
  li t0, 0xBF801820                     ; MDEC data
  li t1, 0x12345678
  sw t1, 0(t0)
  mailbox_seal
.else
  li t0, 0xBF801030
  sw zero, 0(t0)
.endif
hang:
  j hang
  nop

.align 0x800
SW_END:
.close
