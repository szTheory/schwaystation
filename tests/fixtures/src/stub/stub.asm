; The stub kernel: the BIOS image the core maps at BFC00000h when the host supplies none.
; MIT-licensed, written for this project; it holds no Sony byte and no kernel service.
;
;   armips -strequ IMAGE <file> stub.asm
;
; Reset (PSX-SYSTEM sections 3, 7 and 9): write the memory-control boot values, clear I_MASK and
; I_STAT, put one jump at the general exception vector 80000080h, clear SR.BEV, read the launch
; block the loader wrote, set gp and sp and jump to pc0. Until BEV is clear an exception lands
; at BFC00180h, which holds a jump to the same handler body.
; The scratchpad is never touched and no device that is not built is written.
;
; Kernel RAM, all below 10000h:
;   80000000h-8000007Fh  unused vectors (not written)
;   80000080h-8000008Fh  the general-vector jump, four words
;   800000A0h, 800000B0h, 800000C0h  the kernel call entries (not written here)
;   80000200h-8000020Fh  launch block: pc0, gp0, s_addr, s_size (written by the loader)
;   80000210h-8000021Fh  exception record: Cause, EPC, BadVaddr, SR
; The mailbox is the first 256 bytes of a loaded image at 80010000h: magic "SWMB" at 0, status at 8,
; and at 0Ch the address of a handler the image registers (zero for none).
; Fault prevented: a stub whose reset, vector or handler differs from this description, so a
; fixture measures the stub instead of the CPU.
.psx
.create IMAGE, 0xBFC00000

.definelabel LAUNCH, 0x80000200          ; pc0, gp0, s_addr, s_size
.definelabel EXC_REC, 0x80000210         ; Cause, EPC, BadVaddr, SR
.definelabel VECTOR, 0x80000080          ; the general exception vector with BEV clear
.definelabel MAILBOX, 0x80010000
.definelabel MB_MAGIC_HI, 0x424D         ; "SWMB" read as a little-endian word is 424D5753h
.definelabel MB_MAGIC_LO, 0x5753
.definelabel MB_STATUS, 8
.definelabel MB_HANDLER, 0x0C
.definelabel STACK_DEFAULT, 0x801FFF00   ; sp when the image gives no stack
.definelabel SR_BEV, 0x00400000
.definelabel IO, 0xBF801000              ; the I/O page in KSEG1

.org 0xBFC00000
reset:
  ; Memory control: the values the retail boot code writes (PSX-SYSTEM section 3).
  li t0, IO
  li t1, 0x1F000000
  sw t1, 0x000(t0)                       ; DEV0 base
  li t1, 0x1F802000
  sw t1, 0x004(t0)                       ; DEV8 base
  li t1, 0x0013243F
  sw t1, 0x008(t0)                       ; DEV0 delay and size
  sw t1, 0x010(t0)                       ; DEV2 (BIOS) delay and size
  li t1, 0x00003022
  sw t1, 0x00C(t0)                       ; DEV1 delay and size
  li t1, 0x200931E1
  sw t1, 0x014(t0)                       ; DEV4 (SPU) delay
  li t1, 0x00020843
  sw t1, 0x018(t0)                       ; DEV5 (CD-ROM) delay
  li t1, 0x00070777
  sw t1, 0x01C(t0)                       ; DEV8 delay and size
  li t1, 0x00031125
  sw t1, 0x020(t0)                       ; COM_DELAY
  li t1, 0x00000B88
  sw t1, 0x060(t0)                       ; RAM_SIZE: 2 MB in an 8 MB bank
  sw zero, 0x074(t0)                     ; I_MASK
  sw zero, 0x070(t0)                     ; I_STAT

  ; The vector: copy the four-word jump below to 80000080h, and nothing else.
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

  ; With the vector in RAM, exceptions go there: clear BEV.
  mfc0 t0, sr
  li t1, ~SR_BEV
  and t0, t0, t1
  mtc0 t0, sr

  ; Launch: gp = gp0, sp = s_addr + s_size (the default when s_addr is 0), jump to pc0.
  li t0, LAUNCH
  lw t1, 0(t0)                           ; pc0
  lw gp, 4(t0)
  lw t2, 8(t0)                           ; s_addr
  lw t3, 12(t0)                          ; s_size
  beq t1, zero, spin                     ; no image was loaded: stay here
  nop
  bne t2, zero, enter_image
  addu sp, t2, t3                        ; delay slot: the image's stack
  li sp, STACK_DEFAULT
enter_image:
  jr t1
  nop

; Between the reset code and the BEV vector every byte is FFh, opcode 3Fh, which raises an
; exception, so a runaway that reaches the gap is recorded and never slides through nops.
.fill 0xBFC00180 - org(), 0xFF

; The vector with BEV set: an exception during the reset sequence is recorded like any other.
.org 0xBFC00180
  j handler
  nop

; The general handler body. It uses only k0 and k1. It records Cause, EPC, BadVaddr and SR; then,
; if the mailbox is there, sets its status to FFh and jumps to the handler the image registered.
handler:
  mfc0 k0, cause
  lui k1, EXC_REC >> 16
  sw k0, (EXC_REC & 0xFFFF)(k1)
  mfc0 k0, epc
  nop
  sw k0, (EXC_REC & 0xFFFF) + 4(k1)
  mfc0 k0, badvaddr
  nop
  sw k0, (EXC_REC & 0xFFFF) + 8(k1)
  mfc0 k0, sr
  nop
  sw k0, (EXC_REC & 0xFFFF) + 12(k1)
  lui k1, MAILBOX >> 16
  lw k0, 0(k1)
  nop
  xori k0, k0, MB_MAGIC_LO               ; zero in the low half when the low half matches
  lui k1, MB_MAGIC_HI
  bne k0, k1, spin                       ; not a mailbox
  lui k1, MAILBOX >> 16                  ; delay slot: the mailbox again
  ori k0, zero, 0xFF
  sw k0, MB_STATUS(k1)
  lw k0, MB_HANDLER(k1)
  nop
  beq k0, zero, spin
  nop
  jr k0
  nop

spin:
  j spin
  nop

; What the reset code copies to the vector: a jump to the handler body in ROM.
vector_jump:
  lui k0, handler >> 16
  ori k0, k0, handler & 0xFFFF
  jr k0
  nop
.close
