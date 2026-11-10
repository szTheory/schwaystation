; dma_chain.exe: runs DMA transfers on the real CPU and records what the registers and RAM show: the ordering
; table the OTC channel builds, its register readbacks, the master-enable control, three linked lists on the GPU
; channel (the GPU itself is not built) and the time one OTC transfer takes by Timer 2.
; It judges nothing: tests/diag/expected.csv does, one row per record in record order.
;
;   armips -strequ IMAGE <file> dma_chain.asm
;
; The cases are those of the public hardware tests dma/otc-test and dma/chain-looping: the same register writes
; in the same order, the same chains, the same 1000-pass work loop and the same stop with CHCR = 0.
; Records, in order:
;    1  ot_first       the word the transfer writes first, at MADR
;    2  ot_last        the word it writes last, the end marker
;    3  otc_chcr       D6_CHCR read right after the transfer
;    4  otc_dicr       DICR after the transfer
;    5  otc_istat      I_STAT bit 3 after the transfer
;    6  otc_master_off the word at MADR when DPCR has channel 6 disabled
;    7  list_empty     bit 0 finished, bit 1 interrupt flag, for a complete list with no words
;    8  list_single    the same for a list of one node that names itself
;    9  list_double    the same for a list of two nodes that name each other
;   10  otc_time       Timer 2 clocks from before the D6_CHCR write to after the read that waits for the end
; Fault prevented: a DMA engine whose transfers are right when the host calls it directly but wrong behind
; the CPU's loads and stores to the DMA registers, or a recorded value that no row supports.
.include "header.inc"
.include "mailbox.inc"

.definelabel IO, 0xBF801000             ; the I/O ports, uncached
.definelabel OT, 0x80100000             ; the ordering table the transfer builds: 4096 words
.definelabel OT_LAST_WORD, 0x00103FFC   ; where the transfer starts, the physical address of the table's last word
.definelabel PATTERN, 0x80104000        ; four words the master-disabled control leaves alone
.definelabel CHAINS, 0x80104100         ; the three lists
.definelabel WORK, 0x80104200           ; the work loop's counter

.definelabel DPCR_OTC, 0x0F654321       ; channel 6 enabled and the default priorities
.definelabel DPCR_OTC_OFF, 0x07654321   ; channel 6 disabled
.definelabel DPCR_OTC_GPU, 0x0F654B21   ; channels 6 and 2 enabled
.definelabel DICR_OTC, 0x00C00000       ; master enable and channel 6 enable
.definelabel DICR_OTC_CLEAR, 0x7FC00000 ; the same with every flag acknowledged
.definelabel DICR_GPU_CLEAR, 0x7FC40000 ; master, channel 6 and channel 2 enables, every flag acknowledged
.definelabel CHCR_OTC, 0x11000002       ; start and manual trigger with bit 1 as the hardware test writes it
.definelabel CHCR_LIST, 0x01000401      ; from RAM, linked list, start

; record reg: stores the value in reg as the next record. Sets nothing else.
.macro record,reg
  sw s3, 0(s2)
  sw reg, 4(s2)
  addiu s3, s3, 1
  addiu s2, s2, 8
.endmacro

; set_node base, n, value: stores a list header word at word n of the list at base.
.macro set_node,base,n,value
  li t0, value
  sw t0, (n)*4(base)
.endmacro

.org MB_BASE
mailbox_header 8, 0

.org PC0
  mtc0 zero, sr                         ; kernel mode, interrupts off, no coprocessor
  nop
  nop
  li s5, IO
  li s2, MB_BASE + MB_RECORDS           ; the next record's address
  li s3, 0x00010001                     ; its id word: id 1, flags valid

  ; The ordering table: the transfer builds 4096 words downward from the last one.
  sw zero, 0x70(s5)                     ; I_STAT: acknowledge everything
  li t0, DPCR_OTC
  sw t0, 0xF0(s5)
  li t0, DICR_OTC
  sw t0, 0xF4(s5)
  li t0, OT_LAST_WORD
  sw t0, 0xE0(s5)                       ; D6_MADR
  li t0, 4096
  sw t0, 0xE4(s5)                       ; D6_BCR
  li t0, CHCR_OTC
  sw t0, 0xE8(s5)                       ; D6_CHCR: start
  lw t1, 0xE8(s5)                       ; a register read waits for the transfer
  li a0, OT
  lw t2, 0x3FFC(a0)                     ; the first word written
  lw t3, 0(a0)                          ; the last word written
  lw t4, 0xF4(s5)                       ; DICR
  lw t5, 0x70(s5)                       ; I_STAT
  nop
  srl t5, t5, 3
  andi t5, t5, 1
  record t2
  record t3
  record t1
  record t4
  record t5

  ; Master disabled: channel 6's DPCR enable clear, so the start leaves the table as it was.
  li a0, PATTERN
  li t0, 0x11111111
  sw t0, 0(a0)
  li t0, 0x22222222
  sw t0, 4(a0)
  li t0, 0x33333333
  sw t0, 8(a0)
  li t0, 0x44444444
  sw t0, 12(a0)
  li t0, DPCR_OTC_OFF
  sw t0, 0xF0(s5)
  li t0, DICR_OTC_CLEAR
  sw t0, 0xF4(s5)
  sw zero, 0x70(s5)
  li t0, 0x0010400C
  sw t0, 0xE0(s5)
  li t0, 4
  sw t0, 0xE4(s5)
  li t0, CHCR_OTC
  sw t0, 0xE8(s5)
  lw t2, 12(a0)                         ; the word at MADR, which the transfer would have overwritten first
  nop
  sw zero, 0xE8(s5)                     ; leave nothing started for the enable that follows
  li t0, DPCR_OTC_GPU
  sw t0, 0xF0(s5)
  record t2

  ; Three lists on the GPU channel: one the walk completes, and two that never end.
  li a1, CHAINS
  set_node a1, 0, 0x00104104            ; the empty complete list: a node of no words naming the end node
  set_node a1, 1, 0x00FFFFFF
  set_node a1, 4, 0x00104110            ; the single self-referencing list
  set_node a1, 5, 0x00FFFFFF
  set_node a1, 8, 0x00104124            ; the double self-referencing list
  set_node a1, 9, 0x00104120
  set_node a1, 10, 0x00FFFFFF
  li a0, 0x00104100
  jal list_case
  nop
  move s0, v0
  li a0, 0x00104110
  jal list_case
  nop
  move s1, v0
  li a0, 0x00104120
  jal list_case
  nop
  record s0
  record s1
  record v0

  ; The time of one OTC transfer: Timer 2 on the system clock, read before the start and after the read that
  ; waits for the end.
  li t0, DPCR_OTC
  sw t0, 0xF0(s5)
  li t0, DICR_OTC_CLEAR
  sw t0, 0xF4(s5)
  sw zero, 0x70(s5)
  sw zero, 0x124(s5)                    ; Timer 2 MODE: the system clock, a free-running count
  li t0, OT_LAST_WORD
  sw t0, 0xE0(s5)
  li t0, 4096
  sw t0, 0xE4(s5)
  li t0, CHCR_OTC
  lw s0, 0x120(s5)                      ; Timer 2 before
  sw t0, 0xE8(s5)
  lw t1, 0xE8(s5)                       ; waits for the transfer
  lw s1, 0x120(s5)                      ; Timer 2 after
  nop
  subu s1, s1, s0
  andi s1, s1, 0xFFFF
  record s1

  li t0, MB_BASE
  li t1, 10
  sb t1, MB_COUNT(t0)
  mailbox_seal
hang:
  j hang
  nop

; list_case: a0 = physical address of the first node. Starts a linked list on the GPU channel as the hardware
; test does, runs the work loop, reads the channel and DICR, stops the channel with CHCR = 0 and returns in v0
; bit 0 = the transfer finished (D2_CHCR bit 24 clear) and bit 1 = the channel's DICR flag (bit 26).
list_case:
  li t0, 0x04000000
  sw t0, 0x814(s5)                      ; GP1: DMA direction off
  sw zero, 0xA8(s5)                     ; D2_CHCR
  sw a0, 0xA0(s5)                       ; D2_MADR
  li t0, 0x04000002
  sw t0, 0x814(s5)                      ; GP1: DMA direction CPU to GP0
  li t0, DICR_GPU_CLEAR
  sw t0, 0xF4(s5)
  sw zero, 0x70(s5)
  li t0, CHCR_LIST
  sw t0, 0xA8(s5)                       ; D2_CHCR: start
  li t1, 1000                           ; the work loop: 1000 passes of a read, an add and a write
  li t2, WORK
  sw zero, 0(t2)
@@work:
  lw t3, 0(t2)
  nop
  addu t3, t3, t1
  sw t3, 0(t2)
  addiu t1, t1, -1
  bne t1, zero, @@work
  nop
  lw t4, 0xA8(s5)
  lw t5, 0xF4(s5)
  nop
  srl t4, t4, 24
  andi t4, t4, 1
  xori v0, t4, 1
  srl t5, t5, 26
  andi t5, t5, 1
  sll t5, t5, 1
  or v0, v0, t5
  li t0, 0x04000000
  sw t0, 0x814(s5)
  sw zero, 0xA8(s5)                     ; stop the channel
  jr ra
  nop

.align 0x800
SW_END:
.close
