---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 17
subsystem: timers
tags:
  - timers
  - fixture
  - mailbox
  - interrupts
  - profile

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the three counters with their interrupts and the vertical-blank push (plans 09 to 11), the judged-fixture pattern (plans 12 and 16)
provides:
  - a profile-aware mailbox judge (PROFILE and SHIFT) in tests/fixtures/tests.cmake
  - fixtures.timers_lines.ntsc_u and fixtures.timers_lines.pal, a judged fixture (id 9) that takes the vertical blank and two counters' wraps as interrupts
  - six timers_lines rows (vblank_count, div8_ticks, sysclk_ticks per profile), all doc, with ranges computed from the frame constants
  - two negative controls that judge the same run against rows one counter period up and must fail naming both counters
affects:
  - the GPU phase, which appends the dot-clock and horizontal-blank records after id 3
  - any later fixture that needs a per-profile row

actuals:
  tokens: 4500
  tasks: 2
  commits: 2

plan_head_before: 8c3a1595abcbc670c18623be1392a55df4750d4d
plan_head_after: f4afab72d285262b5b76e59783f8f701b3fb23ee
commits: 2

tech-stack:
  added: []
  patterns:
    - "a fixture that installs its own handler at the exception vector, keeps all state in registers the idle loop never uses, and snapshots counters together with the interrupt bits that belong to them"
    - "a counter reset by the vertical blank is read at the same time as the free-running counters, so the delay between a blank and the handler cancels out of the elapsed time"
    - "a negative control that shifts every row by one counter period"

key-files:
  created:
    - tests/fixtures/src/timers_lines.asm
  modified:
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/diag/expected.csv
    - docs/testing.md

key-decisions:
  - "The handler is installed at 80000080h by copying a four-word jump, as the exception-matrix fixture does, not through the mailbox's handler word: the stub kernel writes status FFh into the mailbox on every entry and fetches from the BIOS at 29 cycles a word, so a frame end inside its path reads as a fault at the runner's once-per-frame mailbox read and the path alone is about 700 cycles of jitter"
  - "Timer 1 in synchronisation mode 1 (reset at the vertical blank) gives the time since the blank at the handler's reads; each snapshot subtracts it, so the handler's delay, including a blank taken after a timer handler's return, cancels"
  - "A wrap bit seen set in I_STAT is counted as one more wrap and acknowledged by the same handler entry; the reads of I_STAT, the counts and I_STAT again repeat until bits 0, 4 and 6 agree, so no snapshot is short or long by 10000h"
  - "The jitter is J = 25 cycles from the longest instruction of the whole program (26 cycles: its own, a cache fill of 8, a drain of four queued stores 16, an I/O read 1), not of the idle loop alone, because a frame end can fall inside the handler"

patterns-established:
  - "mailbox_judge -DPROFILE=<region> judges only the rows for * or that region and numbers them in file order"

requirements-completed:
  - GTE-03

coverage:
  - id: D1
    description: "mailbox_judge takes PROFILE and judges only the rows whose profile is * or that region, numbering them in order, and SHIFT for negative controls; the existing * fixtures judge as before"
    requirement: "GTE-03"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.(exc_matrix|gte_vectors|dma_chain|timers_lines\\..*)$'"
        status: pass
    human_judgment: false
  - id: D2
    description: "timers_lines.exe counts the vertical blank as an interrupt on both profiles and records 23 after 23 blanks"
    requirement: "GTE-03"
    verification:
      - kind: integration
        ref: "fixtures.timers_lines.ntsc_u and fixtures.timers_lines.pal, row vblank_count"
        status: pass
    human_judgment: false
  - id: D3
    description: "Timer 2's system-clock over 8 steps between the 3rd and the 23rd blank, counted as wraps times 10000h plus the residue, lie in the doc ranges 15994d..15995e (NTSC) and 19f8a2..19f8b3 (PAL)"
    requirement: "GTE-03"
    verification:
      - kind: integration
        ref: "fixtures.timers_lines.ntsc_u and fixtures.timers_lines.pal, row div8_ticks"
        status: pass
    human_judgment: false
  - id: D4
    description: "The system clocks between the same blanks, counted the same way on a system-clock counter, lie in the doc ranges acca88..accacf (NTSC) and cfc532..cfc579 (PAL)"
    requirement: "GTE-03"
    verification:
      - kind: integration
        ref: "fixtures.timers_lines.ntsc_u and fixtures.timers_lines.pal, row sysclk_ticks"
        status: pass
    human_judgment: false
  - id: D5
    description: "The ranges discriminate: the same run judged against rows one counter period (10000h) up fails and names both div8_ticks and sysclk_ticks, on each profile"
    requirement: "GTE-03"
    verification:
      - kind: integration
        ref: "fixtures.timers_lines.ntsc_u.self_test and fixtures.timers_lines.pal.self_test"
        status: pass
    human_judgment: false

duration: 4min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 17: The profile-aware judge and the timer line records Summary

**A per-profile fixture judge and `timers_lines.exe`, which takes the vertical blank and two root counters' wraps as interrupts and records the blank count, the eighths and the system clocks of 20 frames, judged against NTSC and PAL ranges computed from the frame constants.**

## Performance

- **Duration:** 4 min of execution after the reading and the derivation (started 2026-11-10T07:50:05Z, completed 2026-11-10T07:53Z)
- **Tasks:** 2
- **Files modified:** 5 (one created)

## Accomplishments

- `mailbox_judge` honours the `profile` column (`-DPROFILE`) and a `SHIFT`, about 12 lines in `tests/fixtures/tests.cmake`, outside the gate budget; every earlier `*` fixture judges as before.
- `timers_lines.exe` (fixture id 9) records `vblank_count`, `div8_ticks`, `sysclk_ticks` (ids 1 to 3, contiguous, so the GPU phase appends after them).
- Six `doc` rows, no `hw` row. The run lands inside every range; the NTSC system-clock record is 11324075, the floor of 20 frames, and the PAL one is 13616469.
- The two self tests fail with the rows shifted by 10000h and name both counters' records.

## Range computation

N = 20 frames between the 3rd and the 23rd blank. Frame constants from `src/internal.h`: NTSC 405350378496 / 715909 (566203.775 clocks), PAL 482961862656 / 709379 (680823.456 clocks). Twenty frames between two frame ends are C or C + 1 clocks, C = floor(20 * num / den): NTSC 11324075, PAL 13616469 (all integer arithmetic).

- **Where the blank lands.** The blank is raised where the instruction that crosses the frame end retires, 0 to L - 1 cycles late. L = 26: its own cycle (1), an instruction-cache fill of the fetched word (5) and three more words (3), 8 in all (`docs/hardware/bus.md` "Instruction cache", Fill cost, and "RAM and its mirrors"), the drain of four queued stores before a KSEG1 access (4 x 4 = 16, "Write queue") and an I/O read (1, "Memory control", Costs). A RAM read (5) or a store to a full queue (4 + 1) costs less; no multiply, divide or coprocessor instruction is in the program (`docs/hardware/cpu.md` "Instruction fetch", "Multiply and divide"). So J = L - 1 = 25.
- **What cancels.** Each snapshot is the counter's value minus the time since the blank, read from Timer 1 (reset by the blank), so the handler's delay cancels. What remains is the distance between the read of Timer 0 and of Timer 1 (one cache fill, up to 8) and the first-cycle convention after a reset (2): 10 more clocks each way.
- **sysclk_ticks:** [C - 25 - 10, C + 1 + 25 + 10]: NTSC acca88..accacf (width 47h), PAL cfc532..cfc579 (47h); the limit is 100h.
- **div8_ticks:** the same window P = [C - 25, C + 26] in eighths. Timer 2's read sits at an offset h in [-12, 12] clocks from the blank, which moves floor((T + h) / 8) by -2 to +3, so the difference is [ceil((P_lo - 7) / 8) - 5, floor((P_hi + 7) / 8) + 5]: NTSC 15994d..15995e, PAL 19f8a2..19f8b3 (width 11h); the limit is 20h.
- No bound was taken from a run. The runs, read afterwards, are NTSC 159955 and accaab, PAL 19f8ab and cfc555, inside every range.
- The ps1-tests figures (71,407..71,412 eighths a frame from a PAL crystal forced to NTSC, and the 112,0xx..112,5xx system-clock count that is a 16-bit wrap artefact) back no row.

## Task Commits

1. **Task 1: the profile-aware judge and the vblank-count record** - `c131047` (feat)
2. **Task 2: the system-clock and eighth records with their ranges** - `f4afab7` (feat)

**Plan metadata:** the docs commit that carries this file.

## Files Created/Modified

- `tests/fixtures/src/timers_lines.asm` - the fixture, with the derivation as its header comment
- `tests/fixtures/tests.cmake` - the PROFILE filter and SHIFT, the fixture build, the four registrations
- `tests/fixtures/inventory.txt` - the four tests
- `tests/diag/expected.csv` - the six rows
- `docs/testing.md` - the `profile` column says a test may name one

## Decisions Made

See key-decisions above.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The handler is installed at the vector, not through the mailbox's handler word**
- **Found during:** Task 1, before writing the fixture
- **Issue:** the stub kernel's path writes status FFh into the mailbox on every entry and the runner reads the mailbox once a frame; a frame end inside that path (about 700 cycles an entry, about nine entries a frame) would read as a fault. The same path would also put about 700 cycles of jitter into every snapshot, past the 100h and 20h width limits.
- **Fix:** copy the four-word jump to 80000080h as `exc_matrix.exe` does; the mailbox's handler word is 0.
- **Files modified:** tests/fixtures/src/timers_lines.asm
- **Committed in:** c131047

**2. [Rule 1 - Bug] The plan's derivation of J does not hold; the snapshot is taken against the blank**
- **Found during:** Task 2, deriving the ranges
- **Issue:** J from the idle loop's longest instruction assumes both reads sit at the same delay after the blank. A blank can land while the handler runs for a timer wrap, and then the vblank entry follows the handler's return, so the delay varies by the handler's length, and a frame end can fall in the handler, not the idle loop, so L is the longest instruction of the whole program.
- **Fix:** Timer 1 in synchronisation mode 1 is read with the other counters and subtracted, so the delay cancels; L and J are taken over the whole program (J = 25 instead of 2); the widths are 47h and 11h, inside the limits, so no stop was needed.
- **Files modified:** tests/fixtures/src/timers_lines.asm, tests/diag/expected.csv
- **Committed in:** f4afab7

**3. [Rule 2 - Missing critical] The pending wrap behind the handler (plan-checker note)**
- **Found during:** Task 2
- **Issue:** a wrap that is pending in I_STAT while the handler reads the counters, or that comes between the reads, would leave a snapshot short or long by 10000h.
- **Fix:** I_STAT, the counts and I_STAT again are read until bits 0, 4 and 6 agree; a wrap bit seen set is counted as one wrap and acknowledged by that entry. The negative control proves one period of error is detected.
- **Files modified:** tests/fixtures/src/timers_lines.asm
- **Committed in:** f4afab7

**4. [Rule 3 - Blocking] FLAGS is split as a command line**
- **Found during:** Task 2
- **Issue:** a CMake list cannot carry `--region pal` through the negative control's `CMD` list (it splits on `;`).
- **Fix:** the judge runs `separate_arguments` on FLAGS, so `-DFLAGS=--region pal` is one string; the existing `--no-stop-on-fault` is unchanged.
- **Files modified:** tests/fixtures/tests.cmake
- **Committed in:** c131047

**5. [Rule 2 - Missing critical] The documented column**
- **Found during:** Task 1
- **Issue:** a test that names a profile changes what the column means; `docs/testing.md` described only `*`.
- **Fix:** one sentence in the `profile` row of the judged-fixtures table.
- **Files modified:** docs/testing.md
- **Committed in:** c131047

---

**Total deviations:** 5 auto-fixed (3 Rule 1/2 for the fixture's design, 1 Rule 3, 1 Rule 2 docs)
**Impact on plan:** the plan's outputs are as stated: ids, labels, test names, no `hw` row. Only the means differ (vector copy, a third counter).

## Issues Encountered

None. The `asan` and `be32` legs need clang-21, which this host lacks; hosted CI runs them. The fixture is integer-only, and no core code changed.

## Known Stubs

None.

## Threat Flags

None.

## Next Phase Readiness

The GPU phase can append the dot-clock and horizontal-blank records as ids 4 and up with rows per profile; the judge already numbers per profile. Ready for the next plan of the phase.

## Self-Check: PASSED

- `tests/fixtures/src/timers_lines.asm` exists; commits `c131047` and `f4afab7` are on the branch.
- `cmake --workflow --preset ci` exits 0 (334 tests plus the 2 release tests); `ctest --test-dir build/ci -L '^timers$' --no-tests=error` exits 0; `check_junit.cmake` exits 0.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
