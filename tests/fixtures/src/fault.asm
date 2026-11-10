; fault.exe: a long run of loads and stores, then one first fault of the class KIND names.
;
;   armips -strequ IMAGE <file> -equ KIND <1 to 9> fault.asm
;
;   KIND 1  a read from 1F801030h, a gap in the I/O page         (unmapped_read)
;   KIND 2  a write to 1F801030h                                 (unmapped_write)
;   KIND 3  a jump to 1F801030h                                  (unmapped_fetch)
;   KIND 4  LW from a word address plus 1                        (misaligned_read)
;   KIND 5  SW to a word address plus 1                          (misaligned_write)
;   KIND 6  a jump to a word address plus 3                      (misaligned_fetch)
;   KIND 7  a word whose opcode is not a MIPS I instruction      (reserved_instruction)
;   KIND 8  a COP1 instruction while SR.CU1 is clear             (cop_unusable)
;   KIND 9  a BREAK whose registered handler returns to it       (exception_loop)
;
; The preamble loads and stores 1,000 times, which is 8,000 trace records, more than the 4,096 the
; trace ring holds, so the tail a runner prints is a full ring ending in the fault.
; Fault prevented: a first fault that is missed, mislabelled or reported with a short tail.
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
.if KIND == 9
mailbox_header 5, handler
.else
mailbox_header 5, 0
.endif

.org PC0
  li t0, 0x80020000
  li t2, 1000
preamble:
  lw t1, 0(t0)
  nop                                   ; the load delay slot
  sw t1, 4(t0)
  addiu t2, t2, -1
  bne t2, zero, preamble
  nop

.if KIND == 1
  li t0, 0xBF801030
  lw t1, 0(t0)
  nop
.endif
.if KIND == 2
  li t0, 0xBF801030
  sw zero, 0(t0)
.endif
.if KIND == 3
  li t0, 0xBF801030
  jr t0
  nop
.endif
.if KIND == 4
  li t0, 0x80020001
  lw t1, 0(t0)
  nop
.endif
.if KIND == 5
  li t0, 0x80020001
  sw zero, 0(t0)
.endif
.if KIND == 6
  li t0, 0x80020003
  jr t0
  nop
.endif
.if KIND == 7
  .word 0xFC000000                      ; opcode 3Fh
.endif
.if KIND == 8
  .word 0x44010000                      ; MFC1 at, f0
.endif
.if KIND == 9
  break
.endif
hang:
  j hang
  nop

.if KIND == 9
; Registered with the stub kernel through the mailbox: return to the instruction that raised the
; exception, so the same EPC and ExcCode come round again.
handler:
  mfc0 k0, epc
  nop
  jr k0
  rfe
.endif

.align 0x800
SW_END:
.close
