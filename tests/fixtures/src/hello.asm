; hello.exe: the first image that runs. It reports what the stub kernel handed it, in its mailbox,
; and then jumps to itself.
;   record 1 (id 1): gp, which the header sets to GP0, 80018000h
;   record 2 (id 2): sp, which the stub sets to s_addr + s_size, 80200000h for this header
; Fault prevented: a stub that ignores the header's entry point, global pointer or stack, or a
; core that cannot run an image to the end.
.definelabel GP0, 0x80018000
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
mailbox_header 1, 0

.org PC0
  li t0, MB_BASE
  li t1, 0x00010001                     ; id 1, flags: valid
  sw t1, MB_RECORDS(t0)
  sw gp, MB_RECORDS + 4(t0)
  li t1, 0x00010002                     ; id 2, flags: valid
  sw t1, MB_RECORDS + 8(t0)
  sw sp, MB_RECORDS + 12(t0)
  li t1, 2
  sb t1, MB_COUNT(t0)
  mailbox_seal
hang:
  j hang
  nop

.align 0x800
SW_END:
.close
