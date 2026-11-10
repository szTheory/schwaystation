; hang.exe: a mailbox that stays at status 80h (running), then a jump to itself, so the program
; never reports done or fault and the runner's --stop-on mailbox reaches its frame cap.
; Fault prevented: a runner that waits on a program that never finishes, or reports it as done.
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
mailbox_header 2, 0

.org PC0
hang:
  j hang
  nop

.align 0x800
SW_END:
.close
