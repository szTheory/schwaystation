# Phase 2: The CPU matches the public vectors - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-11-09
**Phase:** 2-The CPU matches the public vectors
**Areas discussed:** unmodelled hardware, stub kernel and boot path, first-fault trace, vector harness and sample,
exceptions and interrupts, public API and runner, gates, docs and fixture toolchain

**Method:** the owner's standing fan-out instruction (general fan-out prompt, pasted mid-discussion) replaced the
interactive questions. Seven advisor agents ran in parallel, one per area, each through every role lens with an
adversarial pass; the recommendations were synthesized and locked without asking, and conflicts between reports were
resolved in CONTEXT.md D-30 to D-35. Calibration tier: `minimal_decisive` (the owner profile is `opinionated`).

---

## Unmodelled hardware (A)

| Option | Description | Selected |
|--------|-------------|----------|
| Unbuilt stub | Per-device `switch` case; reads 0, writes drop, counted and traced, never a fault | ✓ |
| Inert register file | Per-device storage with no semantics; adds state surface phase 3 deletes | |
| Bus error | Wrong: documented devices never fault; every stub boot would fault | |

**Choice:** unbuilt stub; COP2 raises CpU when CU2 is clear and is inert when set.
**Notes:** risk named by the agent: a phase-2 nugget test needing a GPU/timer/DMA readback; resolved by keeping nugget lanes out of phase 2.

## Stub kernel and boot path (B)

| Option | Description | Selected |
|--------|-------------|----------|
| Source + committed byte array + drift test | armips stays the source of truth; the library needs no armips | ✓ |
| CMake builds the stub with armips | Every library and embed build needs a C++17 toolchain | |
| Hand-encoded words | Unreviewable, endian hazard | |

**Choice:** committed array in `src/stub_kernel.c` (the report placed it in `host/`; moved into the core, D-30).
**Notes:** pure side-load and state injection rejected (skips reset behaviour). Three spec contradictions resolved (SP, exception record location, handler).

## First-fault trace (C)

| Option | Description | Selected |
|--------|-------------|----------|
| Always-on ring in the hot loop | 8 to 12 Ir per instruction, breaks the +1% gate | |
| Dual-compiled traced loop | Zero cost when off; ring excluded from the state hash | ✓ |
| Two-pass replay | Needs the traced loop anyway; deferred as an optimisation | |

**Choice:** dual-compiled loop; ring lives in the instance outside the state (the report's runner-side ring would need the v5.0 hooks ABI, D-31).

## Vector harness and sample (D)

| Option | Description | Selected |
|--------|-------------|----------|
| Verbatim upstream slices, one C parser | 2.39 MB, names kept, `cmp`-verifiable | ✓ |
| SWR3 transcode via `sst2bin.py` | 1.94 MB, second format and a Python trust surface | |
| Fetch only | Breaks offline lanes | |

**Choice:** verbatim slices. Measured facts changed three preparation claims (D-24).

## Exceptions and interrupts (E)

| Option | Description | Selected |
|--------|-------------|----------|
| ares counters (2 retirements; 1 for a CAUSE write) | Matches the oracle the vectors come from | ✓ |
| Zero latency | Cheapest, no source supports it | |
| Per-cycle pipeline sampling | Unmeasurable with permitted sources | |

**Choice:** model A, pinned as `doc` rows.

## Public API and runner (F)

| Option | Description | Selected |
|--------|-------------|----------|
| API version 2, additive, consumer-minimal names | Matches DEC.75 and the version gate | ✓ |
| Stay at 1 with feature negotiation | Gate forces a bump per name | |
| One `schway_control` entry | Untyped, invisible to the name gate | |

**Choice:** version 2 with seven functions (six from the report plus `schway_get_trace`); libFuzzer with an in-repo replay `main`; pull TTY ring.

## Gates, docs and fixture toolchain (G)

| Option | Description | Selected |
|--------|-------------|----------|
| Cap 1,520, built-in armips ExternalProject on every leg | Matches CPU-04, about 7 s locally | ✓ |
| Prebuilt armips binaries | Needs a release pipeline or committed executables | |

**Choice:** ceiling at most 1,520 (measured 1,392); `tools/*.py` not counted while CI never runs them; mkpsxiso deferred (GPL-2.0).

---

## Claude's Discretion

Container layout of `sst-sample.bin`, launch-block address, fault and trace record sizes, plan order and waves, md5 source.

## Deferred Ideas

Replay-based capture, nugget/amidog lanes, CPU-bound perf fixture, `--stop-on-unbuilt`, armips caching, `schway_poke` and `schway_get_memory`, BIOS hook and `schway_load_bios` (phase 4), mkpsxiso licence note (phase 6), GTE double-execute (phase 3).
