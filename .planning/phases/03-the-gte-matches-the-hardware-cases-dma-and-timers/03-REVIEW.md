---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
reviewed: 2026-11-10T08:53:00Z
depth: deep
files_reviewed: 26
files_reviewed_list:
  - cmake/Armips.cmake
  - include/schwaystation.h
  - runner/main.c
  - runner/trace.c
  - src/bus.c
  - src/bus.h
  - src/cop0.c
  - src/cpu.c
  - src/cpu.h
  - src/cpu_exec.inc
  - src/dma.c
  - src/dma.h
  - src/frame.c
  - src/gte.c
  - src/gte.h
  - src/instance.c
  - src/internal.h
  - src/irq.c
  - src/sched.c
  - src/sched.h
  - src/state.c
  - src/timers.c
  - src/timers.h
  - tests/cmake/hashes.cmake
  - tests/cmake/notices.cmake
  - tools/gte2bin.py
findings:
  critical: 1
  warning: 1
  info: 7
  total: 9
  necessity: 0
status: issues_found
---

# Phase 3: Code Review Report

**Reviewed:** 2026-11-10T08:53:00Z
**Depth:** deep
**Files Reviewed:** 26
**Status:** issues_found

## Summary

I reviewed the phase's changed code and its cross-module interactions: `sched.c`, `cpu.c`/`cpu_exec.inc`, `dma.c`,
`timers.c`, `bus.c`, `frame.c`, `state.c`, the GTE, the public header and the runner. `CONTEXT.md` decisions D-01 to
D-39 were used to tell a bug from a choice, and nothing the file records as deliberate is reported.

The core is in better shape than the focus list feared. I did not only read it. I built the core from `src/` in the
scratchpad and drove it with six throwaway harnesses. None of this code is committed.

- Mutation fuzz of saved states: about 3,000 hostile states accepted, with fields of `dma.*`, `timers.*`, `gte.*`,
  `irq.*` and `cpu.*` set to their bounds, then run, saved, loaded and saved again. No hang, no crash, and the
  save/load/save bytes were equal in every case.
- 3 million random GTE executions (every function code, edge-biased registers) under ASan and UBSan with the signed
  overflow and implicit-conversion checks on. No finding.
- Random guest programs (I/O stores and loads, `MTC2`, `MFC2`, GTE commands, `SWC2`, interrupts enabled), each run one
  shot and again in random chunks with save/load roundtrips between chunks. About 80 programs were compared byte for
  byte at the same tick, plus timer-storm, self-referencing-list and `BCR = FFFFFFFFh` guests. All were bounded
  (about 6 to 50 ms of host time per frame under ASan) and all agreed.

What I could prove broke:

- One invariant of the state format is violated by the engine itself (CR-01): the core can produce a state that its own
  loader refuses.
- One timing-model error contradicts D-05 (WR-01).

The rest is hygiene.

The permissive seed's provenance checks out. The `THIRD-PARTY-NOTICES.md` ISC text is byte-identical to the root
`LICENSE` of ares at the pinned commit `9408cb43d494`, and I read it through `gh api`. The upstream
`interpreter-gte.cpp` carries no per-file header, so the root licence is the right attribution. `src/gte.c` has the SPDX
line and the copyright line, and the manifest rows and the "Applies to" sections exist. The never-GPL rule holds for
every new file; I found no GPL lineage. `SCHWAY_FAULT_DMA` is consistent across the header, `internal.h` (static
assert), the state bound, `runner/trace.c` and `docs/embedding.md`. The runner prints the hash lines after the
`unbuilt` line, as its header comment says, so `hashes.cmake` anchors correctly.

## Critical Issues

### CR-01: A DMA register read can leave `busy_until` beyond the state's own bound, so the core's saved state is refused on load

**File:** `src/dma.c:688-691` (cause), `src/dma.h:27-28` and `src/dma.c:47-49` (the bound)
**Issue:** The read path of `schway__dma_io` finishes the running unit "as of its end tick" with
`schway__dma_event(inst, d->busy_until)`. That call also starts the next waiting channel at that tick
(`dma_event` -> `dma_start(inst, t)`). The second unit's `busy_until` is then the first unit's end plus the second unit's
own cost, measured from the access tick `ticks`, which has not moved. `SCHWAY__DMA_BUSY_MAX` allows only one maximal
unit past `ticks` (2,691,328), but this path chains two.

Reproduced with the library as built from `src/`. DPCR has every enable set. Channels 3 and 5 have `BCR = 0` and
`CHCR = 11000000h`, so each is a 10000h-word burst. Then any read of a DMA register:

```
before read: cur=3 busy_until=1576960   ticks=0
after read:  cur=5 busy_until=2891776   ticks=0   (DMA_BUSY_MAX = 2691328)
schway_save_state ok, schway_load_state -> 10 (SCHWAY_STATUS_ERR_STATE_FIELD)
```

Channel 3 costs 1,576,960 cycles for 65,536 words at the 24-cycle CD rate, and channel 5 costs 1,314,816 at the PIO
rate. Their sum, 2,891,776, exceeds the bound by about 200,000.

The state is refused whenever the save happens less than about 200,000 cycles after the read. That happens for any
sub-frame `schway_run` budget, and for a guest that starts the pair in the last cycles of a frame. It breaks
`docs/embedding.md` ("Saving, loading and saving again gives the same bytes"), and a debugger or libretro host sees a
save state it cannot load.

The comment on `SCHWAY__DMA_BUSY_MAX` already says the longest CD rate is 40 cycles per word. When the CD end replaces
the stub's 24, ch3 plus any other 10000h-word unit exceeds the bound with no PIO involved: 40 x 65,536 = 2,621,440 plus
4,096 is 2,625,536, and adding a GPU unit of 69,632 gives 2,695,168. Hostile input reaches it; ordinary games do not.

**Fix:** Bound the chain, not one unit. Either raise the limit to two units, since the fast-forward chains at most two:

```c
#define SCHWAY__DMA_BUSY_MAX \
    (2u * (SCHWAY__DMA_UNIT_WORDS_MAX * 40u + SCHWAY__DMA_UNIT_WORDS_MAX / 16u * 17u + 256u))
```

or stop the read path from starting the next unit early. Finish the unit as of its end, leave `cur` pointing at the next
start, and let the scheduled event start it at `busy_until`. Either way, add a test that arms ch3 and ch5 at maximum
length, reads a DMA register, and asserts `schway_load_state(schway_save_state(x))` succeeds. The existing "N cycles
then M" check does not arm two long channels.

## Warnings

### WR-01: A DMA stall does not advance `cpu.cycle`, so GTE and multiply stamps do not decay during it (contradicts D-05)

**File:** `src/cpu.c:204-213`, with `src/cpu_exec.inc:246-251` and `src/cpu_exec.inc:265-270`
**Issue:** D-05 says the engine's busy stamp "decays by 1 per intervening cycle". `gte.ready` and `muldiv_ready` are
stamps in the `cpu.cycle` domain. The stall branch of `schway__cpu_run` advances only `inst->ticks` and `ran`; it never
touches `cpu.cycle`. So a command that finished long ago still looks busy to the next `MFC2`, `CFC2`, `SWC2`, command
or `MFHI`/`MFLO` after a stall.

Measured with a harness: start OTC (4,096 words), issue `NCDT` (44 cycles), then read CHCR (stalls to the end of the
unit), then `MFC2`.

```
ticks before MFC2 = 4413 (cpu.cycle = 78); gte.ready - cpu.cycle = 29
MFC2 cost 30 cycles
```

The command was issued about 4,300 ticks earlier, so on hardware the `MFC2` costs 1 cycle. The same applies to
`MFHI`/`MFLO` after a multiply or divide. The deviation is deterministic and bounded (at most 44 and 36 cycles), but it
shifts every later tick and so every timer reading and hash.

**Fix:** Count the freeze in the CPU's own clock before resynchronising the block clock:

```c
inst->ticks += wait;
inst->cpu.cycle += wait;   /* the engines run during the freeze; their stamps are in this domain */
ran += wait;
schway__sched_sync(inst);
```

`BEHAVIOUR_REVISION` is already bumped in this phase, so the hash shift is accounted for. Add a test of the sequence
above.

## Info

### IN-01: DMA state validation is looser than the invariants the engine keeps

**File:** `src/dma.c:44-74`
**Issue:** I mutated every `dma.*` field to its bounds and ran the result. Nothing there is an event storm, an
out-of-range index or an unbounded stall: `cur` is validated against `chcr`, every array index is a validated range, a
stale `busy_until` of 0 against a late `ticks` is caught up in at most about 4,096 list nodes or 65,535 chopped windows,
and the CPU freeze is capped by `BUSY_MAX` per frame. The gaps are cosmetic but let impossible states load:

- `phase` accepts 0 to 7 while only 0 to 5 exist. A channel wedged in phase 6 or 7 never starts and stays BUSY until the
  guest writes `CHCR = 0`.
- `stall_until` may be up to `ticks + BUSY_MAX` with no unit running. That freezes the CPU for about five frames.
- `cur` is not checked against `phase[cur]`, `remain` is not checked against `phase`, and `busy_until` has no lower
  bound.

**Fix:** Bound `phase` by `DMA_PH_STOP`. Add `check(stall_until <= busy_until || stall_until <= ticks)`, and check
`phase[cur] != DMA_PH_IDLE` when `cur != DMA_NONE`. Use `SCHWAY__DMA_BUSY_MAX` as the limit for the new fields so that
CR-01 and this change are made together.

### IN-02: Sync mode 3 is a fault only on an idle channel; a rewrite of a busy channel runs it as sync mode 0

**File:** `src/dma.c:604-614`, `src/dma.c:503-516`
**Issue:** D-16 makes a sync mode a channel cannot use (mode 3 off OTC) a first fault. The check is under
`d->phase[ch] == DMA_PH_IDLE`. A write of mode 3 to a channel that is in a GAP, waiting for its next node or in
BLOCKS only ORs BUSY back in and stores the sync bits. When the engine later picks the channel, `dma_start` falls
through to the sync-0 burst/chop branch and moves data with no fault latched.
**Fix:** Run the mode-3 check regardless of `phase` (for a busy channel, drop the unusable bits and latch the fault, or
end the transfer), so the reserved mode never moves data.

### IN-03: The DMA fault record uses `addr` and `opcode` differently from every other fault kind

**File:** `src/dma.c:612-613`, `docs/embedding.md:202`
**Issue:** For the bus and CPU faults `addr` is the virtual address and `opcode` the instruction word. The new
`SCHWAY_FAULT_DMA` row puts the physical register address in `addr` (`p & ~3u`) and the written value in `opcode`. The
docs row states this, so it is not a mismatch with the docs. But the runner prints it as `addr=0x1f8010..
op=<value>`, and a host that decodes the record by kind sees a different meaning for the same field.
**Fix:** Put the instruction word at `pc` in `opcode` and the CHCR virtual address (or the value written, kept in the
`note` field) in `addr`. If kept as is, say so in the `schway_fault` comment in the header.

### IN-04: Vestigial code

**File:** `src/timers.c:408-413`, `src/timers.c:437`, `src/timers.c:513`, `src/dma.c:84`, `src/dma.c:409`
**Issue:**
- In `schway__timers_io`, `if (n >= SCHWAY__TIMERS)` is unreachable. `bus.c:481-484` dispatches only slots 10h to 12h, so
  `n` is 0 to 2. Slot 13h is a bus gap.
- `timer.free_after` is written in two places and read nowhere (`grep` finds only the assignments and the state walk). It
  costs a saved field, but nothing depends on it.
- `schway__dma_arbitrate`'s `mask` is `DMA_UNIT_CHANNELS` (all seven) at the only production call site. Only the tests
  call it with another value, so the constant and the parameter are left over from when some channels had no unit.
- `dma_dev_write`/`dma_dev_read` `default:` arms (channel 6) are unreachable, because OTC returns before them.
**Fix:** Remove them. Drop `free_after` at the next `STATE_FORMAT` bump, or add a comment naming its future reader.

### IN-05: The register-lane helpers are copied three times

**File:** `src/bus.c:341-360`, `src/dma.c:143-158`, `src/timers.c:59-70`
**Issue:** `lane_mask`, `reg_get`/`lane_get` and `reg_put`/`lane_put` are three static copies of the same code, so a fix
in one is easy to miss in the others.
**Fix:** One internal header (`schway__lane_mask`, `schway__lane_get`, `schway__lane_put`) for the three modules.

### IN-06: `nodes_done` is a single counter shared by all channels

**File:** `src/dma.h:62`, `src/dma.c:437-441`
**Issue:** The comment says "the list nodes passed in the current step", but one counter serves seven channels and is
reset whenever any list channel starts from idle. Two list channels running together can restart each other's count, so
the 1000h-node yield point can be pushed out. It only matters when `now` is far ahead of the event's tick (a hostile
stored `busy_until`), where it still stops after at most 4,096 nodes per channel start. In normal event flow every node
costs 26 cycles or more and the yield is automatic.
**Fix:** Make it per channel (`nodes_done[SCHWAY__DMA_CHANNELS]`) at the next state-format bump, or note in the comment
that it is a shared bound.

### IN-07: `src/gte.c` header carries the copyright line but only points to the ISC permission text

**File:** `src/gte.c:1-10`
**Issue:** ISC asks that the notice appear "in all copies". The file has the SPDX line, the upstream commit and the
copyright line, and defers the permission text to `THIRD-PARTY-NOTICES.md`. That is the convention in `PROVENANCE.md`
section 4 and the notice text is correct, so this is compliant for the repository as shipped. A copy of `gte.c` alone
would lose the permission text, and `policy.notices` checks manifest rows only, not `src/gte.c`.
**Fix:** Optional. Inline the ISC permission block in the header, or extend the notices gate to check that every
source file with a `MIT AND ISC` SPDX line is named on an "Applies to" line (today `src/gte.c` is, and nothing
enforces it).

---

_Reviewed: 2026-11-10T08:53:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
