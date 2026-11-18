---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 12
subsystem: testing
tags:
  - gte
  - fixtures
  - cop2
  - mailbox
  - vectors

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the 26-case include and the 1,150-case vector file (plan 02), the CPU's COP2 path with the busy stall (plan 06), the computing engine (plan 07)
provides:
  - tests/fixtures/src/gte_vectors.asm, a fixture that replays 26 hardware GTE cases through MTC2, CTC2, COP2, SWC2, MFC2 and CFC2 and records the CRC-32 of the 64 registers read back
  - 26 hw rows for gte_vectors in tests/diag/expected.csv, one per case, each with the table path and test number
  - fixtures.gte_vectors and fixtures.gte_vectors.guard, both labelled fixtures, gte, portable
  - the guard case gte_vectors_guard in boot_tests.c, which recomputes every row's value from the vector file
affects:
  - 03-13
  - phase-3 plans that append fixtures to tests/fixtures/tests.cmake and expected.csv

actuals:
  tokens: 3400
  tasks: 3
  commits: 2

plan_head_before: 1e50f112b3a4a0ed86fbcd982af07377b5b103b8
plan_head_after: f8edc3b6c79a33dbca4c89d10f5e0997c6f5205a
commits: 2

tech-stack:
  added: []
  patterns:
    - "a fixture that needs a different instruction word per case stores the word into a slot in uncached RAM (the KSEG1 alias of an image label) and calls the slot, so no cache line holds the previous word"
    - "an expected value is tied to its source by two paths: a fixture record judged against the CSV, and a C guard that recomputes the CSV value from the committed vector file"

key-files:
  created:
    - tests/fixtures/src/gte_vectors.asm
  modified:
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - tests/fixtures/boot_tests.c
    - tests/diag/expected.csv

key-decisions:
  - "The register moves are emitted as .word encodings of MTC2, CTC2, MFC2 and CFC2 inside macros, because armips does not accept a macro argument as a coprocessor register number; COP2 and SWC2 use the mnemonics"
  - "The looping variant is selected with -definelabel LOOP 1, not -equ LOOP 1, because .ifndef does not accept an equ and the default build has to leave LOOP undefined"
  - "The guard reads the vector file's output words as bytes and takes their CRC-32 directly: the file is already little-endian, so the same code gives the same answer on the big-endian lane"

patterns-established:
  - "Pattern: tests/fixtures/tests.cmake takes a per-fixture extra dependency through schway__fixture_extra, set before the one call and cleared after it"

requirements-completed: []

coverage:
  - id: D1
    description: "gte_vectors.exe runs 26 hardware cases on the CPU's real COP2 path (SR.CU2 by MTC0, registers written 0 to 63 by MTC2 and CTC2, the command from an uncached slot with an SWC2 behind it, registers read 0 to 63 by MFC2 and CFC2) and seals 26 records, each the CRC-32 of the 64 words read back"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.gte_vectors$' --no-tests=error: exit 0"
        status: pass
    human_judgment: false
  - id: D2
    description: "Every gte_vectors row of tests/diag/expected.csv is hw, its value is the CRC-32 of the case's 64 output words from the pinned table, and its citation names the path and the test number with no comma; 26 rows, no invented case"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "tests/diag/expected.csv rows gte_vectors,test_N judged by fixtures.gte_vectors; grep -c '^gte_vectors,' prints 26 and grep '^gte_vectors,' | grep -qv ',hw,' exits 1"
        status: pass
    human_judgment: false
  - id: D3
    description: "fixtures.gte_vectors.guard recomputes each row's CRC-32 from tests/vectors/gte/ps1tests-1150.bin and fails on a drifted value, a non-hw row, a case outside the file, a seventh field or no row at all"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "tests/fixtures/boot_tests.c#gte_vectors_guard (fixtures.gte_vectors.guard); a flipped value in test_904 failed both tests by name, a CSV with no gte_vectors row exited 1"
        status: pass
    human_judgment: false
  - id: D4
    description: "Both tests carry the labels fixtures, gte and portable, so ctest -L '^gte$' covers the fixture half of the first success criterion, and the fixture image digest is in hashes.txt through the existing glob"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -L '^gte$' --no-tests=error: exit 0, 24 tests; grep -q 'fixture gte_vectors.exe sha256' build/ci/hashes.txt: exit 0; check_junit.cmake with tests/inventory/ci.txt: exit 0, 314 tests ran once each"
        status: pass
    human_judgment: false
  - id: D5
    description: "One --bench run of a looping build of the fixture is recorded once as a command and an exit status; no perf gate is added and the looping image is neither registered nor committed"
    requirement: "GTE-01"
    verification:
      - kind: other
        ref: "build/ci/schwaystation-run --exe gte_vectors_loop.exe --bench 600: exit 0, p50_us 4398"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 12: The GTE fixture on the real CPU path Summary

**gte_vectors.exe replays 26 hardware GTE cases through MTC2, CTC2, COP2, SWC2, MFC2 and CFC2 and seals one CRC-32 record per case; every record matches its hw row, and a C guard ties each row back to the pinned vector file.**

## Performance

- **Duration:** about 10 min
- **Started:** 2026-11-10T06:51Z
- **Completed:** 2026-11-10T07:00Z (code; the close-out commit follows)
- **Tasks:** 3 (two committed; the third changed no file)
- **Files modified:** 5 (1 created)

## Accomplishments

- `tests/fixtures/src/gte_vectors.asm` sets SR.CU2 with `mtc0`, then for each case in `gte_cases.inc` writes registers 0 to 63 with zero and then with the inputs (MTC2 for 0 to 31, CTC2 for 32 to 63), issues the command, reads the 64 registers back (MFC2, CFC2, one load delay slot apart) and stores the CRC-32 of the 256 bytes as the next mailbox record. A command word of FFFFFFFFh skips the command. The command is stored into a slot reached through the KSEG1 alias of its address, with an SWC2 of data register 0 right behind it (the busy stall), so no instruction cache line ever holds a previous case's word.
- The first run matched on the first try: all 26 records equal the CRCs computed from the table, so the engine, the CPU's COP2 moves, the load delays and the stall agree with the hardware capture for 22 commands and four register-only cases.
- `tests/diag/expected.csv` holds 26 `gte_vectors` rows, every one `hw`, with the citation `ps1-tests f727802fead1 gte/test-all/tests.c test N` (no comma). The values were computed once from the `.bin` by a throwaway script, not from the core.
- `fixtures.gte_vectors.guard` (`gte_vectors_guard` in `boot_tests.c`) recomputes each row's value from `tests/vectors/gte/ps1tests-1150.bin` in C with the core's CRC-32 and fails on a drifted value, a source other than `hw`, a case outside the file, a seventh field, or no row at all. A flipped digit in one row failed the fixture and the guard, each naming `test_904`.
- `hashes.txt` lists `fixture gte_vectors.exe sha256 68d8b8c28ce279b9d591a15ebc4d983d0c23d4858ee239b7dfef2aecdcb59b00` through its glob; no hash line was written by hand.

## Task Commits

1. **Task 1: Tracer, the fixture replays one table case through COP2 and its CRC record matches the hw row** - `8299739` (feat)
2. **Task 2: All cases, all rows, and the guard that recomputes them from the vector file** - `f8edc3b` (test)
3. **Task 3: Cross-lane check and the one bench run** - no commit: it needed no file change, because the `LOOP` branch was written with the fixture in Task 1

**Plan metadata:** the close-out commit (`docs(03-12): complete ...`) follows this file.

The tracer gate ran in the default end-of-phase mode with an automated-only verify: `cmake --workflow --preset ci` and the `fixtures.gte_vectors` ctest were re-run after Task 1, passed, and the expansion went ahead.

## Files Created/Modified

- `tests/fixtures/src/gte_vectors.asm` - the fixture (id 7), with the `LOOP` switch for the one-off timing build
- `tests/fixtures/tests.cmake` - the `gte_vectors.exe` build with `gte_cases.inc` as an extra dependency, `fixtures.gte_vectors` and `fixtures.gte_vectors.guard`
- `tests/fixtures/inventory.txt` - the two tests, labelled fixtures, gte, portable
- `tests/fixtures/boot_tests.c` - the `gte_vectors_guard` case
- `tests/diag/expected.csv` - the 26 `hw` rows

## Decisions Made

See `key-decisions`. Case numbers chosen for the 26 records are those plan 02 fixed in the include: 51, 138, 151, 201, 251, 301, 351, 401, 451, 501, 551, 601, 651, 701, 751, 801, 851, 904, 951, 1001, 1051, 1101 (one per command) and register-only cases 1, 2, 3, 4. Two pairs of different commands (351 and 401, 501 and 551) have the same output CRC; both values are copied from the table, and the guard confirms them against the vector file.

## Bench run (recorded once, no gate)

- Command, from the repository root: `build/ci/schwaystation-run --exe "$TMPDIR/gte_vectors_loop.exe" --bench 600`, where the image was assembled from the unchanged source with the built armips and `-definelabel LOOP 1`. Exit status 0.
- Report: `frames 600`, `p50_us 4398`, `p90_us 5163`, `p99_us 5441`, `max_us 5668`, `max_rss_kib 4800`. The looping image was never registered or committed; the default build is unchanged and `fixtures.gte_vectors` passes with it.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The loop variant is selected with `-definelabel`, not `-equ`**
- **Found during:** Task 1 (assembling)
- **Issue:** the plan names `-equ LOOP 1`. armips replaces an `-equ` symbol textually, so `.ifndef LOOP` fails with a directive parameter error when it is given, and the default build (which must leave `LOOP` undefined) needs that test; `header.inc` already uses `-definelabel` with `.ifndef` for `PC0`.
- **Fix:** `.ifndef LOOP` then `.definelabel LOOP, 0`, and the variant is built with `-definelabel LOOP 1`.
- **Files modified:** tests/fixtures/src/gte_vectors.asm
- **Verification:** the default build is the registered fixture and passes; the variant assembled and ran under `--bench`.
- **Committed in:** 8299739

**2. [Rule 3 - Blocking] The register moves are `.word` encodings inside macros**
- **Found during:** Task 1 (assembling)
- **Issue:** armips accepts `mtc2 t0, $5` but not a macro argument in the register position (`mtc2 t0, $n`), and has no `.for`, so the 64 moves cannot be unrolled with the mnemonic.
- **Fix:** each move is `.word 0x4880...` with the register number in the instruction and the mnemonic in the comment, expanded through nested macros of 4 and 16. The moves were checked against armips's own encoding of `mtc2 t0, $5` (48882800h).
- **Files modified:** tests/fixtures/src/gte_vectors.asm
- **Verification:** the 26 records match the hardware table.
- **Committed in:** 8299739

---

**Total deviations:** 2 auto-fixed (both blocking, both in the assembler syntax)
**Impact on plan:** no change to any deliverable; the default image, the rows and the tests are as specified.

## Issues Encountered

None beyond the two syntax points above. Task 3's source change (the `LOOP` branch) was already part of the fixture, so it has no commit of its own.

## Known Stubs

None.

## Threat Flags

None. The fixture is test code that runs only inside the emulated instance; the guard reads two repository files given as arguments.

## Not run locally

`asan` and `be32` need clang-21, which this host lacks; hosted CI runs them. The guard computes the CRC over the file's little-endian bytes and the fixture's record is a 32-bit word, so no byte-order dependence was added. The `wasm32` lane was not run either.

## Evidence (commands and exit statuses)

- `cmake --workflow --preset ci`: exit 0 (314 tests)
- `ctest --test-dir build/ci -R '^fixtures\.gte_vectors(\.guard)?$' --no-tests=error`: exit 0
- `ctest --test-dir build/ci -L '^gte$' --no-tests=error`: exit 0 (24 tests)
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0 (314 tests ran once each)
- `grep -q 'fixture gte_vectors.exe sha256' build/ci/hashes.txt`: exit 0
- `sh scripts/hygiene.sh`: exit 0; `cmake --workflow --preset hygiene`: exit 0
- `build/ci/schwaystation-run --exe "$TMPDIR/gte_vectors_loop.exe" --bench 600`: exit 0, p50 4398 us

## Next Phase Readiness

The fixture half of the first success criterion is in place. The property test (plan 13) completes GTE-01. The two fixture files this plan touched, `tests/fixtures/tests.cmake` and `tests/diag/expected.csv`, are free for the next sequential fixture plan.

## Self-Check: PASSED

- Created file exists: `tests/fixtures/src/gte_vectors.asm`.
- Commits `8299739` and `f8edc3b` are ancestors of HEAD; the plan measured 2 commits from the recorded base `1e50f11`.
- Every acceptance command exited as the plan says.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
