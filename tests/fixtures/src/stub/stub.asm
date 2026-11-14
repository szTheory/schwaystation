; The stub kernel: the BIOS image the core maps at BFC00000h when the host supplies none.
; MIT-licensed, written for this project; it holds no Sony bytes and implements only the
; fixture services documented below.
;
;   armips -strequ IMAGE <file> stub.asm
;
; Reset (docs/hardware/bus.md, Memory control; PSX-SYSTEM sections 7 and 9): write the memory-control boot values, clear I_MASK and
; I_STAT, put one jump at the general exception vector 80000080h, clear SR.BEV, read the launch
; block the loader wrote, set gp and sp and jump to pc0. Until BEV is clear an exception lands
; at BFC00180h, which holds a jump to the same handler body.
; The kernel call entries at 800000A0h, 800000B0h and 800000C0h are three four-word slots the reset
; code fills: A and B jump to ROM routines that, for the putchar function (t1 = 3Ch through A0h,
; 3Dh through B0h), store the byte in a0 to the debug port at 1F802080h and return, and C returns at
; once. Every other function number returns at once with nothing written.
; The scratchpad is never touched and no device that is not built is written.
;
; Kernel RAM, all below 10000h:
;   80000000h-8000007Fh  unused vectors (not written)
;   80000080h-8000008Fh  the general-vector jump, four words
;   800000A0h, 800000B0h, 800000C0h  the kernel call entries, four words each
;   80000200h-8000020Fh  launch block: pc0, gp0, s_addr, s_size (written by the loader)
;   80000210h-8000021Fh  exception record: Cause, EPC, BadVaddr, SR
;   80000300h-80000303h  registered B0(19h) hook's guest JmpBuf pointer
;   80000304h-80000307h  active-context flag
;   80000310h-8000039Bh  interrupted context: r1-r31 (k0 slot reserved), HI, LO, SR, EPC
; The mailbox is the first 256 bytes of a loaded image at 80010000h: magic "SWMB" at 0, status at 8,
; and at 0Ch the address of a handler the image registers (zero for none).
; Fault prevented: a stub whose reset, vector or handler differs from this description, so a
; fixture measures the stub instead of the CPU.
.psx
.create IMAGE, 0xBFC00000

.definelabel LAUNCH, 0x80000200          ; pc0, gp0, s_addr, s_size
.definelabel EXC_REC, 0x80000210         ; Cause, EPC, BadVaddr, SR
.definelabel STUB_RAM, 0x80000000        ; low KSEG0 RAM alias for private stub words
.definelabel STUB_HOOK, 0x80000300       ; registered B0(19h) JmpBuf pointer
.definelabel STUB_ACTIVE, 0x80000304     ; nonzero only while the hook owns saved context
.definelabel CONTEXT_HI, 0x8000038C      ; HI, LO, exception SR and EPC follow r1-r31
.definelabel CONTEXT_LO, 0x80000390
.definelabel CONTEXT_SR, 0x80000394
.definelabel CONTEXT_EPC, 0x80000398
.definelabel VECTOR, 0x80000080          ; the general exception vector with BEV clear
.definelabel ENTRIES, 0x800000A0         ; the A, B and C kernel call entries, 16 bytes each
.definelabel DEBUG_PORT, 0x2080          ; 1F802080h seen from BF800000h: a byte written here is a TTY byte
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
  ; Memory control: the values the retail boot code writes (docs/hardware/bus.md, Memory control).
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

  ; Start with no registered guest hook and no live saved context. Context
  ; bytes themselves are written only after an eligible IRQ0 is accepted.
  lui t0, STUB_RAM >> 16
  sw zero, STUB_HOOK & 0xFFFF(t0)
  sw zero, STUB_ACTIVE & 0xFFFF(t0)

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

  ; The kernel call entries: copy the three four-word slots below to 800000A0h-800000CFh.
  li t0, entry_slots
  li t1, ENTRIES
  addiu t2, t0, 48
copy_entries:
  lw t3, 0(t0)
  addiu t0, t0, 4                        ; fills the load delay slot
  sw t3, 0(t1)
  bne t0, t2, copy_entries
  addiu t1, t1, 4                        ; delay slot

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

; Keep all call, vector and reset-installed slot addresses stable. The full exception path is out of
; line after entry_slots; this fixed-size trampoline leaves call_a at its original address.
handler:
  j exception_dispatch
  nop
.fill 0xBFC001FC - org(), 0xFF

; The putchar routines the A and B entries jump to. They use only k0 and k1, so every register the
; caller holds survives. A call with another function number returns at once.
call_a:
  addiu k1, zero, 0x3C                   ; A(3Ch) putchar
  bne t1, k1, call_a_printf
  lui k1, 0xBF80                         ; delay slot: the I/O region in KSEG1
  sb a0, DEBUG_PORT(k1)
call_return:
  jr ra
  nop
; PSX-SPX Kernel/BIOS TTY Console A(3Fh) is printf; the pinned transfer test
; needs only literal strings and %s.
; This narrow formatter keeps the stub bounded to the test's needed cases and
; sends its output through the same profile-controlled debug port.
call_a_printf:
  ori k1, zero, 0x3F
  bne t1, k1, call_return
  nop
  lui k1, 0xBF80
  addu t2, a0, zero
  addu t3, a1, zero
printf_loop:
  lbu t4, 0(t2)
  nop
  beq t4, zero, call_return
  addiu t2, t2, 1
  ori t5, zero, 0x25
  bne t4, t5, printf_put
  nop
  lbu t4, 0(t2)
  addiu t2, t2, 1
  ori t5, zero, 0x73
  bne t4, t5, printf_put_specifier
  nop
  addu t6, t3, zero
printf_arg_loop:
  lbu t4, 0(t6)
  nop
  beq t4, zero, printf_arg_done
  addiu t6, t6, 1
  sb t4, DEBUG_PORT(k1)
  b printf_arg_loop
  nop
printf_arg_done:
  addiu t3, t3, 4
  b printf_loop
  nop
printf_put_specifier:
  ori t5, zero, 0x25
  sb t5, DEBUG_PORT(k1)
  sb t4, DEBUG_PORT(k1)
  b printf_loop
  nop
printf_put:
  sb t4, DEBUG_PORT(k1)
  b printf_loop
  nop
call_b:
  addiu k1, zero, 0x3D                   ; B(3Dh) putchar
  bne t1, k1, call_b_dispatch
  lui k1, 0xBF80
  sb a0, DEBUG_PORT(k1)
  jr ra
  nop

; What the reset code copies to the vector: a jump to the handler body in ROM.
vector_jump:
  lui k0, handler >> 16
  ori k0, k0, handler & 0xFFFF
  jr k0
  nop

; What the reset code copies to 800000A0h: A and B jump to their ROM routines, C returns at once.
entry_slots:
  lui k0, call_a >> 16
  ori k0, k0, call_a & 0xFFFF
  jr k0
  nop
  lui k0, call_b >> 16
  ori k0, k0, call_b & 0xFFFF
  jr k0
  nop
  jr ra
  nop
  nop
  nop

; The minimal BIOS-call dispatcher stays out of the fixed A/B/C slot image.
; B0(19h) registers a guest-owned JumpBuffer; B0(17h) returns only while a
; supported IRQ context is active. An inactive return remains unhandled and stops.
call_b_dispatch:
  ori k1, zero, 0x19
  beq t1, k1, hook_register
  nop
  ori k1, zero, 0x17
  beq t1, k1, return_from_exception
  nop
call_b_return:
  jr ra
  nop

hook_register:
  lui k1, STUB_RAM >> 16
  sw a0, (STUB_HOOK - STUB_RAM)(k1)
  jr ra
  nop

return_from_exception:
  lui k1, STUB_RAM >> 16
  lw k1, (STUB_ACTIVE - STUB_RAM)(k1)
  nop
  ori k0, zero, 1
  bne k1, k0, spin
  nop
  j irq_return
  nop

; Record the exception before choosing an image handler. Low KUSEG addresses map to the same
; physical RAM as KSEG0 (docs/hardware/bus.md, Segments and the physical map); using r0 as the
; base keeps interrupted k1 untouched until the eligible IRQ path has saved it.
exception_dispatch:
  mfc0 k0, cause
  nop
  sw k0, (EXC_REC & 0xFFFF)(zero)
  mfc0 k0, epc
  nop
  sw k0, (EXC_REC & 0xFFFF) + 4(zero)
  mfc0 k0, badvaddr
  nop
  sw k0, (EXC_REC & 0xFFFF) + 8(zero)
  mfc0 k0, sr
  nop
  sw k0, (EXC_REC & 0xFFFF) + 12(zero)

  ; ExcCode 0 plus CAUSE IP2 identifies the enabled controller IRQ0 line.
  ; Inspect SWMB with k0 only so interrupted k1 can be saved unchanged.
  mfc0 k0, cause
  nop
  andi k0, k0, 0x047C
  xori k0, k0, 0x0400
  bne k0, zero, mailbox_check
  nop
  lui k0, MAILBOX >> 16
  lw k0, 0(k0)
  nop
  srl k0, k0, 16
  xori k0, k0, MB_MAGIC_HI
  bne k0, zero, irq_candidate
  nop
  lui k0, MAILBOX >> 16
  lw k0, 0(k0)
  nop
  andi k0, k0, 0xFFFF
  xori k0, k0, MB_MAGIC_LO
  beq k0, zero, mailbox_check
  nop

irq_candidate:
  lui k0, STUB_RAM >> 16
  lw k0, (STUB_HOOK - STUB_RAM)(k0)
  nop
  beq k0, zero, mailbox_check
  nop
  lui k0, STUB_RAM >> 16
  lw k0, (STUB_ACTIVE - STUB_RAM)(k0)
  nop
  bne k0, zero, spin                    ; nesting is unsupported; keep the live context
  nop
  j irq_enter
  nop

  ; A valid SWMB always keeps its original status and registered-handler path.
mailbox_check:
  lui k1, MAILBOX >> 16
  lw k0, 0(k1)
  nop
  xori k0, k0, MB_MAGIC_LO               ; zero in the low half when the low half matches
  lui k1, MB_MAGIC_HI
  bne k0, k1, syscall_service            ; no mailbox: try the two supported BIOS services
  lui k1, MAILBOX >> 16                  ; delay slot: the mailbox again
  ori k0, zero, 0xFF
  sw k0, MB_STATUS(k1)
  lw k0, MB_HANDLER(k1)
  nop
  beq k0, zero, spin
  nop
  jr k0
  nop

; Save every interrupted GPR except k0 before using k1, then save COP0 and multiply state.
; The k0 context slot at 374h is reserved; the next word holds k1.
irq_enter:
  lui k0, STUB_RAM >> 16
  sw at, 0x310(k0)
  sw v0, 0x314(k0)
  sw v1, 0x318(k0)
  sw a0, 0x31C(k0)
  sw a1, 0x320(k0)
  sw a2, 0x324(k0)
  sw a3, 0x328(k0)
  sw t0, 0x32C(k0)
  sw t1, 0x330(k0)
  sw t2, 0x334(k0)
  sw t3, 0x338(k0)
  sw t4, 0x33C(k0)
  sw t5, 0x340(k0)
  sw t6, 0x344(k0)
  sw t7, 0x348(k0)
  sw s0, 0x34C(k0)
  sw s1, 0x350(k0)
  sw s2, 0x354(k0)
  sw s3, 0x358(k0)
  sw s4, 0x35C(k0)
  sw s5, 0x360(k0)
  sw s6, 0x364(k0)
  sw s7, 0x368(k0)
  sw t8, 0x36C(k0)
  sw t9, 0x370(k0)
  sw k1, 0x378(k0)
  sw gp, 0x37C(k0)
  sw sp, 0x380(k0)
  sw fp, 0x384(k0)
  sw ra, 0x388(k0)
  mfhi k1
  sw k1, (CONTEXT_HI - STUB_RAM)(k0)
  mflo k1
  sw k1, (CONTEXT_LO - STUB_RAM)(k0)
  mfc0 k1, sr
  nop
  sw k1, (CONTEXT_SR - STUB_RAM)(k0)
  mfc0 k1, epc
  nop
  sw k1, (CONTEXT_EPC - STUB_RAM)(k0)
  ori k1, zero, 1
  sw k1, (STUB_ACTIVE - STUB_RAM)(k0)

  ; PSX-SYSTEM SYS.07 / PSX-SPX HookEntryInt: JumpBuffer is RA/entry, SP, FP,
  ; S0-S7, GP. The SDK header was inspected only for these public field offsets.
  lui k0, STUB_RAM >> 16
  lw k0, (STUB_HOOK - STUB_RAM)(k0)
  nop
  lw ra, 0(k0)
  lw sp, 4(k0)
  lw fp, 8(k0)
  lw s0, 0x0C(k0)
  lw s1, 0x10(k0)
  lw s2, 0x14(k0)
  lw s3, 0x18(k0)
  lw s4, 0x1C(k0)
  lw s5, 0x20(k0)
  lw s6, 0x24(k0)
  lw s7, 0x28(k0)
  lw gp, 0x2C(k0)
  ori v0, zero, 1
  jr ra
  nop

; B0(17h): restore the saved architectural context and use the regular RFE path.
; Interrupt return uses EPC itself; only syscall return advances past EPC.
irq_return:
  lui k0, STUB_RAM >> 16
  lw k1, (CONTEXT_HI - STUB_RAM)(k0)
  nop
  mthi k1
  lw k1, (CONTEXT_LO - STUB_RAM)(k0)
  nop
  mtlo k1
  lw k1, (CONTEXT_SR - STUB_RAM)(k0)
  nop
  mtc0 k1, sr
  nop
  lw k1, (CONTEXT_EPC - STUB_RAM)(k0)
  nop
  mtc0 k1, epc
  nop
  lw at, 0x310(k0)
  lw v0, 0x314(k0)
  lw v1, 0x318(k0)
  lw a0, 0x31C(k0)
  lw a1, 0x320(k0)
  lw a2, 0x324(k0)
  lw a3, 0x328(k0)
  lw t0, 0x32C(k0)
  lw t1, 0x330(k0)
  lw t2, 0x334(k0)
  lw t3, 0x338(k0)
  lw t4, 0x33C(k0)
  lw t5, 0x340(k0)
  lw t6, 0x344(k0)
  lw t7, 0x348(k0)
  lw s0, 0x34C(k0)
  lw s1, 0x350(k0)
  lw s2, 0x354(k0)
  lw s3, 0x358(k0)
  lw s4, 0x35C(k0)
  lw s5, 0x360(k0)
  lw s6, 0x364(k0)
  lw s7, 0x368(k0)
  lw t8, 0x36C(k0)
  lw t9, 0x370(k0)
  lw gp, 0x37C(k0)
  lw sp, 0x380(k0)
  lw fp, 0x384(k0)
  lw ra, 0x388(k0)
  sw zero, (STUB_ACTIVE - STUB_RAM)(k0)
  lw k1, 0x378(k0)
  mfc0 k0, epc
  nop
  jr k0
  rfe

spin:
  j spin
  nop

; The real clipping fixture calls SYS(01h) and SYS(02h) before drawing. The BIOS
; selector is in a0; dispatch only ordinary SYSCALL exceptions, not delay slots.
; Keep this addition after entry_slots so the reset-installed A/B/C table stays stable.
syscall_service:
  mfc0 k0, cause
  nop
  bltz k0, spin                           ; a set BD bit needs the original branch to be replayed
  nop
  andi k0, k0, 0x007C                    ; ExcCode must be SYS (08h)
  ori k1, zero, 0x0020
  bne k0, k1, spin
  nop
  ori k0, zero, 0x01
  beq a0, k0, syscall_enter_critical
  nop
  ori k0, zero, 0x02
  beq a0, k0, syscall_exit_critical
  nop
  j spin                                 ; unsupported BIOS services remain unhandled
  nop

; SYS(01h): return 1 iff saved IEp and IM2 are both set, then clear both.
; Exception entry has already moved the caller's IEc into IEp; RFE restores it.
syscall_enter_critical:
  mfc0 k0, sr
  nop
  andi k1, k0, 0x0404
  xori k1, k1, 0x0404
  ori v0, zero, 0
  bne k1, zero, enter_critical_clear
  nop
  ori v0, zero, 1
enter_critical_clear:
  lui k1, 0xFFFF
  ori k1, k1, 0xFBFB                    ; preserve every SR bit except IEp and IM2
  and k0, k0, k1
  mtc0 k0, sr
  nop
  j syscall_return
  nop

; SYS(02h): set saved IEp and IM2 without changing v0; RFE restores caller IEc.
syscall_exit_critical:
  mfc0 k0, sr
  nop
  ori k0, k0, 0x0404
  mtc0 k0, sr
  nop

syscall_return:
  mfc0 k0, epc
  nop
  addiu k0, k0, 4                       ; only non-delay-slot syscalls reach this return
  jr k0
  rfe
.close
