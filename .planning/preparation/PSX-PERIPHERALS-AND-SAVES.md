---
title: "PlayStation controllers, memory cards, saves, game identity, patches and parser bounds"
summary: "SIO0 register and /ACK protocol, every pad and gun with its ID bytes and config commands, the 128 KB memory card layout and command set, card file formats and the libretro .srm conventions, save-state constraints, SYSTEM.CNF and rcheevos identity, PPF and GameShark handling, hostile-input bounds and the milestone placement for schwaystation."
read_when: "Before designing or changing the SIO0 module, any controller or memory-card device, the save export, the save-state visitor, the disc identifier, the runner's patch or cheat path, or the loader fuzz targets."
updated: 2026-10-19
---
# PlayStation peripherals and saves (research PER)

| Question | Section |
|---|---|
| Which SIO0 registers and bits exist, what baud and clock polarity do devices need? | 1.1 |
| How does a byte transfer work, when does /ACK fire, when does IRQ7 fire? | 1.2–1.3 |
| Which ID bytes does each device return; what are the config-mode commands? | 2.1–2.3 |
| How do rumble, the multitap, mouse, neGcon, Jogcon, guns and the PocketStation work? | 2.4–2.8 |
| Which peripherals ship in v1.0? | 2.9 |
| What is the memory card layout, checksum and command set? | 3.1–3.3 |
| Which card file formats exist; how do the libretro cores name cards? | 3.4–3.5 |
| How does the core export cards? | 3.6 |
| How big is a save state; what must v1 respect? | 4 |
| How is a disc identified; which bytes does rcheevos hash? | 5 |
| PPF, cheats, what the core exposes to hackers | 6 |
| Which parsers face untrusted bytes and with what bounds? | 7 |
| Which milestone gets each feature? | 8 |

## 1. SIO0: the controller and memory-card port
### 1.1 Registers (SIO0 at 1F801040h; SIO1 is the same block at +10h) [PER.02]
| Address | Name | Bits that matter |
|---|---|---|
| 1F801040h | JOY_TX_DATA (W) / JOY_RX_DATA (R) | W: b0–7 byte to send; a write starts a transfer when TXEN (current or latched at the write) is set. R: b0–7 oldest RX FIFO entry, b8–31 preview of entries 2–4. 8 deep RX FIFO; a 32-bit read pops four entries, a 16-bit read pops one. Empty FIFO returns the last byte then 00h. Access as 8-bit |
| 1F801044h | JOY_STAT (R) | b0 TX FIFO not full, b1 RX FIFO not empty, b2 TX idle, b3 parity error (sticky), b7 /ACK input level (1 = /ACK low), b9 IRQ (sticky), b11–31 baud timer. b4–6 and b8 read 0 on SIO0 |
| 1F801048h | JOY_MODE | b0–1 baud factor (0 or 1 = MUL1, 2 = MUL16, 3 = MUL64), b2–3 char length (3 = 8 bits), b4 parity enable, b5 parity type, b8 clock polarity (0 = high when idle). Devices need 8 bits, no parity, idle-high |
| 1F80104Ah | JOY_CTRL | b0 TXEN, b1 /CS (select) output, b2 RXEN (0 = receive only while /CS low; 1 = force one byte then auto-clear), b4 acknowledge (clears STAT b3, b9), b6 reset, b8–9 RX IRQ depth (1/2/4/8), b10 TX IRQ, b11 RX IRQ, b12 /ACK IRQ enable, **b13 slot select (0 = port 1, 1 = port 2)**. /CS low (b1 = 1) goes to the port b13 names; every other signal is wired to both ports |
| 1F80104Eh | JOY_BAUD | 16-bit reload. Bit rate = 33,868,800 / max((reload × factor) AND NOT 1, 1). Kernel value 0088h: 44h cycles high, 44h low, about 250 kHz; one byte is 8 × 136 = 1,088 CPU cycles |

The kernel's pad driver uses only the /ACK (DSR) interrupt [PER.02]. I_STAT bit 7 is edge-triggered; JOY_STAT b9 is level, so the kernel acknowledges by clearing I_STAT first then setting CTRL b4, and emulators that trigger IRQ7 in zero cycles after the TX write break the kernel, which waits about 100 cycles after sending before acknowledging the old IRQ7 [PER.01]. The ares model computes the transfer length from the baud registers (reload × factor × 8 cycles), then adds a device-specific /ACK delay: 338 cycles (about 10 µs) for a pad and 170 cycles (about 5 µs) for a card, measured from the end of the byte [PER.08].

### 1.2 Byte handshake [PER.01]
SPI-like: LSB first, data out on the falling clock edge, sampled on the rising edge, one byte per direction per transfer. Byte 1 is the address: 01h controller, 81h memory card, 21h Yaroze Access Card (and the PS2 multitap), 61h PS2 DVD remote. A device replies HiZ (FFh) to the address, then its ID low byte, then 5Ah, then payload. /ACK must be held low at least 2 µs; the kernel times out if /ACK does not arrive within 100 µs of the last clock and ignores /ACK in the first 2–3 µs (about 100 cycles) after it. The last byte of a transaction produces no /ACK and therefore no IRQ7. A Sony mouse powers up with /ACK held low until it receives one 01h byte, which blocks the shared data line for every device on both ports; games that only poll port 1 can hang on a port-2 mouse [PER.01].

### 1.3 Timing the core must reproduce (recommendation)
| Event | Model | Source |
|---|---|---|
| TX complete | reload × factor × 8 cycles after the JOY_TX_DATA write (1,088 at 0088h) | [PER.02][PER.08] |
| Pad /ACK then IRQ7 | TX complete + about 340 cycles (10 µs) | [PER.08] |
| Card /ACK then IRQ7 | about 1,500 cycles after the byte starts, except the last byte; Sony cards add about 31,000 cycles after the 7th byte of a Read | [PER.01] |
| IRQ7 never within 100 cycles of the TX write | the kernel's acknowledge-after-send pattern | [PER.01] |
| Config mode watchdog | about 1 s without a command: pad leaves config mode, locks rumble, returns to digital | [PER.01] |

**Recommendation:** a cycle-scheduled SIO0: the write schedules "byte done" at the baud-derived count; the selected device returns (reply byte, ack delay or none); IRQ7 is raised at done + delay only when CTRL b12 is set and the device acked. Devices are pure functions of (input byte, internal state) so the same code serves tests and fuzzing. **Strongest alternative:** instantaneous replies with a fixed 1,500-cycle IRQ7 for everything (what many early emulators did). **Would change if:** a hardware trace shows the pad delay varies with the command; then the delay becomes a per-command table.

## 2. Controllers
### 2.1 ID halfword and payload (reply bytes 2–3, ID low then 5Ah) [PER.01]
| ID | Device | Payload after the ID |
|---|---|---|
| 5A41h | digital pad (SCPH-1010/1080), also a DualShock in digital mode | 2 button bytes, active low: byte 1 = Select b0, L3 b1, R3 b2, Start b3, Up b4, Right b5, Down b6, Left b7; byte 2 = L2 b0, R2 b1, L1 b2, R1 b3, Triangle b4, Circle b5, Cross b6, Square b7 (L3/R3 only on analog pads) |
| 5A73h | analog pad in analog mode (SCPH-1150 Dual Analog, SCPH-1180, SCPH-1200 DualShock; LED red) | buttons as above, then RX, RY, LX, LY, 00h–FFh, centre 80h |
| 5A53h | analog joystick SCPH-1110, or a Dual Analog in "flight" mode (LED green) | as 5A73h |
| 5A12h | mouse SCPH-1030/1090 | halfword 1: b10 right, b11 left (0 = pressed); halfword 2: dX, dY, signed −80h..+7Fh |
| 5A23h | neGcon | halfword 1: Start b3, pad b4–7, R b11, B b12, A b13; halfword 2: twist 00h–FFh; halfword 3: analog I, II, L |
| 5AE3h | Jogcon | neGcon-like plus a dial read optically, more than 360°; halfword 3 b0 "turned left", b1 "turned right"; force-feedback motor, command 4Dh reported |
| 5A63h | GunCon (Namco, cinch type) | halfword 1: A b3, trigger b13, B b14; X, Y (2.7) |
| 5A31h | Konami Justifier / Hyperblaster (IRQ10 type) | Start b3, Back b14, trigger b15; no coordinates in the data (2.7) |
| 5A80h | multitap SCPH-1070 | four pads, 2.5 |
| 5AF3h | any analog pad in config mode | 6 bytes, 2.3 |
| 5A79h, 5A96h, 5AE8h | DualShock 2 pressure mode, Lightspan keyboard, keyboard/sticks | out of scope |
| FFFFh | nothing connected | transfer aborts after the address byte |

Model numbers: SCPH-1010/1080 digital, SCPH-1110 stick, SCPH-1150 (one motor, red/green LED), SCPH-1180 (no motor), SCPH-1200 and SCPH-110 DualShock (two motors), SCPH-1070 multitap [PER.01]. A standard poll is `01h 42h 00h 00h 00h ...`; the two 00h bytes after 42h carry rumble (2.4). Gran Turismo 1 is noted as not switching the pad to analog by itself; the user presses the Analog button [PER.01].

### 2.2 The Net Yaroze
The Yaroze needs the DTL-H3020 Access Card in a memory-card slot: it answers address 21h with /ACK and replies a 0xh byte to the next byte; without it the boot disc refuses [PER.01]. Its pad is a standard digital pad (unverified). Later seed with the Yaroze boot path in PSX-SYSTEM.

### 2.3 Config mode (analog pads with config support; DualShock and SCPH-1150-era firmware differ) [PER.01][PER.08]
| Command | Send | Reply after `HiZ F3h 5Ah` | Meaning |
|---|---|---|---|
| 43h | `01 43 00 xx 00 ×5` | outside config: a normal 42h reply; inside: `00 ×6` | xx = 01h enter config (ID becomes 5AF3h), 00h leave |
| 42h in config | `01 42 00 M2 M1 ...` | buttons + 4 axes, always analog | rumble bytes per 4Dh mapping |
| 44h | `01 44 00 Led Key 00 ×4` | `00 00 Err 00 00 00` | Led 00h digital (LED off), 01h analog (LED on); Key 03h locks the Analog button, 00h–02h unlock; resets the 4Fh reply to digital or analog |
| 45h | `01 45 00 ×7` | `Typ 02 Led 02 01 00` | Typ 01h PSX analog pad, 03h DualShock 2; Led 00h/01h |
| 46h | `01 46 00 ii 00 ×5` | ii = 00h: `00 00 01 02 00 0A`; ii = 01h: `00 00 01 01 01 14` | actuator info; last byte is the drain in units (10 or 20, Sony max 60) |
| 47h | `01 47 00 ×7` | `00 00 02 00 01 00` | constant |
| 4Ch | `01 4C 00 ii 00 ×5` | `00 00 00 dd 00 00`, dd = 04h (ii = 0) or 07h (ii = 1) | constant |
| 4Dh | `01 4D 00 a b c d e f` | the previous six mapping bytes | maps each of the six 42h payload bytes to a motor: 00h = small motor M2 (bit 0), 01h = large motor M1 (0–FFh), FFh = none. Initial FFh ×6. Enable: `00 01 FF FF FF FF` |

Config mode transfers are 9 bytes. ares implements exactly this table (digital 5A41h, analog 5A73h, config 5AF3h; 45h reply `01 02 analogMode 02 01 00`) and starts from analog mode on, config off, mapping FFh ×6 [PER.08].

### 2.4 Rumble [PER.01][PER.08]
Old method (SCPH-1150; kept by SCPH-1200/110 until a config command is used): `01 42 00 xx yy` with xx in 40h–7Fh and yy bit 0 set turns the single motor on (yyxx = 0140h on, 0000h off); digital on/off only. New method: after 4Dh mapping, byte M1 00h–FFh drives the large motor (starts spinning at about 50h–60h, keeps running down to about 38h), M2 bit 0 switches the small motor. Using config commands disables the old method until reset; the 1 s watchdog locks rumble. **Recommendation:** the core reports {large 0–255, small 0/1} per frame; the host maps it to libretro's `RETRO_RUMBLE_STRONG/WEAK`.

### 2.5 Multitap SCPH-1070 [PER.01]
Method 1: `01 42 01 00 ...` returns 5A80h then halfwords 1–4 pad A, 5–8 pad B, 9–12 pad C, 13–16 pad D, missing slots padded FFFFh; an empty slot A aborts after the first byte; a tap without pads returns only `HiZ 80 5A LSB`. Method 2: `0n 42 00 00 ...` with n = 1–4 reads one pad. Memory cards: `8n ...` for slot n. IRQ10 is wired to all four slots. Some titles need the tap on port 2 (Bomberman, Jigsaw Madness, S.C.A.R.S., Zen Nippon Pro Wrestling: Ouja no Tamashii); NBA Live reaches 8 players with two taps. Beetle PSX and PCSX-ReARMed expose per-port multitap options [PER.11][PER.12].

### 2.6 Mouse, neGcon, Jogcon
Payloads in 2.1. Beetle PSX and PCSX-ReARMed expose neGcon twist response (linear/quadratic/cubic) and deadzone and mouse sensitivity as core options [PER.11][PER.12]; the core takes raw axis values and leaves curves to the host.

### 2.7 Light guns against the beam [PER.01]
| Gun | How position reaches the game | Numbers |
|---|---|---|
| GunCon (5A63h) | the gun measures from the video cable: X in 8 MHz clocks since HSYNC, Y in scanlines since VSYNC, delivered in halfwords 2–3 of the 42h reply, read shortly after VBLANK | X valid 04Dh–1CDh; Y PAL 20h–127h, NTSC 19h–F8h; X = 0001h, Y = 0005h "light during VSYNC" (gun pointed off screen or at a dark area), X = 0001h, Y = 000Ah "no light or busy"; about 30,000 cycles between VBLANK and VSYNC on NTSC, a read takes about 15,000 cycles |
| Konami Justifier / Hyperblaster (5A31h) | the sensor pulls controller-port pin 8 (/IRQ10) when it sees the beam; the game enables it with `01 42 00 x0 00` (10h on) and reads Timer 0 (dot) and Timer 1 (line) in the IRQ10 handler | NTSC X = (Timer0 − 140) × 0.198166, PAL × 0.196358; Y = Timer1. Sony's libgun does not acknowledge the old IRQ10 first, so the first read is unreliable |

**Recommendation (v2.0):** the host supplies a cursor in output pixels plus trigger; the GPU phase records the (line, dot) at which each output pixel was emitted and its brightness; GunCon converts to the 8 MHz and line units, the Justifier raises IRQ10 at the matching GPU timestamp if the pixel is bright enough. **Strongest alternative:** geometric conversion with no brightness test. **Would change if:** a title is found that calibrates against a dark screen and expects the error codes.

### 2.8 PocketStation (SCPH-4000) [PER.06][PER.01]
A memory card with an ARM7TDMI (variable clock, max 7.995 MHz), 2 KB SRAM, 16 KB BIOS ROM, 128 KB flash, 32×32 1-bit LCD, 12-bit PCM speaker, five buttons, IrDA and an RTC that the firmware resets to 1999-01-01 at every excuse. On the PSX bus it is an ordinary card (52h/57h/53h) plus commands 58h (ID/version), 59h (prepare file execution), 5Ah (get dir index, flags, serial, date, time), 5Bh/5Ch (execute function and move data card→PSX / PSX→card), 5Dh (custom download notification), 5Eh/5Fh (ComFlags). Its executables live in card files whose names replace "-" by "P" and use directory bytes 50h–5Fh [PER.01]. **Scope:** a later seed: v1 serves it as a plain 128 KB card so games that write PocketStation data still save; the ARM core, LCD and the 58h–5Fh set are a separate module behind the card interface.

### 2.9 v1 scope (recommendation)
**v1.0:** digital pad and DualShock (digital and analog modes, the Analog button as a host input, config commands 43h–47h, 4Ch, 4Dh, both rumble methods, the 1 s watchdog), one memory card per port, SIO0 timing of 1.3. **Strongest alternative:** digital pad only in v1.0 (about 150 lines less). **Would change if:** never; analog mode is needed for most post-1998 titles and the config set is small. The rest is placed in §8.

## 3. Memory cards
### 3.1 Layout: 128 KB = 16 blocks × 8 KB = 1,024 frames × 128 bytes; sector numbers 0–3FFh [PER.01]
| Block 0 frame | Content | Fields |
|---|---|---|
| 0 | header | 00h–01h "MC", 02h–7Eh zero, 7Fh checksum (0Eh) |
| 1–15 | directory for block N | 00h–03h state: 51h in use first or only block, 52h middle, 53h last; A0h free; A1h/A2h/A3h deleted first/middle/last; 04h–07h file size 2000h–1E000h in 8 KB steps; 08h–09h next block minus 1 or FFFFh; 0Ah–1Eh filename; 1Fh 00h; 20h–7Eh don't care; 7Fh checksum |
| 16–35 | broken sector list | 00h–03h sector number (block × 64 + frame) or FFFFFFFFh |
| 36–55 | broken sector replacement data | 128 bytes each, usually FFh |
| 56–62 | unused | FFh |
| 63 | write test | usually a copy of frame 0 (games dummy-write sector 003Fh to clear the fresh bit) |

Checksum: XOR of bytes 00h–7Eh of the frame [PER.01]. Filenames: region "BI" Japan, "BE" Europe, "BA" America, then the 10-character game code (SLPS/SCPS Japan, SLUS/SCUS America, SLES/SCES Europe; SCxS Sony, SLxS licensed), then up to 8 free characters [PER.01]. ares formats a new card as frame 0 and 63 "MC" + 0Eh, directory frames A0h with next = FFFFh and checksum A0h, broken-sector frames FFFFFFFFh with next = FFFFh, everything else FFh [PER.08].

### 3.2 Title frame (frame 0 of a file's first block) [PER.01]
00h–01h "SC"; 02h icon flag 11h one frame, 12h two frames (switch every 16 PAL frames), 13h three frames (every 11); 03h block count; 04h–43h title, 64 bytes Shift-JIS (full-width; the non-Japanese BIOS card manager does not translate it to its European set); 44h–4Fh reserved; 50h–5Fh PocketStation fields; 60h–7Fh 16-entry CLUT (ABGR1555). Frames 1–3 of that block: 16×16 4-bit icons, 128 bytes each.

### 3.3 Command set (all after address 81h; replies after a HiZ byte) [PER.01][PER.08]
| Command | Sequence (send / reply) | End byte |
|---|---|---|
| Read 52h | `81 52 00 00 00 MSB LSB 00 00 00 00 ×128 00 00` / `HiZ FLAG 5A 5D 00 00 5C 5D MSB LSB data ×128 CHK 47` | 47h "G"; Sony cards reply FFFFh and abort for sector > 3FFh; third-party cards mask the sector AND 3FFh and add eight 5Ch bytes |
| Write 57h | `81 57 00 00 00 MSB LSB data ×128 CHK 00 00 00` / `HiZ FLAG 5A 5D 00 00 00 00 ... 5C 5D END` | 47h good, 4Eh "N" bad checksum, FFh bad sector |
| ID 53h (Sony only) | `81 53 00 ...` / `HiZ FLAG 5A 5D 5C 5D 04 00 00 80` | |
| any other | aborts after the command byte (sometimes one more), reply FFh | |

CHK = MSB XOR LSB XOR the 128 data bytes. FLAG starts at 08h on power-up or insertion: bit 3 "directory unread / fresh" cleared by any write, bit 2 write error (reported on the next command), bit 5 set on some third-party cards [PER.01]. ares: `flag.fresh` cleared at the end of a write; a write to sector ≥ 1024 sets the error bit and ends FFh; a checksum mismatch ends 'N' but the data has already been stored byte by byte [PER.08]. Non-Sony and PS2 cards differ (PS2 cards are larger with another directory structure and do not fit the slot) [PER.01].

### 3.4 Card image file formats [PER.01][PER.17]
| Format | Header | Who writes it |
|---|---|---|
| raw 128 KB (131,072 bytes) | none | ePSXe/FPSX `.mcr`, Bleem `.mcd`, pSX `.bin`, SmartLink `.psm`, WinPSM `.ps`, DataDeck `.ddf`, PSXGameEdit `.mc`, MCExplorer `.mci`, MiSTer `.sav`, PS3 `.VM1`, POPStarter `.VMC`, libretro `.srm` |
| DexDrive `.gme` (20F40h bytes) | 0F40h (3,904) bytes: 000h "123-456-STD",00h; 010h `00 00 01 00 01`; 015h 16 copies of directory byte 00h; 025h 16 copies of directory byte 08h; 040h fifteen 100h-byte description strings; F40h the 128 KB image | InterAct DexDrive, MemcardRex |
| VGS `.mem`/`.vgs` | 64 bytes "VgsM" | Connectix Virtual Game Station |
| PlayStation Magazine `.psx` | 256 bytes "PSV" | magazine cover discs |
| PSP `.VMP`, Vita `.BIN` (PocketStation "MCX") | PSP/Vita signed headers (sizes not in the sources read) | Sony firmware |
| single saves | `.mcs` = directory frame + the file's blocks in link order; Xploder/AR `.psx` = 54-byte header (filename 21, title 33) + blocks; `.psv` PS3; `.ps1`, `.mcb`, `.mcx`, `.pda` | save managers |

Some tools omit trailing unallocated blocks, so a raw file shorter than 128 KB is a legal truncated card [PER.01].

### 3.5 How the libretro cores name cards [PER.11][PER.12][PER.13]
| Core | Slot 1 | Slot 2 | Per-game vs shared |
|---|---|---|---|
| Beetle PSX | `<content>.srm` (raw; "internally identical" to `.mcr`) | `<content>.1.mcr` | option "Shared Memory Cards" switches to `mednafen_psx_libretro_shared.0.mcr`/`.1.mcr` (needs the mednafen slot-0 method) |
| PCSX-ReARMed | `<content>.srm`, named after the `.cue`/`.m3u`/`.pbp` | off by default; `pcsx-card2.mcd` shared by every game in the folder | slot types: libretro, game code (serial), shared, none |
| DuckStation (standalone; libretro build withdrawn) | per-game by serial `<serial>_1.mcd`, by title, by file title, shared `shared_card_1.mcd`, or non-persistent (file names from commit history, unverified) | same scheme `_2` | README: per-game switching optional, card editor and importer built in |

Both libretro docs warn that save states contain the cards, so loading a state can roll a card back; RetroArch's "Don't overwrite SaveRAM on loading savestate" is the mitigation [PER.11][PER.12]. Serial-named cards need the serial from §5 before the first card access.

### 3.6 Export (recommendation)
`schway_get_memory(CARD1|CARD2)` returns one fixed 131,072-byte span per slot, writable by the host after load and before the first frame; `save_generation` increments once per completed Write command that ended 47h; the core never touches files, exactly cogmaster's battery contract [PER.19]. The libretro core maps `RETRO_MEMORY_SAVE_RAM` to card 1 as raw 128 KB (`.srm`, byte-identical to Beetle's and PCSX-ReARMed's) and writes card 2 itself as `<content>.1.mcr` (Beetle's name, so users can move between cores); a core option selects per-game-by-serial naming for both. The runner writes `<image>.1.mcr`/`.2.mcr` through a temp file and rename (atomic). Import accepts raw 128 KB, raw shorter files padded FFh (never 00h, which would read as an allocated block with checksum errors), `.gme` by skipping 3,904 bytes after checking "123-456-STD", `.mem` by skipping 64 after "VgsM", `.psx` by skipping 256 after "PSV"; anything else is rejected. On import the loader validates frame checksums and the "MC" header and reports lint, never rewrites. **Strongest alternative:** one 256 KB SAVE_RAM span with both cards (simplest host path, but breaks `.srm` interop with every other core). **Would change if:** a sibling frontend wants both cards in one blob; then add a second kind, keep SAVE_RAM = card 1.

## 4. Save states (v3.0 seed; constraints apply from phase 1)
| Component | Bytes | Note |
|---|---|---|
| main RAM | 2,097,152 | |
| VRAM | 1,048,576 | |
| SPU RAM | 524,288 | |
| scratchpad, GTE (64 × 32-bit), CPU, COP0, cache state | about 5,000 | instruction cache 4 KB if modelled |
| CD-ROM: pending command and parameter FIFO, response FIFO, seek target, current MSF, mode byte, sector buffer (up to 8 × 2,352), XA ADPCM state, shell/disc-change flag, disc index | about 25,000 | settled in PSX-SPU-CDROM-AND-MDEC; the disc identity is a hash, not the image |
| GPU FIFO, registers, display and draw state; MDEC buffers; timers; DMA; interrupt controller | about 10,000 | |
| SIO0: registers, FIFOs, scheduled byte, per-device state (pad mode, config, mapping, card command phase, flag) | about 200 | |
| memory cards | 2 × 131,072 only with `WITH_CARDS` | outside by default; Beetle and PCSX-ReARMed include them; see 3.5 for the rollback hazard |
| total (derived) | about 3.6 MiB without cards, 3.9 MiB with | |

libretro expects `retro_serialize_size()` constant across a session (quirks: `RETRO_SERIALIZATION_QUIRK_*` for the rest) [PER.14]. Rewind at 60 fps on a 3.6 MiB state is 216 MiB/s before the frontend's delta compression; run-ahead saves and loads once or twice per frame, so load must validate without allocating. **Constraints for v1:** every module's state is a fixed-size struct of plain integers, no pointers, visited by one function per struct in SIZE / WRITE / VALIDATE / APPLY modes as cogmaster does [PER.19]; the disc is referenced by its identity (§5) plus a swap index; the host-provided image is never inside the state. **Recommendation:** cards are outside the state by default (loading a state never rolls a card back, ECO C.06); a `WITH_CARDS` flag includes both spans for TAS and movie use; `card.c` commits a frame only when the Write command completes, so a state taken mid-write stays consistent either way (ARCHITECTURE 13). **Strongest alternative:** cards always inside, as Beetle and PCSX-ReARMed do, with a libretro-side "don't overwrite SaveRAM" flag. **Would change if:** run-ahead or netplay desynchronises on a card write with cards outside; then the flag defaults on for those hosts.

## 5. Game identity and region
### 5.1 SYSTEM.CNF and the boot executable [PER.03]
Root-directory text file: `BOOT = cdrom:\XXXX_NNN.NN;1 [arg]`, `TCB = 4` (hex, max threads), `EVENT = 10` (hex, 16 events), `STACK = 801FFF00`; lines end 0Dh 0Ah (bare 0Ah tolerance is unknown). The executable name is the cover's game code with "-" replaced by "_" and the last two digits in the extension (`SLUS_123.45`); case-insensitive; Wild Arms puts it in `EXE\SCUS_946.06`. Without SYSTEM.CNF the BIOS boots `PSX.EXE`. The PS-EXE header is 800h bytes: "PS-X EXE" at 0, initial PC at 10h, GP at 14h, load address at 18h, size at 1Ch (multiple of 800h, excluding the header), data/bss at 20h–2Ch, SP base and offset at 30h/34h, and at 4Ch "Sony Computer Entertainment Inc. for Japan/Europe/North America area" (not checked by the BIOS, often zero in homebrew).

### 5.2 Region [PER.04][PER.05]
Sector 4 of the system area holds "          Licensed  by          " then "Sony Computer Entertainment Inc." (JP), "...Euro" + " pe   " (EU) or "...Amer" + "  ica " (US). The drive's GetID returns the SCEx wobble string: SCEI Japan, SCEA America, SCEE Europe; the BIOS refuses a mismatch and a Yaroze returns spaces for non-SCEA discs. **Recommendation:** region = serial prefix table (5.3) first, licence string second, EXE marker third; the three agree on every retail disc in the libretro dat (unverified claim, verified per disc by the lint); a disagreement is lint and the serial wins. The video standard follows the region profile unless the host overrides.

### 5.3 Serials (tested: the libretro-database redump dat, 13,407 entries, 12,721 distinct serial lines [PER.15])
| Prefix | Count | Region |
|---|---|---|
| SLPS / SLPM / SCPS / SCPM / SIPS / PCPX / PAPX / PBPX | 8,194 / 3,270 / 1,032 / 10 / 42 / 330 / 100 / 30 | Japan (SC = Sony, SL = licensed, P = press or demo) |
| SLES / SCES / SCED / SLED | 6,204 / 1,698 / 668 / 178 | Europe (ED = demo) |
| SLUS / SCUS / LSP / PUPX / SPUS | 3,348 / 834 / 418 / 4 / 2 | America (LSP = Lightspan) |
| SLKA / SCKA / SCZS / ESPM / DTL | 6 / 2 / 12 / 6 / 10 | Korea, Asia, demos, development |

Regions in the dat: Japan 6,489, USA 2,386, Europe 2,307, Germany 688, France 583, Italy 377, Spain 299, others. The dat keys each disc by serial, track CRC/MD5/SHA-1 and size; multi-disc sets share a title with distinct serials [PER.15].

### 5.4 RetroAchievements hash (rcheevos, MIT, pinned) [PER.09][PER.10]
`rc_hash_psx`: open track 1, read the first 2,047 bytes of SYSTEM.CNF, find `BOOT`, skip spaces and `=`, strip `cdrom:` and leading `\`, take the name up to a space or `;` (max 63 chars); if not found use `PSX.EXE`. Read the EXE's first 32 bytes; if it starts "PS-X EXE" the hashed length is the 32-bit size at 1Ch plus 2,048 (header included); otherwise the directory size. MD5 = the executable name as found (no path prefix, no `;1`) followed by the executable bytes. The name is included because some engines differ only by data files. **Recommendation:** the runner's `schwaystation-run --identify` prints serial, region, boot name, EXE SHA-256 and this MD5 so achievement and database lookups match rcheevos byte for byte; the core only needs the serial.

## 6. ROM hackers, translators, cheats
### 6.1 PPF (PlayStation Patch File) [PER.16]
| Version | Header | Records | Trailer |
|---|---|---|---|
| PPF1 ("PPF10", method 0) | 56 bytes: magic 5, method 1, description 50 | u32 LE offset, u8 count, data | none |
| PPF2 ("PPF20", method 1) | 56 + 4 (original size) + 1,024 block check (image bytes from 9320h) | u32 LE offset, u8 count, data | optional file_id.diz |
| PPF3 ("PPF30", method 2) | 60 bytes: magic, method, description 50, image type (0 BIN, 1 GI), blockcheck flag, undo flag, dummy; + 1,024 validation block (from 9320h for BIN, 80A0h for GI) when blockcheck = 1, so 60 or 1,084 bytes | u64 LE offset, u8 count, data, then the same count of undo bytes if undo = 1 | `@BEGIN_FILE_ID.DIZ` ... `@END_FILE_ID.DIZ` + u16 length, at most 3,072 bytes, after the patch data |

PPF1/PPF2 layouts are from the PPF3 developer text's description of methods 0 and 1 and file(1)'s magic (unverified beyond the header). Offsets are into the raw 2,352-byte track image, so a patch for a `.bin` cannot be applied to a `.chd` or `.iso` without re-encoding. xdelta and BPS treat the image as a byte stream; BPS carries three CRC32s and is what translation groups post for `.bin` files (BPS layout in cogmaster's CART.31; unverified here). DuckStation applies PPF automatically; neither libretro core's documentation mentions PPF [PER.13][PER.11][PER.12].

**Recommendation:** no patching in the core. The runner's `--patch` applies PPF1/2/3 (with undo and blockcheck) and BPS to a temporary copy or, for PPF, through a sector overlay: a sorted list of (byte offset, length, bytes) consulted by the disc reader before returning a sector, which costs nothing per unpatched sector and avoids rewriting a 700 MB image. The library seeds `schway_set_sector_hook(fn)` so a host can serve any sector itself (v2.0, also the hook translation testers use for live edits). **Strongest alternative:** apply to a copy only. **Would change if:** Playstead wants in-place overlay for `.chd` input; the overlay already handles it because it sits after decompression.

### 6.2 Cheats [PER.07][PER.18][PER.14]
GameShark/Action Replay code `TTaaaaaa dddd` with the address in KSEG0 (80aaaaaa): 30 write byte, 80 write halfword; v2.2+: D0–D3 halfword equal/not/less/greater and E0–E3 byte conditionals (execute the next code), 10/11 halfword and 20/21 byte increment/decrement; v2.41+: D4 buttons-if, D5/D6 buttons on/off, C0 "if equal, enable all codes" (the master code), C1 delay, C2 memory copy (two lines), 50 slide `5000nnbb dddd` + `aaaaaaaa ??ee` (nn repeats, bb address step, ee value step; CD devices need a `00000000 0000` separator); Caetla adds C3/91 indirect and 12/22 32-bit. Limits: 60 conditionals, 30 inc/dec. Signedness and alignment of the compares are undocumented. libretro delivers the string through `retro_cheat_set(index, enabled, code)` and `retro_cheat_reset()` [PER.14]; Beetle and PCSX-ReARMed accept RetroArch cheats, not native cheat files [PER.11][PER.12]. **Recommendation (v1):** nothing in the core; the libretro layer parses the string into (type, address, value) and applies writes via `poke` at frame start, conditionals evaluated in order; the real device hooks VSync, so frame-start application matches (unverified for mid-frame polling games).

### 6.3 What the library offers hackers (seeds)
Bus peek/poke on RAM, scratchpad, VRAM and SPU RAM; the sector hook; a read/write/execute watch callback; deterministic rerun from a state plus an input log; the `.sym` loader for PSn00bSDK/PsyQ-style maps settled in the debug phase (unverified layout).

## 7. Hostile-input bounds
Fuzz targets in v1.0 (ENGINEERING 3 names them; ARCHITECTURE 13): `fuzz.cue`, `fuzz.exe`, `fuzz.cnf`, `fuzz.memcard`, `fuzz.state`, `fuzz.gpu`, `fuzz.cdrom`, `fuzz.spu`, `fuzz.mdec` and `fuzz.sio` (random bus bytes into the pad and card devices); `fuzz.card_import` (.gme/.mem/.psx headers), `fuzz.chd` (libchdr wrapper), `fuzz.sbi`, `fuzz.ppf`, `fuzz.cheat` and `fuzz.m3u` arrive in v2.0 when each parser lands; each with a `fuzz.regress.*` replay as in cogmaster [PER.19].

| Parser | Owner | Bounds | Rejection |
|---|---|---|---|
| cue sheet | core loader | ≤ 64 KiB text, ≤ 99 tracks, ≤ 99 indexes per track, FILE names ≤ 255 bytes with no path escape (`..`, absolute); MODE1/2048, MODE2/2352, MODE2/2336, AUDIO only; MSF fields ≤ 99:59:74; total image ≤ 1 GiB (largest known PSX disc with audio is under 800 MB; 1 GiB leaves room) | `BAD_CUE` with line number |
| bin/track files | core loader | size a multiple of the sector size for the declared mode; ≥ 16 sectors for the data track | `BAD_IMAGE` |
| chd | libchdr (BSD-3), wrapped | hunk ≤ 1 MiB, ≤ 1 GiB logical, CD metadata tracks ≤ 99; every libchdr return checked; decompression into a caller buffer of exact size | `BAD_CHD` |
| SYSTEM.CNF | core identify | first 2,047 bytes only (rcheevos's bound), ASCII, keys BOOT/TCB/EVENT/STACK, boot name ≤ 63 chars, no `..`; TCB ≤ 0x40, EVENT ≤ 0x100, STACK in KUSEG/KSEG0 RAM | `BAD_SYSCNF`; missing file falls back to PSX.EXE, missing both → `NO_BOOT_EXE` |
| PS-EXE (disc or direct `.exe`/`.psexe` load) | core | ≥ 800h bytes, size at 1Ch a multiple of 800h and = file size − 800h (homebrew tolerance: lint if larger), load address + size inside the 2 MiB RAM (KUSEG, KSEG0 or KSEG1 forms), PC inside the loaded range or BIOS, SP inside RAM or 0 | `BAD_PSEXE` |
| memory card image | core import | exactly 131,072, or shorter padded FFh; header formats by magic (3.6); frame checksums and "MC" are lint only; directory state, size and next-block fields are never trusted by the core (the game's kernel reads them as data) | `BAD_CARD_SIZE`, `BAD_CARD_HEADER` |
| card bus | card device | sector index masked to 0–3FFh (Sony behaviour: abort with FFFFh); byte phase counter bounded by the command table; checksum mismatch ends 4Eh | n/a (bus) |
| pad bus | pad device | command byte outside the table → reply FFh and abort; config-mode payload bytes copied into fixed 6-byte arrays | n/a |
| save state | core | exact `state_size`; magic, format version, behaviour revision, profile, disc identity, CRC-32 over the body, every field VALIDATEd (indexes, FIFO depths, phases, MSF, DMA addresses) before APPLY; ticks ≤ 2^62 | `BAD_STATE_*` with field name |
| SBI | core loader | "SBI\0", then 4-byte groups (MSF + format 1/2/3) with 10/3/3 payload bytes, ≤ 10,000 entries, MSF inside the image | `BAD_SBI` |
| PPF / BPS | runner | PPF: magic, method ≤ 2, 60 or 1,084 header, record offset + count ≤ image size, undo bytes present when flagged, file_id ≤ 3,072; BPS: varints with overflow check, source size and CRCs must match | `BAD_PATCH` |
| cheat string | libretro layer | 8 + 4 hex digits per line, ≤ 64 lines, types from 6.2; address masked to RAM | code ignored, log line |
| m3u | libretro layer | ≤ 32 entries, each resolved relative to the m3u's directory, no path escape | `BAD_M3U` |

## 8. Milestone placement
| Feature | Milestone | Reason |
|---|---|---|
| SIO0 registers, timing, IRQ7; digital pad; DualShock analog, config, rumble | v1.0 | every title needs a pad; analog mode is the default post-1998 |
| one memory card per port, raw export with generation counter, `.srm` + `.1.mcr`, import of `.gme`/`.mem`/`.psx` headers | v1.0 | saving is table stakes; import is 40 lines |
| serial, region, boot EXE identity; rcheevos MD5 in the runner | v1.0 | per-game cards and the libretro database key on the serial |
| multitap SCPH-1070 (both methods, cards on the tap) | v2.0 | 4-player titles in the whole-library pass |
| mouse, neGcon, Jogcon (no force feedback), Dual Analog flight mode (5A53h) | v2.0 | payload-only devices; Jogcon motor seed |
| GunCon, Justifier with GPU-timestamped beam | v2.0 | needs the GPU phase's per-pixel timestamps |
| sector overlay hook, cheat application in the libretro layer | v2.0 | hackers and translators in the library pass |
| save states (visitor frozen), rewind, run-ahead, disc-swap state, movies | v3.0 | format freeze after the library pass; constraints respected from phase 1 |
| PocketStation ARM core, LCD, 58h–5Fh | later seed | separate CPU; v1 stores its files as plain card data |
| SIO1 link cable (two-instance), Yaroze Access Card and boot | later seed | needs a two-core harness; about 70 titles use the cable |
| PS2 DualShock 2 pressure mode, keyboards, DVD remote | out of scope | not PlayStation 1 devices |

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| PER.01 | T2 | psx-spx (consoledev.net edition) docs/controllersandmemorycards.md | 6d7d1bc106a7 | handshake, /ACK timing, IDs, payloads, config commands, rumble, multitap, guns, card layout, commands, flag, timings, file formats, Yaroze card, model numbers |
| PER.02 | T2 | psx-spx docs/serialinterfacessio.md | 6d7d1bc106a7 | SIO0 registers, baud formula, 0088h, FIFO behaviour, acknowledge order, link-cable list |
| PER.03 | T2 | psx-spx docs/cdromfileformats.md (SYSTEM.CNF and EXE section) | 6d7d1bc106a7 | SYSTEM.CNF lines, PS-EXE header, PSX.EXE fallback, boot-name convention |
| PER.04 | T2 | psx-spx docs/cdromformat.md (system area) | 6d7d1bc106a7 | licence string per region, sector 4 layout |
| PER.05 | T2 | psx-spx docs/cdromdrive.md (GetID) | 6d7d1bc106a7 | SCEI/SCEA/SCEE, Yaroze behaviour |
| PER.06 | T2 | psx-spx docs/pocketstation.md | 6d7d1bc106a7 | PocketStation hardware, commands 58h–5Fh |
| PER.07 | T2 | psx-spx docs/cheatdevices.md | 6d7d1bc106a7 | GameShark code types and limits |
| PER.08 | T3 | github.com/ares-emulator/ares ares/ps1/peripheral/{io,memory-card/memory-card,dualshock/dualshock}.cpp (ISC; LICENSE read at the pin) | 9408cb43d494 | register bit layout, ack delays 338/170 cycles, card phase machine, format, flag bits, config replies |
| PER.09 | T1 | github.com/RetroAchievements/rcheevos src/hash/rc_hash_disc.c (MIT; LICENSE read) | dfdc64ea035c | PSX hash algorithm |
| PER.10 | T1 | docs.retroachievements.org/developer-docs/game-identification.html | accessed 2026-10-19 | hash rule statement |
| PER.11 | T1 | docs.libretro.com/library/beetle_psx/ | accessed 2026-10-19 | `.srm`/`.1.mcr`, shared cards, device types, multitap, SBI, states include cards |
| PER.12 | T1 | docs.libretro.com/library/pcsx_rearmed/ | accessed 2026-10-19 | `.srm`, `pcsx-card2.mcd`, slot types, options, SBI |
| PER.13 | T1 | github.com/stenzek/duckstation README.md (docs only; code never opened; libretro docs page is 404) | accessed 2026-10-19 | PPF auto-apply, SBI, devices, cheat database; per-game names from commit titles (unverified) |
| PER.14 | T1 | github.com/libretro/libretro-common include/libretro.h | a3194c18ba65 | `retro_cheat_set`, serialize size, quirks, SAVE_RAM |
| PER.15 | T1 | github.com/libretro/libretro-database metadat/redump/Sony - PlayStation.dat (version 2026.10.07) | 12c21cf79f1e | serial prefix counts, regions, entry layout (tested: `grep -o 'serial "[A-Z]*' \| sort \| uniq -c`) |
| PER.16 | T2 | github.com/meunierd/ppf ppfdev/PPF3.txt (PPF 3.0 developer text by Icarus/Paradox; no licence stated, read as a format description only) | 4f24f8c784fa | PPF3 header, record, undo, file_id |
| PER.17 | T1 | github.com/ShendoXT/memcardrex README (GPL project; docs only) | accessed 2026-10-19 | format-to-tool list, single-save formats |
| PER.18 | T2 | doc.kodewerx.org/hacking_psx.html (EnHacklopedia) | accessed 2026-10-19 | code type table cross-check |
| PER.19 | T1 | cogmaster .planning/preparation/ARCHITECTURE.md §6–7 (owner's project) | 2026-10-18 revision | battery span and generation counter, visitor, fuzz target shape |
| PER.20 | T2 | redump.org and wiki.redump.org | connection refused 2026-10-19 | dumping rules and SBI hosting (unverified); counts taken from PER.15 instead |
