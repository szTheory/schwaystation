; The mailbox every fixture shares: 256 bytes at the start of the loaded image, 80010000h.
;
;   00h  magic "SWMB"
;   04h  CRC-32 (IEEE) of bytes 10h..FFh, little-endian
;   08h  status: 80h running, 81h waiting for input, 00h done, FFh fault
;   09h  protocol version, 1
;   0Ah  record count, at most 30
;   0Bh  fixture id
;   0Ch  address of a handler the stub kernel jumps to after an exception, 0 for none
;   10h + 8n  record n: id u16, flags u16 (bit 0 valid, bit 1 timed out, bit 2 profile dependent), value u32
;
; The stub kernel sets the word at 08h to 000000FFh when it takes an exception, so a fault clears the
; version, count and id bytes with it; the CRC is not rewritten and no longer matches.
; Fault prevented: a fixture and the stub kernel or the runner reading the same byte as two things.
.definelabel MB_BASE, 0x80010000
.definelabel MB_CRC, 0x04
.definelabel MB_STATUS, 0x08
.definelabel MB_VERSION, 0x09
.definelabel MB_COUNT, 0x0A
.definelabel MB_ID, 0x0B
.definelabel MB_HANDLER, 0x0C
.definelabel MB_RECORDS, 0x10
.definelabel MB_STATUS_RUNNING, 0x80
.definelabel MB_STATUS_DONE, 0x00

; mailbox_header id, handler: the 16 header bytes, status running, then the 240 zero bytes that hold
; the records. Place it with .org MB_BASE.
.macro mailbox_header,id,handler
  .ascii "SWMB"
  .word 0                               ; CRC, written by mailbox_seal
  .byte MB_STATUS_RUNNING
  .byte 1
  .byte 0
  .byte id
  .word handler
  .fill 0x100 - 0x10
.endmacro

; mailbox_seal: computes the CRC-32 of bytes 10h..FFh bit by bit (the reflected polynomial
; EDB88320h, initial value and final XOR FFFFFFFFh), stores it at 04h and sets the status to done.
; It uses a0-a2, v0 and t0-t3.
.macro mailbox_seal
  li a0, MB_BASE + MB_RECORDS
  li a1, MB_BASE + 0x100
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
  bne a0, a1, @@byte
  nop
  nor v0, v0, zero
  li t0, MB_BASE
  sw v0, MB_CRC(t0)
  sb zero, MB_STATUS(t0)
.endmacro
