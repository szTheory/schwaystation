; hello.exe: the toolchain's first image. A mailbox, then a jump to itself.
; Fault prevented: an assembler or header that changes the bytes a fixture is built from.
.include "header.inc"

.org 0x80010000
.ascii "SWMB"                         ; mailbox magic
.word 1                               ; version
.word 0x80                            ; status
.word 0                               ; count
.word 1                               ; fixture id
.fill 0x80010100 - org()

.org PC0
hang:
  j hang
  nop

.align 0x800
SW_END:
.close
