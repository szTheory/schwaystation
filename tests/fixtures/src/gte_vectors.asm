; gte_vectors.exe: replays hardware-captured geometry engine cases through the CPU, on real COP2 instructions,
; and records what the engine returns. It judges nothing: tests/diag/expected.csv does, one row per record in
; record order, each row the CRC-32 of the case's 64 output words as the hardware table holds them.
;
;   armips -strequ IMAGE <file> [-definelabel LOOP 1] gte_vectors.asm
;
; Each case in gte_cases.inc (record: case number, command word, 64 input words) runs in the order the table
; was captured in, and in that order only:
;   1. SR.CU2 is set once with MTC0.
;   2. Registers 0 to 63 are written with zero, then with the inputs: 0 to 31 through MTC2, 32 to 63 through CTC2.
;   3. Unless the command word is FFFFFFFFh (registers only), the command is issued with COP2 from a slot in
;      uncached RAM (the word is stored there, so no cache line holds the old command), and an SWC2 of data
;      register 0 follows at once: the store has to wait for the engine, which is the busy stall.
;   4. Registers 0 to 63 are read back, 0 to 31 with MFC2 and 32 to 63 with CFC2, one load delay slot apart.
;   5. The CRC-32 of those 256 bytes (little-endian words) becomes the next mailbox record.
; With LOOP defined the cases run again and again and the mailbox is never sealed: only for timing the
; fixture by hand, never registered as a test.
; Fault prevented: a GTE whose results are right when called directly but wrong behind the CPU's COP2
; moves, load delays and stall, or a recorded value that no hardware case supports.
.include "header.inc"
.include "mailbox.inc"
.ifndef LOOP
.definelabel LOOP, 0
.endif

.definelabel CASE_BYTES, 66 * 4
.definelabel KSEG1, 0x20000000          ; added to a KSEG0 address it names the same RAM, uncached
.definelabel GTE_RUN_CASES, 1

; gte_w_d n / gte_w_c n: write word n of the buffer at a0 to data register n (MTC2) or control register n
; (CTC2). The register number is in the instruction, so each of the 64 is its own copy.
.macro gte_w_d,n
  lw t0, (n)*4(a0)
  nop                                   ; the load delay slot
  .word 0x48800000 | (8 << 16) | ((n) << 11)      ; mtc2 t0, $n
.endmacro
.macro gte_w_c,n
  lw t0, (n + 32)*4(a0)
  nop
  .word 0x48C00000 | (8 << 16) | ((n) << 11)      ; ctc2 t0, $n
.endmacro
; gte_r_d n / gte_r_c n: read data register n (MFC2) or control register n (CFC2) into word n of the buffer at a1.
.macro gte_r_d,n
  .word 0x48000000 | (8 << 16) | ((n) << 11)      ; mfc2 t0, $n
  nop                                   ; the load delay slot
  sw t0, (n)*4(a1)
.endmacro
.macro gte_r_c,n
  .word 0x48400000 | (8 << 16) | ((n) << 11)      ; cfc2 t0, $n
  nop
  sw t0, (n + 32)*4(a1)
.endmacro
; The same four-wide, then 16-wide, expansion for each of the four.
.macro gte_w_d4,n
  gte_w_d n
  gte_w_d n + 1
  gte_w_d n + 2
  gte_w_d n + 3
.endmacro
.macro gte_w_c4,n
  gte_w_c n
  gte_w_c n + 1
  gte_w_c n + 2
  gte_w_c n + 3
.endmacro
.macro gte_r_d4,n
  gte_r_d n
  gte_r_d n + 1
  gte_r_d n + 2
  gte_r_d n + 3
.endmacro
.macro gte_r_c4,n
  gte_r_c n
  gte_r_c n + 1
  gte_r_c n + 2
  gte_r_c n + 3
.endmacro
.macro gte_w_d16,n
  gte_w_d4 n
  gte_w_d4 n + 4
  gte_w_d4 n + 8
  gte_w_d4 n + 12
.endmacro
.macro gte_w_c16,n
  gte_w_c4 n
  gte_w_c4 n + 4
  gte_w_c4 n + 8
  gte_w_c4 n + 12
.endmacro
.macro gte_r_d16,n
  gte_r_d4 n
  gte_r_d4 n + 4
  gte_r_d4 n + 8
  gte_r_d4 n + 12
.endmacro
.macro gte_r_c16,n
  gte_r_c4 n
  gte_r_c4 n + 4
  gte_r_c4 n + 8
  gte_r_c4 n + 12
.endmacro

.org MB_BASE
mailbox_header 7, 0

.org PC0
  li t0, 0x40000000                     ; SR.CU2 set, kernel mode, interrupts off
  mtc0 t0, sr
  nop
  nop
  li s4, scratch
run_all:
  li s0, gte_cases
  li s1, GTE_RUN_CASES
  li s2, MB_BASE + MB_RECORDS           ; the next record's address
  li s3, 0x00010001                     ; its id word: id 1, flags valid

case_loop:
  li a0, zero_words
  jal gte_write64
  nop
  addiu a0, s0, 8                       ; the 64 inputs follow the case number and the command word
  jal gte_write64
  nop
  lw t1, 4(s0)                          ; the command word
  nop
  li t2, 0xFFFFFFFF
  beq t1, t2, registers_only
  nop
  li t2, 0x01FFFFFF                     ; the 25-bit command field, behind COP2 with the CO bit set
  and t1, t1, t2
  lui t2, 0x4A00
  or t1, t1, t2
  li t0, cop2_slot + KSEG1
  sw t1, 0(t0)
  jalr t0
  nop
registers_only:
  li a1, out_buf
  jal gte_read64
  nop
  li a0, out_buf
  jal crc256
  nop
  sw s3, 0(s2)                          ; record: id word, then the CRC of the 64 words
  sw v0, 4(s2)
  addiu s3, s3, 1
  addiu s2, s2, 8
  addiu s0, s0, CASE_BYTES
  addiu s1, s1, -1
  bne s1, zero, case_loop
  nop
.if LOOP
  j run_all
  nop
.else
  li t0, MB_BASE
  li t1, GTE_RUN_CASES
  sb t1, MB_COUNT(t0)
finish:
  mailbox_seal
hang:
  j hang
  nop
.endif

; gte_write64: a0 = 64 words, written to registers 0 to 63. Clobbers t0.
gte_write64:
  gte_w_d16 0
  gte_w_d16 16
  gte_w_c16 0
  gte_w_c16 16
  jr ra
  nop

; gte_read64: reads registers 0 to 63 into the 64 words at a1. Clobbers t0.
gte_read64:
  gte_r_d16 0
  gte_r_d16 16
  gte_r_c16 0
  gte_r_c16 16
  jr ra
  nop

; crc256: v0 = CRC-32 (reflected polynomial EDB88320h, initial value and final XOR FFFFFFFFh) of the 256
; bytes at a0, the same bit-by-bit loop mailbox_seal uses. Clobbers a0, a2, a3 and t1-t3.
crc256:
  addiu a3, a0, 256
  li a2, 0xEDB88320
  li v0, 0xFFFFFFFF
@@byte:
  lbu t1, 0(a0)
  addiu a0, a0, 1                       ; fills the load delay slot
  xor v0, v0, t1
  li t2, 8
@@bit:
  andi t3, v0, 1
  srl v0, v0, 1
  beq t3, zero, @@next
  nop
  xor v0, v0, a2
@@next:
  addiu t2, t2, -1
  bne t2, zero, @@bit
  nop
  bne a0, a3, @@byte
  nop
  nor v0, v0, zero
  jr ra
  nop

; The command slot. The loop stores the command word over the first instruction and calls this through its
; uncached alias, so the fetch sees the new word whatever the instruction cache holds. s4 is the scratch word
; the SWC2 stores data register 0 to.
cop2_slot:
  cop2 0
  swc2 $0, 0(s4)
  jr ra
  nop

.align 4
scratch:
  .word 0
out_buf:
  .fill 256
zero_words:
  .fill 256
.include "gte_cases.inc"

.align 0x800
SW_END:
.close
