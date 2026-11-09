---
phase: 02-the-cpu-matches-the-public-vectors
plan: 25
subsystem: fixtures
tags:
  - fixtures
  - exceptions
  - cop0
  - mailbox
  - expected-csv

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the stub kernel and the mailbox protocol (02-16, 02-17), --stop-on mailbox (02-23), the first-fault line and --no-stop-on-fault (02-24), interrupt latencies (02-19)
provides:
  - exc_matrix.exe, a fixture with its own handler at 80000080h that records CAUSE, EPC and BadVaddr for every exception class into 30 mailbox records
  - tests/diag/expected.csv, 30 doc rows, one per record, judged by record order
  - the mailbox_judge script case in tests/fixtures/tests.cmake and the test fixtures.exc_matrix
  - a runner that names the first fault a program made when it ran on under --no-stop-on-fault
  - docs/hardware/cop0.md Exception matrix fixture section and docs/testing.md Judged fixtures section
affects:
  - 02-26
  - 02-27

actuals:
  tokens: 6600
  tasks: 3
  commits: 3

plan_head_before: 65f0cea2a101fc3a1319fb76a8e2d7e1112f2094
plan_head_after: cefca40059507015b32ce3f749384b754921832a
commits: 3

tech-stack:
  added: []
  patterns:
    - "a fixture arms three registers (trigger address, resume address, BadVaddr flag) and one shared handler records and returns, so a case is five lines of assembly"
    - "the nth CSV row of a fixture judges the record with id n; a hex value or lo..hi; an allow line is dated and an unmatched one is an error"

key-files:
  created:
    - tests/fixtures/src/exc_matrix.asm
    - tests/diag/expected.csv
  modified:
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - runner/main.c
    - docs/runner.md
    - docs/hardware/cop0.md
    - docs/testing.md

key-decisions:
  - "The mailbox judge pairs rows and records by order (row n is record id n) and lives in the fixtures script case, so tests/cmake and the 1494-line gate total are untouched"
  - "ExcCode keeps the last exception's code while a line is merely pending, so the masked-line and CAUSE-after-I_STAT rows read 1Ch (DBE from the case before) and not 0"
  - "The handler-entry record counts every entry so far (11), not only the masked window; a masked line adds none"
  - "Under --no-stop-on-fault the runner prints the first fault at the end of the run and keeps the status the run would have had"

patterns-established:
  - "A fixture-judging row cites a document by name with no comma and no semicolon; hw rows need a hardware result"

requirements-completed:
  - CPU-02

coverage:
  - id: D1
    description: "exc_matrix.exe puts its own handler at 80000080h and records CAUSE, EPC and BadVaddr for SYSCALL and BREAK with and without BD, AdEL on LH and LW, AdES on SW, overflow, reserved instruction, cop1 unusable and a data bus error, then a masked and an unmasked vblank, in 30 records"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.exc_matrix$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "One record holds CAUSE read right after an I_STAT write, after a KSEG1 read drained the write queue, as a doc row pinning the model in which the write commits inside its own instruction"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "tests/diag/expected.csv#cause_after_istat_write via fixtures.exc_matrix"
        status: pass
    human_judgment: false
  - id: D3
    description: "tests/diag/expected.csv has 30 doc rows and the mailbox judge fails a record outside its row, a row with no record, a record with no row and an allow line that applies to no record"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.exc_matrix$' (negative cases run once with cmake -DCASE=mailbox_judge on altered copies, exit 1 each)"
        status: pass
    human_judgment: false
  - id: D4
    description: "With --no-stop-on-fault the run goes past the faulting rows and the runner prints the latched first fault, asserted as kind=misaligned_read"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "fixtures.exc_matrix with FAULT_KIND=misaligned_read"
        status: pass
    human_judgment: false
  - id: D5
    description: "docs/hardware/cop0.md lists the matrix rows and that the CAUSE row pins a model; docs/testing.md documents how a fixture is judged, the CSV columns, the doc and hw sources and tests/diag/allow.txt"
    requirement: CPU-02
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.' --no-tests=error"
        status: pass
    human_judgment: false

duration: 25min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 25: The exception matrix fixture Summary

**exc_matrix.exe records every exception class, with and without a branch delay slot, in a 30-record mailbox that 30 doc rows in tests/diag/expected.csv judge, and the runner now names a fault a program ran past under --no-stop-on-fault**

## Performance

- **Duration:** about 25 min
- **Tasks:** 3 (one tracer, two auto)
- **Files modified:** 8 (2 created)

## Accomplishments

- The fixture installs its own four-word jump at 80000080h over the stub's and takes 11 triggers; one shared handler stores CAUSE, EPC minus the trigger address and, for address errors, BadVaddr, then returns with `jr` and `rfe`. For an interrupt it acknowledges I_STAT so the line goes false.
- The tail of the matrix uses the frame driver's vertical blank with I_MASK bit 0: the line read through SR masking, the CAUSE value read in the instruction after an I_STAT write (a doc row for the write-commits-in-its-own-instruction model), and the interrupt taken after `MTC0` to SR, with EPC at the `MTC0` plus 8.
- `mailbox_judge` reads the runner's `mailbox ID=VALUE` lines and the fixture's rows; row n is record n. A negative case for each failure (wrong value, extra row, extra record, unmatched allow line) was run once on altered copies and exits 1; an allow line for the changed row is accepted.
- `fixtures.exc_matrix` runs `--exe exc_matrix.exe --stop-on mailbox --no-stop-on-fault --frames 30` and requires `kind=misaligned_read`.

## Task Commits

1. **Task 1: Tracer, SYSCALL without BD recorded and judged** - `aba6f46` (test)
2. **Task 2: Every row of the matrix and the CAUSE-after-I_STAT row** - `991a734` (test)
3. **Task 3: Document the matrix and the judge** - `cefca40` (docs)

**Plan metadata:** the commit that adds this summary (docs: complete plan)

## Decisions Made

See `key-decisions`. The expected values were derived from the ExcCode numbers and the entry rules in the documents before the fixture ran; every row but three matched at the first run. The three that did not were my mistakes about which bits persist, not core faults (see Deviations).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] The runner printed no fault line under --no-stop-on-fault**
- **Found during:** Task 2
- **Issue:** The plan's third truth (and the Diagnostics page) say the runner still prints the latched first fault when stop-on-fault is off, but `runner/main.c` printed it only when the core had stopped the run on a fault, so `--no-stop-on-fault` printed nothing.
- **Fix:** `print_fault` returns whether there was a fault (kind other than none) and the end-of-run report runs under `faulted || keep_going`; the status stays what the run would have had. The Diagnostics paragraph in `docs/runner.md` and the header comment say so.
- **Files modified:** `runner/main.c`, `docs/runner.md`
- **Verification:** `fixtures.exc_matrix` asserts `kind=misaligned_read`; the runner and fixtures labels and the ci workflow pass (240 of 240).
- **Committed in:** `991a734`

**2. [Rule 3 - Blocking] armips refuses SYSCALL and BREAK in a delay slot**
- **Found during:** Task 2
- **Issue:** `b label` followed by `syscall` or `break` fails to assemble ("can't be in a delay slot").
- **Fix:** the two delay-slot triggers are `.word 0x0000000C` and `.word 0x0000000D`, with a comment.
- **Files modified:** `tests/fixtures/src/exc_matrix.asm`
- **Committed in:** `991a734`

**3. [Rule 1 - Bug] Three first-draft doc rows ignored that ExcCode persists**
- **Found during:** Task 2
- **Issue:** I wrote 00000400, 00000000 and 00000400-with-count-0 for the masked-line CAUSE, the handler-entry count and the CAUSE-after-I_STAT rows. CAUSE ExcCode is written only by an exception, so after the data bus error the field still reads 07h (CAUSE 41Ch with the line, 1Ch without), and the entry counter holds the 11 earlier entries.
- **Fix:** rows set to 0000041C, 0000000B and 0000001C with citations that say why; the docs page says so.
- **Files modified:** `tests/diag/expected.csv`, `docs/hardware/cop0.md`
- **Committed in:** `991a734`, `cefca40`

---

**Total deviations:** 3 auto-fixed (2 bugs, 1 blocking)
**Impact on plan:** the runner fix is the plan's own third truth; no scope creep.

## Issues Encountered

None beyond the deviations. The gate-line total is unchanged at 1494 of 1520: the judge lives in `tests/fixtures/tests.cmake`, which the gate does not count (the plan places it there).

## Authentication Gates

None.

## Push policy

No push, workflow dispatch or pull request action was taken, as instructed; nothing in this plan needed one. The hosted run for these commits stays with the later plans.

## Known Stubs

None.

## Threat Flags

None.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Ready for 02-26. The next fixture that reports through the mailbox adds rows to `tests/diag/expected.csv` and one `fixtures.<name>` test that calls `mailbox_judge`.
- `fixtures.exc_matrix` pins the interrupt latency (EPC at the `MTC0` plus 8) and the CAUSE-after-I_STAT model as `doc` rows; a measured value changes them through `tests/diag/allow.txt`.

## Self-Check: PASSED

- FOUND: tests/fixtures/src/exc_matrix.asm, tests/diag/expected.csv
- FOUND: aba6f46, 991a734, cefca40 (ancestors of HEAD)
- `cmake --workflow --preset ci` exit 0 (240 of 240); `cmake --workflow --preset hygiene` exit 0 (21 of 21)

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
