---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 16
subsystem: dma
tags:
  - dma
  - fixture
  - mailbox
  - otc
  - linked-list
  - timer

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: OTC, the sync mode 2 lists and the stop by CHCR 0 (plans 08 and 14), Timer 2 (plan 09), the mailbox fixture pattern (plan 12)
provides:
  - fixtures.dma_chain, a judged fixture (id 8) that runs OTC and the chain-looping cases on the real CPU
  - ten dma_chain rows in tests/diag/expected.csv, each doc or hw with a comma-free citation
  - the OTC time of 4,096 words read through Timer 2, as a doc range with the measured value
affects:
  - 03-17 and later plans of the phase that register fixtures or read expected.csv
  - phase-4 GPU (the list cases use the GPU channel with the GPU unbuilt)

actuals:
  tokens: 2850
  tasks: 2
  commits: 2

plan_head_before: 5a3fe76143b1335935d165ed2c833235c9abdbfe
plan_head_after: 75dff473dac17cea37dd924fd2804863ff4e3bf1
commits: 2

tech-stack:
  added: []
  patterns:
    - "a fixture that reproduces a public hardware test's register writes in order and records its outcomes as values, bit 0 finished and bit 1 interrupt flag in one record"
    - "a timing row is a doc range, never hw: the model value widened by a margin, with the measured hardware figure named as corroboration only"

key-files:
  created:
    - tests/fixtures/src/dma_chain.asm
  modified:
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/diag/expected.csv
    - docs/hardware/dma.md

key-decisions:
  - "The first word is the one written first, at MADR (00103FF8h, doc), and the last is the end marker 00FFFFFFh (hw); the table sits at 80100000h so both are fixed numbers"
  - "A list outcome is one record: bit 0 the finished flag and bit 1 the DICR flag of the channel, so the empty list is 3 and the two looping lists are 0"
  - "The master-off control pre-fills the four words with the hardware test's own pattern so the word at MADR keeps 44444444h, and writes CHCR 0 afterwards so no start is left waiting for the re-enable"
  - "No citation carries a semicolon or a comma: the judge splits rows with a CMake list, so a semicolon would add a field"

patterns-established:
  - "list_case: the one subroutine that starts a chain on the GPU channel, runs the 1000-pass loop, reads CHCR and DICR, and stops with CHCR 0"

requirements-completed:
  - GTE-02

coverage:
  - id: D1
    description: "dma_chain.exe builds a 4,096-word ordering table with OTC from assembled code and records the first word written (00103FF8h), the end marker (00FFFFFFh), D6_CHCR after the transfer (00000002h), DICR (C0C00000h after writing 00C00000h) and I_STAT bit 3 (1); each is judged against its row"
    requirement: "GTE-02"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.dma_chain$' (inside cmake --workflow --preset ci)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The master-off control (DPCR channel 6 enable clear, table pre-filled) leaves the first word written at 44444444h"
    requirement: "GTE-02"
    verification:
      - kind: integration
        ref: "tests/diag/expected.csv dma_chain,otc_master_off judged by fixtures.dma_chain"
        status: pass
    human_judgment: false
  - id: D3
    description: "The empty complete list reads finished and interrupt (3), and the single and the double self-referencing lists read not finished and no interrupt (0), each after the 1000-pass work loop and stopped with CHCR 0"
    requirement: "GTE-02"
    verification:
      - kind: integration
        ref: "tests/diag/expected.csv dma_chain,list_empty and list_single and list_double judged by fixtures.dma_chain"
        status: pass
    human_judgment: false
  - id: D4
    description: "One OTC transfer of 4,096 words timed by Timer 2 on the system clock reads 4,362 clocks, inside the doc range 10F4h..1130h (4,340..4,400)"
    requirement: "GTE-02"
    verification:
      - kind: integration
        ref: "tests/diag/expected.csv dma_chain,otc_time judged by fixtures.dma_chain"
        status: pass
    human_judgment: false
  - id: D5
    description: "The ten dma_chain rows have six comma-free fields, source doc or hw, and the timing row is doc with a psx-spx citation"
    requirement: "GTE-02"
    verification:
      - kind: other
        ref: "test $(grep -c '^dma_chain,' tests/diag/expected.csv) -ge 10 and the NF-not-6 and timing-row greps from the plan, exit 0"
        status: pass
    human_judgment: false
  - id: D6
    description: "A wrong row is caught: a list_single expected 1 and an otc_time range ending at 1100h each fail the test naming the record"
    requirement: "GTE-02"
    verification:
      - kind: other
        ref: "one-off edit of expected.csv, ctest -R '^fixtures\\.dma_chain$' failed naming both rows, file restored from a copy"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 16: The dma_chain fixture Summary

**An assembled fixture runs OTC, the master-off control, the empty and two self-referencing GPU-channel lists and a Timer 2 reading of the OTC time on the real CPU, with ten `doc` or `hw` rows in `expected.csv` that judge them (OTC time measured at 4,362 clocks against a 4,340..4,400 range).**

## Performance

- **Duration:** 5 min (from the previous plan's last commit to this plan's last code commit)
- **Completed:** 2026-11-10T07:36:00Z
- **Tasks:** 2
- **Files modified:** 5 outside `.planning`

## Accomplishments

- `tests/fixtures/src/dma_chain.asm` (fixture id 8): the same register writes in the same order as the public `dma/otc-test` and `dma/chain-looping` programs. It records ten values: the first and last ordering-table word, CHCR 00000002h, DICR C0C00000h, I_STAT bit 3, the master-off control, three list outcomes and the Timer 2 OTC time.
- `fixtures.dma_chain` is registered through `mailbox_judge` with `-DFRAMES=30` under the labels `dma fixtures portable`, so `ctest -L '^dma$'` now runs 33 tests.
- Ten rows in `tests/diag/expected.csv`. `hw` only where the value is the literal an upstream hardware test asserts or prints (end marker, CHCR readback, the master-off word, the three list outcomes), each citing the file, line and `psx.log` line at `f727802fead1`. `doc` for the first word, DICR, I_STAT and the timing range, which psx-spx states or the model gives.
- The OTC time is a `doc` row: 4,340..4,400 clocks written `10f4..1130`, the psx-spx per-word model (4,352) widened by a margin. Nugget's 4,358 on two consoles is named in the citation as corroboration only. The core reads **4,362** clocks through Timer 2 (reads and write overhead included).
- `docs/hardware/dma.md` names the fixture, the range and the measured value.

## Task Commits

1. **Task 1: Tracer - the fixture runs one OTC and its first record matches its row** - `42763ab` (feat)
2. **Task 2: Every record, the Timer 2 timing range, and the label check** - `75dff47` (feat)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^fixtures\.dma_chain$'` were run, passed, and the expansion task went ahead.

## Files Created/Modified

- `tests/fixtures/src/dma_chain.asm` - the fixture: OTC, master-off control, `list_case` subroutine, Timer 2 timing
- `tests/fixtures/tests.cmake` - `schway__fixture(dma_chain.exe ...)` and the `fixtures.dma_chain` test with its `Fault prevented:` comment
- `tests/fixtures/inventory.txt` - the new test in byte order
- `tests/diag/expected.csv` - ten `dma_chain` rows
- `docs/hardware/dma.md` - a paragraph naming the fixture, the range and the measured time

## Decisions Made

See `key-decisions` above.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] A documentation change for a behaviour pinned by a new test**
- **Found during:** Task 2
- **Issue:** the plan's file list names no docs file, but the project rule is that a test and its documentation land together, and `docs/hardware/dma.md` states the OTC timing the fixture now pins.
- **Fix:** one paragraph in the OTC section: the fixture, the range, the `doc` label and the measured 4,362.
- **Files modified:** docs/hardware/dma.md (23,607 bytes, under its budget)
- **Verification:** `ctest -R '^docs\.'` exit 0 inside the ci workflow; no planning identifier in the page
- **Committed in:** 75dff47

**2. [Interpretation] The plan's example timing citation contains a semicolon**
- **Found during:** Task 2
- **Issue:** the plan suggests `psx-spx DMA channels timing OTC 1 clock per word; corroborated by nugget ...`. The judge reads each row through a CMake list, where a semicolon starts another field, so the row would fail the six-field check.
- **Fix:** the citation uses "and" instead, still comma-free and still matching the plan's acceptance pattern.
- **Files modified:** tests/diag/expected.csv
- **Committed in:** 75dff47

**3. [Interpretation] The plan's closing check mentions `hashes.txt` listing `dma_chain.exe`**
- **Found during:** verification
- **Issue:** `hashes.txt` holds the runner's five frame and image hashes, not fixture images, so no `dma_chain.exe` line exists or is needed; the fixture is covered by `fixtures.dma_chain` and `check_junit`.
- **Fix:** none; noted here.

---

**Total deviations:** 3 (1 documentation addition, 2 interpretations)
**Impact on plan:** none on the plan's tests, names or files.

## Issues Encountered

- None blocking. The first run gave every expected value, the timing read 4,362 on the first try inside 4,340..4,400.

## Not runnable locally

The `asan`, `be32` and `wasm32` lanes need clang-21 or wasmtime that this host lacks; hosted CI runs them. `cmake --workflow --preset ci` passes locally (329 tests, `check_junit` exit 0, `ctest -L '^dma$'` 33 of 33). The `fixtures` directory is in `ci.txt` and `wasm32.txt`, so `fixtures.dma_chain` runs on the wasm32 lane in hosted CI.

## Known Stubs

None. The GPU channel's device end stays the stand-in sink, and the fixture's GP1 writes are the unbuilt GPU accesses the runner already counts (`unbuilt dev=gpu n=9`).

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for the next plan of the phase. A later plan that builds the GPU keeps `list_case` working: its list outcomes are the same hardware rows.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*

## Self-Check: PASSED
