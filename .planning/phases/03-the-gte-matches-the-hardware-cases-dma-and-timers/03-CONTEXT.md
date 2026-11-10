# Phase 3: The GTE matches the hardware cases; DMA and timers - Context

**Gathered:** 2026-11-09
**Status:** Ready for planning

<domain>
## Phase Boundary

The GTE (COP2), the seven DMA channels and the three root counters become real and pass their tests. The phase
delivers `gte.c` with the 1,150 hardware-captured ps1-tests cases, the GTE cycle counts and busy stall, the
interrupt double-execute; `dma.c` with DPCR/DICR, the per-word costs, chopping, linked lists with the 1000h-node
guard, OTC and the CPU stall; `timers.c` with sync modes, targets, the write hold and every IRQ mode; and the
small scheduler spine (`sched.c`, the block clock, the stall hook) that the three share. Requirements: GTE-01 to
GTE-03. Success criteria: ROADMAP.md, Phase 3.

Out of scope here: the GPU line machine and every beam signal (phase 4, GAME-02: dot-clock and hblank records pin
there), the GPU, SPU, CD-ROM and MDEC ends of the DMA channels (each replaces one local stub in its own phase),
the nugget and ps1-tests nightly lanes (they need the pinned mipsel GCC and BIOS-less programs the stub cannot
run), a CPU-bound perf gate, the 2-instruction GTE write-to-use latency, and PGXP.

How this context was made: six independent advisor agents each took one area through every role lens with an
adversarial pass (the owner's standing fan-out method, `.planning/AUTONOMOUS.md`). This context locks their
synthesis. Where reports disagreed, the resolution and its reason are in D-30 to D-38. Nothing here reopens a DEC
row; places where a preparation document is wrong are listed in D-39 and fixed by the docs plan.

</domain>

<decisions>
## Implementation Decisions

### The GTE (gray area A)
- **D-01:** `src/gte.c` is a **permissive seed** from ares `ares/ps1/cpu/interpreter-gte.cpp` at `9408cb43d494`
  (ISC), ported to integer C. The advisor's port passed 1150/1150. PROVENANCE row 95 ("own code, ares read")
  becomes a section-4 seed row with an SPDX line, a THIRD-PARTY-NOTICES section and the zero-diff retirement
  proof the section requires. Reuse beats rewriting (ponytail ladder, rule 1). **Independence is kept elsewhere:**
  D-04's reference model is written from psx-spx and the vectors without opening the seed. — **Reversibility:**
  reversible.
- **D-02:** Shape: `uint32_t schway__gte_exec(struct schway__gte *g, uint32_t op)` returns the busy cycles; it
  takes only the GTE struct (no instance, no bus), so tests call it directly. One `switch (op & 0x3F)` dispatches
  (DEC.24, no pointer tables). `sf` and `lm` are decoded into locals, never stored. Register file: two `u32[32]`
  arrays in architectural form with read fix-ups (SXYP, IRGB/ORGB, sign-extended IR/VZ/H/RT33). LZCR is derived on
  write; FLAG bit 31 is recomputed on write and on exit. 16-bit registers are stored as 16-bit patterns.
- **D-03:** Numerics: 64-bit accumulators, 44-bit MAC clipping, ares flag placement, the documented IR3/lm, FC,
  mx = 3 and H sign-extend quirks. Signed shifts are banned (ENGINEERING): use `*4096` and a `sar64` helper;
  widening multiplies only. The seed's `* 1.0f` and every "float"/"double" word in comments are removed
  (`hygiene.no_float` scans comments). The UNR table is a literal `static const uint8_t[257]`; a test regenerates
  it from the formula and compares. Unlisted opcodes (ares mirrors 00h and 1Ah) are a `doc`-tier no-op, never a
  hardware claim. A directed test covers the Wipeout NCLIP encoding `0140006h` (the 1,150 cases never set fake
  bits 20 to 24).
- **D-04:** **Reference model and property test.** `tests/gte/ref.c` is a separate lineage: plain ints, one
  function per command, written from psx-spx and the hardware cases with no shared helpers and no packed words,
  by a different plan than the seed and without reading it. It must itself pass all 1,150 cases, which anchors it
  to hardware data; the property test is then a differential test of two hardware-validated models. It uses
  `tests/prop.h` (integer splitmix64, XFER-073), at least **1,000 cases per opcode per run** (about 22,000),
  biased to edges (0, 7FFFh, 8000h, 7FFFFFFFh, 80000000h, FFFFFFFFh, 44-bit boundaries), compares all 64 outputs
  including FLAG, and asserts the case count. The advisors' mutation run shows the FC, mx = 3 and clipping quirks
  are thinly pinned by the table, so the reference and a directed test must model them.
- **D-05:** Cycles and stall: `gte_ready` (u64) beside `muldiv_ready`, same convention
  (`ready = cycle + (cycles - 1) + n`), stall clamped to 44 like `cpu_muldiv_wait`. The wait applies to **MFC2,
  CFC2, SWC2 and a new command**, not to MTC2, CTC2 or LWC2 (nugget `tests/gte-timing/RESULTS.md` at
  `9db6d88d23c4`, MIT, hardware). It decays by 1 per intervening cycle. The command executes at issue. The
  2-instruction write-to-use latency is a documented deviation and a seed. The preparation claim that stalls apply
  to "reads" and that SWC2 is immediate is wrong (D-39).
- **D-06:** **Interrupt double-execute** lives in the existing `schway__cop0_irq_entry_cop2` hook. The phase-2
  review finding IN-01 is a real bug that this plan must fix first: `cpu_irq_enter` hands the hook the word at
  EPC, which in a branch delay slot is the branch; the hook must peek the pending instruction at `pc`, and
  execute it only for ExcCode 0 and `op >> 25 == 0x25`. The test covers the delay-slot case.
- **D-07:** Transcoder and vectors: `tools/gte2bin.py` (DEC.23) is stdlib-only, run as `python3 -I`, with no
  timestamps; it asserts that the git-blob SHA-1 of its input equals the pin blob `a03aa72173f1...`
  (`a03aa72173f122bf916039f438c8cde89e61e526`, 3,874,318 bytes, ps1-tests `f727802fead1`, MIT; the pin answers 200
  on `gh api`). Output `tests/vectors/gte/ps1tests-1150.bin`: a 4-byte count, then fixed 516-byte records (`u32`
  opcode, 64 input words, 64 output words, little-endian, `FFFFFFFFh` = register-only), 593,404 bytes. Names are
  dropped (case N is "test N"). One ctest runs the file (`--expect 1150`) with 3-4 negative controls (empty,
  truncated, a flipped field) and asserts the histogram (50 register-only plus 22 opcodes at 50 each). **Replay
  order is exact:** write registers 0 to 63 through the real MTC2/CTC2 path, run the command, read 0 to 63; poking
  registers fails about 1,005 cases. Python is allowed under rule 7 (`tools/sst_slice.py` precedent, CI never
  runs it); verification is the manifest sha256 plus a run-once `python3 -I tools/gte2bin.py` and `cmp`, recorded
  as a command and an exit status.
- **D-08:** The `gte_vectors` fixture exercises the **real CPU path** (SR.CU2 via `mtc0`, MTC2, CTC2, COP2, MFC2,
  CFC2, SWC2, the stall). It has at most 30 records (the mailbox caps at 30). Each record is the CRC-32 of the 64
  read-back words of one table case: one case per command (22) plus a few register-only cases, copied verbatim
  from the table and chosen so flags are set. **No invented corner cases:** they would have no `hw` source (the
  corners go to the directed unit tests of D-03). Inputs are a generated `tests/fixtures/src/gte_cases.inc`
  (about 20 to 35 KB, from an extended `gte2bin.py`), listed in the manifest with the notice. A guard test
  recomputes each expected CRC from the committed `.bin` and compares it with `expected.csv` (two paths to one
  value). The fixture test carries the labels `fixtures,gte,portable`, so `ctest -L gte` covers success criterion
  1; it stays in `tests/fixtures` because inventories must match the asan lane, which has no armips.

### DMA (gray area B)
- **D-09:** **Data-atomic, time-evented** (refines DEC.13, no reopening). The engine is a scheduler device. One
  *unit* is a sync-0 burst (at most 10000h words), one chopping DMA window, one sync-1 block per DREQ, or one
  sync-2 node (header plus at most 255 words). A unit moves its words at once through a new fault-free
  `schway__bus_ram_word()` that reuses `ram_locate`, then sets `busy_until = now + cost`. An event at
  `busy_until` clears BUSY, sets the DICR flag and the I_STAT bit 3 edge; a CPU window follows; then the next
  unit. Run-to-completion per CHCR write is rejected (65,535 x 65,535 words is a hang) and so is per-word
  interleave (33.8M events a second).
- **D-10:** **Stall is freeze-until, not a charge.** A bus access that waits on the bus (a RAM or I/O **read**, an instruction-fetch
  miss, or the **fifth queued write**, because writes are buffered in the 4-deep write queue and only a full queue
  waits) while `now < busy_until` sets `stall_until`; `cpu_run` then advances `ticks`,
  within the budget and running events, before the next instruction. Scratchpad, I-cache hits, COP0 and GTE are
  not bus accesses. This keeps the overrun at one instruction, so the 1,024 INSN margin (`cpu.h:79`,
  `frame.c:58`) holds; charging a 64K-word OTC (about 70,000 cycles) to one instruction would break it. Never
  stall while a channel only waits for DREQ. A stall that crosses frame end ends the frame at that instruction.
  Rejected: the E advisor's one-instruction charge with a 2^20-word cap (D-31).
- **D-11:** Costs per word: 1 for MDEC, GPU and OTC; 4 for SPU; 24 or 40 for CD; 20 for PIO; plus 17/16 on the
  RAM row. Chopping window model (derived from the 64 hardware rows of `dma/chopping/psx.log`, mean error 1.1%,
  max 11%): per window `ceil(17*dw/16) + 6 + min(cpuWindow, 32*dw)`; sync-1 `blocks * (17*bs/16 + 11)`. Tests use
  **bands**, and the docs label these rows `derived`. The 1.6x CPU slowdown during `chain-looping` is calibrated
  to one log: a band, not a pin.
- **D-12:** The **1000h-node guard is a yield, not an abort**: after 1000h nodes the step returns and time
  advances; the channel stays BUSY and raises no flag or IRQ. The hardware log `dma/chain-looping/psx.log` shows a
  self-referencing chain with `finished = false, irq = false`. GTE-02 and DEC.18's word "terminates" is read as
  "the step returns and the frame still ends with STOP_FRAME"; the REQUIREMENTS text is amended (D-39). The list
  end test is bit 23 of the next address.
- **D-13:** DPCR: a lower priority value wins and a tie goes to the higher channel number; the CPU priority field
  has no effect (nugget `RESULTS.md`, measured). Master enable off means the channel does not start. A DPCR write
  re-arbitrates (ps1-tests `dma/dpcr` and nugget start a channel by enabling it later). DICR bit 31 is not stored:
  it is bit 15 OR (bit 23 AND any flag); I_STAT bit 3 is raised on a 0-to-1 edge only. Flags are set only if the
  mask AND bit 23 allow it (psx-spx; ares differs and is not followed). Pin nugget's DICR reads (84840000h,
  84800000h, 04000000h, 84008000h) with the edge and no-edge cases.
- **D-14:** OTC (DMA6): needs bit 24, bit 28 and the DPCR enable; ignores direction, step, sync and chopping;
  CHCR reads back 00000002h (50000002h when bits 28 and 30 were written); a CHCR read right after a long OTC
  already sees completion (the read stalls the CPU, D-10). MADR wraps at FFFFFCh. A DMA word address past the RAM
  window drops the word, is **not** a first fault, and does not touch DICR bit 15 (unverified on hardware, so it is
  not built).
- **D-15:** Device interface: `dma.c` has one `switch (ch)` per direction calling
  `schway__<dev>_dma_write(inst, word)` or `_dma_read(inst, *word)`, each returning extra busy cycles; until the
  device exists the case is a local stub. The stub sink counts words and keeps a rolling CRC-32 per channel in the
  state (reusing `crc32.c`); the stub source reads 0 (D-01 of phase 2). DREQ is `schway__dma_dreq(inst, ch, level)`,
  a latched level in the state that devices call later. Later phases replace one case each. A callback or vtable
  table is rejected (`.data.rel.ro` under PIC, function pointers in state, rule 4).
- **D-16:** A sync mode a channel cannot use (mode 3 on a channel other than OTC) is a **first fault with a new
  kind `SCHWAY_FAULT_DMA` (10)**. It is an additive public change: API version 2 to 3, a consumer in
  `tests/api/v3.c`, an abi target, the `runner/trace.c:69` kind table and the fault range in `state.c:70`. The
  new name appears in `docs/embedding.md` (`docs.api_names`). Without it hostile DMA would be silent (XFER-017).
  No DMA trace event: `trace_push` is static in `cpu.c` and `on_dma` is a v5.0 hook; the `unbuilt` footer line
  stays for the GPU, SPU, CD and MDEC ends.
- **D-17:** Fixture **`dma_chain`** (a new row in the CONFORMANCE matrix) at `mailbox@30`, built from OTC and
  `chain-looping` because the GPU is not built. Records: first and last OT word (FFFFFFh), CHCR 00000002h, DICR
  C0C00000h after writing 00C00000h, I_STAT bit 3, a master-off control with the OT unchanged, an empty complete
  chain (finished and IRQ), a single and a double self-referencing chain (not finished, no IRQ), and one OTC
  timing range of 4340..4400 read through Timer 2 (nugget measured 4,358 for 4,096 words on SCPH-5501 and 1001;
  the model gives 4,352). The CPU-stall ratio stays out of the fixture: it is pinned in `tests/dma` and recorded
  in `docs/hardware/dma.md`.

### Root counters (gray area C)
- **D-18:** **Lazy closed-form catch-up plus one derived `next_deadline`.** Per counter: value at a base tick,
  `hold_until`, flags, paused. Counter value and the reached bits are pure O(1) functions of `now`; every
  1F8011xxh access and every I_STAT read catches up first. A pulse-mode repeat storm collapses to one
  `schway__irq_raise` per span (I_STAT set is idempotent); toggle mode needs only the parity of the hit count; a
  deadline is scheduled only for the next hit that would produce a new I_STAT edge, and recomputed on an I_STAT
  write. A test asserts every fired deadline is strictly greater than `now` (zero-progress guard). Per-step
  ticking and a full event queue are rejected.
- **D-19:** Test oracle: a ~100-line naive per-cycle reference in `tests/timers`, driven by the seeded splitmix64
  harness with random register operations at random catch-up cadences, against the lazy model. It is the guard
  against off-by-one in the hold and dwell.
- **D-20:** Clock sources: model **sysclk and sysclk/8 only** in this phase. Timer 2's /8 phase is the absolute
  tick modulo 8, stateless. `timers.c` exposes `schway__timers_vblank(inst, tick, level)`,
  `schway__timers_hblank(inst, tick, level)` and a dot feed; `frame.c` pushes vblank at the existing frame end
  (phase-2 D-26) and unit tests push the rest. **Without a GPU line machine, Timer 0 on dot or hblank and Timer 1
  on hblank do not advance** and the access counts as `unbuilt dev=timer` in the existing footer (not a fault: a
  game may legitimately read the counter). No fake beam. For phase 4: dots are integer, `dots += (phase +
  line_clocks) / div; phase = (phase + line_clocks) % div`; the GPU clock comes from ticks by the accumulator
  (`acc < 451584`); **never** `ticks * num` (2^62 x 2^20 overflows u64, and `be32`/`wasm32` have no `__int128`),
  and never the `*11/7` shortcut of `instance.c:204`.
- **D-21:** Semantics follow psx-spx and ares (the closed form is our own code; any ares text copied carries a
  notice): Timer 2 sync modes 0 and 3 stop the counter and 1 and 2 free-run. A MODE write zeroes the counter,
  starts the hold, sets bit 10 to 1, re-arms one-shot, and derives `paused` from the current level. Reset at
  target has period target + 2 (psx-spx example 0,0,1,0,0,1 at target 1 is the first test); FFFFh wrap has no
  extra dwell; COUNT written above target does not compare until it wraps. A MODE read clears bits 11 and 12.
  Pulse mode leaves bit 10 at 1 (ares); toggle mode flips bit 10 and raises I_STAT bit 4+n only on the assert
  edge, so it fires every second event; the toggle level lives inside the timer (`irq.c` is edge-set only).
  The write hold is tested as "the value shows for 2 cycles" (the preparation claim of ares `WAIT_CYCLES = 2` is
  wrong: ares has 1, D-39).
- **D-22:** `timers_lines` records in this phase are the vblank count over N frames, Timer 2 sysclk/8 ticks per
  frame and sysclk elapsed. The per-frame sysclk/8 value (about 70,775) exceeds 16 bits, so the fixture counts
  FFFFh IRQs (Timer 2 /8, bit 5, repeat) plus the residue, which also exercises the IRQ path. **Every P3 timer row
  is `doc`, never `hw`.** Reason: the ps1-tests `timers/psx.log` shows 71,407..71,412 sysclk/8 ticks per frame,
  about 0.9% above our NTSC frame; that console matches a PAL crystal forced to NTSC (897,619 x 451,584 / 709,379
  = 571,416), so it cannot back an NTSC `hw` row. Ranges are computed from the frame constants in `frame.c`
  (exact ratio 715909/451584, not 11/7) and include the fixed ISR-path overhead. P3 ids are contiguous so GAME-02
  appends the dot-clock and hblank records. The judge must honour the `profile` column (about 6 lines in
  `tests/fixtures/tests.cmake`, outside `policy.gate_budget`; today it reads rows by fixture name only).

### Integration, API, state and hashes (gray area E)
- **D-23:** **`sched.c` with a derived `next_deadline`** (never stored). Each device exposes `<dev>_next(inst)`;
  the minimum is recomputed after any device register write and after a state load. `schway__cpu_run` takes
  `limit = min(budget_left, frame_room, next_deadline - ticks)`; the fast path gains no instruction because the
  existing `ran >= limit` compare already does this. A register write that moves a deadline sets
  `block_end |= SCHWAY__BLOCK_EVENT`, as I_STAT/I_MASK already do. Events are seen at the first instruction
  boundary at or after the deadline. Ad-hoc per-device polls are rejected.
- **D-24:** **Always-on block clock.** `inst->ticks` moves only at block end (`cpu_exec.inc:980`) and the block
  start exists only in the traced copy. Promote `t0`/`c0` (`cpu.c:143` `trace_now`) to non-state CPU fields set at
  block start, giving `now = blk_tick + (cpu.cycle - blk_cycle) + (*cycles - 1)` at every device access. Without
  it a COUNT poll loop reads stale values. `fault.ticks` uses it too (`instance.c:57`).
- **D-25:** State: field-by-field little-endian walk in `body_io`: `cpu`, `cop0`, `gte` right after
  `schway__cpu_state_io`; after bus and irq, `dma`, then `timers`, then `fault`. GTE: the two `u32[32]` and
  `gte_ready`. DMA: struct-of-arrays raw MADR, BCR, CHCR for 7 channels, DPCR, DICR raw bits, `busy_until`, the
  current channel, per-channel phase and remaining counts, `nodes_done`, the DREQ mask, `stall_until`, the
  per-channel word counters and stub CRC. Timers serialize the **normalized view as of `ticks`** (value, target,
  mode bits, hold remaining, reached, irq line, one-shot done, paused, free-after) with **no base tick**, so the
  hash does not depend on when a catch-up happened. **`next_deadline` is not state** (deviates from ARCHITECTURE
  section 5: a hostile stored deadline becomes an event storm or an endless wait). VALIDATE: `busy_until <= ticks +
  DMA_BUSY_MAX`, `gte_ready` clamped, MADR <= FFFFFFh, CHCR masks, channel index <= 7, an active engine implies
  CHCR bit 24. `SCHWAY__STATE_FORMAT` 2 to 3 and `SCHWAY__BEHAVIOUR_REVISION` 2 to 3 **once**, in plan 1
  (`tests/core/test_state.c:517-518` pin both). Old states are refused by the format check (`state.c:206`).
  The "N cycles then M equals N+M" check is extended with timers and DMA armed.
- **D-26:** Stub replacement: keep the shape of `unbuilt_n[18]` (the runner reads it by `count ==
  SCHWAY_DEVICE_UNMAPPED+1`, `runner/main.c:510`); the `DMA`, `TIMER` and `GTE` slots stay 0. Phase-2 fixture
  frame hashes do not change (the picture is the test card, and nothing in the stub or fixtures touches DMA,
  timers or COP2 apart from I_STAT). Delete `tests/cpu/cop2_prephase.c` and inventory lines 17-18; turn
  `tests/cpu/exception_tests.c:598-607` into a real double-execute assertion; flip the DMA and timer ranges to
  owned in `tests/core/bus_unbuilt.c`; `tests/fixtures/src/unbuilt.asm` MODE 0 writes DPCR and must change;
  `docs/runner.md` (`unbuilt dev=dma`, line 211), `docs/hardware/cpu.md:162` and `bus.md:121` are rewritten.
- **D-27:** Runner: one new flag, **`--hash-state`**, which runs `schway_save_state` and hashes the body after the
  20-byte header (no core API). Reason: with no GPU every frame hash is the test card whatever the CPU did, so P3
  devices are invisible to cross-lane equality otherwise; ARCHITECTURE section 7 already defines a state hash. It
  needs one REQUIREMENTS line (D-39). The `portable` fixture mailbox records on `be32` and `wasm32` are the other
  cross-lane evidence.
- **D-28:** Hostile boundedness: units are bounded by D-09; the frame end preempts background DMA chunks and the
  state carries the remainder to the next frame; timer storms are bounded by D-18. Tests: BCR = FFFFFFFFh on all
  seven channels and a self-referencing list assert emulated time <= frame + the unit bound. A hostile 4-frame
  audio overflow (`audio_dropped`) is acceptable and documented.

### Fixtures, labels and CI (gray area D)
- **D-29:** The honesty rule for `hw`: a value is `hw` only if it is copied from an MIT hardware artefact at pin
  `f727802fead1` (or the nugget pin) with the path and case or line named in the citation field; never from our
  core or another emulator. The docs say "hardware-captured per upstream, not verified here". psx-spx and PX.12
  prose is `doc`; fitted models are `derived`. The judge keeps its two sources, `doc|hw` (CONFORMANCE lists five;
  it is amended). Citation fields contain no commas (the judge splits on commas).
- **D-30:** Labels `gte`, `dma`, `timers`, each with `portable`; new directories `tests/gte`, `tests/dma`,
  `tests/timers` each with `inventory.txt` and `tests.cmake`, registered in `ci.txt`, `asan.txt` and `wasm32.txt`
  (not `nofp`, `hygiene` or `nightly`). One executable per directory with the case name as an argument. Evidence
  commands use `-L '^gte$'` (substring regex footgun). `gte.regs.sweep` and `dma.regs.sweep` in the style of
  `bus_unbuilt` run under ASan/UBSan.
- **D-31:** CI: no new job. On every lane including `be32` and `wasm32`: the 1,150 cases, the property test, the
  fixtures, the portable subset. No new frame-hash lines (the frame is the test card); fixture image digests join
  `hashes.txt` through its glob. Fuzz: no new target; `tests/fuzz/seeds.c` gains `CODE_DMA`, `CODE_TIMERS` and
  `CODE_GTE` rows (about 60 lines: self-referencing chain, chain into scratchpad/BIOS/unmapped, OTC length 0 and
  10000h at MADR FFFFFCh, BCR = FFFFFFFFh on all seven, Timer target 0 with an IRQ storm and no handler, SR.CU2
  with MTC2/CTC2 of FFFFFFFFh and every function code with sf/lm). The nugget and ps1-tests nightly lanes stay
  deferred.
- **D-32:** **Cheap CI win, applied this phase:** armips builds in about 87 s of the 125 s macOS-x64 build step and
  about 41 s on windows-x64 (run 42739240299, 189 s wall, 696 runner-seconds). `-DCMAKE_UNITY_BUILD=ON` on the
  pinned armips measured 17.5 CPU-s against 51.2 locally with identical `hello` and `exc_matrix` digests. One line
  in `cmake/Armips.cmake`; the eight-lane digest comparison validates it; if MSVC fails to build it, fall back to
  `actions/cache` keyed on the armips pin.
- **D-33:** Perf: no new gate (rule 7). The test card (1,620,424 Ir/frame, +1% = 16.2k) never runs the CPU, GTE or
  DMA, so lazy devices cost it a few hundred Ir at most: do not step devices in the no-EXE `advance()` path.
  Because `cpu_run` changes, the Ir/frame gate is re-measured once and recorded. The agent records one `--bench`
  wall-clock run of a looping `gte_vectors` once; a CPU-bound Ir fixture is a seed bound to phase 4 `gpu_motion`.

### Gates, docs, provenance and process (gray area F)
- **D-34:** Gate budget: plan 1 ratchets the limit in `tests/hygiene/tests.cmake` and `docs/testing.md` from 1,520
  to the measured **1,483** (`wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*`), which phase 2 never did.
  REQUIREMENTS names the phase-3 ceiling as at most 1,520, ratcheted to the measured total at ship. **One new
  gate:** a manifest-driven `policy.notices`, at most 30 lines in `tests/cmake/notices.cmake`, with a
  `.self_test`. Fault it prevents: a committed third-party binary or table whose notice section does not name it,
  or a notice naming a path that does not exist (every `manifest.txt` row whose notice column is not `-` must
  appear on an "Applies to" line of THIRD-PARTY-NOTICES.md, and every path on such a line must exist). GTE-01 names
  the notice and todo 005 binds it. If it measures over 37 lines, defer it; never raise the cap and never move
  gate logic into `tests/*/tests.cmake` to dodge the count. The rest of todo 005 (SPDX scan, `.info` check) is
  rebound to the first non-MIT licence or SPDX-bearing derived file that needs it. No "bin matches transcoder"
  gate (D-07 covers it).
- **D-35:** Provenance: new THIRD-PARTY-NOTICES section "Applies to `tests/vectors/gte/ps1tests-1150.bin` and
  `tests/fixtures/src/gte_cases.inc`": upstream URL, path, the full 40-hex commit
  `f727802fead11f1daa7549285548392ef87749cb`, the `.bin` SHA-256, "transcoded by `tools/gte2bin.py`; values
  unchanged", and the full MIT text with "Copyright (c) 2020 Jakub Czekański". A second section for the ares ISC
  seed (D-01). `manifest.txt` gains a row for each (a `.bin` path matches the ROM-name pattern, so the row is
  required). `pins.txt` gains `ps1tests-gte-test-all f727802fead1 a03aa72173f1... 3874318`. The ps1-tests
  executables stay fetch-only (MPL `libpsn00b` bytes). The never-open list is unchanged.
- **D-36:** Docs: add `docs/hardware/gte.md`, `dma.md`, `timers.md` (each well under 61,440 bytes; `docs.budgets`
  globs `docs/*/*.md`); update `bus.md`, `cpu.md` (the stale line near 320 saying the CPU takes no interrupts is
  review WR-06 and becomes false when timer and DMA IRQs arrive), `runner.md`, `testing.md` (the labels) and
  `maintaining.md` (the `gte2bin` procedure); README untouched; the header stays terse and every new `schway_`
  name appears in `docs/embedding.md`. Hardware comments cite the public `docs/hardware` pages, psx-spx and pins,
  never planning IDs and never a preparation file that points back. PSX-SYSTEM sections 2 to 6 and PSX-GPU-AND-GTE
  section 5 shrink to pointers (the phase-2 catch-up included, keeping the Sources rows so `[PX.nn]` IDs resolve).
- **D-37:** Plan shape (rule 14): **plan 1** lands the CMake skeleton, `gte.h`, `dma.h`, `timers.h`, `sched.h`
  with the state structs, reset and `state_io` stubs, the `struct schway` members, the `body_io` calls and the
  single format/revision bump, the source lists, `tests/{gte,dma,timers}` with inventories and `ci.txt` lines, thin
  `bus.c` dispatch calls, the budget ratchet, `policy.notices`, the manifest rows and notice sections.
  **Plan 2 (sequential spine)** touches `cpu.c`, `cpu_exec.inc`, `frame.c`: block clock, `cpu_run` limit,
  `sched_run`, the stall hook, the `cpu_cop2` dispatch, and the D-06 hook fix. **Wave 3 (parallel, disjoint
  files):** `gte.c` split in three plans of at most 30 minutes each (registers/flags/UNR, command groups, the
  harness plus the reference and property test, the last by a plan that does not read the seed), `dma.c`,
  `timers.c`, each with its own tests and its `docs/hardware/*.md`. **Wave 4 (sequential, shared fixture files):**
  `gte_vectors`, `dma_chain`, `timers_lines` with `expected.csv`, the judge `profile` fix, the runner flag, the
  stale-test cleanup, the fuzz seeds, the armips unity build, the docs plan. Phase 2 ran 27 plans in 13 waves and
  two plans overran (02-09 at 55 min on armips waits, 02-22 at 36 min).
- **D-38:** Process: the branch `phase/03-the-gte-matches-the-hardware-cases-dma-and-timers` is correct. The PR
  title is `feat: match the GTE hardware cases and add DMA and timers` (passes `scripts/check-title.sh`);
  release-please (`bump-minor-pre-major`) cuts 0.3.0. A third review-fix round or a second gap round needs new
  evidence (XFER-085); review findings about test or tooling code become seeds. The 13 open phase-2 review
  findings (WR-01..06, IN-01..07 in `02-REVIEW-DISPOSITION.md`, no todo files) each get one todo: IN-01 is folded
  into plan 2 (D-06), WR-06 into the docs plan, IN-03 and IN-05 are tooling seeds, the rest are bound to phase 4.
  Todo 005 is folded (trimmed, D-34); 003 (pins gate) and 001, 002, 004, 014, 016, 017, 022 are explicitly
  deferred (rebound to the first caller or SEED-007); 006, 009, 013 are phase 4 or 6; 010, 011, 012, 015, 021 are
  SEED-007.

### Conflicts between reports, and how they were resolved
- **D-30a:** GTE seed or write fresh (A vs F): seed, with the independence kept in the reference model (D-01,
  D-04). A ported and ran it; ponytail's ladder and the owner's "permissive seed with its notice" taste prefer
  reuse; F's concern (a reference written from the same reading is a tautology) is answered by separating lineages.
- **D-31a:** DMA stall (B vs E): B's freeze-until (D-10). E's one-instruction charge needs a word cap and breaks
  the overrun margin; B's units are already bounded.
- **D-32a:** GTE fixture corner cases (A's 8 vs D's none): D. Without `hw` source a corner has no honest row; the
  directed tests carry them.
- **D-33a:** Property-test size (A 500 per opcode, D about 20,000 total): 1,000 per opcode (D-04).
- **D-34a:** NTSC `hw` timer row (D's 71,300..71,500): rejected; it would fail our own 70,775 frame (D-22). D's 11/7
  frame figure is wrong for NTSC; C's exact ratio is used.
- **D-35a:** Timer 0/1 without GPU (C: latch a first fault): softened to unbuilt-counted, no fault (D-20), since a
  guest may read the counter.
- **D-36a:** DMA fixture (B `dma_otc`, D `dma_chain`): one fixture `dma_chain` holding both sets (D-17).
- **D-37a:** Timer state (C normalized view vs E stored `last_tick`): C; the hash must not depend on catch-up
  cadence (D-25).

### Adversarial pass (recorded, not reopened)
- The DMA chopping fit comes from one log and the CPU-gap ratio from one log: tests use bands and the docs say
  `derived` (D-11). A seeded model that fits its own log is evidence of fit, not of hardware.
- Changing `cpu_run` is the one change that can move the Ir gate; it is re-measured once (D-33). Hashes of every
  phase-2 fixture are asserted unchanged, and a change is a stop-and-triage, not an update (D-26).
- `SCHWAY_FAULT_DMA` is the only public change; it is bounded to one enum value and one test consumer (D-16).
- Ares-seed text must not leak into the reference: the reference plan lists only psx-spx, the vectors and the
  nugget results as inputs (D-04).

### Preparation and requirement corrections
- **D-39:** Corrections the planner and docs plan apply, recorded so no later step rediscovers them: REQUIREMENTS
GTE-02 "terminates by the guard" becomes "the guard yields and the frame still ends"; GTE-01 and CONFORMANCE
"64 records" becomes "at most 30"; CONFORMANCE section 4 mailbox offsets follow `tests/fixtures/src/mailbox.inc`;
the CONFORMANCE fixture matrix gains `dma_chain`; PX.21 "112,0xx..112,5xx system clocks per frame" is a 16-bit
wrap artefact of a 71,4xx sysclk/8 count (do not pin it); ARCHITECTURE sections 1 and 5 (deadline is derived, the
overrun bound reads 1,024 plus the unit bound, `gte.c`/`dma.c`/`timers.c` are phase 3), PSX-SYSTEM (DICR bits 0 to
6 versus ares 0 to 5, a list node above 8 MB cannot raise a DMA error because the bit-23 test fires first, ares
`WAIT_CYCLES` is 1) and DEC.51 ("zero third-party bytes" needs a manifest row); the phase-3 requirements gain one
line for `--hash-state`.

### Claude's Discretion
Container layout of `gte_cases.inc`, which table cases fill the 30 records, field names inside the new state
structs, the DMA unit enum, the exact stub CRC placement, plan order within a wave and md5/CRC source, the
choice of the `--hash-state` output line format (after the existing runner lines), and the order of the
`tests/fuzz/seeds.c` rows.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Rules and settled choices
- `AGENTS.md` (loaded as `CLAUDE.md`): rules 1, 3, 4, 5, 6, 7, 8, 11, 12, 14
- `.planning/preparation/DECISIONS.md`: DEC.13, DEC.18, DEC.22 to DEC.24, DEC.51, DEC.75, the open-questions table
- `.planning/LESSONS.md`: re-read at every discuss and plan step (XFER-017, XFER-073, XFER-083, XFER-085)
- `.planning/AUTONOMOUS.md`: discuss and verify steps, CI speed-ups
- `.planning/REQUIREMENTS.md`: GTE-01 to GTE-03, GAME-02 (the beam records pin there)
- `.planning/phases/02-the-cpu-matches-the-public-vectors/02-CONTEXT.md` and `02-REVIEW-DISPOSITION.md`

### Hardware and design
- `.planning/preparation/PSX-GPU-AND-GTE.md` section 5 (GTE; ares-derived, so the independent reference plan, which D-04 keeps clear of the seed, must NOT read it and uses the psx-spx GTE page instead); `PSX-SYSTEM.md` sections 4 to 6 (DMA, timers, interrupts)
- `.planning/preparation/ARCHITECTURE.md` sections 1, 2, 5, 6, 7 (some claims are wrong, see D-39)
- `.planning/preparation/CONFORMANCE.md` sections 2, 4, 5, 8; `PSXDEV-EXTRACTS.md` PX.12, PX.21; `ENGINEERING.md` sections 2 to 4, 8, 10, 11
- `PROVENANCE.md`, `THIRD-PARTY-NOTICES.md`, `ASSET_POLICY.md`, `tests/fixtures/manifest.txt`, `pins.txt`

### External sources (fetch with `gh api` at a public pin; never open the emulators rule 1 names)
- JaCzekanski/ps1-tests at `f727802fead1` (MIT): `gte/test-all/tests.c`, `dma/chain-looping`, `dma/chopping`, `dma/otc-test`, `dma/dpcr`, `timers`
- nugget (MIT) at `9db6d88d23c4`: `tests/gte-timing/RESULTS.md`, `tests/dma/dma.c`, `RESULTS.md`; `tests/scanline-timing` at `b56902dd1eb0`
- ares (ISC) at `9408cb43d494`: `ares/ps1/cpu/interpreter-gte.cpp`, `ares/ps1/dma`, `ares/ps1/timer` (idea source only for DMA and timers)
- psx-spx timers and DMA pages (https://psx-spx.consoledev.net/)

### Bound todos
- `.planning/todos/pending/005-*` (folded, trimmed); the review-finding todos of D-38 (to be created)

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/cpu.c`, `src/cpu_exec.inc` (`cpu_muldiv_wait` at 204-209 is the model for the GTE clamp), `src/irq.c`
  (`schway__irq_raise`, I_STAT/I_MASK), `src/bus.c` (`ram_locate`, the unbuilt cases at 436-450), `src/state.c`
  (`body_io`), `src/crc32.c`, `src/frame.c`, `src/instance.c`.
- `tests/core/` white-box pattern; `tests/fixtures/` (armips fixtures, `mailbox.inc` with CRC at 04h, status 08h,
  version 09h, count 0Ah, id 0Bh, cap 30 records), `tests/diag/expected.csv`, `tests/fuzz/seeds.c`,
  `tools/sst_slice.py` (Python-tool precedent), `cmake/Armips.cmake`.

### Established Patterns
- Dispatch by `switch`, never a `const` pointer table; guest memory through `rd/wr16/32le`; every behaviour change
  with a test that the `ci` workflow runs; no skipped test; hashes identical on eight lanes.

### Integration Points
- `bus.c` dispatches DMA and timer ranges to `schway__dma_io` and `schway__timers_io` (plan 1 stubs); `cpu_run`
  takes the deadline limit (plan 2); `frame.c` pushes the vblank edge; the GPU, SPU, CD and MDEC phases each
  replace one DMA stub case and feed the timer entry points.

</code_context>

<specifics>
## Specific Ideas

- Advisor measurements worth keeping: the 1,150-case table is 50 register-only plus 22 opcodes at 50 each; the
  transcoded `.bin` is 593,404 bytes (advisor A's run: sha256 `ed630d2f...58d2`, byte-identical twice); mutation of
  the FC quirk fails 12 cases, mx = 3 fails 7, clipping only at the end fails 2, RTPT IR0 on every vertex fails 3.
- Measured gate total 1,483 against a 1,520 limit; test card 1,620,424 Ir/frame; phase-2 PR run 189 s wall.
- The owner's method held: copy and paste of permissive code over a dependency (the ares GTE seed), a literal
  UNR table over a runtime generator, and no new CI job.

</specifics>

<deferred>
## Deferred Ideas

- Dot-clock and hblank records, the GPU line machine feed, the integer dot phase (phase 4, GAME-02).
- The GPU, SPU, CD and MDEC DMA endpoints (their own phases); `schway_load_bios` (phase 4).
- The nugget and ps1-tests nightly lanes (pinned mipsel GCC, phase 4 at the earliest); DMA trace events and
  `on_dma` hooks (v5.0); the 2-instruction GTE write-to-use latency; PGXP and any GTE sideband.
- A CPU-bound Ir/frame perf fixture (bound to phase 4 `gpu_motion`); `policy.fixture_budget` (still deferred, 593 KB
  is far under the 4 MB cap); the SPDX scan and `.info` licence check of todo 005; `policy` pins gate (todo 003).
- The 40-clock CD-ROM per-word cost: channel 3 is pinned at 24 in this phase; the CD-ROM device end (phase 6) selects 40 from its delay setting, and GTE-02's "24 or 40" is amended to say so.
- DICR bit 15 and bit 6 behaviour, and every CHCR write while a channel is busy other than the CHCR = 0 stop the
  upstream chain-looping test writes between cases (unverified on hardware, unmodelled).

### Reviewed Todos (not folded)
- 001-004, 006, 009-017, 021, 022: not bound to phase 3 (see D-38 for where each goes).

</deferred>

---

*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Context gathered: 2026-11-09*
