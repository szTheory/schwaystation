---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 26
subsystem: gte
tags:
  - gte
  - numerics
  - divide
  - rtps
  - vectors

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the seeded GTE register file, the replay harness and the 1,150-case container (plan 05)
provides:
  - 64-bit accumulators with the MAC, IR, Z FIFO, screen FIFO and OTZ saturations and their FLAG bits in the seed's placement
  - sar64, an arithmetic right shift built on unsigned arithmetic, and no signed left shift anywhere in the engine
  - the reciprocal table as a literal static const uint8_t unr_table[257] and the divide that reads it
  - RTPS, RTPT, NCLIP, AVSZ3, AVSZ4, SQR and OP in the engine's command switch
  - gte.vectors.part1 widened to 400 cases, gte.unr_table and gte.nclip_fake_bits
  - the numerics, divide and first-commands sections of docs/hardware/gte.md
affects:
  - 03-07
  - 03-13
  - phase-3 plans that port the other fifteen commands, write the reference model or run the GTE under the CPU

actuals:
  tokens: 7700
  tasks: 2
  commits: 2

plan_head_before: d67a0a36d4f758e47353a790add32d7128bbc8da
plan_head_after: 8b5b737f691950cead14ba3c25c72ee7176d924b
commits: 2

tech-stack:
  added: []
  patterns:
    - "a command builds FLAG in a local run struct together with sf and lm, and the register is written once when the command ends"
    - "every value is an int64_t, so each multiply is a widening multiply; a signed left shift is a multiply by a constant and a right shift is sar64"
    - "a table that the hardware cases pin only at some entries is a literal in the source plus a test that regenerates it from the documented formula"

key-files:
  created: []
  modified:
    - src/gte.c
    - src/gte.h
    - tests/gte/gte_tests.c
    - tests/gte/tests.cmake
    - tests/gte/inventory.txt
    - docs/hardware/gte.md

key-decisions:
  - "A MAC1 to MAC3 value is checked against 44 bits after every addition of a matrix row and wrapped to 44 bits; the IR clamp takes the low 32 bits of the shifted MAC, as the seed does, because the hardware cases do not separate it from a clamp on the full value and the seed passes them all"
  - "IR3 in the projection takes its flag from the value over 1000h clamped as if lm were clear and its register from MAC3 clamped with the command's lm"
  - "a command that computes clears FLAG first by building it in a local and recomputes bit 31 last; the fifteen commands not yet ported and the unlisted numbers still only recompute bit 31, so they change no register"
  - "the table accessor schway__gte_unr_entry lives in gte.h so the test reaches the static literal without including the source file"

patterns-established:
  - "Pattern: a numerics helper that is only used by a later task is added in that task, so each commit builds warning-free"

requirements-completed: []

coverage:
  - id: D1
    description: "RTPS matches its 50 hardware cases end to end on integer numerics, with the 50 register-only cases still passing"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "schwaystation-test-gte vectors tests/vectors/gte/ps1tests-1150.bin --expect 100 --ops reg,01 (100 cases, 0 failures, exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "RTPS, RTPT, NCLIP, AVSZ3, AVSZ4, SQR and OP match their 50 hardware cases each and the 50 register-only cases, 400 replayed in the upstream order"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gte\\.vectors\\.part1$' (--expect 400, 0 failures, exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The reciprocal table literal equals the formula in all 257 bytes"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.unr_table$' (exit 0; a one-entry corruption of the literal fails it while gte.vectors.part1 still passes)"
        status: pass
    human_judgment: false
  - id: D4
    description: "NCLIP issued with the unused command bits set (0140006h and three other patterns) gives the same 64 outputs and cycles as 6, on triangles of both windings and with stale FLAG and MAC0"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.nclip_fake_bits$' (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The engine's numerics hold no floating point, no signed right shift and no signed left shift, and the GTE page describes the numerics, the divide and the commands without a planning identifier"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset hygiene (hygiene.no_float among 23 tests, exit 0); grep -qE planning-id pattern over docs/hardware/gte.md src/gte.c tests/gte/gte_tests.c exits 1 as required; ctest -R '^docs\\.' exit 0"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 26: GTE numerics, the divide and the first seven commands Summary

**Integer-only GTE numerics (64-bit accumulators, 44-bit MAC flags, an unsigned-built arithmetic shift and the 257-byte reciprocal table with its divide) carry RTPS, RTPT, NCLIP, AVSZ3, AVSZ4, SQR and OP through all 400 replayed hardware cases, with a formula check of the table and a fake-command-bits case for NCLIP**

## Performance

- **Duration:** 5 min
- **Started:** 2026-11-10T05:40:10Z
- **Completed:** 2026-11-10T05:45:30Z (code and docs; the close-out commit follows)
- **Tasks:** 2
- **Files modified:** 6 outside `.planning`

## Accomplishments

- `src/gte.c` now holds the numerics: a `run` struct carries FLAG, `sf` and `lm` for one command; `check_mac` sets the 44-bit overflow bits (30, 29, 28 positive, 27, 26, 25 negative) and the 32-bit MAC0 bits (16, 15); `set_mac` shifts and stores; `sat_ir`, `set_otz`, `push_sz` and `push_sxy` apply the IR, OTZ, Z FIFO and screen clamps and flag bits. Every value is an `int64_t`, there is no signed shift, `sar64` is built on unsigned arithmetic, and the seed's single-precision factor and every comment word naming a non-integer type are gone, so `hygiene.no_float` passes.
- The divide reads `unr_table`, a literal `static const uint8_t[257]` generated from the psx-spx formula, and `schway__gte_unr_entry` lets `gte.unr_table` regenerate and compare all 257 bytes. The check earns its place: corrupting one entry fails it while all 400 hardware cases still pass, so the cases alone do not pin the table.
- The tracer, RTPS, matched its 50 cases on the first run (100 replayed with the register-only ones). Task 2 added RTPT (IR0 from the last vertex only), NCLIP, AVSZ3, AVSZ4, SQR and OP and `gte.vectors.part1` became `--expect 400 --ops reg,01,06,0c,28,2d,2e,30`, which also matched on the first run.
- `gte.nclip_fake_bits` issues NCLIP as `0140006h`, `1F00006h`, `0000B46h` and `0F80006h` on triangles of both windings (areas 100 and -100 and one at the range edges) with a stale FLAG and MAC0, and requires the same 64 reads and cycles as `6`.
- `docs/hardware/gte.md` gained "The numerics", "The divide" and "The commands built so far", the state paragraph and the vectors paragraph now say what is built (14,450 bytes against 61,440; `docs.budgets` passes).

## Task Commits

1. **Task 1: Tracer - the numerics, the UNR divide and RTPS match RTPS's 50 hardware cases** - `ff59ad0` (feat)
2. **Task 2: The other six commands, the UNR and fake-bit tests, and the numerics section of the page** - `8b5b737` (feat)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran with an automated-only verify: `cmake --workflow --preset ci`, `ctest -R '^gte\.vectors\.part1$'` and `cmake --workflow --preset hygiene` were re-run after the tracer commit, all exited 0, and the expansion task went ahead.

## Files Created/Modified

- `src/gte.c` - the numerics, the table, the divide, the projection and the six other commands; the cycle switch is unchanged
- `src/gte.h` - the `schway__gte_unr_entry` declaration
- `tests/gte/gte_tests.c` - the `unr_table` and `nclip_fake_bits` cases and their usage text
- `tests/gte/tests.cmake`, `tests/gte/inventory.txt` - `gte.vectors.part1` widened to 400, the two new tests, with `Fault prevented:` comments
- `docs/hardware/gte.md` - the numerics, divide and commands sections

## Decisions Made

See `key-decisions` above. One point for the plan that ports the other fifteen commands: `set_mac_ir`, `check_mac`, `extend`, `mat_elem` and `sat_ir` are the shared helpers, and the clamp of IR to the low 32 bits of the shifted MAC (the seed's behavior) is the one place the numerics may differ from a clamp on the full 44-bit value when `sf` is clear. The 1,150 cases do not distinguish the two, so the reference-model differential in plan 03-13 is where it would show.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] A declaration added to `src/gte.h`**
- **Found during:** Task 2 (the `unr_table` case)
- **Issue:** the plan names `unr_table` as a `static const` literal in `src/gte.c`, and `src/gte.h` is not in the plan's file list, but a test in another translation unit cannot read a static array.
- **Fix:** `uint8_t schway__gte_unr_entry(uint32_t i)` in `gte.c`, declared in `gte.h` with a one-line comment; it returns 0 past the end so the sweep cannot read outside the table.
- **Files modified:** `src/gte.c`, `src/gte.h`
- **Verification:** `gte.unr_table` passes and fails on a corrupted entry; the register sweep is unaffected.
- **Committed in:** `8b5b737`

---

**Total deviations:** 1 auto-fixed (1 blocking)
**Impact on plan:** one header line for testability; no behavior or scope change.

## Issues Encountered

- Not run locally: the `asan` and `be32` lanes need clang-21, which this host lacks; hosted CI runs them. The engine uses fixed-width integers only, widening multiplies on `int64_t`, unsigned shifts and no signed shift, so UBSan has nothing signed to flag; `gte.regs.sweep` already drives every command number on edge-value registers for that lane.
- A first `sed -i` edit of `tests/gte/tests.cmake` failed on this host (BSD `sed` wants a backup suffix) and changed nothing; the edit was made with the editor tool and the first gate run was repeated after it.
- The broken-windows ledger `.planning/WINDOWS.md` does not exist, so the stubs below were not appended to it.

## Known Stubs

| File | Stub | Resolved by |
|------|------|-------------|
| src/gte.c | `schway__gte_exec` still returns only the cycle count and recomputes FLAG bit 31 for the fifteen commands not ported (DPCS, INTPL, MVMVA, NCDS, CDP, NCDT, NCCS, CC, NCS, NCT, DCPL, DPCT, GPF, GPL, NCCT); the 750 cases that run them fail until they land | plan 03-07 |

`docs/hardware/gte.md` says so in its "State of the engine" section.

## Threat Flags

None. No endpoint, auth path, file access or schema at a trust boundary is added; the new test cases read no file and the engine takes only its own struct.

## Evidence (commands and exit statuses)

- `cmake --workflow --preset ci`: exit 0 (258 tests, 100% passed)
- `ctest --test-dir build/ci -R '^gte\.vectors\.part1$' --no-tests=error --output-on-failure` after Task 1: exit 0
- `build/ci/tests/schwaystation-test-gte vectors tests/vectors/gte/ps1tests-1150.bin --expect 100 --ops reg,01`: exit 0 (100 cases, 0 failures)
- `build/ci/tests/schwaystation-test-gte vectors tests/vectors/gte/ps1tests-1150.bin --expect 400 --ops reg,01,06,0c,28,2d,2e,30`: exit 0 (400 cases, 0 failures)
- `ctest --test-dir build/ci -L '^gte$' --no-tests=error --output-on-failure`: exit 0 (12 of 12, the four gte tests among them)
- `ctest --test-dir build/ci -R '^gte\.(vectors\.part1|unr_table|nclip_fake_bits)$' --no-tests=error`: exit 0
- `cmake --workflow --preset hygiene`: exit 0
- Acceptance: `grep -q 'static const uint8_t unr_table\[257\]' src/gte.c && grep -q 'sar64' src/gte.c` exit 0; `grep -q -- '--expect 100 --ops reg,01' tests/gte/tests.cmake` exit 0 (at the Task 1 commit); `grep -q -- '--expect 400' tests/gte/tests.cmake` exit 0; the planning-identifier grep over the page, `src/gte.c` and `tests/gte/gte_tests.c` exit 1 as required
- Mutation check: one table entry changed, `ctest -R '^gte\.'` failed with only `gte.unr_table` failing; restored, all four passed

## Next Phase Readiness

The numerics, the divide and the shared helpers are in place, so plan 03-07 adds the other fifteen commands and the full cycle table through them, and plan 03-13's reference model remains independent of this file (it reads `tests/gte/vectors.h`, which holds no seed text).

## Self-Check: PASSED

- Modified files exist: `src/gte.c`, `src/gte.h`, `tests/gte/gte_tests.c`, `tests/gte/tests.cmake`, `tests/gte/inventory.txt`, `docs/hardware/gte.md`.
- Commits `ff59ad0` and `8b5b737` are ancestors of HEAD; the plan measured 2 commits from the recorded base `d67a0a3`.
- Every task's acceptance commands exited as the plan says; `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` pass.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
