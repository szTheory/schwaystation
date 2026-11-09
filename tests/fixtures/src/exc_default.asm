; exc_default.exe: executes BREAK and leaves the stub kernel's exception handler to deal with it.
;
;   armips -strequ IMAGE <file> [-definelabel USE_HANDLER 1] exc_default.asm
;
; Without USE_HANDLER the mailbox's handler word is 0: the stub records Cause, EPC, BadVaddr and SR at
; 80000210h, sets the mailbox status to FFh and stops in its own loop.
; With USE_HANDLER 1 the word holds the address of the handler below: the stub does the same and then
; jumps to it, and the handler stores Cause as record 1, seals the mailbox and sets status 00h.
; Fault prevented: a stub whose default handler records the wrong registers, or does not hand the
; exception to a handler an image registered.
.ifndef USE_HANDLER
.definelabel USE_HANDLER, 0
.endif
.include "header.inc"
.include "mailbox.inc"

.org MB_BASE
.if USE_HANDLER
mailbox_header 2, handler
.else
mailbox_header 2, 0
.endif

.org PC0
trigger:
  break

.if USE_HANDLER
handler:
  mfc0 k0, cause
  li k1, MB_BASE
  sw k0, MB_RECORDS + 4(k1)
  li k0, 0x00010001                     ; id 1, flags: valid
  sw k0, MB_RECORDS(k1)
  li k0, 0x02010100                     ; the stub's FFh at 08h cleared version, count and id: status 00h,
  sw k0, MB_STATUS(k1)                  ; version 1, count 1, id 2
  mailbox_seal
done:
  j done
  nop
.endif

.align 0x800
SW_END:
.close
