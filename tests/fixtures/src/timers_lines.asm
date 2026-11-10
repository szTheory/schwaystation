; timers_lines.exe: counts the vertical blanks the frame driver raises, taking each one as an interrupt through
; its own exception handler at 80000080h, and records the count in the mailbox. It judges nothing:
; tests/diag/expected.csv does, one row per record in record order, for the machine profile the test names.
;
;   armips -strequ IMAGE <file> timers_lines.asm
;
; The handler runs with interrupts off, counts one blank per entry that finds I_STAT bit 0 set, acknowledges
; it and returns with `jr` and `rfe`. The idle loop is a branch to itself. The run ends in the handler that
; takes the 23rd blank, which writes the records and seals the mailbox.
; Records, in order:
;    1  vblank_count   the blanks counted when the run ends
; Fault prevented: a vertical blank that the CPU never takes as an interrupt, takes twice or loses behind the
; handler's return, or a recorded value that no row supports.
.include "header.inc"
.include "mailbox.inc"

.definelabel IO, 0xBF801000             ; the I/O ports, uncached
.definelabel VECTOR, 0x80000080         ; the general exception vector, kernel RAM
.definelabel BLANKS, 23                 ; the blank on which the run ends

.org MB_BASE
mailbox_header 9, 0

.org PC0
  mtc0 zero, sr                         ; kernel mode, interrupts off, nothing unmasked, BEV clear
  nop
  nop
  mtc0 zero, cause
  ; This fixture's handler: copy the four-word jump below over the stub's at 80000080h.
  li t0, vector_jump
  li t1, VECTOR
  lw t2, 0(t0)
  lw t3, 4(t0)
  lw t4, 8(t0)
  lw t5, 12(t0)
  sw t2, 0(t1)
  sw t3, 4(t1)
  sw t4, 8(t1)
  sw t5, 12(t1)
  li s0, IO
  li s1, 0                              ; the blanks counted
  sw zero, 0x70(s0)                     ; I_STAT: acknowledge everything
  li t0, 1
  sw t0, 0x74(s0)                       ; I_MASK: the vertical blank only
  li t0, 0x0401                         ; SR: interrupt mask bit 10 and IEc
  mtc0 t0, sr
idle:
  b idle
  nop

handler:
  lw t0, 0x70(s0)                       ; I_STAT
  nop
  andi t1, t0, 1
  beq t1, zero, handler_return
  nop
  sw zero, 0x70(s0)                     ; acknowledge
  addiu s1, s1, 1
  li t2, BLANKS
  beq s1, t2, finish
  nop
handler_return:
  mfc0 k0, epc
  nop
  jr k0
  rfe

finish:
  li t0, MB_BASE
  li t1, 0x00010001                     ; id 1, flags: valid
  sw t1, MB_RECORDS(t0)
  sw s1, MB_RECORDS + 4(t0)
  li t1, 1
  sb t1, MB_COUNT(t0)
  mailbox_seal
hang:
  j hang
  nop

; What the code above copies to the vector: a jump to the handler.
vector_jump:
  lui k0, handler >> 16
  ori k0, k0, handler & 0xFFFF
  jr k0
  nop

.align 0x800
SW_END:
.close
