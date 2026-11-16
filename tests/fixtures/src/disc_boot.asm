; Booted from SYSTEM.CNF under OpenBIOS, this guest records controller TOC/GetID
; answers and transfers sixteen Mode2 sectors with CD-ROM DMA3 before sealing.
; Sector bytes remain in the RAM window so the host fixture can check them independently.
.include "header.inc"
.include "mailbox.inc"

.definelabel CDROM, 0xBF801800
.definelabel DPCR, 0xBF8010F0
.definelabel DMA3_MADR, 0xBF8010B0
.definelabel DMA3_BCR, 0xBF8010B4
.definelabel DMA3_CHCR, 0xBF8010B8
.definelabel DATA_BUFFER, 0x80040000
.definelabel DATA_BYTES, 32768

.org MB_BASE
mailbox_header 21, 0

.org PC0
  li s0, CDROM
  li s1, MB_BASE

  ; This first record proves the guest actually ran after the BIOS handoff.
  li t0, 0x00010001
  sw t0, MB_RECORDS(s1)
  li t0, 0x424F4F54
  sw t0, MB_RECORDS + 4(s1)
  li t0, 1
  sb t0, MB_COUNT(s1)

  ; GetTN returns status, first track and last track.
  sb zero, 0(s0)
  li t0, 0x13
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  lbu t1, 1(s0)
  lbu t2, 1(s0)
  sll t1, t1, 8
  sll t2, t2, 16
  or s2, t0, t1
  or s2, s2, t2
  jal cd_ack3
  nop
  li t0, 0x00010002
  sw t0, MB_RECORDS + 8(s1)
  sw s2, MB_RECORDS + 12(s1)

  ; GetTD for track one.
  sb zero, 0(s0)
  li t0, 1
  sb t0, 2(s0)
  li t0, 0x14
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  lbu t1, 1(s0)
  lbu t2, 1(s0)
  sll t1, t1, 8
  sll t2, t2, 16
  or s3, t0, t1
  or s3, s3, t2
  jal cd_ack3
  nop
  li t0, 0x00010003
  sw t0, MB_RECORDS + 16(s1)
  sw s3, MB_RECORDS + 20(s1)

  ; GetTD for track two.
  sb zero, 0(s0)
  li t0, 2
  sb t0, 2(s0)
  li t0, 0x14
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  lbu t1, 1(s0)
  lbu t2, 1(s0)
  sll t1, t1, 8
  sll t2, t2, 16
  or s4, t0, t1
  or s4, s4, t2
  jal cd_ack3
  nop
  li t0, 0x00010004
  sw t0, MB_RECORDS + 24(s1)
  sw s4, MB_RECORDS + 28(s1)

  ; GetID has an INT3 acknowledgement and a delayed INT2 answer.
  sb zero, 0(s0)
  li t0, 0x1A
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  jal cd_wait
  nop
  li t2, 0
  move s5, zero
id_first_word:
  lbu t0, 1(s0)
  sll t1, t2, 3
  sllv t0, t0, t1
  or s5, s5, t0
  addiu t2, t2, 1
  slti t1, t2, 4
  bne t1, zero, id_first_word
  nop
  li t2, 0
  move s6, zero
id_second_word:
  lbu t0, 1(s0)
  sll t1, t2, 3
  sllv t0, t0, t1
  or s6, s6, t0
  addiu t2, t2, 1
  slti t1, t2, 4
  bne t1, zero, id_second_word
  nop
  jal cd_ack2
  nop
  li t0, 0x00010005
  sw t0, MB_RECORDS + 32(s1)
  sw s5, MB_RECORDS + 36(s1)
  li t0, 0x00010006
  sw t0, MB_RECORDS + 40(s1)
  sw s6, MB_RECORDS + 44(s1)

  ; Set the 2048-byte data window, then Setloc to LBA 32 (00:02:32).
  sb zero, 0(s0)
  sb zero, 2(s0)
  li t0, 0x0E
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  sb zero, 0(s0)
  li t0, 0x00
  sb t0, 2(s0)
  li t0, 0x02
  sb t0, 2(s0)
  li t0, 0x32
  sb t0, 2(s0)
  li t0, 0x02
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  ; Enable DMA3 and CD-ROM data access. Each sector is one 512-word DMA block.
  li t3, 0xBF801000
  li t0, 0x0FEDCBA9
  sw t0, 0x0F0(t3)
  sb zero, 0(s0)
  li t0, 0x80
  sb t0, 3(s0)
  sb zero, 0(s0)
  li t0, 0x06
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  li s7, 0
read_sector:
  jal cd_wait
  nop
  ; ReadN reports INT1 once each sector is available.
  li t0, 1
  beq v0, t0, sector_ready
  nop
  jal cd_ack
  nop
  b read_sector
  nop
sector_ready:
  li t0, DATA_BUFFER
  sll t1, s7, 11
  addu t0, t0, t1
  sw t0, 0x0B0(t3)
  li t0, 512
  sw t0, 0x0B4(t3)
  li t0, 0x11000000
  sw t0, 0x0B8(t3)
dma_wait:
  lw t0, 0x0B8(t3)
  nop
  lui t1, 0x0100
  and t0, t0, t1
  bne t0, zero, dma_wait
  nop
  jal cd_ack1
  nop
  addiu s7, s7, 1
  slti t0, s7, 16
  bne t0, zero, read_sector
  nop

  ; Stop the stream before a seventeenth sector can be delivered.
  sb zero, 0(s0)
  li t0, 0x09
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  sb zero, 0(s0)
  sb zero, 3(s0)

  ; Guest CRC-32/IEEE over the 32 KiB DMA result. The host recomputes this
  ; from the generated image and also compares every transferred byte.
  li s2, DATA_BUFFER
  li s3, DATA_BUFFER + DATA_BYTES
  li s4, 0xFFFFFFFF
  li s5, 0xEDB88320
crc_byte:
  lbu t0, 0(s2)
  addiu s2, s2, 1
  xor s4, s4, t0
  li t1, 8
crc_bit:
  andi t2, s4, 1
  srl s4, s4, 1
  beq t2, zero, crc_next
  nop
  xor s4, s4, s5
crc_next:
  addiu t1, t1, -1
  bne t1, zero, crc_bit
  nop
  bne s2, s3, crc_byte
  nop
  nor s4, s4, zero

  ; Append the transferred sector count and guest CRC-32/IEEE. Records one
  ; through six retain the independently checked TOC and GetID responses.
  li t0, 0x00010007
  sw t0, MB_RECORDS + 48(s1)
  li t0, 16
  sw t0, MB_RECORDS + 52(s1)
  li t0, 0x00010008
  sw t0, MB_RECORDS + 56(s1)
  sw s4, MB_RECORDS + 60(s1)
  li t0, 8
  sb t0, MB_COUNT(s1)
  mailbox_seal
hang:
  j hang
  nop

; Poll the current controller interrupt in index bank one. The host frame cap
; turns a missing response into a failed fixture instead of an unbounded guest.
cd_wait:
  li t0, 1
  sb t0, 0(s0)
cd_wait_loop:
  lbu t0, 3(s0)
  nop
  andi t0, t0, 7
  beq t0, zero, cd_wait_loop
  nop
  move v0, t0
  jr ra
  nop

cd_ack3:
  li v0, 3
  b cd_ack
  nop
cd_ack2:
  li v0, 2
  b cd_ack
  nop
cd_ack1:
  li v0, 1
cd_ack:
  li t2, 1
  sb t2, 0(s0)
  addiu t0, v0, -1
  li t1, 1
  sllv t1, t1, t0
  sb t1, 3(s0)
  sb zero, 0(s0)
  jr ra
  nop

; The OpenBIOS loader reads the executable in 16-sector chunks. Keep its last
; read-ahead sector inside the data track before the following CD-DA pregap.
.fill 0xC000

.align 0x800
SW_END:
.close
