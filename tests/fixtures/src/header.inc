; The 2048-byte PS-EXE header every fixture emits, then the code origin.
;
;   armips -strequ IMAGE <file> [-definelabel PC0 <addr>] [-definelabel GP0 <value>] <fixture>.asm
;
; A fixture includes this file first, puts its 256-byte mailbox at 80010000h and its code at 80010100h
; (PC0 unless overridden), and ends with `.align 0x800`, the label SW_END and `.close`.
; Fields and offsets are those a loader reads (psx-spx "PSX EXE Format").
; Fault prevented: a header whose size, entry point or load size disagrees with the image,
; so a loader maps the wrong bytes or starts at the wrong address.
.psx
.ifndef PC0
.definelabel PC0, 0x80010100
.endif
.ifndef GP0
.definelabel GP0, 0
.endif
.create IMAGE, 0
.ascii "PS-X EXE"                     ; 00h magic
.fill 8                               ; 08h reserved
.word PC0                             ; 10h pc0
.word GP0                             ; 14h gp0
.word 0x80010000                      ; 18h t_addr
.word SW_END - 0x80010000             ; 1Ch t_size, the image after the header, a multiple of 800h
.word 0, 0, 0, 0                      ; 20h d_addr, d_size, b_addr, b_size
.word 0x801FF000                      ; 30h s_addr
.word 0x1000                          ; 34h s_size
.fill 0x4C - org()                    ; 38h reserved
.ascii "Sony Computer Entertainment Inc. for North America area"   ; 4Ch region text
.fill 0x800 - org()
.headersize 0x80010000 - 0x800        ; the image loads at 80010000h
