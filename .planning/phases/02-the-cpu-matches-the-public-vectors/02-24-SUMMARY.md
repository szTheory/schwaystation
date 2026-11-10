---
phase: 02-the-cpu-matches-the-public-vectors
plan: 24
subsystem: runner
tags:
  - runner
  - trace
  - first-fault
  - exit-table
  - fixtures
  - unbuilt

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: schway_get_trace, SCHWAY_RUN_TRACE and the 4,096-record ring (02-21), --stop-on mailbox, the hang report and the exit-table check (02-23)
provides:
  - schway__trace_write_record, the one formatter of a trace record as twelve tab-separated fields, with the event, device and fault-kind names
  - an --exe run that stops at the program's first fault, prints the fault line and exits 7; --no-stop-on-fault runs on
  - --trace first-fault, which prints the 4,096-record tail after a fault line or a hang line
  - the unbuilt line at the end of any --exe run that touched a device the core does not build
  - fault.asm, one fault per class after an 8,000-record preamble, and unbuilt.asm
  - docs/runner.md Diagnostics section and exit status 7
affects:
  - 02-25
  - 02-26
  - 02-27

actuals:
  tokens: 7200
  tasks: 3
  commits: 4

plan_head_before: 9d9c46f9dbd7c3030cef8bbb8c3ec4485ac891e1
plan_head_after: 00b235edee7842d10806821f35194079e5062c60
commits: 4

tech-stack:
  added: []
  patterns:
    - "one fixture source assembled once per class with -equ KIND, registered in a foreach over the kind names"
    - "a script-mode CASE (trace) counts twelve-field lines and checks the last event, so the 4,096-line tail is asserted with a tolerant regex and no exact text"
    - "the unbuilt counters are read through the state visitor, so the runner touches no internal header"

key-files:
  created:
    - runner/trace.c
    - runner/trace.h
    - tests/runner/test_trace.c
    - tests/fixtures/src/fault.asm
    - tests/fixtures/src/unbuilt.asm
  modified:
    - runner/main.c
    - runner/exit_status.h
    - runner/CMakeLists.txt
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - docs/runner.md

key-decisions:
  - "an --exe run goes through schway_run with max_cycles UINT64_MAX once per frame, so the stop-on-fault and trace flags apply; a run without --exe still uses schway_run_frame"
  - "the fault is acted on right after the frame's tty text and before the mailbox read, the png and the hash, so status 7 wins and nothing prints after the fault, the tail and the unbuilt line"
  - "--trace and --no-stop-on-fault need --exe (a usage error otherwise), which also gives --trace the 600-frame default cap the plan asks for"
  - "the unbuilt line's n is the count of the last device touched, not a total, and the line is printed once after the fault or hang lines and before any hash"
  - "the tail is printed after a fault line and after a hang line only; an EXE that neither faults nor hangs prints no tail under --trace first-fault"

requirements-completed:
  - CPU-05

coverage:
  - id: D1
    description: "schway__trace_write_record formats a record as twelve tab-separated fields with 0x%08x hex for pc, opcode, addr and value, and the event and device names; one golden line per event, the extreme values and every device and kind name are pinned"
    requirement: "CPU-05"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^runner\\.trace\\.golden$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "An --exe run stops at its first fault by default, prints the fault line with the kind enum's names and exits 7; status 7 is in the exit table and docs/runner.md; --trace and --no-stop-on-fault are refused without --exe, with a wrong value or given twice"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^(fixtures\\.trace\\.unmapped_read|runner\\.exit_table(\\.self_test)?|runner\\.usage\\.exe)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "--trace first-fault prints exactly 4,096 twelve-field lines after the fault line, the last a fault event, and the literal criterion command with no --frames exits 7 with a fault line"
    requirement: "CPU-05"
    verification:
      - kind: e2e
        ref: "ctest --test-dir build/ci -R '^fixtures\\.trace\\.criterion5$' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "build/ci/schwaystation-run --exe build/ci/fixtures/fault_unmapped_read.exe --trace first-fault --frames 30 > build/ci/tests/fault.txt; test $? -eq 7 && test \"$(awk -F'\\t' 'NF == 12' build/ci/tests/fault.txt | wc -l)\" -eq 4096 (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "fault.asm assembled per class faults once after an 8,000-record preamble as each of the nine kinds; each class exits 7, names its kind, has a 4,096-line tail ending in a fault event, and the exception loop's note is 1000"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.trace\\.(unmapped_read|unmapped_write|unmapped_fetch|misaligned_read|misaligned_write|misaligned_fetch|reserved_instruction|cop_unusable|exception_loop)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "An EXE that touched unbuilt devices prints one unbuilt line with the last device, its count and the last pc: unbuilt.exe writing GPU and DMA registers exits 0 with it, and a write to 1F801030h exits 7 as unmapped_write"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.(unbuilt_footer|gap_write)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "A hang under --trace first-fault prints its line and then the 4,096-line tail, and an EXE that never faults prints no fault line"
    requirement: "CPU-05"
    verification:
      - kind: other
        ref: "build/ci/schwaystation-run --exe build/ci/fixtures/hang.exe --stop-on mailbox --trace first-fault --frames 3 | awk -F'\\t' 'NF == 12' | wc -l gives 4096; hello.exe under --tty --stop-on mailbox prints no fault line (fixtures.hello.criterion4 exit 0)"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/runner.md documents --trace first-fault and --no-stop-on-fault in the synopsis, options and examples, and a Diagnostics section with the fault line, kinds, the 12-field tail, the 4,096 limit, the unbuilt line and status 7, inside the page budget"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 239 of 239 tests including docs.budgets and runner.exit_table) and cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake (exit 0, 239 tests ran once each)"
        status: pass
    human_judgment: false

duration: 12min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 24: The runner diagnoses a faulting EXE Summary

**`schwaystation-run --exe X --trace first-fault` prints one `fault frame=... kind=...` line and the last 4,096 trace records as twelve tab-separated fields through one formatter, exits 7, and appends an `unbuilt dev=... n=... last_pc=...` line whenever the program touched a device the core does not build; nine crafted fixtures pin every fault class.**

## Performance

- **Duration:** about 12 min
- **Completed:** 2026-11-09T23:13:00Z
- **Tasks:** 3
- **Files modified:** 13 (five created)

## Accomplishments

- `runner/trace.c` holds `schway__trace_write_record` and the three name functions (event, device, fault kind). The line is `frame line dot cycle pc opcode event addr width device value note`; `pc`, `opcode`, `addr` and `value` are `0x` and eight lowercase hex digits, the rest decimal. The function refuses a buffer too small for the line and a NULL argument.
- `main.c` runs an `--exe` through `schway_run` once per frame with `SCHWAY_RUN_STOP_ON_FAULT` (off under `--no-stop-on-fault`) and `SCHWAY_RUN_TRACE` (under `--trace first-fault`). On `SCHWAY_STOP_FAULT` it prints the fault line from `schway_get_fault`, the tail when asked, and exits 7. A hang prints the same tail after its line.
- The unbuilt line is built from the state visitor's `bus.unbuilt_n`, `bus.last_unbuilt.dev` and `bus.last_unbuilt.pc`, and is printed at the end of any `--exe` run with a non-zero count (the fault footer, exit 8 and a clean exit all carry it). No `--stop-on-unbuilt` flag and no core stop reason for a hang were added.
- `fault.asm` loads and stores 1,000 times (8,000 records), then faults once by `-equ KIND`. The nine classes all stop at their first fault with a full 4,096-line tail; the exception loop registers a handler through the mailbox that returns to its BREAK, and its fault event carries 1000 in `note`.
- `unbuilt.asm` has two modes: GPU and DMA writes with a sealed mailbox (exit 0 with the unbuilt line), and a write to 1F801030h (exit 7, `unmapped_write`).
- The gate-line total is unchanged at **1494 lines** against the 1520 limit: the trace check is a CMake script case inside the existing fixtures test file, not a new gate script.

## Task Commits

1. **Task 1: Tracer - an unmapped read in a crafted EXE prints the fault line and its tail and exits 7** - `ce547c0` (feat)
2. **Task 2: Every fault class, the golden lines, the unbuilt footer and the gap write** - `f25e756` (test)
3. **Task 3: The runner page's diagnostics section and the full ci check** - `ab49fdc` (docs)
4. **Refusal tests for the two new options (deviation 2)** - `00b235e` (test)

**Plan metadata:** recorded in the docs commit that carries this file.

The tracer gate was run at the Task 1 commit's own state (only the unmapped-read fixture registered): the `fixtures.` tests, `runner.exit_table` and the JUnit inventory check passed (229 tests) before the commit.

## Files Created/Modified

- `runner/trace.c`, `runner/trace.h` - the record formatter and the names
- `runner/main.c` - `--trace`, `--no-stop-on-fault`, the per-frame `schway_run`, the fault line, tail and unbuilt line
- `runner/exit_status.h`, `runner/CMakeLists.txt` - status 7 `fault`, `trace.c` in the runner's source list
- `tests/runner/test_trace.c`, `tests/runner/tests.cmake`, `tests/runner/inventory.txt` - `schwaystation-test-trace-format` and `runner.trace.golden`
- `tests/fixtures/src/fault.asm`, `tests/fixtures/src/unbuilt.asm` - the two fixture sources
- `tests/fixtures/tests.cmake`, `tests/fixtures/inventory.txt` - the `trace` script case, eleven fixtures, nine `fixtures.trace.<kind>`, `fixtures.trace.criterion5`, `fixtures.unbuilt_footer`, `fixtures.gap_write`
- `docs/runner.md` - synopsis, options, examples, the Diagnostics section and the status 7 row

## Decisions Made

See `key-decisions` above. The fault line's `line` and `dot` print 0, because the core's fault record keeps 0 until the GPU keeps a beam (`docs/hardware/cpu.md`); the tail's derived `line` and `dot` are not 0. The page says so.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] The two new options' usage errors are tested**
- **Found during:** after Task 3
- **Issue:** the plan registers no test for the refusals of `--trace` (no `--exe`, a value other than `first-fault`, given twice) or of `--no-stop-on-fault` without `--exe`, and every behaviour change lands with a test.
- **Fix:** seven `refuse` lines in the `exe` case of `runner.usage.exe`.
- **Files modified:** tests/runner/tests.cmake
- **Verification:** `ctest -R '^runner\.usage\.exe$'` exit 0
- **Committed in:** `00b235e`

### Plan-shape notes

- The exit table's status 7 row landed with Task 1 and the rest of the page with Task 3, as the plan's file lists put them, so `runner.exit_table` passes at the Task 1 commit.
- `main.c` landed whole in Task 1, including the hang tail and the unbuilt line that the plan lists under Task 2, because the file is one unit and the tracer's gate runs the full `--exe` path. Task 2's commit holds the tests for them.
- `--trace first-fault` "implies the same 600-frame default cap": the runner requires `--exe` for `--trace`, and `--exe` already gives the 600-frame default, so no separate rule was needed.

---

**Total deviations:** 1 auto-fixed (1 missing critical); three plan-shape notes above.
**Impact on plan:** no scope growth. All thirteen listed files were touched; the extra test lines sit in `tests/runner/tests.cmake`, which the plan already lists.

## Issues Encountered

- The `$` and `^` of a CMake regular expression anchor the whole output, not a line, so the fixture tests use `(^|\n)` and `(\n|$)` as the existing mailbox tests do.

## Known Stubs

None.

## Threat Flags

None. The runner reads the core's trace ring, fault record and state through the public API only, the new fixtures hold no BIOS, disc or state bytes, and no network, file-write or environment access was added.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-25 and later plans can rely on exit 7 and on `fault frame=` lines; the tail's 12-field layout is pinned by `runner.trace.golden`.
- Plan 02-26 measures the gate total (1494 lines, limit 1520); this plan added no gate code.
- No push, hosted run or pull-request step was part of this plan; the push policy for this phase is unchanged.

## Self-Check: PASSED

- Files: runner/trace.c, runner/trace.h, tests/runner/test_trace.c, tests/fixtures/src/fault.asm, tests/fixtures/src/unbuilt.asm present.
- Commits ce547c0, f25e756, ab49fdc and 00b235e are ancestors of HEAD; `git rev-list --count 9d9c46f9dbd7c3030cef8bbb8c3ec4485ac891e1..HEAD` was 4 before this file.
- `cmake --workflow --preset ci` exit 0 (239 of 239), check_junit exit 0 (239 tests ran once each), `cmake --workflow --preset hygiene` exit 0; the three acceptance commands exit 0.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
