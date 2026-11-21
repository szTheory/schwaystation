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
.definelabel STR_BUFFER, 0x80050000
.definelabel MDEC_INPUT_BUFFER, 0x80060000
.definelabel MDEC_INPUT_BYTES, 128
.definelabel MDEC_OUTPUT_BUFFER, 0x80061000
.definelabel HOST_CONTROL, 0x80070000
.definelabel STR_LBA_FRAME_BCD, 0x54
.definelabel STR_SECTOR_COUNT, 10

.org MB_BASE
mailbox_header 21, 0

.org PC0
  li s0, CDROM
  li s1, MB_BASE

  ; Enable the SPU CD input and both output channels at full volume.
  li t0, 0xBF801DB0
  li t1, 0x7FFF
  sh t1, 0(t0)
  sh t1, 2(t0)
  li t0, 0xBF801DAA
  li t1, 0xC001
  sh t1, 0(t0)

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

.if PBP_MODE == 0
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
.else
  ; The owned PBP fixture has one data track and no track two.
  li t0, 0x00010004
  sw t0, MB_RECORDS + 24(s1)
  sw zero, MB_RECORDS + 28(s1)
.endif

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
  ; Pause also queues a delayed INT2; consume it before issuing another command.
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack2
  nop
  sb zero, 0(s0)
  sb zero, 3(s0)

  ; Play three authored XA sectors from LBA 51 through 53 via the CD filter and SPU queue.
  sb zero, 0(s0)
  li t0, 0x48
  sb t0, 2(s0)
  li t0, 0x0E
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  li t0, 7
  sb t0, 2(s0)
  li t0, 3
  sb t0, 2(s0)
  li t0, 0x0D
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  sb zero, 0(s0)
  sb zero, 2(s0)
  li t0, 2
  sb t0, 2(s0)
  li t0, 0x51
  sb t0, 2(s0)
  li t0, 0x02
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  li t0, 0x06
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  ; Leave enough cycles for the seek and the first authored XA packets.
  li t1, 1500000
xa_wait:
  addiu t1, t1, -1
  bne t1, zero, xa_wait
  nop
  sb zero, 0(s0)
  li t0, 0x09
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack2
  nop

.if PBP_MODE == 0
  ; Setloc to the generated track's INDEX 1 (LBA 214), then Play from that point.
  ; This skips its two-second pregap while still exercising CD-DA sector playback.
  sb zero, 0(s0)
  li t0, 1
  sb t0, 2(s0)
  li t0, 0x0E
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  sb zero, 0(s0)
  sb zero, 2(s0)
  li t0, 4
  sb t0, 2(s0)
  li t0, 0x64
  sb t0, 2(s0)
  li t0, 0x02
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  li t0, 0
  sb t0, 2(s0)
  li t0, 0x03
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  ; Let the generated CD-DA tone supply its complete sector sequence.
  li t1, 1210000
cdda_wait:
  addiu t1, t1, -1
  bne t1, zero, cdda_wait
  nop
  sb zero, 0(s0)
  li t0, 0x09
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack2
  nop

  ; CD-DA playback leaves its last eight raw sectors in RDDATA. Drain those
  ; queued sectors before reading STR data, or DMA3 would consume stale audio.
  li t3, 0xBF801000
  li t0, 0x80
  sb t0, 3(s0)
  li t0, STR_BUFFER
  sw t0, 0x0B0(t3)
  li t0, 4704
  sw t0, 0x0B4(t3)
  li t0, 0x11000000
  sw t0, 0x0B8(t3)
cdda_drain_wait:
  lw t0, 0x0B8(t3)
  nop
  lui t1, 0x0100
  and t0, t0, t1
  bne t0, zero, cdda_drain_wait
  nop
  sb zero, 3(s0)
.endif

  ; Read and DMA every STR chunk to RAM before stripping the 20h-byte headers.
  sb zero, 0(s0)
  li t0, 0
  sb t0, 2(s0)
  li t0, 0x0E
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  sb zero, 0(s0)
  sb zero, 2(s0)
  li t0, 2
  sb t0, 2(s0)
  ; MOVIE.STR begins at LBA 54; Setloc takes BCD MSF, including the 150-sector offset.
  li t0, STR_LBA_FRAME_BCD
  sb t0, 2(s0)
  li t0, 0x02
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  li t0, 0x80
  sb t0, 3(s0)
  li t0, 0x06
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  li t3, 0xBF801000
  move s7, zero
str_read_sector:
  jal cd_wait
  nop
  li t0, 1
  bne v0, t0, str_read_sector
  nop
  li t0, STR_BUFFER
  sll t1, s7, 11
  addu t0, t0, t1
  sw t0, 0x0B0(t3)
  li t0, 512
  sw t0, 0x0B4(t3)
  li t0, 0x11000000
  sw t0, 0x0B8(t3)
str_dma_wait:
  lw t0, 0x0B8(t3)
  nop
  lui t1, 0x0100
  and t0, t0, t1
  bne t0, zero, str_dma_wait
  nop
  jal cd_ack1
  nop
  addiu s7, s7, 1
  slti t0, s7, STR_SECTOR_COUNT
  bne t0, zero, str_read_sector
  nop

  ; Stop ReadN and consume both responses before decoding the captured frame.
  sb zero, 0(s0)
  li t0, 0x09
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack2
  nop
  sb zero, 0(s0)
  sb zero, 3(s0)

  ; Demux the frame size from the first STR header and copy its declared stream.
  ; The authored bitstream fits one 32-word input block.
  li s2, STR_BUFFER
  lw s4, 12(s2)
  nop
  beq s4, zero, str_invalid
  nop
  li t0, MDEC_INPUT_BYTES
  sltu t1, t0, s4
  bne t1, zero, str_invalid
  nop
  andi t1, s4, 3
  bne t1, zero, str_invalid
  nop
  addiu s3, s2, 32
  li s2, MDEC_INPUT_BUFFER
  srl s5, s4, 2
str_copy_words:
  lw t0, 0(s3)
  addiu s3, s3, 4
  sw t0, 0(s2)
  addiu s2, s2, 4
  addiu s5, s5, -1
  bne s5, zero, str_copy_words
  nop
  li t0, 0xFE00FE00
  li t1, MDEC_INPUT_BUFFER + MDEC_INPUT_BYTES
mdec_input_pad:
  sw t0, 0(s2)
  addiu s2, s2, 4
  bne s2, t1, mdec_input_pad
  nop

  ; Decode one 16x16 RGB24 macroblock through DMA0 and DMA1.
  li t0, 0xBF801824
  li t1, 0x60000000
  sw t1, 0(t0)
  li t0, 0xBF801820
  li t1, 0x38000020
  sw t1, 0(t0)
  li t3, 0xBF801000
  li t0, MDEC_OUTPUT_BUFFER
  sw t0, 0x090(t3)
  li t0, 0x000100C0
  sw t0, 0x094(t3)
  li t0, 0x01000200
  sw t0, 0x098(t3)
  li t0, MDEC_INPUT_BUFFER
  sw t0, 0x080(t3)
  li t0, 0x00010020
  sw t0, 0x084(t3)
  li t0, 0x01000201
  sw t0, 0x088(t3)
mdec_dma_wait:
  lw t0, 0x088(t3)
  lw t1, 0x098(t3)
  nop
  lui t2, 0x0100
  and t0, t0, t2
  and t1, t1, t2
  or t0, t0, t1
  bne t0, zero, mdec_dma_wait
  nop

  ; Set a 16x16 native RGB24 display and upload the decoded pixels via DMA2.
  li t0, 0x04000002
  sw t0, 0x814(t3)
  li t0, 0x05000000
  sw t0, 0x814(t3)
  li t0, 0x06080000
  sw t0, 0x814(t3)
  li t0, 0x07004000
  sw t0, 0x814(t3)
  li t0, 0x08000011
  sw t0, 0x814(t3)
  li t0, 0x03000000
  sw t0, 0x814(t3)
  li t0, 0xA0000000
  sw t0, 0x810(t3)
  sw zero, 0x810(t3)
  li t0, 0x00100018
  sw t0, 0x810(t3)
  li t0, MDEC_OUTPUT_BUFFER
  sw t0, 0x0A0(t3)
  li t0, 0x00C00001
  sw t0, 0x0A4(t3)
  li t0, 0x01000201
  sw t0, 0x0A8(t3)
gpu_dma_wait:
  lw t0, 0x0A8(t3)
  nop
  lui t1, 0x0100
  and t0, t0, t1
  bne t0, zero, gpu_dma_wait
  nop

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

  ; The host integration can open the tray after this mailbox completes.
  ; Record the actual INT5 response and re-query the live TOC into unused RAM.
  li t0, 1
  sb t0, 0(s0)
control_wait_int5:
  lbu t0, 3(s0)
  nop
  andi t0, t0, 7
  li t1, 5
  bne t0, t1, control_wait_int5
  nop
  li t0, HOST_CONTROL
  sw t1, 0(t0)
  sb zero, 0(s0)
  lbu t2, 1(s0)
  lbu t2, 1(s0)
  li v0, 5
  jal cd_ack
  nop

  ; A closed tray's next NOP response clears the lid-open status bit.
  sb zero, 0(s0)
  li t0, 0x01
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop

  ; GetTN records the track range after the tray closes.
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
  or t0, t0, t1
  or t0, t0, t2
  li t1, HOST_CONTROL
  sw t0, 4(t1)
  jal cd_ack3
  nop

  ; GetTD for track two records the changed playlist member's INDEX 1.
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
  or t0, t0, t1
  or t0, t0, t2
  li t1, HOST_CONTROL
  sw t0, 8(t1)
  jal cd_ack3
  nop

  ; The host can open the tray a second time and remove the selected image.
  li t0, 1
  sb t0, 0(s0)
control_wait_remove_int5:
  lbu t0, 3(s0)
  nop
  andi t0, t0, 7
  li t1, 5
  bne t0, t1, control_wait_remove_int5
  nop
  li t0, HOST_CONTROL
  sw t1, 12(t0)
  sb zero, 0(s0)
  lbu t2, 1(s0)
  lbu t2, 1(s0)
  li v0, 5
  jal cd_ack
  nop

  ; Once the host closes an empty tray, GetTN returns INT5/error 80h.
  sb zero, 0(s0)
  li t0, 0x01
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  jal cd_ack3
  nop
  sb zero, 0(s0)
  li t0, 0x13
  sb t0, 1(s0)
  jal cd_wait
  nop
  lbu t0, 1(s0)
  lbu t1, 1(s0)
  nop
  sll t1, t1, 8
  or t0, t0, t1
  li t1, HOST_CONTROL
  sw t0, 16(t1)
  jal cd_ack3
  nop
control_hang:
  j control_hang
  nop

str_invalid:
  li t0, 0xFF
  sb t0, MB_STATUS(s1)
str_invalid_hang:
  j str_invalid_hang
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
