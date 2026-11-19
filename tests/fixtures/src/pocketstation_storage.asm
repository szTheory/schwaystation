; Exercise physical and selected-file PocketStation storage through the real
; JOY registers while retaining the ordinary 53h card ID transcript.
.include "header.inc"

.definelabel IO, 0xBF801000
.definelabel DEBUG_EXIT, 0xBF802082

.macro exchange_expect,byte,ack,expected,failure
  li a0, byte
  li a1, ack
  jal sio_exchange
  nop
  li t0, expected
  bne v0, t0, failure
  nop
.endmacro

.org PC0
  mtc0 zero, sr
  nop
  nop
  mtc0 zero, cause
  li s0, IO
  li t0, 0x000C
  sh t0, 0x048(s0)
  li t0, 0x0088
  sh t0, 0x04E(s0)
  li t0, 0x1003
  sh t0, 0x04A(s0)

  ; Preserve the 53h GetID success check.
  exchange_expect 0x81, 1, 0xFF, fail_id
  exchange_expect 0x53, 1, 0x08, fail_id
  exchange_expect 0, 1, 0x5A, fail_id
  exchange_expect 0, 1, 0x5D, fail_id
  exchange_expect 0, 1, 0x5C, fail_id
  exchange_expect 0, 1, 0x5D, fail_id
  exchange_expect 0, 1, 0x04, fail_id
  exchange_expect 0, 1, 0, fail_id
  exchange_expect 0, 1, 0, fail_id
  exchange_expect 0, 0, 0x80, fail_id

  ; Physical 5Ch write, followed by the matching 5Bh read.
  li a0, 0x5C
  li a1, 0x08000100
  li a2, 8
  li a3, 1
  li t4, 0x61
  jal pocket_begin
  nop
  li t0, 8
  bne v0, t0, fail_physical_write
  nop
  la s1, physical_payload
  li s2, 8
physical_write_loop:
  lbu a0, 0(s1)
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, fail_physical_write
  nop
  addiu s1, s1, 1
  addiu s2, s2, -1
  bne s2, zero, physical_write_loop
  nop
  exchange_expect 0, 0, 0xFF, fail_physical_write

  li a0, 0x5B
  li a1, 0x08000100
  li a2, 8
  li a3, 1
  li t4, 0x62
  jal pocket_begin
  nop
  li t0, 8
  bne v0, t0, fail_physical_read
  nop
  la s1, physical_payload
  li s2, 8
physical_read_loop:
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  lbu t0, 0(s1)
  nop
  bne v0, t0, fail_physical_read
  nop
  addiu s1, s1, 1
  addiu s2, s2, -1
  bne s2, zero, physical_read_loop
  nop
  exchange_expect 0, 0, 0xFF, fail_physical_read

  ; Standard 57h writes seed a two-block file directory (block 1 -> block 7).
  li a0, 1
  la a1, directory_first
  li t4, 0x63
  jal card_write_frame
  nop
  li a0, 7
  la a1, directory_last
  li t4, 0x63
  jal card_write_frame
  nop

  ; Select directory entry one with the ordinary 59h metadata transcript.
  exchange_expect 0x81, 1, 0xFF, fail_index
  li a0, 0x59
  li a1, 1
  jal sio_exchange
  nop
  beq v0, zero, index_flag_ok
  nop
  li t0, 0x08
  bne v0, t0, fail_index
  nop
index_flag_ok:
  exchange_expect 0, 1, 0x06, fail_index
  exchange_expect 0, 1, 0, fail_index
  exchange_expect 1, 1, 0, fail_index
  exchange_expect 0, 1, 0, fail_index
  exchange_expect 0, 1, 0, fail_index
  exchange_expect 0, 1, 0, fail_index
  exchange_expect 0, 0, 0, fail_index

  ; The virtual write crosses block 1's last four bytes into selected block 7.
  li a0, 0x5C
  li a1, 0x02001FFC
  li a2, 8
  li a3, 1
  li t4, 0x65
  jal pocket_begin
  nop
  li t0, 8
  bne v0, t0, fail_virtual_write
  nop
  la s1, virtual_payload
  li s2, 8
virtual_write_loop:
  lbu a0, 0(s1)
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, fail_virtual_write
  nop
  addiu s1, s1, 1
  addiu s2, s2, -1
  bne s2, zero, virtual_write_loop
  nop
  exchange_expect 0, 0, 0xFF, fail_virtual_write

  li a0, 0x5B
  li a1, 0x02001FFC
  li a2, 8
  li a3, 1
  li t4, 0x66
  jal pocket_begin
  nop
  li t0, 8
  bne v0, t0, fail_virtual_read
  nop
  la s1, virtual_payload
  li s2, 8
virtual_read_loop:
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  lbu t0, 0(s1)
  nop
  bne v0, t0, fail_virtual_read
  nop
  addiu s1, s1, 1
  addiu s2, s2, -1
  bne s2, zero, virtual_read_loop
  nop
  exchange_expect 0, 0, 0xFF, fail_virtual_read

  ; Invalid extra-flash address must terminate with FF/no-ACK; reread proves
  ; the previously committed physical bytes remain intact.
  li a0, 0x5B
  li a1, 0x06000300
  li a2, 1
  move a3, zero
  li t4, 0x67
  jal pocket_begin
  nop
  li t0, 0xFF
  bne v0, t0, fail_bad_address
  nop
  li a0, 0x5B
  li a1, 0x08000100
  li a2, 8
  li a3, 1
  li t4, 0x68
  jal pocket_begin
  nop
  li t0, 8
  bne v0, t0, fail_unchanged
  nop
  la s1, physical_payload
  li s2, 8
unchanged_read_loop:
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  lbu t0, 0(s1)
  nop
  bne v0, t0, fail_unchanged
  nop
  addiu s1, s1, 1
  addiu s2, s2, -1
  bne s2, zero, unchanged_read_loop
  nop
  exchange_expect 0, 0, 0xFF, fail_unchanged

  ; An unimplemented storage function has the explicit FF/no-ACK result.
  exchange_expect 0x81, 1, 0xFF, fail_unsupported
  exchange_expect 0x5B, 1, 0, fail_unsupported
  exchange_expect 0x02, 0, 0xFF, fail_unsupported

  ; Abort an in-flight physical write after two bytes, reset SIO, then confirm
  ; that the destination still contains its initialized FF bytes.
  li a0, 0x5C
  li a1, 0x08002000
  li a2, 4
  li a3, 1
  li t4, 0x69
  jal pocket_begin
  nop
  li t0, 4
  bne v0, t0, fail_abort_start
  nop
  la s1, abort_payload
  li s2, 2
abort_write_loop:
  lbu a0, 0(s1)
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, fail_abort_data
  nop
  addiu s1, s1, 1
  addiu s2, s2, -1
  bne s2, zero, abort_write_loop
  nop
  sh zero, 0x04A(s0)
  li t0, 0x000C
  sh t0, 0x048(s0)
  li t0, 0x0088
  sh t0, 0x04E(s0)
  li t0, 0x1003
  sh t0, 0x04A(s0)
  li a0, 0x5B
  li a1, 0x08002000
  li a2, 4
  li a3, 1
  li t4, 0x69
  jal pocket_begin
  nop
  li t0, 4
  bne v0, t0, fail_abort_read_start
  nop
  li s2, 4
abort_read_loop:
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0xFF
  bne v0, t0, fail_abort_read_data
  nop
  addiu s2, s2, -1
  bne s2, zero, abort_read_loop
  nop
  exchange_expect 0, 0, 0xFF, fail_abort_end

  sh zero, 0x04A(s0)
  li a0, 0x51
  b fixture_exit
  nop

; a0 = 5Bh/5Ch, a1 = FUNC 01 address, a2 = requested length,
; a3 = wait for the LEN2 ACK (zero expects the invalid-request no-ACK).
; t4 = caller's failure code. Returns LEN2 in v0.
pocket_begin:
  move t5, ra
  move t1, a0
  move t2, a1
  move t3, a2
  move s4, a3
  li a0, 0x81
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0xFF
  bne v0, t0, pocket_begin_bad
  nop
  move a0, t1
  li a1, 1
  jal sio_exchange
  nop
  beq v0, zero, pocket_begin_flag_ok
  nop
  li t0, 0x08
  bne v0, t0, pocket_begin_bad
  nop
pocket_begin_flag_ok:
  li a0, 1
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0xFF
  bne v0, t0, pocket_begin_bad
  nop
  li a0, 0
  li a1, 1
  jal sio_exchange
  nop
  li t0, 5
  bne v0, t0, pocket_begin_bad
  nop
  andi a0, t2, 0xFF
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, pocket_begin_bad
  nop
  srl t0, t2, 8
  andi a0, t0, 0xFF
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, pocket_begin_bad
  nop
  srl t0, t2, 16
  andi a0, t0, 0xFF
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, pocket_begin_bad
  nop
  srl t0, t2, 24
  andi a0, t0, 0xFF
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, pocket_begin_bad
  nop
  andi a0, t3, 0xFF
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, pocket_begin_bad
  nop
  move a0, zero
  move a1, s4
  jal sio_exchange
  nop
  beq s4, zero, pocket_begin_reject
  nop
  jr t5
  nop
pocket_begin_reject:
  li t0, 0xFF
  bne v0, t0, pocket_begin_bad
  nop
  move v0, t0
  jr t5
  nop
pocket_begin_bad:
  move a0, t4
  b fixture_exit
  nop

; a0 = card sector, a1 = frame pointer, t4 = failure code.
; Computes the standard 57h checksum, then uses the wire transcript to commit.
card_write_frame:
  move t5, ra
  move t1, a0
  move t2, a1
  srl t0, t1, 8
  andi t0, t0, 0xFF
  andi t3, t1, 0xFF
  xor t3, t3, t0
  li s7, 128
card_checksum_loop:
  lbu t6, 0(t2)
  nop
  xor t3, t3, t6
  addiu t2, t2, 1
  addiu s7, s7, -1
  bne s7, zero, card_checksum_loop
  nop
  move t2, a1

  exchange_expect 0x81, 1, 0xFF, card_frame_bad
  li a0, 0x57
  li a1, 1
  jal sio_exchange
  nop
  beq v0, zero, card_frame_flag_ok
  nop
  li t0, 0x08
  bne v0, t0, card_frame_bad
  nop
card_frame_flag_ok:
  exchange_expect 0, 1, 0x5A, card_frame_bad
  exchange_expect 0, 1, 0x5D, card_frame_bad
  exchange_expect 0, 1, 0, card_frame_bad
  srl t0, t1, 8
  andi a0, t0, 0xFF
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, card_frame_bad
  nop
  andi a0, t1, 0xFF
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, card_frame_bad
  nop
  li s7, 128
card_data_loop:
  lbu a0, 0(t2)
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, card_frame_bad
  nop
  addiu t2, t2, 1
  addiu s7, s7, -1
  bne s7, zero, card_data_loop
  nop
  move a0, t3
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, card_frame_bad
  nop
  exchange_expect 0, 1, 0x5C, card_frame_bad
  exchange_expect 0, 1, 0x5D, card_frame_bad
  li a0, 0
  li a1, 0
  jal sio_exchange
  nop
  li t0, 0x47
  bne v0, t0, card_frame_bad
  nop
  jr t5
  nop
card_frame_bad:
  move a0, t4
  b fixture_exit
  nop

; a0 = byte to transmit; a1 = wait for /ACK after receiving.
sio_exchange:
  sb a0, 0x040(s0)
  li t6, 10000
sio_rx_wait:
  lhu t0, 0x044(s0)
  nop
  andi t0, t0, 2
  bne t0, zero, sio_rx_ready
  nop
  addiu t6, t6, -1
  bne t6, zero, sio_rx_wait
  nop
  li a0, 0x52
  b fixture_exit
  nop
sio_rx_ready:
  lbu v0, 0x040(s0)
  beq a1, zero, sio_done
  nop
  li t6, 10000
sio_ack_wait:
  lhu t0, 0x044(s0)
  nop
  andi t0, t0, 0x80
  bne t0, zero, sio_ack_release
  nop
  addiu t6, t6, -1
  bne t6, zero, sio_ack_wait
  nop
  li a0, 0x53
  b fixture_exit
  nop
sio_ack_release:
  li t6, 10000
sio_release_wait:
  lhu t0, 0x044(s0)
  nop
  andi t0, t0, 0x80
  beq t0, zero, sio_done
  nop
  addiu t6, t6, -1
  bne t6, zero, sio_release_wait
  nop
  li a0, 0x54
  b fixture_exit
  nop
sio_done:
  jr ra
  nop

fail_id:
  li a0, 0x55
  b fixture_exit
  nop
fail_physical_write:
  li a0, 0x61
  b fixture_exit
  nop
fail_physical_read:
  li a0, 0x62
  b fixture_exit
  nop
fail_directory_write:
  li a0, 0x63
  b fixture_exit
  nop
fail_index:
  li a0, 0x64
  b fixture_exit
  nop
fail_virtual_write:
  li a0, 0x65
  b fixture_exit
  nop
fail_virtual_read:
  li a0, 0x66
  b fixture_exit
  nop
fail_bad_address:
  li a0, 0x67
  b fixture_exit
  nop
fail_unchanged:
  li a0, 0x68
  b fixture_exit
  nop
fail_abort_start:
  li a0, 0x69
  b fixture_exit
  nop
fail_abort_data:
  li a0, 0x6B
  b fixture_exit
  nop
fail_abort_read_start:
  li a0, 0x6C
  b fixture_exit
  nop
fail_abort_read_data:
  li a0, 0x6D
  b fixture_exit
  nop
fail_abort_end:
  li a0, 0x6E
  b fixture_exit
  nop
fail_unsupported:
  li a0, 0x6A
  b fixture_exit
  nop
fixture_exit:
  sh zero, 0x04A(s0)
  li t0, DEBUG_EXIT
  sb a0, 0(t0)
hang:
  b hang
  nop

.align 4
physical_payload:
  .byte 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
virtual_payload:
  .byte 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x17, 0x28
abort_payload:
  .byte 0x91, 0x82, 0x73, 0x64
directory_first:
  .byte 0x51, 0, 0, 0, 0, 0x40, 0, 0, 6, 0
  .fill 117, 0
  .byte 0x17
directory_last:
  .byte 0x53, 0, 0, 0, 0, 0x40, 0, 0, 0xFF, 0xFF
  .fill 117, 0
  .byte 0x13

.align 0x800
SW_END:
.close
