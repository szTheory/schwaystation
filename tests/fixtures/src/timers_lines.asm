; timers_lines.exe: measures what the frame driver and the root counters do over 20 frames and records it in
; the mailbox. It judges nothing: tests/diag/expected.csv does, one row per record in record order, for the
; machine profile the test names.
;
;   armips -strequ IMAGE <file> timers_lines.asm
;
; The fixture takes every interrupt through its own exception handler at 80000080h (the stub kernel's is not
; used: it fetches from the BIOS at 29 cycles a word and writes status FFh into the mailbox on each entry, which
; the runner reads at a frame's end as a fault). The idle loop is a branch to itself. Timer 0 counts the system
; clock and Timer 2 the system clock over 8, each raising an interrupt at FFFFh every time; Timer 1 counts the
; system clock and is reset by the vertical blank (synchronisation mode 1). I_MASK holds bits 0, 4 and 6.
;
; The handler runs with interrupts off. It reads I_STAT, the counts of Timers 0, 2 and 1 and I_STAT again, and
; reads them again when bits 0, 4 or 6 changed between the two reads of I_STAT, so the counts and the bits
; belong to one instant. Each wrap bit it saw set is one more wrap counted (the wrap that is pending behind
; the handler is counted whether or not its own entry has happened yet, so no snapshot is short by 10000h);
; it acknowledges exactly the bits it saw. On a blank it counts it and, at the 3rd and again at the 23rd, takes
; a snapshot of each counter as wraps * 10000h + count, minus the time since the blank (Timer 1's count: for
; Timer 2 that time over 8). The snapshot is therefore the counter's value at the blank itself, whatever
; delay lay between the blank and the handler's reads.
; Records, in order:
;    1  vblank_count   the blanks counted when the run ends
;    2  div8_ticks     Timer 2 steps, in eighths of a system clock, between the 3rd blank and the 23rd
;    3  sysclk_ticks   system clocks between the 3rd blank and the 23rd
;
; Why the rows have the ranges they have (every number below is computed from the documents, never taken from
; a run). N = 20 frames. A frame is num / den system clocks and a frame end lies where the exact accumulator
; puts it, so 20 frames between two frame ends are floor(20 * num / den) or one more:
;   NTSC  num 405350378496, den 715909: 11324075 or 11324076 clocks (frame 566203.775 clocks)
;   PAL   num 482961862656, den 709379: 13616469 or 13616470 clocks (frame 680823.456 clocks)
; The blank is raised where the instruction that crosses the frame end retires, 0 to L - 1 cycles late, L being
; the longest instruction of this program: its own cycle, an instruction-cache fill of the fetched word (5 for
; the word and 1 for each of 3 more, bus.md "Instruction cache", Fill cost, and "RAM and its mirrors"), the
; drain of a full write queue before a KSEG1 access (4 stores at 4 cycles, bus.md "Write queue") and a read of
; an I/O port (1 cycle, bus.md "Memory control", Costs). A RAM read (5 cycles) or a store to a full queue
; (4 and 1) costs less; no multiply, divide or coprocessor instruction is used (cpu.md "Multiply and divide"
; and "Instruction fetch": a cached hit costs only the instruction's own cycle).
;   L = 1 + 8 + 16 + 1 = 26, so the jitter J = L - 1 = 25 cycles on each blank.
; The elapsed time between the two blanks is then in [C - 25, C + 1 + 25] clocks, C the floor above. The
; handler's delay cancels because the snapshots are taken at the blank. What does not cancel is the distance
; between the read of Timer 0 and the read of Timer 1, which holds one instruction-cache fill (up to 8 cycles)
; on a cold line, and the convention of the first cycle after a reset (2 at most), each way: 10 more clocks on
; each side for sysclk_ticks:
;   NTSC  [11324050 - 10, 11324101 + 10] = acca88..accacf
;   PAL   [13616444 - 10, 13616495 + 10] = cfc532..cfc579
; For div8_ticks the same time is read in eighths. Timer 2 steps at every tick that is a multiple of 8, and the
; snapshot of Timer 2 is read at an offset h from the blank that lies in [-12, 12] clocks (the order of the
; reads, the reset convention and one cache fill), which moves floor((T + h) / 8) by -2 to +3 against
; floor(T / 8). The difference of two snapshots is therefore the window P over 8 rounded either way, plus or
; minus 5 eighths: [ceil((P_lo - 7) / 8) - 5, floor((P_hi + 7) / 8) + 5] with P the window above before the
; 10-clock margin (P_lo = C - 25, P_hi = C + 26):
;   NTSC  15994d..15995e
;   PAL   19f8a2..19f8b3
; Fault prevented: a vertical blank or a root counter that is right for one machine profile and wrong for the
; other, a counter whose wraps are lost behind an interrupt, or a recorded value that no row supports.
.include "header.inc"
.include "mailbox.inc"

.ifndef TIMING
.definelabel TIMING, 0
.endif
.if TIMING
.definelabel TIMING_IO, 0xBF801000
.definelabel TIMING_VECTOR, 0x80000080
.definelabel TIMING_FIRST, 3
.definelabel TIMING_LAST, TIMING_FIRST + WINDOW
.definelabel TIMING_IRQS, 0x11             ; VBlank and Timer 0 wrap

.org MB_BASE
mailbox_header 9, 0

.org PC0
  mtc0 zero, sr
  nop
  nop
  mtc0 zero, cause
  li t0, timing_vector_jump
  li t1, TIMING_VECTOR
  lw t2, 0(t0)
  lw t3, 4(t0)
  lw t4, 8(t0)
  lw t5, 12(t0)
  sw t2, 0(t1)
  sw t3, 4(t1)
  sw t4, 8(t1)
  sw t5, 12(t1)
  li s0, TIMING_IO
  move s1, zero                         ; VBlank events
  move s2, zero                         ; Timer 0 wraps, including a pending wrap
  move s3, zero                         ; dot count at the first sampled blank
  move s4, zero                         ; HBlank count at the first sampled blank
  move s5, zero                         ; dots in the phase-restoring window
  move s6, zero                         ; HBlanks in the window
  move s7, zero                         ; lines per frame
  sw zero, 0x70(s0)
  li t0, TIMING_IRQS
  sw t0, 0x74(s0)
  lui t0, 0x0800                         ; GP1(08h): choose the documented dot divider
  ori t0, t0, DISPLAY_MODE
  sw t0, 0x814(s0)
  sw zero, 0x100(s0)
  li t0, 0x0160                         ; Timer 0: free-running dot clock, repeat IRQ at FFFFh
  sw t0, 0x104(s0)
  sw zero, 0x110(s0)
  li t0, 0x0100                         ; Timer 1: count every HBlank edge
  sw t0, 0x114(s0)
  li t0, 0x0401                         ; IEc and the interrupt-controller mask
  mtc0 t0, sr
timing_idle:
  b timing_idle
  nop

timing_handler:
timing_snapshot:
  ; Keep the Timer 0 sample at one CPU endpoint offset. In PAL /4, an 8-cycle
  ; offset advances 12 then 13 GPU clocks at the phase-restoring VBlank pair;
  ; this two-read preamble makes both offsets 22 clocks. The ratio carry is
  ; documented in docs/hardware/gpu.md and the trace evidence in docs/testing.md.
  lw t8, 0x70(s0)                      ; first I_STAT read fixes the interrupt-handler cadence
  lw t0, 0x70(s0)
  lw t1, 0x100(s0)                      ; Timer 0 dot count
  lw t2, 0x110(s0)                      ; Timer 1 HBlank count
  lw t3, 0x70(s0)
  nop
  xor t4, t0, t3
  andi t4, t4, TIMING_IRQS
  bne t4, zero, timing_snapshot
  nop
  andi t4, t0, 0x10
  srl t4, t4, 4
  addu s2, s2, t4                       ; count a pending wrap before acknowledging it
  andi t4, t0, TIMING_IRQS
  nor t4, t4, zero
  sw t4, 0x70(s0)
  andi t4, t0, 1
  beq t4, zero, timing_return
  nop
  addiu s1, s1, 1
  sll t4, s2, 16
  addu t4, t4, t1                       ; cumulative Timer 0 dots including wraps
  li t5, TIMING_FIRST
  bne s1, t5, timing_not_first
  nop
  move s3, t4
  move s4, t2
timing_not_first:
  li t5, TIMING_LAST
  bne s1, t5, timing_return
  nop
  subu s5, t4, s3                       ; dot total across a phase-restoring window
  subu s6, t2, s4                       ; one HBlank edge per line
  li t5, WINDOW
  divu s6, t5
  mflo s7
  j timing_finish
  nop
timing_return:
  mfc0 k0, epc
  nop
  jr k0
  rfe

timing_finish:
  li t0, MB_BASE
  li t1, 0x00010001
  sw t1, MB_RECORDS(t0)
  sw s1, MB_RECORDS + 4(t0)
  li t1, 0x00010002
  sw t1, MB_RECORDS + 8(t0)
  sw s5, MB_RECORDS + 12(t0)
  li t1, 0x00010003
  sw t1, MB_RECORDS + 16(t0)
  sw s7, MB_RECORDS + 20(t0)
  li t1, 0x00010004
  sw t1, MB_RECORDS + 24(t0)
  sw s6, MB_RECORDS + 28(t0)
  li t1, 4
  sb t1, MB_COUNT(t0)
  mailbox_seal
timing_hang:
  j timing_hang
  nop

timing_vector_jump:
  lui k0, timing_handler >> 16
  ori k0, k0, timing_handler & 0xFFFF
  jr k0
  nop
.align 0x800
SW_END:
.else
.definelabel IO, 0xBF801000             ; the I/O ports, uncached
.definelabel VECTOR, 0x80000080         ; the general exception vector, kernel RAM
.definelabel FIRST, 3                   ; the blank whose snapshot starts the window
.definelabel LAST, 23                   ; the blank whose snapshot ends it and ends the run
.definelabel IRQ_BITS, 0x51             ; I_STAT bits 0 (blank), 4 (Timer 0) and 6 (Timer 2)

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
  li s2, 0                              ; the wraps of Timer 0 counted
  li s3, 0                              ; the wraps of Timer 2 counted
  li s4, 0                              ; Timer 0's snapshot at the first blank
  li s5, 0                              ; Timer 2's snapshot at the first blank
  sw zero, 0x70(s0)                     ; I_STAT: acknowledge everything
  li t0, IRQ_BITS
  sw t0, 0x74(s0)                       ; I_MASK
  li t0, 0x0060                         ; the system clock, reset at FFFFh, interrupt at FFFFh every time
  sw t0, 0x104(s0)                      ; Timer 0 MODE
  li t0, 0x0003                         ; the system clock, reset at the beginning of the vertical blank
  sw t0, 0x114(s0)                      ; Timer 1 MODE
  li t0, 0x0260                         ; the system clock over 8, otherwise as Timer 0
  sw t0, 0x124(s0)                      ; Timer 2 MODE
  li t0, 0x0401                         ; SR: interrupt mask bit 10 and IEc
  mtc0 t0, sr
idle:
  b idle
  nop

handler:
snapshot:
  lw t0, 0x70(s0)                       ; I_STAT
  lw t1, 0x100(s0)                      ; Timer 0 count
  lw t2, 0x120(s0)                      ; Timer 2 count
  lw t3, 0x110(s0)                      ; Timer 1 count: the time since the last blank
  lw t4, 0x70(s0)                       ; I_STAT again
  nop
  xor t5, t0, t4
  andi t5, t5, IRQ_BITS
  bne t5, zero, snapshot                    ; a wrap or a blank came between the reads: read again
  nop
  andi t5, t0, 0x10
  srl t5, t5, 4
  addu s2, s2, t5                       ; Timer 0's wraps, the pending one included
  andi t5, t0, 0x40
  srl t5, t5, 6
  addu s3, s3, t5                       ; Timer 2's wraps, the pending one included
  andi t5, t0, IRQ_BITS
  nor t5, t5, zero
  sw t5, 0x70(s0)                       ; acknowledge exactly the bits that were counted
  andi t6, t0, 1
  beq t6, zero, handler_return
  nop
  addiu s1, s1, 1
  sll t5, s2, 16
  addu t5, t5, t1
  subu t5, t5, t3                       ; Timer 0 at the blank, in system clocks
  sll t6, s3, 16
  addu t6, t6, t2
  srl t7, t3, 3
  subu t6, t6, t7                       ; Timer 2 at the blank, in eighths
  li t8, FIRST
  bne s1, t8, not_first
  nop
  move s4, t5
  move s5, t6
not_first:
  li t8, LAST
  bne s1, t8, handler_return
  nop
  subu s6, t6, s5                       ; the eighths in the window
  subu s7, t5, s4                       ; the system clocks in the window
  j finish
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
  li t1, 0x00010002
  sw t1, MB_RECORDS + 8(t0)
  sw s6, MB_RECORDS + 12(t0)
  li t1, 0x00010003
  sw t1, MB_RECORDS + 16(t0)
  sw s7, MB_RECORDS + 20(t0)
  li t1, 3
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
.endif
.close
