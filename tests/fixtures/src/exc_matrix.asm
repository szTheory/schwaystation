; exc_matrix.exe: puts its own exception handler at 80000080h (the stub kernel's default handler is not
; used), takes each exception class the CPU has, and records what the handler saw in the mailbox. It
; judges nothing: tests/diag/expected.csv does, one row per record in record order.
;
; Each case arms three registers, then runs one trigger instruction:
;   s0  the trigger's address (the branch's address when the trigger sits in its delay slot)
;   s1  where the handler resumes, the instruction after the trigger or after the branch pair
;   s2  1 when the exception is an address error, so the handler also records BadVaddr
; The handler records CAUSE, then EPC minus s0, then BadVaddr for an address error, and returns to s1
; with `jr` and `rfe`. For an interrupt it also acknowledges I_STAT, so the line goes false.
; s5 counts handler entries, s6 is the next record's id word (id, flags valid) and s7 its address.
;
; The three tail cases use the vertical blank the frame driver raises at the end of a frame, with
; I_MASK bit 0 set: the line seen while SR masks it, the CAUSE value read in the instruction right after
; an I_STAT write, and the interrupt once SR unmasks it.
; Fault prevented: an exception whose CAUSE, EPC or BadVaddr is wrong for its class, with or without a
; branch delay slot, goes unseen because no fixture records it.
.include "header.inc"
.include "mailbox.inc"

.definelabel I_STAT, 0xBF801070
.definelabel I_MASK, 0xBF801074
.definelabel VECTOR, 0x80000080

.org MB_BASE
mailbox_header 3, 0

; rec reg: a record, its id word then reg.
.macro rec,reg
  sw s6, 0(s7)
  sw reg, 4(s7)
  addiu s6, s6, 1
  addiu s7, s7, 8
.endmacro

; arm trigger, resume, badvaddr_flag
.macro arm,trig,res,badv
  li s0, trig
  li s1, res
  li s2, badv
.endmacro

.org PC0
  mtc0 zero, sr                         ; kernel mode, interrupts off, nothing unmasked, BEV clear
  mtc0 zero, cause
  li s5, 0
  li s6, 0x00010001                     ; id 1, flags: valid
  li s7, MB_BASE + MB_RECORDS
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

  ; SYSCALL, not in a delay slot.
  arm t_syscall, r_syscall, 0
t_syscall:
  syscall
r_syscall:

  ; SYSCALL in the delay slot of a taken branch.
  arm t_syscall_bd, r_syscall_bd, 0
t_syscall_bd:
  b r_syscall_bd
  .word 0x0000000C                      ; SYSCALL, which armips will not place in a delay slot
r_syscall_bd:

  ; BREAK, then BREAK in a delay slot.
  arm t_break, r_break, 0
t_break:
  break
r_break:
  arm t_break_bd, r_break_bd, 0
t_break_bd:
  b r_break_bd
  .word 0x0000000D                      ; BREAK, likewise
r_break_bd:

  ; AdEL on a misaligned LH and LW, AdES on a misaligned SW. 80000001h, 80000002h and 80000003h are RAM,
  ; so only the alignment is wrong; the exception comes before any access.
  arm t_lh, r_lh, 1
  li t2, 0x80000001
t_lh:
  lh t0, 0(t2)
r_lh:
  arm t_lw, r_lw, 1
  li t2, 0x80000002
t_lw:
  lw t0, 0(t2)
r_lw:
  arm t_sw, r_sw, 1
  li t2, 0x80000003
t_sw:
  sw zero, 0(t2)
r_sw:

  ; Overflow: 7FFFFFFFh plus 1 in ADD.
  arm t_add, r_add, 0
  li t0, 0x7FFFFFFF
  li t1, 1
t_add:
  add t2, t0, t1
r_add:

  ; A reserved instruction: opcode 3Fh.
  arm t_reserved, r_reserved, 0
t_reserved:
  .word 0xFFFFFFFF
r_reserved:

  ; Coprocessor 1 is unusable: MFC1 with SR.CU1 clear.
  arm t_cop1, r_cop1, 0
t_cop1:
  .word 0x44080000
r_cop1:

  ; A data bus error: a word read from the first expansion region, 1F000000h.
  arm t_dbe, r_dbe, 0
  li t2, 0xBF000000
t_dbe:
  lw t0, 0(t2)
r_dbe:

  ; The vertical blank with I_MASK bit 0 set and SR masking it: CAUSE shows the line, no handler runs.
  li t1, I_STAT
  li t3, I_MASK
  li t0, 1
  sw t0, 0(t3)
wait_vblank_1:
  lw t0, 0(t1)
  nop
  andi t0, t0, 1
  beq t0, zero, wait_vblank_1
  nop
  mfc0 t2, cause
  nop
  rec t2
  rec s5

  ; CAUSE in the instruction right after an I_STAT write. The KSEG1 read first empties the write queue.
  lw t0, 0(t1)
  nop
  sw zero, 0(t1)
  mfc0 t2, cause
  nop
  rec t2

  ; The next blank, then SR unmasks it: the handler runs two instructions after the MTC0.
wait_vblank_2:
  lw t0, 0(t1)
  nop
  andi t0, t0, 1
  beq t0, zero, wait_vblank_2
  nop
  arm t_unmask, r_unmask, 0
  li t0, 0x0401                         ; IM bit 10 and IEc
t_unmask:
  mtc0 t0, sr
  nop
r_unmask:
  nop
  mtc0 zero, sr

  ; The records are done: count them and seal the mailbox.
  li t0, MB_BASE
  andi t1, s6, 0xFFFF
  addiu t1, t1, -1
  sb t1, MB_COUNT(t0)
  mailbox_seal
hang:
  j hang
  nop

handler:
  mfc0 k0, cause
  mfc0 k1, epc
  addiu s5, s5, 1
  rec k0
  subu k1, k1, s0
  rec k1
  beq s2, zero, no_badvaddr
  nop
  mfc0 k0, badvaddr
  nop
  rec k0
no_badvaddr:
  mfc0 k0, cause                        ; an interrupt (ExcCode 0) is acknowledged at the source
  nop
  andi k0, k0, 0x7C
  bne k0, zero, handler_return
  nop
  li k1, I_STAT
  sw zero, 0(k1)
handler_return:
  jr s1
  rfe

; What the code above copies to the vector: a jump to the handler.
vector_jump:
  lui k0, handler >> 16
  ori k0, k0, handler & 0xFFFF
  jr k0
  nop

.align 0x800
SW_END:
.close
