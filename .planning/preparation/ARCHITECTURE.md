---
title: "Architecture: time base, modules and bus, public C API, I/O contracts, state, observability, execution engine, GPU and audio pipelines, build order, conflicts"
summary: "The technical spine of the schwaystation core: one CPU-cycle timeline with a lazy event scheduler, the module and register-owner map, the schway_ C API with its contracts and stop reasons, the video, audio, input, disc and memory-card contracts, the fixed-size visitor state, counters and first fault, the determinism rules and the three hash definitions, the interpreter written for a cached interpreter and JIT seed, the span rasteriser, the SPU/CD/MDEC pipeline, the four v1.0 phases, the Ir budgets, and every conflict between the preparation files with its settled answer."
read_when: "Before planning or writing core, runner or libretro code; before adding a module, API call, profile field, hash or stop reason; whenever two preparation files disagree."
updated: 2026-10-19
---

# schwaystation architecture

## Where to look
| Question | Section |
|---|---|
| What is a tick, how do devices catch up, how are frames and overruns bounded? | 1 |
| Which module owns which registers; how the bus dispatches; I-cache, scratchpad, unmapped | 2 |
| Every public function and struct with its contract; the header sketch | 3 |
| Video, audio, input, disc, memory-card and TTY contracts | 4 |
| State layout, the visitor, CD and disc-swap state, sizes | 5 |
| Counters, first fault, TTY, trace format, hooks ABI | 6 |
| Determinism rules and the frame, audio and state hash definitions | 7 |
| The interpreter and the constraints that keep a cached interpreter and JIT possible | 8 |
| The GPU rasteriser and scan-out, and the seams for enhancements | 9 |
| SPU, CD-ROM, MDEC pipeline and audio delivery | 10 |
| Build order and module dependencies per phase | 11 |
| Performance posture: budgets, hot paths, what is measured where | 12 |
| Open conflicts between preparation files and their settlements | 13 |

Citations: [ARCH.01] PSX-SYSTEM, [ARCH.02] PSX-GPU-AND-GTE, [ARCH.03] PSX-SPU-CDROM-AND-MDEC, [ARCH.04] PSX-PERIPHERALS-AND-SAVES, [ARCH.05] PSX-ECOSYSTEM-AND-USERS, [ARCH.06] PSXDEV-EXTRACTS, [ARCH.07] CONFORMANCE, [ARCH.08] ENGINEERING, [ARCH.09] HOSTS-AND-INTEGRATION; "§n" and "Cn" are that file's sections and choices. Settled kickoff choices [ARCH.12] are built on, never reopened. Choices read **Rec**, **Alt**, **Flip**. Names: library and CMake target `schwaystation`, public prefix `schway_`, internals `schway__<module>_` with module prefixes `cpu_ cop0_ gte_ bus_ dma_ timers_ irq_ gpu_ spu_ cdrom_ mdec_ sio_ card_ exe_ bios_ state_` (§13 row 9). The core never opens a file, window or device, reads a clock or the environment, starts a thread, keeps a mutable global, uses floating point, allocates after load, prints, aborts or exits [ARCH.08 §2, §10][ARCH.10 §1].

## 1. Time base and frame boundary
| Constant | NTSC | PAL | Source |
|---|---|---|---|
| Tick = one CPU cycle | 33,868,800 Hz | same | [ARCH.01 C1] |
| GPU clock per CPU cycle (exact ratio, accumulator num/den) | 715,909/451,584 | 709,379/451,584 | [ARCH.01 §1] |
| GPU clocks per line, lines per progressive frame | 3413, 263 | 3406, 314 | [ARCH.01 C2] |
| GPU clocks per frame | 897,619 | 1,069,484 | [ARCH.13] |
| CPU cycles per frame (rational) | 405,350,378,496/715,909 = 566,203.775 | 482,961,862,656/709,379 = 680,823.456 | [ARCH.13] (tested) |
| Frame rate as emulated | 53,693,175/897,619 = 59.8173 Hz | 53,203,425/1,069,484 = 49.7468 Hz | [ARCH.13] |
| Interlaced: fields alternate 263/262 (313/312) lines, 525 (625) per pair; field rate | 1,431,818/23,891 = 59.9313 Hz | 2,128,137/42,575 = 49.9856 Hz | [ARCH.13] (derived) |
| Audio samples per frame = cycles/768 | 527,799,972/715,909 = 737.244 (737 or 738) | 628,856,592/709,379 = 886.489 (886 or 887) | [ARCH.13] |
| SPU tick, CD sector at 1x | every 768 cycles; every 451,584 cycles | same | [ARCH.01 §1][ARCH.03 §1.1, §2.4] |

- **Counter and accumulator.** One `uint64_t ticks` per instance. The GPU keeps `gpu_acc` (u32, < 451,584) and `gpu_clock` (u64): advancing n cycles adds n × num to the accumulator and moves whole 451,584 quanta into `gpu_clock`; all other devices are exact in cycles [ARCH.01 C1]. The pair (ticks, gpu_acc) is the CPU/GPU phase and is a state field [ARCH.01 §9]. Dot clocks divide `gpu_clock` by 10/8/7/5/4; hblank is raised when the line's GPU clock passes X2; vblank IRQ when vcounter reaches Y2 and clears at Y1 [ARCH.02 §4.1].
- **The CPU drives the bus.** `cpu` executes instructions; every fetch, load and store calls `bus_read/bus_write(addr, width)` which charges the access cost (§2) and may raise an exception. Time advances only in the CPU loop (per instruction or per block, §8) and in the DMA engine.
- **One deadline, lazy devices.** `bus.next_deadline` = min over devices of `next_event`; the CPU compares `ticks >= next_deadline` at block ends and before every I/O access [ARCH.08 §10]. On a hit, `sched_run()` fires every device whose event is at or before `ticks`, in ascending event order, ties in the fixed order timers, gpu, dma, cdrom, spu, sio, mdec, then recomputes the minimum. A register access first calls that device's `catch_up(ticks)`:

| Device | `next_event` | Catches up before | Cost bound per catch-up |
|---|---|---|---|
| gpu | next line edge (hblank at X2, line end, Y1/Y2) and the busy budget end [ARCH.02 4.A] | GP0/GP1 access, GPUSTAT read, DMA2, frame end | lines elapsed (≤ 314); drawing is immediate at the last command word, not deferred [ARCH.02 7.A] |
| timers | next target or FFFFh hit computed from the clock source; dot and hblank sources are fed by gpu events [ARCH.01 C6] | any 1F8011xxh access | O(1) per counter |
| dma | end of the running chunk; DREQ changes from gpu, spu, cdrom, mdec | 1F8010xxh access | one chunk (≤ 256 cycles) |
| cdrom | next INT delivery, sector arrival (451,584 >> speed), seek end [ARCH.03 CD-TIME] | 1F80180xh access | O(1) plus one sector decode |
| spu | next 768-cycle sample tick | 1F801C00h-1FFFh access, DMA4, frame end | samples elapsed × 24 voices |
| sio | byte-done and ack ticks [ARCH.04 §1.3] | 1F80104xh access | O(1) |
| mdec | block decode end (1000 cycles per block [ARCH.03 MDEC-IDCT]) | 1F80182xh access, DMA0/1 | one block |
| irq, cop0 | none: combinational; CAUSE bit 10 is computed on read and at instruction boundaries [ARCH.01 §6] | n/a | O(1) |

- **Scheduler interface** (all in `bus.c`, no callbacks): `sched_set(dev, tick)` records a device's next event and lowers `next_deadline`; `sched_run()` is the slow path above; `<dev>_catch_up(ticks)` is called by the owner's register handlers and by `sched_run`; a device with nothing pending sets `UINT64_MAX`. Devices are enumerated in one `switch`, so adding one is a case, not a table entry.
- **DMA and the CPU stall.** A channel starts at the CHCR write (or at DREQ) and the engine runs whole slice blocks or list nodes atomically at the current tick, charging 1/4/24-40 cycles per word plus 17 per 16-word RAM row [ARCH.01 C5, §4][ARCH.02 §4.3]; `dma_busy_until` records the end. The CPU keeps running from I-cache hits, scratchpad, COP0 and GTE; its first RAM or I/O access, or fifth queued write, waits until `dma_busy_until` [ARCH.01 §4]. Bursts longer than 256 cycles (OTC, CD, large VRAM blocks) are split into chunks of at most 256 cycles with a deadline check between chunks; the chunking changes no observable tick. Chopping alternates CPU and DMA windows as CHCR bits 16-22 say.
- **Frame boundary.** The frame origin is the vertical sync: the GPU clock at which vcounter wraps to line 0. `run_frame` runs until `ticks` reaches the tick at which the accumulator will deliver that GPU clock, stopping at the first instruction boundary at or after it; the overshoot is charged to the next frame so cumulative ticks stay exact. Input set by `set_input` is latched into `sio` at the start of the call [ARCH.12]. In interlaced modes the field toggles at that origin, fields alternate 263/262 (313/312) lines, and a field is a frame for the API (derived). **Overrun bound** `SCHWAY_OVERRUN_MAX = 512` cycles: the longest instruction with its stalls (DIV 36 + a 29-cycle BIOS word fetch + a 44-cycle GTE stall = 109 [ARCH.01 §2, §3][ARCH.02 §5.2]) plus one DMA chunk (≤ 256) is below it; `core.overrun_bound` measures the maximum over the fixture set and fails above 512 [ARCH.13]. **Rec** vsync origin. **Alt** the VBLANK IRQ tick (Y2), which a game moves with GP1(07h). **Flip** a TAS consumer needs frames aligned to Y2.
- Nothing a program does stops time: an exception loop, a HALT-less spin or a GPU hammer advance ticks; a loaded state's ticks are capped at 2^62. Stop reasons of §3 bound every run call; the core never reads a wall clock (`core.no_host_clock` [ARCH.08 §2]).

## 2. Modules and the bus
| Module (file) | Owns | Registers or ranges owned | Lands |
|---|---|---|---|
| `instance.c`, `testcard.c` | public API, config, profile, frame driver, counters, first fault, test card | none | P1 |
| `cpu.c` (+ `cpu_decode.c`, `cpu_exec.inc` compiled twice) | GPRs, HI/LO with completion tick, PC and delay records, load-delay queue, I-cache (256 × 4 words, tags, valid bits), write queue count, block loop | FFFE0130h cache control | P2 |
| `cop0.c` | SR, CAUSE, EPC, BadVaddr, DCIC, BPC/BDA/masks, PRID, `trigger(code)` with BD/BT, RFE, the GTE-in-delay-slot quirk [ARCH.01 §2] | COP0 r0-r31 | P2 |
| `gte.c` | 64 registers, 44-bit accumulators, FLAG, UNR table generated at load, busy counter [ARCH.02 5.A] | COP2 | P2 |
| `bus.c` | `ticks`, `next_deadline`, physical decode, RAM 2 MB with RAM_SIZE mirrors, scratchpad 1 KB, BIOS 512 KB, memory-control delay registers and the derived per-port cost table, bus faults, dirty bitmap, `peek/poke` | 1F801000h-1F801020h, 1F801060h, RAM, scratchpad, BIOS, EXP1/2/3 windows | P2 |
| `dma.c` | 7 channels, DPCR, DICR, chunked engine, stall rule, OTC | 1F801080h-1F8010FFh | P2 regs, P3 wired |
| `timers.c` | 3 root counters, sync modes, 2-cycle holds, IRQ modes [ARCH.01 C6] | 1F801100h-1F80112Fh | P2 |
| `irq.c` | I_STAT edge latches, I_MASK, the level into CAUSE bit 10 | 1F801070h, 1F801074h | P2 |
| `gpu.c` (+ `gpu_raster.c`, `gpu_display.c`) | GP0 FIFO and parser, E1-E6, GP1 state, VRAM 1 MB, line state machine, busy budget, texture/CLUT cache state, rasteriser, scan-out | 1F801810h, 1F801814h | P3 |
| `spu.c` | 24 voices, ADSR, noise, reverb, capture rings, 512 KB RAM, FIFO, IRQ, CD input queue, mixer | 1F801C00h-1F801FFFh | P1 silence, P4 |
| `cdrom.c` (+ `cdxa.c`) | index model, FIFOs, command FSM, INT queue, seek model, sector buffer, XA decoder and 6:7 resampler, CD-DA, GetID, lid, `.sbi` bad-Q list | 1F801800h-1F801803h | P4 |
| `mdec.c` | command parser, quant and scale tables, IDCT, colour, output FIFO | 1F801820h, 1F801824h | P4 |
| `sio.c` (+ `pad.c`) | SIO0 registers, baud timer, byte scheduler, slot select, pad devices (digital, DualShock with config and rumble) [ARCH.04 §1, §2.9] | 1F801040h-1F80104Fh (SIO1 1F801050h-105Fh: registers only, no device) | P3 |
| `card.c` | two 131,072-byte spans, command FSM, flag byte, generation counters, frame commit | bus address 81h on either slot | P3 |
| `exe.c`, `bios.c` | PS-EXE validation and side-load at the 80030000h hook or through the stub kernel [ARCH.01 C7]; BIOS digest table, stub detection, OpenBIOS acceptance | none | P2 |
| `state.c` | visitor, header, validate, CRC-32 | none | visitor P1, format v3.0 |
| `host/disc_image.c` (`schway_disc_image`, MIT, outside the determinism boundary) | cue/bin, iso, m3u parsers behind the `schway_disc` callbacks; ccd, chd, pbp, ecm in v2.0 | none | P4 |
| `runner/`, `libretro/` | `schwaystation-run` (hashes, probe, mailbox, TTY, first fault, exit table) and the ~350-line `schwaystation_libretro` adapter (XRGB8888, batch audio, RetroPad and analog to `schway_input`, SAVE_RAM = card 1, disk control, options) [ARCH.09 §1, §4] | none | P1 |

Dependency direction, no cycles: `instance → cpu → {cop0, gte, bus}`; `bus → {dma, timers, irq, gpu, spu, cdrom, mdec, sio}`; `sio → {pad, card}`; `dma → {gpu, spu, cdrom, mdec}` through their FIFO entry points; `cdrom → spu` (CD input queue); devices never call `cpu`; interrupt lines are state `irq` holds and `cop0` reads [ARCH.10 §3].
- **Dispatch** is `switch`, never `const` pointer tables (they land in `.data.rel.ro` under PIC and fail `abi.globals` [ARCH.08 §2]): the top 3 address bits select the segment (KSEG2 only answers FFFE0130h; user mode outside KUSEG raises AdEL), the physical address is masked to 29 bits, then one `switch` on address class (RAM, scratchpad, I/O page, EXP2, EXP1, BIOS, EXP3) and inside the I/O page a `switch ((addr >> 4) & 0xFF)` to the owner [ARCH.01 C4]. The two hot cases (RAM, scratchpad) are tested first.
- **Costs.** Each DEVn delay register write recomputes a per-port table {read8, read16, read32, write8, write16, write32} from the formula (N+1 cycles, COM fields, 8-bit ports as 1ST + 3 × SEQ) stored in the instance; RAM read 4 + words (nocash's 6 as the first fixture candidate), RAM write 1 while the 4-deep queue has room [ARCH.01 §3]. The boot values are the profile default so a side-loaded EXE sees retail timing [ARCH.01 C4].
- **I-cache and scratchpad.** The fetch path checks the direct-mapped I-cache (tag = physical bits 31:12, four valid bits per line, fill from the missed word to the line end, Isc and TAG isolation rules); KSEG1 and BIOS fetches bypass it [ARCH.01 §2]. The scratchpad is data only; a fetch from it raises IBE through the exception path [ARCH.08 §10]. Stores never invalidate the cache; the 1 KiB dirty bitmap (2 MB / 1 KiB = 256 bytes) is a block-cache seam, not a hardware model (§8).
- **Unmapped.** Reads and fetches outside any device raise DBE/IBE, writes are dropped; both record the first fault (§6). No open bus exists on this machine [ARCH.01 §3]. Unwritable or partially decoded registers behave per their files; a write the file calls impossible is a first fault, never a crash.

## 3. Public C API
| Function | Contract |
|---|---|
| `schway_get_versions(out)` | library x.y.z, `SCHWAY_API_VERSION`, ABI, behaviour revision, state format, card layout |
| `schway_instance_size(cfg)`; `schway_create(cfg, &inst)`; `schway_create_in(mem, n, cfg, &inst)` | the instance is one fixed-size block (≈ 4 MB, §5); `create` takes it from the config's allocator {alloc, free with size, user}, `create_in` from caller memory (max_align_t aligned, n ≥ `instance_size`); no allocation after `load_bios` (`core.no_alloc_after_load` [ARCH.08 §8]) |
| `schway_destroy(inst)` | frees through the recorded allocator; NULL-safe; `create_in` instances are not freed |
| `schway_set_profile(inst, const schway_profile*)` | region {ntscj, ntscu, pal}, board (§7 table), gpu version, cd firmware and region letter, `ram_fill` {ZERO, SEEDED + u64}, `debug_port` (stub kernel TTY and exit ports), `licensed` override [ARCH.01 C8][ARCH.03 CD-REGION]; rejected after the first frame (`PROFILE_LOCKED`) |
| `schway_load_bios(inst, bytes, n, schway_bios_info*)` | exactly 524,288 bytes, or the own stub kernel (2 KiB, recognised by its magic); SHA-256 reported and matched against the vendored digest table (name-independent); unknown digests accepted with `BIOS_UNKNOWN` flag; OpenBIOS is a known digest; bytes copied; no HLE [ARCH.01 C7][ARCH.09 §1] |
| `schway_probe_exe(bytes, n, schway_exe_info*)` | pure: header fields, lint flags, region marker; never emulates |
| `schway_load_exe(inst, bytes, n)` | validated per [ARCH.04 §7] then staged; applied at the 80030000h hook under a kernel, or at reset under the stub kernel with SP from `s_addr + s_size` or 801FFF00h when 0 [ARCH.01 C7][ARCH.06 PX.68] |
| `schway_open_disc(inst, const schway_disc*)`; `schway_set_lid(inst, open)`; `schway_swap_disc(inst, const schway_disc*)` | the core keeps the callback struct by value and calls it only inside `run_frame`/`run`; `swap_disc` requires the lid open; the TOC is read once at open; disc identity (SHA-256 of the first 16 data sectors plus serial) is recorded for state validation [ARCH.03 DISC-IF][ARCH.04 §5] |
| `schway_load_memcard(inst, slot, bytes, n)`; `schway_get_memory(inst, kind, schway_span*)`; `schway_memcard_generation(inst, slot)` | raw 131,072 bytes (shorter padded FFh, headered formats are the host's conversion); spans RAM, SCRATCHPAD, VRAM, SPU_RAM, BIOS (read-only), CARD1, CARD2, fixed from load to destroy; generation +1 per completed Write ending 47h [ARCH.04 §3.6] |
| `schway_reset(inst, kind)` | POWER applies the §7 power-on table and `ram_fill`; RESET keeps RAM, scratchpad, VRAM and SPU RAM [ARCH.01 C9] |
| `schway_set_input(inst, const schway_input*)` | latched at the next frame start (§1); device per port {NONE, DIGITAL, DUALSHOCK}, buttons, four axes, analog button, lid is not here |
| `schway_run_frame(inst, schway_run_result*)` | one frame (§1); stop {FRAME, FAULT, HOOK}; overshoot cycles; lag flag; rumble out |
| `schway_run(inst, const schway_budget*, schway_run_result*)` | bounded by `max_cycles` and `audio_capacity`; stop {FRAME, BUDGET, AUDIO_FULL, HOOK, FAULT, LID}; `LID` is returned when the drive is asked to read with the lid open and a swap is pending; never sleeps [ARCH.08 §14] |
| `schway_get_video(inst, schway_video*)`; `schway_get_vram(inst, schway_span*)` | the last frame's display image and metadata (§4); the read-only 1024 × 512 u16 VRAM view [ARCH.02 8.A] |
| `schway_get_audio(inst, schway_audio*)` | drains the samples since the last drain into the caller's buffer; undrained samples beyond the ring (4 frames) are dropped and counted |
| `schway_get_info(inst, schway_info*)` | current display geometry, dot divider, fps num/den of the current mode, sample rate 44,100/1, max samples per frame, profile, PAR |
| `schway_state_size(inst)`; `schway_save_state`; `schway_load_state`; `schway_validate_state`; `schway_state_visit(inst, visitor, user)` | constant size from load to destroy; load validates everything before applying any byte; the visitor is public so hosts can hash or diff fields (v3.0 freeze, visitor from P1) |
| `schway_peek(inst, space, addr, width, &value)`; `schway_poke(...)` | spaces BUS, RAM, SCRATCHPAD, VRAM, SPU_RAM; peek has no side effects (no FIFO pops, no acks, no cache fills, no fault record); poke only on RAM, scratchpad, VRAM, SPU RAM |
| `schway_get_cpu/gte/gpu/spu/cdrom/sio_view(inst, out)` | register snapshots for the runner and debuggers; pure |
| `schway_get_counters(inst, out)`; `schway_get_fault(inst, out)`; `schway_get_tty(inst, buf, n, &written)` | §6; TTY drains a 4 KiB ring |
| `schway_set_hooks(inst, const schway_hooks*)` | v5.0; NULL clears; selects the traced loop copy (§6) |
| `schway_status_string(status)` | static text per status, with the milestone for refusals ("unsupported: CHD (planned v2.0)") |
| Helper `schway_disc_image_open(path, allocator, &img)`, `_disc(img)`, `_probe(img, out)`, `_close(img)` (`host/schway_disc_image.h`, MIT, same repository) | parses cue/bin, iso, m3u (disc index selectable) in v1 with the bounds of [ARCH.04 §7]; returns a filled `schway_disc`; opens files, so it lives outside the core and the hash lanes link it only in the runner; ccd, chd (libchdr), pbp, ecm are v2.0 providers behind the same struct [ARCH.03 DISC-IF][ARCH.08 §7] |

```c
#define SCHWAY_API_VERSION 1
typedef uint32_t schway_status;   /* OK, ERR_ARGUMENT, ERR_STRUCT_SIZE, ERR_NO_MEMORY, ERR_BUFFER_TOO_SMALL,
   ERR_BIOS_SIZE, ERR_BIOS_UNKNOWN (warning class), ERR_EXE_INVALID, ERR_EXE_RANGE, ERR_DISC_NONE, ERR_LID_CLOSED,
   ERR_CARD_SIZE, ERR_PROFILE_REJECTED, ERR_PROFILE_LOCKED, ERR_STATE_MAGIC, ERR_STATE_VERSION, ERR_STATE_CRC,
   ERR_STATE_FIELD, ERR_STATE_DISC, ERR_UNSUPPORTED_DEVICE, ERR_UNSUPPORTED_MEDIA ("unsupported: CHD (planned v2.0)") */
typedef struct schway schway;
typedef struct { uint32_t size; schway_allocator allocator; uint32_t audio_ring_frames; } schway_config;
typedef struct { uint32_t size; uint32_t region, board, gpu_rev, cd_firmware, cd_region, ram_fill; uint64_t seed;
                 uint32_t flags; /* DEBUG_PORT, LICENSED, UNLOCK */ } schway_profile;
typedef struct { uint32_t size; uint32_t stop, overshoot_cycles, lag, fault_present; uint32_t rumble[2]; /* large<<8|small */
                 uint64_t frame, ticks; } schway_run_result;
typedef struct { uint32_t size; uint64_t max_cycles; uint32_t audio_capacity; uint32_t flags; } schway_budget;
typedef struct { uint32_t size; const void *pixels; uint32_t width, height, pitch_bytes, bpp; /* 16 or 24 */
                 uint32_t field, interlaced, vres, hres, dot_div, region, x1, y1, start_x, start_y, enabled, rgb24;
                 uint64_t frame; } schway_video;                                  /* fields after size are the metadata tuple */
typedef struct { uint32_t size; int16_t *samples; uint32_t capacity, written, dropped; } schway_audio;
typedef struct { uint32_t size; struct { uint32_t device; uint16_t buttons; uint8_t axes[4]; uint8_t analog_button; } port[2];
                 uint32_t reserved[8]; } schway_input;
typedef struct { uint32_t size; void *user; uint32_t (*read_sector)(void *user, uint32_t lba, uint8_t out[2352], uint8_t sub[96]);
                 uint32_t (*toc)(void *user, schway_toc *out); uint32_t (*bad_q)(void *user, uint32_t lba);
                 uint32_t flags; /* HAS_SUBCODE, LICENSED_STRING */ } schway_disc;
typedef struct { uint32_t size; uint32_t present, kind, device, width; uint64_t frame, ticks; uint32_t line, dot;
                 uint32_t pc, opcode, addr; uint8_t last_cd_command; } schway_fault;
```
| Rule | Detail (sibling conventions [ARCH.11][ARCH.10 §4]) |
|---|---|
| Size tag | every boundary struct starts with `uint32_t size`; input structs below the first released size → `ERR_STRUCT_SIZE`, between first and current → missing fields default to zero, larger accepted only if the extra bytes are zero; output structs: the library writes min(size, its own); `_Static_assert` pins every offset on every target |
| Fields | fixed-width integers; enums as `uint32_t`; no `bool`, bit-fields or pointers into core state except the two read-only views (VRAM, display) which are valid until the next run call; rates as num/den |
| Statuses | append-only; an error changes no state (validate, then apply); a failed load keeps the previous content; a refusal names the milestone in `schway_status_string()` |
| Versions | library x.y.z from `version.txt`; `SCHWAY_API_VERSION` +1 per header addition; ABI = SOVERSION on incompatible layout change (appending to a size-tagged struct is not one); behaviour revision bumps with any hash change; state format and card layout numbers in the state header and `get_versions` [ARCH.08 §5][ARCH.09 §7] |
| Threads | an instance is single-threaded; instances share nothing; callbacks run on the caller's thread inside run calls only |

## 4. I/O contracts
| Stream | Contract |
|---|---|
| Video | per frame boundary: (a) `get_vram` = the whole VRAM as `uint16_t[512][1024]`, the hash domain for ps1-tests images [ARCH.02 §6]; (b) `get_video` = the display frame: width `(((X2 - X1) / div) + 2) & ~3`, height Y2 - Y1 rows of the current field, pixels `uint16_t 0BBBBBGGGGGRRRRR` (bpp 16) or packed 3 bytes R, G, B per pixel (bpp 24, §13 row 6), row-major at `pitch_bytes`, plus the metadata tuple {field, interlaced, vres, hres, dot_div, region, X1, Y1, start_x, start_y, enabled, rgb24, frame}; display off → all zero with `enabled = 0` [ARCH.02 8.A]. Interlaced output is field-sequential at native height; weaving, bobbing, borders (ares nominal offsets), aspect and filters are host helpers outside the hash [ARCH.09 §8]. The libretro adapter converts to XRGB8888 [ARCH.09 §1]. PAR = dot divider and region, reported not applied. |
| Audio | `int16_t` stereo interleaved at exactly 44,100 Hz: the SPU output after main volume and final saturation, CD-DA/XA and reverb included, one pair per 768-cycle tick; per frame the count is the ticks that fell inside the frame (737/738 NTSC, 886/887 PAL); never padded or resampled; the host resamples [ARCH.03 OUT, §6] |
| Input | `schway_input` latched at frame start; pads are polled by the game through SIO0, so a change shows when the game polls; DualShock axes 0-255 centre 128; `analog_button` is an edge the pad device detects; lag frame = no SIO0 transfer to a pad on either port during the frame, peeks excluded [ARCH.09 §5] |
| Rumble out | per frame `{large 0-255, small 0/1}` per port in `schway_run_result` [ARCH.04 §2.4] |
| Disc | `schway_disc`: `read_sector(lba)` returns 2352 raw bytes plus 96 subcode bytes (or none, flag), `toc` fills up to 99 tracks {type, index0, index1, pregap}, `bad_q(lba)` answers the `.sbi` list; the core synthesises Q when no subcode, calls `read_sector` only inside run calls (a slow host stalls the frame, never the drive), reads at most 16 sectors ahead into its ring [ARCH.03 §6, DISC-IF] |
| Memory cards | two spans of 131,072 bytes fixed from load to destroy, writable by the host before the first frame; generation counters; the core does no file I/O; the libretro adapter maps SAVE_RAM to card 1 and writes `<content>.1.mcr` itself [ARCH.04 §3.6][ARCH.09 §1] |
| TTY | bytes from the kernel putchar entry (PC = A0h/B0h with r9 = 3Ch/3Dh) and, with `debug_port`, byte writes to 1F802080h; a write to 1F802082h ends the run with `HOOK` and the byte as `exit_code`; 4 KiB ring drained by `get_tty`; never printed by the core [ARCH.06 PX.68][ARCH.09 §5] |

## 5. State
- **Layout.** The instance is one struct of fixed-size plain-integer sub-structs (no pointers, no bit-fields); the state body is the visitor's byte-wise little-endian walk of those fields, not a memcpy [ARCH.11][ARCH.04 §4].
- **Visitor.** One function per struct runs as SIZE, WRITE, VALIDATE then APPLY, plus RECORD, which emits every field's bounds so a test can sweep out-of-range values; VALIDATE walks everything first so APPLY cannot fail; pointers and FIFO indexes are re-derived after APPLY [ARCH.10 §7]. Exists from P1 (the test card round-trips); format frozen in v3.0 [ARCH.04 §8].
- **Public visitor.** `schway_state_visit(inst, visitor, user)` calls `visitor(user, const char *path, uint32_t kind, void *bytes, uint32_t n)` once per field in WRITE order with a dotted path (`gpu.vram`, `cpu.r[3]`, `cdrom.int_queue`); hosts use it for field-level diffs and partial hashes; the core's own save, load, size and hash paths are the same walk with four internal modes, so a field cannot exist in one and not the other.
- **Header**, 64 bytes: 8-byte magic; u16 format version; u16 header size; u32 total size; u32 behaviour revision; u32 API version; u32 packed profile; 16-byte disc identity prefix; u32 BIOS digest prefix; u32 body CRC-32; u64 frame; u64 ticks; u32 flags (WITH_CARDS); 4 reserved. A state with another disc identity loads only with `ERR_STATE_DISC` overridden by a flag; ticks ≤ 2^62.

| Module | Contributes (bytes, derived) |
|---|---|
| cpu, cop0, gte | GPRs, HI/LO and completion tick, PC, delay records, load queue, I-cache tags and words (4 KB + 1 KB tags), write-queue count; COP0 r0-31; GTE 64 registers, busy counter: ≈ 6,000 |
| bus, irq, dma, timers | ticks, gpu_acc, next_deadline, memory control and cost table, RAM 2,097,152, scratchpad 1,024, I_STAT/I_MASK, 7 channels, DPCR/DICR, `dma_busy_until`, 3 counters: ≈ 2,099,000 |
| gpu | VRAM 1,048,576, FIFO 16 words and the owed-word parser state, E1-E6, GP1 fields, line state (hcounter, vcounter, field, dot phase), budget, CLUT cache tag: ≈ 1,049,000 |
| spu | 512 KB RAM, 24 voices (counters, envelopes, history), reverb address and ring, noise, FIFO, capture index, CD input queue (two sectors), sample phase: ≈ 535,000 |
| cdrom, mdec | pending command, parameter and result FIFOs, INT queue, status, mode, seek target and end tick, current MSF, sector buffer 8 × 2,352, XA history and six-step counter, lid, disc index, last command; MDEC tables, FIFOs, block state: ≈ 30,000 |
| sio, pad, card | registers, FIFOs, scheduled byte, slot, pad mode/config/mapping/watchdog, card command phase, flag, pending 128-byte frame: ≈ 600 |
| profile, counters, fault | §3 profile, §6 counters, first-fault record: ≈ 300 |
| cards | 2 × 131,072 only with WITH_CARDS (§13 row 5) |
| total | ≈ 3.72 MB; ≈ 3.98 MB with cards; `state.bytes` ceiling 4,194,304 [ARCH.08 §14] |

- **CD and disc-swap state.** The disc bytes are never inside the state; the state holds the disc identity, the disc index (m3u position, supplied by the host at `open_disc`), the lid flag, the TOC digest and every byte the controller already fetched (sector buffer, ring). On load the host must have the same disc open or `ERR_STATE_DISC` is returned before any byte applies.
- Not saved: host input not yet latched, hooks, trace ring, caller buffers, the audio ring (drained before save by the runner; the adapter saves between frames when it is empty).

## 6. Observability
- **Per-line record** of the last frame (≈ 8 bytes × 314, in the state): display enable, GPU busy stall cycles, DMA2 words, timer 0/1 IRQ flags, the SIO transfer flag; the runner's `--trace lines` prints it and the ledger's `boots@` predicate reads display enable from it.
- **Counters** (always on, in the state, reset by POWER): per frame `instructions`, `icache_misses`, `bus_faults`, `exceptions` by code, `irqs` by bit, `dma_words` per channel, `gp0_words`, `primitives` by class, `pixels_written`, `gpu_budget_stalls`, `spu_irqs`, `cd_commands`, `cd_sectors`, `sio_transfers`, `lag_frames`, `audio_dropped`, `tty_bytes`. **Flip** to traced builds only if Cachegrind shows > 1 % [ARCH.08 §8].
- **First fault**, from P1 [ARCH.07 §4]: the first event since reset classed as impossible or unmodelled: unmapped read/write/fetch, misaligned access, reserved instruction, cop unusable outside COP0/COP2, an unimplemented GP0 or CD command, a DMA sync mode a channel cannot use, a write to a read-only register with side effects, or the same EPC taken by an exception 1,000 times. Record `schway_fault` {kind, device, width, frame, ticks, line, dot, pc, opcode, addr, last_cd_command}; the runner prints `fault frame=N line=L dot=D pc=... op=... addr=... width=... dev=... kind=...` [ARCH.09 §4]. Recording is one branch on cold paths.
- **TTY** per §4; the runner prefixes `tty `.
- **Trace** (`--trace first-fault` keeps the last 4,096 records in a ring; `--trace cpu` streams), one tab-separated line per record: `frame line dot cycle pc opcode event addr width device value note`, events `exec rd wr irq dma gp0 gp1 cd spu fault`, written by one function so tests assert its shape [ARCH.07 §4].
- **Hooks ABI (v5.0)**: `schway_hooks` {size, user, `on_instruction(user, pc, opcode)`, `on_bus(user, kind, addr, width, value)`, `on_dma`, `on_irq`, `on_line`, `on_frame`, `on_tty`}; each may return STOP, which ends the run with `HOOK`. `cpu_exec.inc` is compiled twice (`SCHWAY_TRACE` 0 and 1); the traced copy is chosen once per run call only when hooks are installed; proof: Ir/frame within 64 of a `SCHWAY_TRACE_LOOP=OFF` build [ARCH.08 §8].

## 7. Determinism and hashes
Integer-only, no mutable globals, byte-wise LE serialisation, no punning or packed structs, hashes byte-identical on six native legs plus `be32` and `wasm32`; every intentional hash change bumps the behaviour revision [ARCH.08 §2, §9]. Everything below is a profile field, lives in the instance and the state, and is named in movie headers [ARCH.01 C8, §9].

| Profile field | Values (default first) | Source |
|---|---|---|
| `region`, GPU clock | ntscu (53,693,175), ntscj (53,693,175), pal (53,203,425) | [ARCH.01 §1] |
| `board` and derived `cpu`, `gpu_rev`, `spu`, `cd_firmware`, `ram_ctrl_boot`, ports | pm41 default: cxd8606, v2, cxd2938, vC3, 0B88h; v0 rows per [ARCH.02 1.A] | [ARCH.01 C8] |
| `cd_region`, `licensed`, `unlock` | from region; GetID compares the disc's licence string; `licensed` forces the licensed answer; `unlock` accepts the vC1+ sequence | [ARCH.03 CD-REGION] |
| `ram_fill`, seed | ZERO; SEEDED(u64) by splitmix64 | [ARCH.01 §9] |
| Power-on | SR 00400000h, PRID 2, BPCM/BDAM FFFFFFFFh, PC BFC00000h; memory control at maximum delays then the boot values under the stub; DPCR 07654321h; timers 0 with bit 10 set; I_STAT/I_MASK 0; GPUSTAT 14802000h; beam line 0, gpu_acc 0 | [ARCH.01 §9][ARCH.02 §1.1] |

| Hash | Definition | Excluded |
|---|---|---|
| Frame | SHA-256 over the metadata tuple serialised as 13 LE u32 (field, interlaced, vres, hres, dot_div, region, x1, y1, start_x, start_y, enabled, rgb24, bpp) followed by the display rows: LE u16 per pixel (bpp 16) or 3 bytes per pixel (bpp 24), no padding | borders, weaving, XRGB conversion, the VRAM outside the display area (its own `--hash-vram` digest exists for ps1-tests) |
| Audio | SHA-256 over the int16 LE stereo stream exactly as emitted (L, R per sample), accumulated across the frames of a run | host resampling, volume, dropped-sample recovery |
| State | SHA-256 over the state body as WRITE emits it (header excluded, so frame and ticks are inside the body copy, CRC is not) | cards unless WITH_CARDS, hooks, trace ring, audio ring |

Guest memory is little-endian throughout (RAM, VRAM halfwords, SPU RAM, PS-EXE, sectors), so every guest access goes through `rd16le`/`rd32le`/`wr16le`/`wr32le` helpers over byte arrays and never through a host-endian pointer cast; the `be32` lane is where a violation surfaces [ARCH.08 §9].
Hostile inputs and their bounds are [ARCH.04 §7]'s table; the fuzz targets are [ARCH.08 §3]'s (entry points renamed `schway_*`). Property tests [ARCH.08 §3]: `run(N)` then `run(M)` equals `run(N+M)`; save, load, continue equals uninterrupted; peeking all spaces leaves the state hash unchanged.

## 8. Execution engine
**Rec** an interpreter whose shape already obeys [ARCH.08 §10]'s constraints, so the v1.x cached interpreter and a JIT seed add a block cache and a code generator without touching the semantics. **Alt** a plain fetch-decode-execute loop (faster to write, must be rewritten). **Flip** none.
| Element | Design |
|---|---|
| Decode | `cpu_decode(word, paddr) -> schway__cpu_op` {op index u8, rs, rt, rd, shamt, imm16 sign-extended, target, flags {ends_block, is_load, is_cop2}}: a pure function; `exec` takes the record and never re-reads RAM |
| Block loop | `cpu_run_until(deadline)`: fetch through the I-cache model, decode, execute, charge cycles, until `block_end()` is true: after the delay slot of any branch or jump, SYSCALL, BREAK, RFE, a COP0 write, a 4 KiB page edge, 128 instructions, or `pending_event` set (an I_STAT/I_MASK/SR change, a deadline hit, a DMA stall) |
| Cycle accounting | per instruction into a block total; the deadline is checked at block ends and before every load or store that leaves RAM/scratchpad; `ticks` is advanced at those points, so a block cache can charge the same totals |
| Delays | load-delay queue (target, value, ready) applied after the next instruction; a write to the pending target cancels it; LWL/LWR pair interlock [ARCH.06 PX.26]; branch delay record with `BD` for `trigger()`; MFC2/CFC2 share the queue [ARCH.02 §5.6] |
| Mul/div, GTE | HI/LO `ready_tick`: an early read stalls to it (6/9/13, 36); GTE `busy_until`: a COP2 access stalls [ARCH.01 §2][ARCH.02 §5.2] |
| Exceptions | `cop0_trigger(code)` exits the block with exact EPC, BD, BT and the target; the GTE-in-delay-slot quirk executes the COP2 op inside the path [ARCH.01 §2] |
| Interrupts | CAUSE bit 10 computed from I_STAT & I_MASK at every instruction boundary while `pending_irq` is set (2-instruction delay as ares [ARCH.01 §6]); the flag also ends blocks so a cache never runs through an acceptance point |
| Self-modifying code | every RAM store sets a bit in the 1 KiB dirty bitmap; the cached interpreter invalidates blocks on dirty pages before the next fetch; the I-cache model stays in the fetch path for every mode |
| Scratchpad | data only; the block cache never maps it |
| Equivalence | `core.exec_mode_equivalence` compares every fixture's hashes between modes; the hashed path stays the interpreter until green on all eight lanes |
Dispatch is one `switch` per opcode class in `cpu_exec.inc`; COP0 and COP2 ops are cases that call `cop0.c`/`gte.c`. The decoder is a pure function so the vector runner [ARCH.07 §1.1] can execute single records: `schway__cpu_step_one(inst, &bus_samples)` (test-only entry, internal namespace) loads a vector's initial state including both delay latches, runs one instruction with a flat-RAM bus and records every fetch, read and write with size, address, value and the cycle it fell in. Nothing in the vector path is a second decoder.

## 9. GPU rasteriser and display
- **Parser → record.** `gpu_parse` consumes GP0 words into a `schway__gpu_prim` {kind, flags (gouraud, textured, raw, abe, quad, polyline, size), up to 4 vertices {x, y s16, r, g, b u8, u, v u8, sub_x, sub_y, w u32 (zero in v1)}, clut, page, size}; environment E1-E6 are applied at parse; the record is rendered when its last word arrives [ARCH.02 7.A]. The vertex slot for sub-pixel data and W and the one-function `gpu_backend(prim)` entry are the enhancement seams [ARCH.02 §8].
- **Triangle.** Reject when the span exceeds 1023 × 511; make clockwise, drop zero area; three edge functions in int32 (11-bit coordinates, products < 2^23) with the top-left bias [ARCH.02 §3.6]; bounding box clipped to the draw area after the offset; per row the inner span start/end come from stepping the edges (no per-pixel edge test); attribute setup once per triangle: one integer reciprocal of twice the area (32-bit, area < 2^21) gives dR/dx, dR/dy, dG, dB, dU, dV in 32-bit fixed point with 16 fractional bits (adjusted until `uv-interpolation` and `triangle` match pixel for pixel [ARCH.02 3.A]); per pixel: three adds, texel fetch, dither table lookup, blend, mask test, one 16-bit store [ARCH.02 §9]. No division and no 64-bit arithmetic in the span.
- **Variants.** One `static inline` span body with constant flag parameters, called from eight sites (texture, raw, abe, dither), so the compiler specialises without macros [ARCH.08 §10].
- **Pixel pipeline** in the hardware order: texel (4/8-bit via the CLUT cache row, 15-bit direct; 0000h discarded), modulation `(texel5 × colour8) >> 7` (v0 crops colour), dither 4 × 4 table added to 8-bit channels then `>> 3` for gouraud or modulated polygons and all lines, never rectangles [ARCH.02 §3.1-3.4]; blend per 5-bit channel in the four modes (width settled by `transparency`/`quad`, §13); mask: skip when destination bit 15 set and PBC, write with bit 15 = PBW | texel bit 15.
- **Lines** as integer DDA over the major axis, both ends inclusive, always dithered; **rectangles** as a separate span loop with per-pixel UV increment and the v2 flip [ARCH.02 §3.7-3.8]; **fills and copies** with the exact masking of [ARCH.02 §2.3], copies wrapping at the VRAM edge.
- **Busy budget.** Setup constant plus pixels × a per-class table seeded from the ares-derived column, charged after rendering, drained on `gpu_clock`; bits 26 and 28 read 0 and DMA2 stalls while unspent [ARCH.02 4.A]. Unknowns (lines, clipped pixels) are a locator, not a gate, until a RESULTS row exists [ARCH.06 PX.19].
- **Scan-out** at the frame origin: `gpu_display` copies the display area from VRAM into the instance's frame buffer (640 × 512 × 3 bytes max) honouring GP1(05h) start, the X1/X2 width formula, Y1/Y2 height, the dot divider, 24-bit packing (3 bytes per pixel straight from VRAM bytes) and, when interlaced, the rows of the current field; display off writes zeros. The metadata tuple is filled at the same instant. GPUSTAT bit 31 follows the ares rule [ARCH.02 §4.1].
- VRAM is one `uint16_t[512][1024]` in the instance; textured spans walk rows with page base and window masks hoisted; the 4 × 4 × 256 dither table and the CLUT row cache are per instance [ARCH.02 §9].

| Later enhancement | Seam kept in v1 (never in the hashed path, `policy.no_enhancement_in_core` [ARCH.08 §8]) |
|---|---|
| Internal resolution, PGXP-style precision, perspective-correct UV | `schway__gpu_prim` keeps `sub_x`, `sub_y`, `w` per vertex (zero in v1); `gte.c` exposes pre-truncation MAC0/IR and SZ per vertex through a debug sideband keyed by the SXY FIFO; `gpu_backend(prim)` is one function a host renderer can replace by consuming the record stream through a hook [ARCH.02 §8] |
| True colour, texture filtering | the pixel write path takes 8-bit channels and truncates in one place; the texel fetch is one function taking fractional UV (integers in v1) |
| Deinterlacing, CRT shaders, widescreen | field bit and parity in the metadata; the VRAM view; native PAR in `schway_info`; the OFX/H projection is one function |
| Light guns (v2.0) | `gpu_display` can record the (line, dot) of each emitted pixel and its brightness behind a flag [ARCH.04 §2.7] |

## 10. SPU, CD-ROM, MDEC and audio delivery
- **SPU** runs one integer tick per 768 cycles from the scheduler, never per cycle: voices 1-24 in hardware order (BRR decode with the three-sample history carried across blocks, gaussian table, ADSR step table, pitch modulation, noise), capture rings, reverb at every second tick with the documented formula and FIR, the CD input queue, mixing in int32 with saturation at the documented points, IRQ checks at every RAM access; `spu_catch_up` before any register access [ARCH.03 SPU-MODEL, SPU-INTERP, SPU-MIX]. Writes are applied at the tick [ARCH.03 §1.9]. Each tick pushes one stereo pair into the audio ring (4 frames deep).
- **CD-ROM** is a command FSM clocked in cycles only: acknowledge 50,000, Init second 476,000, Pause second 2,170,000 >> speed (7,666 when paused), INT1 every 451,584 >> speed, seek by the integer sqrt model in 1/75 s ticks; no randomness [ARCH.03 CD-TIME]. A sector arrival calls `read_sector` through the ring (16 ahead), applies the Setmode delivery order (XA to the decoder, data to the buffer, filter check), decodes XA with the 7-phase resampler and queues 44.1 kHz pairs (two sectors deep, underflow = silence) [ARCH.03 §2.3, §2.5, §6]; GetID answers from the profile and the disc's licence string; the lid raises the unsolicited INT5 and `swap_disc` installs a new TOC [ARCH.03 §2.6]. LibCrypt: `bad_q(lba)` makes GetlocP return the previous good position [ARCH.03 §2.7].
- **MDEC** decodes one block per 1000 cycles from its input FIFO (fed by DMA0 in 20h-word blocks), integer two-pass IDCT with Q14 scale values and the ares rounding, Q16 colour constants, packs 4/8/15/24-bit output into the output FIFO for DMA1 [ARCH.03 MDEC-IDCT]. The FMV path DMA3 → DMA0 → DMA1 → DMA2 is only DREQ plumbing in `dma.c`.
- **Volumes.** CD-DA and XA pass the CD controller's ATV matrix (saturating per term) then the SPU's CD input volume, so Mute/Demute and ATV writes are cycle-exact at the sector they affect; the SPU main volume and sweeps apply last [ARCH.03 §1.8, §2.5].
- **Delivery.** `run_frame` leaves the frame's pairs in the ring; `get_audio` drains into the caller buffer; `run` with `audio_capacity` stops with `AUDIO_FULL` so a handheld or the browser page can run once per audio period [ARCH.08 §14]. The audio hash taps the ring at push time, so dropping samples at the host never changes it.

## 11. Build order for v1.0
| Phase | Tracer first (one path through every layer) | Then widen to | Done when |
|---|---|---|---|
| P1 A test frame in RetroArch | `testcard.c` → frame buffer and metadata → `schwaystation-run --hash` → `schwaystation_libretro` → hosted RetroArch screenshot [ARCH.09 §3] | six legs, `be32`, `wasm32`, asan, nofp, hygiene; visitor round trip; first-fault record; `hello.exe` assembled on every leg; `--probe` of a PS-EXE; attested release; install line | runner and adapter hashes equal on eight lanes; screenshot equals `--png` |
| P2 The CPU matches the vectors | `cpu_decode` + `cpu_exec.inc` against the committed SST sample through the bus-sample hook → `cop0` exceptions → `gte` against the 1150-case table [ARCH.07 §1] | `bus.c` decode, costs, I-cache, faults; `irq`, `timers`, `dma` registers; `exe.c` under the stub kernel; `exc_matrix`, `gte_vectors`; nugget cpu/cop0/gte nightly; `fuzz.exe`, `fuzz.state`; first-fault line | `ctest -L vectors -L gte -L cpu` green on eight lanes; `--trace first-fault` prints for a crafted fault |
| P3 Games play in RetroArch | one triangle from a unit test → `gpu_motion` through DMA2 lists → `pad_counter` with SIO0 and the DualShock → `memcard_roundtrip` → RetroArch session [ARCH.07 §2] | rasteriser variants, fills, copies, lines, rectangles, mask, 24-bit, interlace; timers on dot/hblank (`timers_lines` per profile); DMA stall and chopping; `card.c` spans and generations; OpenBIOS built in CI and booting to its shell; the ledger and ratchet; the `library` preset with the owner's dumps | fixture frame, mailbox and movie predicates pinned on eight lanes; boot-to-play in RetroArch; ledger rows for the fixtures |
| P4 Sound and discs | `spu_tone` PCM digest from the SPU alone → `cdrom.c` FSM against the ps1-tests timing log → `disc_image` cue/bin → `disc_boot` through OpenBIOS → libretro audio and m3u disc control | XA, CD-DA, reverb presets, capture rings, `mdec.c` against ps1-tests PNGs, `.sbi`, `fuzz.cue/cdrom/spu/mdec`, PCSX-Redux + OpenBIOS locator nightly | audio and frame hashes of `disc_boot` equal on eight lanes; a game disc from the library preset plays with sound in RetroArch |

Module dependencies per phase: P1 needs only `instance`, `testcard`, `state`, runner, adapter; P2 adds `cpu cop0 bus irq exe bios` (the stub kernel lands with `bios.c`); P3 adds `gte dma timers`; P4 adds `gpu sio pad card` and wires `dma` to `gpu`; P5 adds `spu`; P6 adds `cdrom cdxa mdec` and `host/disc_image`. Within a phase, plans that share no inventory run as parallel waves (opcode groups; `bus`/`timers`/`irq`; rasteriser/sio/card; spu/cdrom/mdec) [ARCH.10 §11]. Each phase ends in a release [ARCH.08 §5].

## 12. Performance posture
- **Budgets** [ARCH.08 §8]: Ir/frame per PR on three fixtures (test card ≤ 40 M, GTE/GPU ≤ 150 M, CD+SPU ≤ 120 M initial ceilings, replaced by measured +1 %); device rows M1 100 M, Pi 4 19 M, A53 12.5 M; the ecosystem target is ≤ 30 M Ir per NTSC frame on BIOS boot plus three titles [ARCH.05 §4]. Frame work: 566,204 CPU cycles, 897,619 GPU clocks, 737 SPU samples × 24 voices per NTSC frame (§1).
- **Hot paths, in order:** the instruction loop (fetch through I-cache, decode, `switch`, bus cost), RAM access (two compares then a load), the GPU span (textured gouraud quads dominate at ≈ 3 GPU clocks per pixel on hardware [ARCH.02 §4.4]), the SPU voice loop (24 × 737 per frame), BRR and XA decode, the deadline check. Keep each in one struct of contiguous fixed-width fields; no function pointers in the loop; no 64-bit arithmetic in GPU spans (wasm32) [ARCH.02 §9].
- **Memory**: instance ≈ 3.7 MB plus the 640 × 512 × 3 frame buffer and the 16-sector ring; one 4 MB state buffer at the host; a 32 MB wasm32 linear memory with no growth and no threads holds all of it [ARCH.08 §14]. `size_t` arithmetic for state and span sizes is done in `uint32_t` with overflow checks (ILP32 on `be32` and `wasm32`).
- **Measured where:** `perf` job (Cachegrind, ubuntu x64, GCC 15) per PR; nightly `bench` tails on three hosts; `core.no_busy_loop` proves an idle drive and SPU cost a fixed bound; `core.exec_mode_equivalence` once the cached interpreter exists; `quality.json` ceilings `ir_per_frame.<fixture>`, `state.bytes`, `size.wasm` [ARCH.08 §8, §14].

## 13. Open conflicts and how each is settled
| # | Conflict | Positions | Settlement | File(s) that must change |
|---|---|---|---|---|
| 1 | CHD in v1 | HOSTS §1 and §4 accept `chd` and list it in the `.info`; SCM §3 DISC-IF puts CHD via libchdr in the v1 helper; CONFORMANCE §3.3 parses `.chd` in the library preset; ENGINEERING §7 ships v1 without CHD | no CHD in v1: the adapter and runner refuse `.chd` with "unsupported: CHD (planned v2.0)", `ERR_UNSUPPORTED_MEDIA`, exit 77; `.info` lists `cue|bin|img|iso|m3u|exe|psexe`; the library preset registers cue/bin only until v2.0 | HOSTS §1, §4; SCM §3; CONFORMANCE §3.2-3.3 (applied 2026-10-19) |
| 2 | Line length and reported fps | SYS C2 emulates 3413/3406 but reports rates from 3412.5/3405.02; HOSTS §1 reports 2,250,000/37,609 and 15,625/314; GPU §4.1 lists 59.826/49.761 | emulate 3413/3406 and report the emulated rates 53,693,175/897,619 and 53,203,425/1,069,484 (field rates in §1), so fps and samples per frame agree exactly and host DRC sees no drift; the nominal figures stay in docs as hardware nominal; the hardware counter test in SYS §10 decides the constants later | SYS C2 report clause; HOSTS §1 AV table; GPU §4.1 note (applied 2026-10-19) |
| 3 | Samples per frame | SCM OUT 6,283,333/8,448 (11/7 clock); HOSTS 737.14/886.23; ENGINEERING §14 ≈ 735 | 527,799,972/715,909 = 737.244 NTSC, 628,856,592/709,379 = 886.489 PAL (§1) | SCM §1.9, §6; HOSTS §1; ENGINEERING §14 (applied 2026-10-19) |
| 4 | CPU cycles per frame | ENGINEERING §8 ≈ 565,000/677,000; ECO §4 566,120 | 566,203.775 NTSC, 680,823.456 PAL (§1) | ENGINEERING §8; ECO §4 (applied 2026-10-19) |
| 5 | Cards inside states | PER §4 includes them; ECO C.06 "cards never inside states"; HOSTS §1 says loading a state can roll a card back | outside by default; `WITH_CARDS` flag includes them for TAS; `card.c` commits a frame only when the Write command completes, so a mid-write state stays consistent | PER §4 (applied 2026-10-19) |
| 6 | 24-bit frame encoding | GPU 8.A converts to u32 00RRGGBB; HOSTS §1 says 3 bytes per pixel | 3 bytes per pixel, straight from VRAM bytes (lossless, smaller, one copy) | GPU 8.A (applied 2026-10-19) |
| 7 | Runner exit statuses | HOSTS §4: 0 done, 1 failure, 2 usage, 3 hash, 4 save size, 5 state, 6 fixture, 77; CONFORMANCE §4: 0 pass, 1 predicate, 2 first fault, 3 hang, 77, 78 | HOSTS' table plus 7 first fault, 8 hang, 78 infrastructure error; one X-macro table | CONFORMANCE §4 (applied 2026-10-19) |
| 8 | Mailbox placement | HOSTS §4: 16 bytes at 1F8003F0h, magic `SCHW`; CONFORMANCE §2 and §4: 256 bytes at `t_addr` 80010000h, magic `SWMB` | CONFORMANCE's (the stub kernel uses the scratchpad top for its header copy and exception record [ARCH.06 PX.68]) | HOSTS §4 (applied 2026-10-19) |
| 9 | Symbol prefixes | ENGINEERING §2, §10 and HOSTS §7 use `schwaystation_` public and `schwaystation__` internal; the kickoff settles `schway_` and module prefixes | public `schway_`, internals `schway__<module>_` (the module prefix lives inside the namespace so static libretro builds cannot collide); libretro option keys and file names keep `schwaystation`; `abi.symbols` checks both | ENGINEERING §2, §3, §10; HOSTS §7; PER §3.6, §7 (applied 2026-10-19) |
| 10 | Stop reasons | ENGINEERING §14 names six; HOSTS §7 names six others | one enum: FRAME, BUDGET, AUDIO_FULL, HOOK (breakpoint, mailbox, tty, exit port), FAULT, LID | ENGINEERING §14; HOSTS §7 (applied 2026-10-19) |
| 11 | MDEC in v1 | CONFORMANCE §8 leaves it to ARCHITECTURE | in v1, P4, after CD-ROM and SPU | CONFORMANCE §8 (applied 2026-10-19) |
| 12 | Avocado and Rustation licences | the kickoff brief called them MIT; GPU §6, SCM §0, ECO §1.1 found GPL-2 | never opened; PROVENANCE records both | PROVENANCE (records both as GPL-2, applied 2026-10-19) |
| 13 | Blend arithmetic width, size rejection 1023 vs 1024, stepper precision | GPU §10 open | settled by the named ps1-tests images in P4; until then the core follows psx-spx (5-bit, 1023/511) | none; GPU §10 records the result (applied 2026-10-19: no edit needed) |
| 14 | Fuzz target names | ENGINEERING §3 `fuzz.cue exe cnf memcard state gpu cdrom spu mdec`; PER §7 `fuzz.syscnf psexe card card_import sio chd sbi ppf cheat m3u`; CONFORMANCE §6 `gp0 cd` | ENGINEERING's nine names for v1 plus `fuzz.sio` (random bus bytes into pad and card devices, cheap and in scope); `sbi`, `m3u`, `card_import`, `ppf`, `cheat`, `chd` arrive with their features | PER §7; CONFORMANCE §6; ENGINEERING §3 gains `fuzz.sio` (applied 2026-10-19) |
| 15 | Stub kernel debug ports | PSXDEV PX.68 proposes 1F802080h/1F802082h (unverified port, outside the default 128-byte DEV8 window) | decoded by the core only under the profile flag `debug_port`, independent of DEV8 size, never a fault; retail profiles leave the flag clear | PSXDEV-EXTRACTS PX.68 (applied 2026-10-19) |

## Sources
| ID | Tier | Source | Pin or access date | Supports |
|---|---|---|---|---|
| ARCH.01 | T1 | `.planning/preparation/PSX-SYSTEM.md` (this project's research) | 2026-10-19 | tick, ratio, line counts, CPU, bus, DMA, timers, interrupts, BIOS, profiles, power-on, C1-C9 |
| ARCH.02 | T1 | `.planning/preparation/PSX-GPU-AND-GTE.md` | 2026-10-19 | GPU commands, rules, timing, GTE, oracles, frame format, rasteriser notes, choices 1.A-8.A |
| ARCH.03 | T1 | `.planning/preparation/PSX-SPU-CDROM-AND-MDEC.md` | 2026-10-19 | SPU, CD, MDEC models, DISC-IF, CD-TIME, HASH, OUT, SYNC |
| ARCH.04 | T1 | `.planning/preparation/PSX-PERIPHERALS-AND-SAVES.md` | 2026-10-19 | SIO0, pads, cards, state constraints, identity, hostile bounds |
| ARCH.05 | T1 | `.planning/preparation/PSX-ECOSYSTEM-AND-USERS.md` | 2026-10-19 | complaints, horizons, device budgets |
| ARCH.06 | T1 | `.planning/preparation/PSXDEV-EXTRACTS.md` | 2026-10-19 | farm facts, LWL/LWR interlock, stub kernel ports, toolchain pins |
| ARCH.07 | T1 | `.planning/preparation/CONFORMANCE.md` | 2026-10-19 | vectors, fixtures, mailbox, first fault, trace record, phases |
| ARCH.08 | T1 | `.planning/preparation/ENGINEERING.md` | 2026-10-19 | C rules, lanes, Ir gate, fast-path constraints, stop reasons, state ceiling |
| ARCH.09 | T1 | `.planning/preparation/HOSTS-AND-INTEGRATION.md` | 2026-10-19 | libretro contract, runner, probe, TTY, lag frame, C API groups |
| ARCH.10 | T1 | cogmaster `.planning/preparation/ARCHITECTURE.md` (owner's sibling; section structure, deadline dispatcher, visitor, build-order and conflict-table patterns) | `bc256f2c1bb7` | structure and conventions; no SMS fact reused |
| ARCH.11 | T1 | muhtari `include/muhtari.h`, `src/internal.h`, `src/instance.c`, `src/bus.c` (owner's sibling; size tags, first-released sizes, `_Static_assert` layout pins, allocator hook, `next_deadline`, run_frame shape) | `ecbe7f52aa37` | API conventions; no 7800 fact reused |
| ARCH.12 | T1 | `.planning/preparation/BRIEF.md` and the kickoff's settled choices (names, tick, integer-only, interpreter seams, video, audio, disc, BIOS, cards, input, observability, six phases) | 2026-10-19 | the constraints every section builds on |
| ARCH.13 | T1 | Arithmetic run here (Python `fractions`): 263 × 3413 × 451,584/715,909, 314 × 3406 × 451,584/709,379, the field rationals over 525/625 lines, samples per frame, overrun 36 + 29 + 44 = 109 | 2026-10-19 (tested) | §1 rationals, §12 frame work, the overrun bound |
