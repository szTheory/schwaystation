# Phase 2: The CPU matches the public vectors - Context

**Gathered:** 2026-11-09
**Status:** Ready for planning

<domain>
## Phase Boundary

The R3000A matches the public vectors before any game runs. The phase delivers the CPU interpreter (decode, execute,
load and branch delay, MULT/DIV timing), COP0 and exceptions, the bus (physical decode, memory control, I-cache,
write queue, scratchpad), the I_STAT/I_MASK registers, PS-EXE loading under an own stub kernel, `--probe`, and the
first-fault trace. Requirements: CPU-01 to CPU-05. Success criteria: ROADMAP.md, Phase 2.

Out of scope here (phase 3 or later): the GTE, DMA channels, root counters, GPU, SIO, SPU, CD-ROM, MDEC,
`schway_load_bios`, OpenBIOS, and the nugget and amidog nightly lanes (never named by CPU-01 to CPU-05; they are
seeds that return with the first fixture that needs them).

How this context was made: seven independent research agents each took one gray area through every role lens, with
an adversarial pass. This context locks their synthesis, as the owner's fan-out method asks. Where two reports
disagreed, the resolution is recorded below with its reason (D-30 to D-35). Nothing here reopens a DEC row.

</domain>

<decisions>
## Implementation Decisions

### Unmodelled hardware (gray area A)
- **D-01:** Documented-but-unbuilt devices are **"unbuilt"**, not faults and not a register file. Addresses: DMA
  1F801080h-10FFh, timers 1100h-112Fh, SIO 1040h-105Fh, CD 1800h-1803h, GPU 1810h-1817h, MDEC 1820h-1827h, SPU
  1C00h-1FFFh. Any width: reads return 0, writes drop. One `switch` case per device calls
  `schway__bus_unbuilt(dev, addr, width, wr, val)`. Misalignment still raises AdEL/AdES before the bus. Memory
  control (1000h-1020h, 1060h) and I_STAT/I_MASK are real in this phase. The ARCHITECTURE section 2 "P2" rows for
  dma and timers are superseded by CPU-03. — **Reversibility:** reversible — each later phase deletes its own cases.
- **D-02:** True gaps stay DBE/IBE and a first fault (DEC.26): I/O page holes, KUSEG/KSEG2 holes, a fetch from the
  scratchpad. **EXP1 and EXP3 reads are gaps** (ares agrees); EXP2 answers only the two `debug_port` addresses
  1F802080h/82h and every other EXP2 address is a gap. Writes to a gap are dropped and are a first fault.
- **D-03:** A device enum `schway__dev` {RAM, SCRATCH, BIOS, MEMCTL, IRQ, DMA, TIMER, CDROM, GPU, MDEC, SIO, SPU,
  EXP1, EXP2, EXP3, GTE, UNMAPPED} names the `dev=` field of the fault line. Unbuilt accesses are counted
  (`unbuilt_n[dev]`, saturating u32) and the last one is kept (`last_unbuilt`: dev, addr, width, pc); both live in
  the state. The runner prints one `unbuilt dev=.. n=.. last_pc=..` line in the first-fault footer and on exit 8,
  which is what explains a hang polling GPUSTAT. No `--stop-on-unbuilt` flag (rule 8).
- **D-04:** COP2/GTE before phase 3: SR.CU2 clear raises CpU with CE=2 for COP2 commands, MFC2/CFC2/MTC2/CTC2, BC2,
  LWC2 and SWC2 (the address-error check comes first for LWC2/SWC2). CU2 set: MFC2/CFC2 write 0 through the normal
  load-delay queue; MTC2/CTC2 and commands are no-ops; LWC2 does the real bus read and discards it; SWC2 does the
  real bus write of 0; each counts as unbuilt `dev=gte`. `gte.c` replaces this in phase 3 with no CPU change, since
  decode already carries `is_cop2`. COP1 and COP3 with their CU bit clear raise CpU; **only COP1/COP3 latch a first
  fault** (ARCHITECTURE section 6 words it "outside COP0/COP2"); a CpU on COP0/COP2 is an architectural exception a
  guest handler may take.
- **D-05:** Pinning tests: `tests/core/bus_unbuilt.c` sweeps 1F801000h-1F801FFFh at widths 1, 2, 4 and requires every
  address to be owned, unbuilt (no fault, reads 0) or a gap (DBE and a fault); `tests/cpu/cop2_prephase.c` covers
  CU2 clear and set; a runner test shows an EXE writing GPU and DMA registers exits 0 with an unbuilt footer and a
  write to 1F801030h exits 7; one fuzz seed hammers unbuilt and gap addresses.

### Stub kernel and boot path (gray area B)
- **D-06:** The stub is real MIPS, assembled from `tests/fixtures/src/stub/stub.asm` by the pinned armips and
  committed as a **`uint8_t` byte array in `src/stub_kernel.c`** (core-owned; see D-30 for why not `host/`). A test
  `fixtures.stub_drift` assembles the stub on each native leg and `memcmp`s it with the array. The image digest is a
  row in `hashes.txt`. Cross lanes (`be32`, `wasm32`) use the array and need no armips. The file is ASCII text,
  listed in `tests/fixtures/manifest.txt` (licence MIT, source `stub.asm`, sha256 of the `.c`); PROVENANCE's stub row
  gets the image digest. — **Reversibility:** costly — the array layout is read by the loader and by phase 4's BIOS path.
- **D-07:** With no BIOS image the core maps the stub at BFC00000h and resets there (the real-MIPS path: memory
  control writes, BEV clear after the vectors are copied, handler install). Reads or fetches past the stub image
  inside the BIOS window are DBE/IBE and a first fault (a runaway never slides through a nop sled). The real-BIOS
  80030000h hook belongs to phase 4 (GAME-05); its first caller adds `schway_load_bios`.
- **D-08:** Launch block: 16 bytes in kernel RAM (0..FFFFh) holding `pc0`, `gp0`, `s_addr`, `s_size`, written by
  `schway_load_exe`. The stub reads it, sets `gp`, sets `sp = s_addr + s_size` (801FFF00h when `s_addr` is 0 — one
  value for every path; ARCHITECTURE C7's 801FFFF0h loses) and jumps to `pc0`. The scratchpad is never touched by
  the stub, so fixtures keep all 1 KB. The block address is chosen from the stub's own layout and documented in
  `docs/hardware/cpu.md`; the planner checks it does not clash with the vector copies.
- **D-09:** TTY: the stub implements A(3Ch) and B(3Dh) putchar as real MIPS (about 12 instructions each) writing to
  1F802080h; no C hook at A0h/B0h. `--tty` sets `schway_profile.flags & SCHWAY_PROFILE_DEBUG_PORT`; clear (retail,
  libretro) means the writes are silently ignored. Exit port 1F802082h: byte 00h gives exit 0, any other byte gives
  exit 6, and the raw byte is printed.
- **D-10:** Default exception handler: the stub installs one at 80000080h that records Cause, EPC, BadVaddr and SR
  in kernel RAM (not the scratchpad; PX.68's 1F800300h is dropped) and sets mailbox status FFh; it jumps to the
  mailbox word at 0Ch when that is non-zero. A fixture may overwrite the vector. `exc_matrix` overrides it and the
  stub's default stays untested by that fixture, so a separate `hello`-class fixture or unit test pins the default.
- **D-11:** Mailbox: the first 256 bytes of the loaded image at `t_addr` (80010000h), magic `SWMB`. `--stop-on
  mailbox` stops at a frame end when magic is `SWMB` and status is 00h (exit 0) or FFh (exit 6). A CRC mismatch in a
  mailbox record is a fixture failure, exit 6. No valid magic by the frame cap is a hang, exit 8.

### First-fault trace and fault classes (gray area C)
- **D-12:** `schway_fault` {kind, device, width, frame, ticks, line, dot, pc, opcode, addr; `last_cd_command` joins in
  phase 6} lives in the instance **and the state** (ARCHITECTURE section 6), latches only while `kind == NONE` (first
  fault wins), on cold paths only. Phase 2 adds it with a state-format and behaviour-revision bump (`src/` has no
  fault record today). — **Reversibility:** one-way — a state-format bump and a published struct.
- **D-13:** Latched kinds: `unmapped_read`, `unmapped_write`, `unmapped_fetch`, `misaligned_read`,
  `misaligned_write`, `misaligned_fetch`, `reserved_instruction`, `cop_unusable` (COP1/COP3 only, D-04),
  `exception_loop`. Never faults: SYSCALL, BREAK, overflow, interrupts, and the LWL/LWR/SWL/SWR unaligned idioms.
  An unmapped access that is a normal DBE/IBE exception a guest handler takes still latches (the guest cannot
  recover a bus error by design on this machine).
- **D-14:** Exception loop = the same EPC and ExcCode taken 1,000 consecutive times, counter reset by any different
  EPC; ExcCode 0 (interrupts) is excluded. **Double fault:** an IBE raised while fetching from an exception vector
  stops the run at once with stop reason FAULT (ARCHITECTURE section 13 row 10) and a fault record.
- **D-15:** Trace tail: the `cpu_exec.inc` loop is **dual-compiled** (`SCHWAY_TRACE` 0 and 1, DEC.24). The traced copy
  is chosen once per run call only when the budget requests tracing, and writes a 4,096-record ring that lives **in
  the instance, outside the state and every hash** (D-31). One function, `schway__trace_write_record`, formats both
  `--trace first-fault` and (later) `--trace cpu`: 12 tab-separated fields (`frame line dot cycle pc opcode event addr
  width device value note`), `0x%08x` hex, `device=none` for CPU kinds; the last line of a first-fault tail is a
  `fault` event taken from the fault record. The `fault frame=N line=L dot=D pc=... op=... addr=... width=...
  dev=... kind=...` line uses the kind enum's names; exit 7. Exception-loop faults carry the loop count in `note`.
  The records include rd/wr events, so the tail spans about 1.5k to 2.5k instructions; this is documented, not fixed.
  The position fields come from block-start ticks plus accumulated cycles. Replay-based capture (run, then re-run
  with the ring) is deferred and bound to the first plan that runs more than a few hundred frames.
- **D-16:** `--stop-on-fault` (budget flag `SCHWAY_RUN_STOP_ON_FAULT`) is on by default in the runner; `exc_matrix`
  runs with it off and asserts the latched record, which gives classification coverage for free. `hang` is runner
  only: no new core stop value; the frame cap is reached with no done status, the runner prints `hang frames=N
  pc=...` plus the same tail, exit 8.
- **D-17:** Tests pin it: a golden line per trace event; an integration EXE that runs more than 4,096 records of
  preamble then faults once per class (exit 7, tolerant regex `^fault frame=`, a tail of exactly 4,096 twelve-field
  lines, last event `fault`); `core.trace_hash_equal` (state, frame and audio hashes identical with tracing on and
  off); a `j self` EXE exits 8. The Ir "traced build within 64 of untraced" proof is a v5.0 seed, not phase 2.

### Vector harness and sample (gray area D)
- **D-18:** The test-only entry `schway__cpu_step_one` lives in the cpu module, calls the same executor as the real
  run path and reaches the bus through its one bus interface, so there is no second decoder. It ships in the static
  archive (`abi.symbols` already allows `schway__` names; no writable data); test executables link
  `schwaystation::schwaystation` and include `src/` as `tests/core` does. No `SCHWAY__TESTING` define, no second
  build of the executor.
- **D-19:** The committed sample is **verbatim upstream records**, the first 100 of each of the 55 files at
  51e227447d0b, count patched to 100: 5,500 cases, 2,394,288 bytes (measured). This replaces the SWR3 transcode of
  CONFORMANCE 1.1 and its `sst2bin.py`: one C parser serves the sample and the nightly files, `cmp` against upstream
  proves no transcoding bug, and case names survive for triage. `tools/sst_slice.py` (about 30 lines, `python3 -I`)
  is a maintainer tool that CI never runs. The container for 55 slices in the single file `sst-sample.bin`
  (CPU-01's name) is the planner's choice (a u16 count, then a u32 length per slice is enough). Mark the file
  `binary` in `.gitattributes`; list it in `tests/fixtures/manifest.txt`. — **Reversibility:** costly — committed
  bytes and a harness parser.
- **D-20:** Pins are blob SHA-1s: `pins.txt` rows name each file's git blob SHA-1 and size, fetched with
  `gh api repos/SingleStepTests/r3000/git/blobs/<sha>` (rule 12). Nightly `vectors-full` fetches the 55 blobs,
  verifies each with `git hash-object`, runs the same harness, and checks that the first 100 records of each full
  file equal the committed sample, so no Python runs in CI. Nothing is committed or uploaded.
- **D-21:** Parser: explicit little-endian shifts over a byte buffer, never casts. Reject a short or long file, a
  name longer than 50 bytes, a cycle count over 8, a case count over 1,000, a val or addr high dword that is not 0;
  assert R0 is 0. The zero-case guard: the harness exits 1 when it parsed fewer cases than the expected count, a
  CMake argument (5,500 or 55,000); its negative control runs an empty or truncated file under `expect_fail.cmake`
  (this is also todo 023's regression). `allow.txt` is `YYYY-MM-DD <file> <case-hex> <upstream-issue-URL>`, starts
  empty, and an unmatched line is an error.
- **D-22:** Bus comparison: the harness bus is a replay bus with no RAM; each access is compared with `cycles[k]` in
  list order. Reads and fetches return the recorded value, writes are compared with it, and extra or missing cycles
  fail at the end. A failure prints file, case name, cycle index and field. The I-cache, scratchpad and MMIO are
  bypassed; both delay latches are loaded from the case; **SR is set to 0** (without it 20 to 25 percent of ADD/SUB
  cases fail mysteriously, because overflow does take the exception in these vectors).
- **D-23:** Two named harness rules, neither an `allow.txt` line: (a) misaligned LH/LHU/LW/SH/SW (310 of the 5,500
  cases) are no-ops with a fetch cycle in the vectors, but the core raises AdEL/AdES, so the harness derives
  misalignment from opcode and effective address and asserts the exception (vector 80000080h, ExcCode 4 or 5,
  fetch-only bus) instead of the final state; (b) LWL/LWR/SWL/SWR are split into 1, 2 or 4 byte chunks upstream, so
  the harness coalesces them to the covered byte set. If the executor cannot emit the chunk pattern, a bus-hook field
  is added, not a flag in the shared path. A pass is weak evidence (ares generated the set, PX.37): the vectors never
  justify a delay-edge, interlock or exception behaviour beyond what the project's own tests show.
- **D-24:** The preparation text is wrong in three places and the plan that lands the harness fixes it in the same PR:
  CONFORMANCE 1.1 says about 420 B per case and 2.3 MB (measured 431 B per case at one cycle); PX.41 labels the
  state tail backwards (after the six registers come branch target, slot, take, then load target and value); and
  CONFORMANCE 1.1/1.1a under-claim coverage, since overflow, BREAK and SYSCALL do take the exception in the vectors
  (address errors are the part that is not covered as exceptions).

### Exceptions and interrupts (gray area E)
- **D-25:** Interrupt latency **model A (ares)**: CAUSE bit 10 is the live AND of I_STAT and I_MASK, updated at the
  write that changes it, with no delay on reads. A false-to-true transition of (IP & IM & IEc) sets a retirement
  stamp; the exception is taken 2 retirements later for a device edge, an I_MASK write or an SR write, and 1 later
  for a CAUSE write; RFE takes effect immediately. The stamp is a field set on the transition, never a side effect
  of a query. `doc` rows in `exception_tests`: `mtc0` CAUSE gives EPC = pc+4; `mtc0` SR enabling a pending IRQ gives
  EPC = pc+8; an I_MASK write gives EPC = pc+8. Any change is a dated `allow.txt` line. The 2 for device edges and SR
  writes is ares' inference, unmeasured on silicon (nugget uses nops).
- **D-26:** `exc_matrix` CAUSE row: `mfc0` right after an I_MASK write or I_STAT clear reads bit 10 already updated,
  provided no I/O write is still queued (the fixture drains the queue with a KSEG1 read first). The pinned model is
  "the write commits inside its own instruction"; whether silicon shows the old value while the write is queued is
  the open question, and the row stays `doc`. **I_STAT source in this phase:** the frame driver raises I_STAT bit 0
  (vblank) at the vsync edge that ends `schway_run_frame`, so the fixture and IRQ tests have a real source before
  the GPU exists; the phase-4 line machine takes over the same edge. Unit tests use an internal `schway__irq_raise`.
- **D-27:** Entry rules: a delay-slot exception gives BD=1, BT=taken, TAR=target, EPC=the branch (a not-taken branch
  gives BD=1 and TAR=fall-through); interrupts return to EPC+0; entry discards pending delay-slot state; a CAUSE
  write in a delay slot is taken after the slot with BD=0. BadVaddr updates only on AdEL/AdES. CE is set only for
  CpU (ExcCode 0Bh). With CUz set, BCzF is always taken and BCzT never. BEV=1 vectors at BFC00180h, else 80000080h.
  SR writable mask F04FFF3Fh; RFE shifts bits 2-5 into 0-3. CAUSE bits 8-9 only are writable. Tier: T1 nugget results
  for these, T2 psx-spx otherwise; the unverified ones (CE on non-CpU exceptions, ares copying bits 27:26) stay `doc`.
- **D-28:** Delays and mul/div: a pending load is cancelled by any write to the same register and completes before an
  interrupt handler runs; a failed load leaves rt untouched; LWL/LWR are interlocked; MFC0 has one delay slot.
  MULT classifies on the rs XOR sign, MULTU on raw rs (6/9/13 cycles by magnitude; DIV/DIVU 36); a MULT or DIV
  issued while busy replaces the running one (the stall is the new operation's own time); `mtlo`/`mthi` set the
  busy stamp to 0 and only that stall is pinned. HI/LO and the stamp survive exceptions. The GTE double-execute bug
  gets a hook in the interrupt entry path now and is implemented and pinned in phase 3 with the GTE.
- **D-29:** Cache control: 256 lines of 4 words, per-word valid bits, fill from the missed word to line end, IBLKSZ
  ignored (as ares); an isolated load returns the line's word whether or not the tag matches; with TAG+IS1 a store
  writes the tag and its low 4 data bits become the valid bits. The write queue is 4 deep, the fifth write stalls,
  a KSEG1 access drains it; pin depth and drain, with timing as `doc`. A write to I_STAT/I_MASK or `mtc0` SR/CAUSE
  ends a block (the DEC.20 mid-block exit) so a future cache never runs through an acceptance point.

### Conflicts between reports, and how they were resolved
- **D-30:** Report B put the stub bytes in `host/` and loaded them through `schway_load_bios`; that function is not
  in the v1 header and its first caller is phase 4. **Resolution: the stub lives in `src/stub_kernel.c`, inside the
  core.** It is our own MIT code, not BIOS bytes; the runner, the libretro adapter and the wasm host all get it with
  no extra call; and phase 2 adds no function without a caller (rule 7). Everything else in B stands.
- **D-31:** Report C put the trace ring in the runner as a hook consumer; the hooks ABI is a v5.0 seed and freezing
  callbacks now would be premature. **Resolution: the ring lives in the instance, outside the state and all hashes,
  filled by the traced loop copy, and the runner reads it through `schway_get_trace`** (F's missing accessor).
  Cost: about 128 KiB of instance memory, fixed, no allocation after load.
- **D-32:** Reports B and F differed on staging an EXE. **Resolution: no staging buffer.** `schway_load_exe`
  validates fully, then writes the text segment into RAM and the launch block, after reset and before the first
  frame; a reset erases it and the host loads again. Phase 4 decides how the same EXE reaches the 80030000h hook of a
  real BIOS (a staging area may be needed there; the signature does not change).
- **D-33:** `fuzz.exe` and `fuzz.regress.exe` stay report-only until SEED-007 promotes them (CPU-05), and the
  nightly `fuzz` preset is new in this phase.
- **D-34:** Mailbox read cadence: B said each run slice, F said once per frame. **Resolution: once per frame** via
  `schway_peek` (RAM space), so the core has no mailbox stop reason.
- **D-35:** CONFORMANCE says the stub installs no handler; PX.68 and the roadmap say it does. **Resolution:** D-10.

### Public API and runner (gray area F)
- **D-36:** `SCHWAY_API_VERSION` becomes 2, additive; SOVERSION stays 1 (appending to size-tagged structs is not a
  layout break). Statuses are append-only: `ERR_EXE_INVALID` 11, `ERR_EXE_RANGE` 12. Stop numbers are locked at
  FRAME 1, BUDGET 2, AUDIO_FULL 3, HOOK 4, FAULT 5, LID 6; define 2, 4 and 5 now and 3 and 6 when something first
  returns them. `tests/api/v1.c`'s two assertions become `>= 1` with its comment amended; `tests/abi/tests.cmake`
  gains a v2 target; `tests/api/v2.c` touches every new name. — **Reversibility:** one-way — a published ABI.
- **D-37:** New functions (13 becomes 20): `schway_probe_exe(bytes, n, schway_exe_info *)`, `schway_load_exe(inst,
  bytes, n)`, `schway_run(inst, const schway_budget *, schway_run_result *)`, `schway_peek(const schway *, space,
  addr, width, uint32_t *)` (spaces RAM 1 and SCRATCHPAD 2), `schway_get_tty(inst, schway_tty *)` (a pull ring of
  4 KiB with a `dropped` counter, not in the state or any hash), `schway_get_fault(const schway *, schway_fault *)`,
  and `schway_get_trace` (D-31). New structs: `schway_exe_info` (pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr,
  b_size, s_addr, s_size, `licence_area` 0 none, 1 Japan, 2 Europe, 3 North America, `lint`, `hash_len` = t_size +
  0800h), `schway_budget` (size, max_cycles u64, flags), `schway_fault`, `schway_tty`, `schway_trace`. Appends:
  `schway_profile.flags` with `SCHWAY_PROFILE_DEBUG_PORT`, `schway_run_result.exit_code` and `fault_present`;
  `SCHWAY__PROFILE_SIZE_FIRST` becomes a literal 8. No `schway_poke`, `schway_get_memory` or `schway_control`.
  `schway_peek` takes `const schway *`, so the compiler enforces that it records no fault, bumps no counter and
  touches no TTY; a test hashes the state before and after across all addresses and widths.
- **D-38:** The loader validates in u64 before writing any byte: `n` in 0800h..0800h + 2 MiB; `t_size` a multiple of
  0800h and at most `n - 0800h`; `t_addr` word-aligned, in KUSEG/KSEG0/KSEG1, with `(t_addr & 1FFFFFFFh) + t_size <=
  200000h`; `pc0` non-zero and word-aligned; `sp` zero or inside RAM. Overlap of the low 64 KiB and a longer-than-header
  file are lint flags, not rejections. `src/exe.c` holds one validator that `schway_probe_exe` and `schway_load_exe`
  share. sha256, md5 (a pinned permissive source), `serial` (`n/a` for a bare EXE), `region_guess` and `exe_name`
  live in `runner/probe.c`. `host/` is untouched.
- **D-39:** Runner exit table (X-macro, `runner/exit_status.h`): add 6, 7 and 8 now; a hostile or invalid EXE exits 1;
  exit 3 waits for `--expect`. `schway_run_frame` keeps its semantics (stops at FRAME or HOOK). The exit port ends a
  run with HOOK and `exit_code`.
- **D-40:** Fuzz: `LLVMFuzzerTestOneInput` with `-max_len` 2 MiB + 2 KiB, nightly `-fsanitize=fuzzer,address,undefined`
  with the clang-21 already on the CI image; a dependency-free replay `main` seeded from muhtari `replay_main.h`
  (pinned to a public commit, verified with `gh api`); seeds are built in memory by `tests/fuzz/seeds.c` (valid,
  `t_addr` at the RAM end, wrap, KSEG2, `pc0` 0, `t_size` 0, off-by-one sizes) so nothing enters the manifest; only
  minimised crashers are committed, as hex text. No AFL++ or honggfuzz.

### Gates, docs and the fixture toolchain (gray area G)
- **D-41:** `policy.gate_budget` is re-pinned at plan time to **at most 1,520** (measured today: exactly 1,392 with
  zero headroom). Its parts: about 12 for the fixture rows in `hashes.cmake` (named by CPU-04), 2 for `budgets.cmake`,
  and the folded todos 007 and 008 (D-45), whose seed gates the planner measures from the cogmaster pin. If the
  measured total would pass 1,520, the planner drops todo 008, then 007, back to deferred, never raises the cap
  (XFER-085). Ratcheted to the measured total at ship. `tools/*.py` do not count while CI never runs them.
- **D-42:** Named new gates, each tied to a requirement: `fixtures.armips_pin.self_test` (CPU-04; script mode against
  a `file://` fixture with a wrong digest, through `expect_fail.cmake`, no network); the vectors zero-case guard
  (CPU-01); the trace-shape test (CPU-05); `tests/api/v2.c` and `fixtures.stub_drift` (CPU-04). Deferred as seeds: a
  sample-SHA-256 check, `policy.fixture_budget`, a planning-ID scan of docs and code (review the new hardware pages by
  hand for `[CF.nn]`, `PX.nn`, `XFER`), `policy.timeout_scaled`, and a general `fixtures.tool_pin`.
- **D-43:** armips at `62adab4ef30d`: one `pins.txt` row (full 40-character archive URL and SHA-256).
  `cmake/Armips.cmake` uses `file(DOWNLOAD ... EXPECTED_HASH)` and `file(ARCHIVE_EXTRACT)` so a wrong digest fails
  configure and no POSIX shell is needed on Windows, then builds a host-side `ExternalProject` with
  `-DARMIPS_USE_STD_FILESYSTEM=ON -DARMIPS_LIBRARY_ONLY=OFF`. The `ExternalProject` must not inherit
  `CMAKE_OSX_DEPLOYMENT_TARGET=10.13` (std::filesystem needs 10.15), the `cl` C-compiler setting or warnings-as-
  errors. Built on the six native legs plus `be32` and `wasm32` (host-native ubuntu, never cross-compiled); the
  hygiene, nofp and asan lanes skip it. The `hello.exe` image digest and its `--exe hello.exe --frames N --hash` line
  go into `hashes.txt`, so `hash-equality` still compares eight files. No caching in this phase (measure in SEED-007).
  **Plan 1 builds armips on the hosted legs first**, as a probe before dependent plans (phase 1's 01-14 pattern),
  because MSVC on Windows x64 and arm64 is unproven upstream (its CI covers only windows-2022 x64).
- **D-44:** Docs: land `docs/hardware/cpu.md`, `cop0.md` and `bus.md` (the bus page also covers memory control,
  I-cache, scratchpad and I_STAT) and update `runner.md`, `testing.md` (CI job rows, todo 020) and `maintaining.md`
  (armips pin bump). `budgets.cmake` also globs `docs/*/*.md` at 61,440 bytes (it globs only `docs/*.md` today).
  Code comments cite these pages, never a planning ID. The header stays terse; README is 3.7 KB of 16 KB.
- **D-45:** Todos folded into this phase because their first caller is here: 007 (exit-table check, first exit 6 and 7),
  008 (`docs.api_names`, first public names after phase 1), 019 (dev-preset fortify allowlist, if a plan edits
  `schway_core_options` or `abi.cmake`), 020 (testing.md CI rows), 023 (see D-46). 022 stays pending (a Windows
  flake to triage, not a gate).
- **D-46:** Todo 023 is closed by wrapping `schway_add_test(PASS_REGEX)` in `expect_fail.cmake -DCODE=0`, which already
  compares exit status and regex (0 counted lines, in `cmake/SchwaystationTests.cmake`); its regression test goes in
  `tests/policy/tests.cmake`: a command that exits 1 and prints the matching text must fail.
- **D-47:** Labels and inventories: directories `cpu`, `bus`, `vectors`, `fuzz`, `exe` and `fixtures`, each with an
  `inventory.txt` registered in `tests/inventory/ci.txt`; labels `cpu`, `bus`, `vectors`, `fuzz`, `fixtures`, with
  `portable` on every deterministic test. `fuzz.regress.exe` runs on every leg with no skip. A nightly inventory
  `tests/inventory/nightly.txt` holds `vectors-full`; it is report-only for its first 10 runs on `main`
  (CONFORMANCE section 5) and has a `pins.txt` row.
- **D-48:** `workflow.test_gate_timeout` (4 seconds) bounds the GSD post-merge run of `cmake --workflow --preset ci`
  and was derived from a fresh 1.62 s run. The armips build (about 7 s on an 18-core machine) makes a fresh phase-2
  run longer, so it is **re-derived at phase 2 ship** as twice a measured fresh run, written with GSD's config
  writer under an empty `GSD_HOME`; do not carry 4 forward. The `build` job timeout comments are raised from measured
  runs in the `hygiene.timeout_comments` format.
- **D-49:** mkpsxiso is deferred to phase 6 and is **GPL-2.0 with five submodules** (checked with `gh api`), not MIT as
  the brief says. Its licence does not reach its output (CONFORMANCE 7) and rule 1 forbids copying GPL code, not
  running a tool, but DEC rows and PROVENANCE need a correction and an owner-visible note at phase 6 discuss. The
  armips submodule text is also wrong: at the pin only `ext/filesystem` is a submodule, `ext/tinyformat` is vendored
  inside the tarball, and with std::filesystem on neither needs a second fetch. Fix CONFORMANCE section 2 and PX.50
  in the PR that adds `cmake/Armips.cmake`.

### Plan-time amendments (2026-11-09, recorded while planning)
- **D-33 amended:** `fuzz.regress.exe` is an ordinary gating CTest on every leg, because AGENTS rule 14 says no test is
  non-gating and D-47 already says it runs on every leg with no skip. Only the nightly `fuzz` campaign (a time-boxed
  libFuzzer job) is report-only. CPU-05's "report-only until a tune-up promotes them" is read that way; plan 03 edits
  CPU-05's wording to match. SEED-007 promotes the nightly campaign, not the regress replay.
- **D-37 layout note:** `schway_budget` is laid out `{size u32, flags u32, max_cycles u64}` (first-size 16), not the
  order D-37 lists, because it has no padding on any lane. Field names, widths and meaning are unchanged.
- **D-22 amended (measured against upstream ADDU, 1,000 of 1,000 cases):** the initial state of every vector case also
  carries non-zero EPC, TAR and CAUSE, and the final comparison reads them, so the harness loads EPC, TAR and CAUSE
  together with the registers, HI, LO, PC and both delay latches before the single step. A state record is 172 bytes
  with 4-byte tail fields (not the 166 of CONFORMANCE 1.1). The plan names the next-PC model the harness uses.
- **D-47 amended:** `vectors-full` is a nightly job outside the `CI required` aggregate, so it reports without
  blocking a merge; it has no run counter (rule 8 forbids lifetime counters) and promotion is a hand edit. The `exe`
  label and the `tests/diag/` directory (the `exc_matrix` rows) are part of the inventory set. Every deterministic
  test, `vectors.r3000.full` included, carries `portable`.
- **D-06 vs D-43 reconciled:** cross lanes (`be32`, `wasm32`) run the committed stub array, and D-43 still builds armips
  on them to assemble the fixtures; the two are consistent, D-06's "need no armips" means the stub array does not.
- **Rule 8 reading for hosted waits:** no plan caps hosted fix-and-watch attempts by a count; each further attempt
  names the new evidence it acts on, and the session is bounded by time and tokens.

### Claude's Discretion
- The container layout of the 55 slices in `sst-sample.bin` (D-19); the launch-block address (D-08); record sizes of
  the fault and trace structs; the order of plans and waves (rule 14: plan 1 lands the CMake skeleton and the
  per-directory inventories; later plans run in parallel waves where they share no inventory or header, so
  `cpu`/`cop0`, `bus`/`irq`, `exe`/stub/runner and the fixtures toolchain are natural waves); how `schway_trace`
  records are laid out for copy-out; which md5 source seeds `runner/md5.c`.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Rules and settled choices
- `AGENTS.md` — project rules (rules 1, 3, 4, 7, 11, 12, 14 matter most here)
- `.planning/preparation/DECISIONS.md` — DEC.19 to DEC.26 (CPU), DEC.11, DEC.49, DEC.75, DEC.81, open questions table
- `.planning/LESSONS.md` — re-read at every discuss and plan step (XFER-005, XFER-085, XFER-100)
- `.planning/REQUIREMENTS.md` — CPU-01 to CPU-05, FRAME-05, FRAME-09
- `.planning/ROADMAP.md` — Phase 2 goal and success criteria

### Hardware and design
- `.planning/preparation/PSX-SYSTEM.md` sections 2, 3, 6, 7, 9, 10 — R3000A, COP0, memory map, interrupts, boot, power-on
- `.planning/preparation/ARCHITECTURE.md` sections 2, 3, 5, 6, 8, 11, 13 — modules and bus, API, state, observability, engine, conflicts (rows 7, 8, 10, 15)
- `.planning/preparation/PSXDEV-EXTRACTS.md` — PX.13, PX.25, PX.26, PX.31, PX.37, PX.41 (state tail labels are wrong, see D-24), PX.50, PX.53, PX.64, PX.66, PX.68
- `.planning/preparation/CONFORMANCE.md` sections 1, 1.1, 1.1a, 2, 4, 5, 7 — vectors, fixtures, mailbox, first-fault format, CI tiers
- `.planning/preparation/ENGINEERING.md` sections 2, 3, 4, 8, 10, 11 — C rules, fuzz, CI, perf, docs graduation
- `.planning/preparation/HOSTS-AND-INTEGRATION.md` sections 3, 4, 7 — runner options and exit table
- `ASSET_POLICY.md`, `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md` — what may be committed and noticed

### External sources named by the research (fetch with `gh api` at a public pin; never open GPL emulators)
- SingleStepTests/r3000 at 51e227447d0b (README, `transcode_json.py`, the 55 blobs) — MIT
- Kingcom/armips at 62adab4ef30d — MIT; nugget tests `cop0`, `cop0-sr`, `cop-branch`, `sr-write`, `mult-timing` (MIT)
- ares (ISC): `ares/ps1/cpu/` (`exceptions.cpp`, `interpreter-scc.cpp`, `interpreter-ipu.cpp`, `delay-slots.cpp`, `icache.cpp`, `memory.cpp`, `write-queue.cpp`) and `ares/ps1/interrupt/interrupt.cpp`
- szTheory/muhtari `tests/fuzz/replay_main.h` and `seeds.c` (replay and seed pattern); szTheory/cogmaster at f09ac97096ac (`tests/cmake/exit_table.cmake`, `api_names.cmake`)

### Bound todos
- `.planning/todos/pending/007-exit-table-gate.md`, `008-api-names-gate.md`, `019-dev-preset-fortify-helpers.md`, `020-testing-doc-rehearsal-rows.md`, `023-test-helper-exit-status.md`

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/instance.c`, `src/internal.h`, `src/state.h`, `src/state.c`: the instance, profile lock, state visitor and
  header (20-byte header, CRC-32) that the CPU, bus, fault record and counters join; `src/frame.c` and `src/testcard.c` hold the frame driver the vblank edge (D-26) extends.
- `runner/`: `main.c`, `exit_status.h` (the X-macro table), `sha256.c`, `crc32.c`, `png.c`, `bench.c`, `stop.c` — the runner gains `--exe`, `--tty`, `--stop-on`, `--probe`, `--trace`, `--frames` capped runs.
- `tests/cmake/expect_fail.cmake`, `hashes.cmake`, `line_budget.cmake`, `budgets.cmake`, `check_junit.cmake`, `cmake/SchwaystationTests.cmake`: negative controls, hash-equality, the gate ceiling and docs budgets.
- `tests/core/` (`test_core.c`, `test_state.c`): the pattern for white-box tests that include `src/`.
- `pins.txt`, `scripts/fetch-pin.sh`, `cmake/toolchains/`: pin handling and the `be32` and `wasm32` toolchains.

### Established Patterns
- Dispatch by `switch`, never a `const` pointer table (DEC.24; tables become `.data.rel.ro` under PIC and fail `abi.globals`).
- All guest memory goes through `rd16le`/`rd32le`/`wr16le`/`wr32le` helpers over byte arrays; the `be32` lane is where a violation shows.
- Every behaviour change lands with a test that `cmake --workflow --preset ci` runs (rule 5); every gate has a negative control; no skipped test.
- Hashes are identical on six native legs plus `be32` and `wasm32`; an intentional hash change bumps the behaviour revision.

### Integration Points
- `src/bus.c` is the only place guest addresses are decoded; the stub (D-06 to D-11), `schway__cpu_step_one` (D-18) and the traced loop (D-15) all reach memory through it.
- Phase 3 replaces the unbuilt cases (D-01) and the COP2 no-ops (D-04); phase 4 adds the BIOS hook and `schway_load_bios`; phase 6 re-opens the mkpsxiso licence note (D-49).
- The Ir/frame gate measures the test card, which never runs the CPU loop, so CPU-loop cost is invisible to it until a CPU-bound fixture exists; a CPU-bound perf fixture is a seed bound to the first plan that adds one (phase 3's `gte_vectors`).

</code_context>

<specifics>
## Specific Ideas

- The owner's method: fan out per decision, consider every role lens and an adversarial pass, prefer primary sources,
  and "another copy and paste is better than another dep". Applied: the sample is verbatim upstream bytes with one C
  parser and no converter, the stub bytes are copied into the core rather than built by a dependency, and the fuzz
  replay `main` is copied from the owner's own muhtari.
- Measured while researching: the gate total is exactly 1,392; armips builds in about 7 s on an 18-core Mac (51 CPU-s,
  61 translation units); the SST set is 55 files, 55,000 cases, 23,946,132 bytes, 431 B per one-cycle case.

</specifics>

<deferred>
## Deferred Ideas

- Replay-based first-fault capture (run, then re-run with the ring) — bound to the first plan that runs more than a few hundred frames.
- The nugget and amidog nightly lanes — return with the first fixture that needs them (phase 3 or 4).
- A CPU-bound Ir/frame perf fixture and the "traced build within 64 Ir" proof — seeds, bound to phase 3's first fixture and the v5.0 hooks.
- A `--stop-on-unbuilt` flag; armips build caching; a general `fixtures.tool_pin`; a sample SHA-256 gate; a planning-ID scan; `policy.timeout_scaled` (D-42).
- `schway_poke` and `schway_get_memory` — their first callers are cheats and RetroAchievements (phase 4 or later).
- How an EXE reaches a real BIOS's 80030000h hook, and `schway_load_bios` — phase 4.
- mkpsxiso GPL-2.0 correction and owner note — phase 6 discuss (D-49).
- The GTE double-execute bug — implemented and pinned in phase 3 (D-28).

### Reviewed Todos (not folded)
- 022 Windows dll_crt consumer flake: a test bug to triage, not part of CPU scope.
- 001-006, 009-018: not bound to phase 2 (their first callers are later or are CI-process gates); unchanged.

</deferred>

---

*Phase: 02-the-cpu-matches-the-public-vectors*
*Context gathered: 2026-11-09*
