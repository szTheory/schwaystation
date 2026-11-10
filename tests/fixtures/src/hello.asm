; hello.exe: the first image that runs. It reports what the stub kernel handed it, in its mailbox,
; and then jumps to itself. With the profile's debug-port flag set it also prints two lines: `hello port`
; through the debug port at 1F802080h, then `hello putchar` through the kernel's putchar, A(3Ch).
;   record 1 (id 1): gp, which the header sets to GP0, 80018000h
;   record 2 (id 2): sp, which the stub sets to s_addr + s_size, 80200000h for this header
; Fault prevented: a stub that ignores the header's entry point, global pointer or stack, or a
; core that cannot run an image to the end; a TTY byte that is lost, reordered or sent by the wrong
; source.
.definelabel GP0, 0x80018000
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
mailbox_header 1, 0

.org PC0
  ; The line through the port: one byte store at a time.
  li s0, msg_port
  li s1, 0xBF802080
port_loop:
  lbu t0, 0(s0)
  addiu s0, s0, 1                       ; fills the load delay slot
  beq t0, zero, port_done
  nop
  sb t0, 0(s1)
  j port_loop
  nop
port_done:
  ; The line through the kernel: A(3Ch) takes the byte in a0, the call is `jal 800000A0h`.
  li s0, msg_putchar
putchar_loop:
  lbu a0, 0(s0)
  addiu s0, s0, 1
  beq a0, zero, putchar_done
  nop
  jal 0x800000A0
  li t1, 0x3C                           ; delay slot: the function number
  j putchar_loop
  nop
putchar_done:
  li t0, MB_BASE
  li t1, 0x00010001                     ; id 1, flags: valid
  sw t1, MB_RECORDS(t0)
  sw gp, MB_RECORDS + 4(t0)
  li t1, 0x00010002                     ; id 2, flags: valid
  sw t1, MB_RECORDS + 8(t0)
  sw sp, MB_RECORDS + 12(t0)
  li t1, 2
  sb t1, MB_COUNT(t0)
  mailbox_seal
hang:
  j hang
  nop

msg_port:
  .ascii "hello port"
  .byte 10, 0
msg_putchar:
  .ascii "hello putchar"
  .byte 10, 0

.align 0x800
SW_END:
.close
