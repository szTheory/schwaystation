; Read the standard memory-card ID through JOY MMIO and report a distinct
; failure byte for every transport failure or unexpected response. The 53h
; GetID reply follows docs/hardware/sio.md (GetID).
.include "header.inc"

.definelabel IO, 0xBF801000
.definelabel DEBUG_EXIT, 0xBF802082

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

  li a0, 0x81
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0xFF
  bne v0, t0, fixture_bad_response
  nop
  li a0, 0x53
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0x08
  bne v0, t0, fixture_bad_response
  nop

  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0x5A
  bne v0, t0, fixture_bad_response
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0x5D
  bne v0, t0, fixture_bad_response
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0x5C
  bne v0, t0, fixture_bad_response
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0x5D
  bne v0, t0, fixture_bad_response
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  li t0, 0x04
  bne v0, t0, fixture_bad_response
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, fixture_bad_response
  nop
  move a0, zero
  li a1, 1
  jal sio_exchange
  nop
  bne v0, zero, fixture_bad_response
  nop
  move a0, zero
  move a1, zero
  jal sio_exchange
  nop
  li t0, 0x80
  bne v0, t0, fixture_bad_response
  nop

  sh zero, 0x04A(s0)
  li a0, 0x51
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

fixture_bad_response:
  li a0, 0x55
fixture_exit:
  sh zero, 0x04A(s0)
  li t0, DEBUG_EXIT
  sb a0, 0(t0)
hang:
  b hang
  nop

.align 0x800
SW_END:
.close
