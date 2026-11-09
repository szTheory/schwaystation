; exit_port.exe: writes one byte to the exit port at 1F802082h and loops, so the runner's outcome for the
; byte is the whole test.
;
;   armips -strequ IMAGE <file> -equ CODE <byte> exit_port.asm
;
; The port acts only with the profile's debug-port flag set. Byte 00h is a pass; any other byte is the
; exit code the runner reports.
; Fault prevented: an exit byte that is lost or reported as another value, or a zero that fails the run.
.include "header.inc"

.org PC0
  li t0, 0xBF802082                     ; the port in KSEG1
  li t1, CODE
  sb t1, 0(t0)
hang:
  j hang
  nop

.align 0x800
SW_END:
.close
