; mailbox_fail.exe: a sealed mailbox that the runner must call a failure, two ways.
;
;   armips -strequ IMAGE <file> -equ MODE <0 or 1> mailbox_fail.asm
;
;   MODE 0  status FFh (fault) with a valid CRC: the fixture reports failure
;   MODE 1  status 00h (done) with a CRC that does not match bytes 10h..FFh
; One record, id 1 with value 12345678h, so the lines the runner prints for a fault can be checked.
; Fault prevented: a runner that reports a fault status as done, or trusts a mailbox whose CRC is wrong.
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
mailbox_header 3, 0

.org PC0
  li t0, MB_BASE
  li t1, 0x00010001                     ; id 1, flags: valid
  sw t1, MB_RECORDS(t0)
  li t1, 0x12345678
  sw t1, MB_RECORDS + 4(t0)
  li t1, 1
  sb t1, MB_COUNT(t0)
  mailbox_seal
  li t0, MB_BASE
.if MODE == 0
  li t1, 0xFF
  sb t1, MB_STATUS(t0)
.else
  lw t1, MB_CRC(t0)
  nop                                   ; the load delay slot
  xori t1, t1, 1
  sw t1, MB_CRC(t0)
.endif
hang:
  j hang
  nop

.align 0x800
SW_END:
.close
