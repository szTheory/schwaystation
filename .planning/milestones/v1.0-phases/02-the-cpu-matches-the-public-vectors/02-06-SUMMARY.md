---
phase: 02-the-cpu-matches-the-public-vectors
plan: 06
subsystem: cpu
tags: [cpu, r3000, vectors, singlesteptests, replay-bus, delay-slots, harness]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: cpu and bus state structs, state format 2 (plan 02-02)
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: the committed 5,500-case sample container (plan 02-04)
provides:
  - schway__cpu_step_one, the one-instruction entry over the executor in src/cpu_exec.inc, with an optional replay log
  - struct schway__cpu_cycle and struct schway__cpu_log, the replay bus
  - load-delay and branch-delay latches in struct schway__cpu, walked in the state with bounds
  - schwaystation-test-vectors, the one C parser and replay comparison for the sample and later the full files
  - the ALU, shift, logic, set and LUI opcodes, 2,100 vector cases passing
  - docs/hardware/cpu.md and the corrected vector layout in the preparation text
affects: [02-07, 02-08, 02-09, 02-10, 02-12, 02-14, 02-17]

actuals:
  tokens: 12000
  tasks: 3
  commits: 4

plan_head_before: 20fe8b2514c9206e4fa0b16044751c6b91e21b95
plan_head_after: 311772260a3bc8a0fb7545e1af65fed42242c9d8
commits: 4

tech-stack:
  added: []
  patterns:
    - "one executor file (src/cpu_exec.inc) included by cpu.c; the replay entry and the run loop share fetch, decode and execute, with access front-ends that replay a log or call the bus"
    - "the vector harness is one program: explicit little-endian parser, replay bus from the case's cycles, field-by-field final-state compare, and a zero-case guard"
    - "negative controls are table-driven through expect_fail.cmake, one per compared field"

key-files:
  created:
    - src/cpu_exec.inc
    - tests/vectors/vectors.c
    - docs/hardware/cpu.md
  modified:
    - src/cpu.h
    - src/cpu.c
    - src/internal.h
    - tests/vectors/tests.cmake
    - tests/vectors/inventory.txt
    - .planning/preparation/CONFORMANCE.md
    - .planning/preparation/PSXDEV-EXTRACTS.md

key-decisions:
  - "The vectors' state tail is the next-PC model: pc plus the branch latch (slot, take, target), with next pc = target while take is set and pc + 4 otherwise; every instruction clears the latch, so the model needed no change after measuring the sample"
  - "The vectors' load latch is the pending load that lands as the current instruction ends (register or -1, value); the core holds no-load as register 32 and the harness maps -1 to 32"
  - "Final latch values after a non-branch instruction are all zero (target too), matching the vectors"
  - "The harness exits 1 for a failure, a malformed file or fewer cases than --expect, and 2 for a bad command line; an allow line for a file that ran and matched no failing case is an error"
  - "cpu_physical passes KUSEG and KSEG2 addresses unchanged and drops the segment bits of KSEG0 and KSEG1; the replay log records the virtual address the instruction used"

requirements-completed: [CPU-01]

coverage:
  - id: D1
    description: "schway__cpu_step_one runs one instruction through the shared executor with a replay log; the log records each access and returns recorded values for reads and fetches; no test-only define and no second decoder"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.r3000\\.sample$' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "git grep -q SCHWAY__TESTING -- src tests (exit 1, no match)"
        status: pass
    human_judgment: false
  - id: D2
    description: "The ALU, shift, logic, set and LUI opcodes (ADDIU ADDU AND ANDI LUI NOR OR ORI SLL SLLV SLT SLTI SLTIU SLTU SRA SRAV SRL SRLV SUBU XOR XORI) pass all 2,100 cases, comparing the cycles, 32 GPRs, HI, LO, PC, EPC, TAR, CAUSE and both delay latches"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -L vectors --no-tests=error (exit 0, 11 tests)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0, 94 tests)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The harness refuses an empty file and a file cut inside a slice with exit 1, and fails a run that replayed fewer cases than --expect"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.r3000\\.(empty|truncated)\\.self_test$' --no-tests=error (exit 0)"
        status: pass
      - kind: integration
        ref: "build/ci/tests/schwaystation-test-vectors tests/vectors/r3000/sst-sample.bin --only ADDU --expect 101 (exit 1)"
        status: pass
    human_judgment: false
  - id: D4
    description: "A flipped bit in the expected final r5, EPC, TAR, CAUSE, branch target or load value fails with a final-field mismatch line naming that field, so no register, COP0 field or latch can pass uncompared"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.r3000\\.flip\\.' --no-tests=error (exit 0, 6 tests)"
        status: pass
    human_judgment: false
  - id: D5
    description: "A dated allow line covers exactly the failing case it names, and a line that matches no failing case in a file the run covered is an error"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^vectors\\.r3000\\.allow(_stale)?\\.self_test$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The load-delay and branch-delay latches are saved and restored with bounded fields, and the harness builds clean under address and undefined-behaviour sanitizers"
    requirement: "CPU-01"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -L core --no-tests=error (exit 0, 24 tests)"
        status: pass
      - kind: other
        ref: "cc -fsanitize=address,undefined on vectors.c and src/*.c, sample run, exit 0"
        status: pass
    human_judgment: false
  - id: D7
    description: "docs/hardware/cpu.md covers registers, decode, the ALU class, the delay latches and the vector tests without planning IDs, and CONFORMANCE 1.1 and PX.41 describe the committed container, the 172-byte states, the 431-byte one-cycle case and the corrected tail order"
    requirement: "CPU-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.budgets$' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "the plan acceptance greps: no planning ID in docs/hardware/cpu.md (exit 1), 431 and 172 present and no SWR3 or sst2bin in CONFORMANCE.md (exit 0, 0, 1)"
        status: pass
    human_judgment: false

duration: 13min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 06: Vector harness and the ALU class Summary

**One-instruction entry `schway__cpu_step_one` over the shared executor, a replay-bus vector harness with a zero-case guard and per-field negative controls, and the 21 ALU, shift, logic, set and LUI opcodes passing all 2,100 public vector cases.**

## Performance

- **Duration:** 13 min
- **Started:** 2026-11-09T19:13:30Z
- **Completed:** 2026-11-09T19:27:00Z
- **Tasks:** 3
- **Files modified:** 10 (3 created, 7 modified)

## Accomplishments

- `src/cpu_exec.inc` holds fetch, decode and execute of one instruction, included by `src/cpu.c`; its access front-ends replay a `struct schway__cpu_log` when one is attached and otherwise reach the bus with the physical address, so `schway__cpu_step_one` and the future run loop share one decoder.
- `tests/vectors/vectors.c` parses the container with explicit little-endian shifts (rejecting empty, short, truncated and long files, over-long names, cycle counts over 8, case counts over 1,000, non-zero high dwords, non-zero R0), loads each initial state into a freshly reset CPU with SR 0, steps once, and compares every bus cycle and the final state field by field. A failure prints the file, case name, cycle index or `final <field>`, and the wanted and got values.
- The next-PC model of the vectors is the branch latch with `pc`: while a taken branch is pending the next PC is its target, otherwise `pc + 4`; the latch is cleared by every instruction that is not a branch (final target 0 too). The load latch is the pending load landing as the current instruction ends, cancelled by a write to the same register. Both models matched the sample on the first run, so nothing in the plan's model changed.
- 2,100 cases of 21 files pass; negative controls cover an empty file, a file cut at 1,000 bytes, a run with fewer cases than `--expect` (101 against 100), a flipped bit in each of six final fields, and the allow-line rules.
- `docs/hardware/cpu.md` and the preparation corrections: CONFORMANCE 1.1 now describes the committed container with 172-byte states and about 431 bytes per one-cycle case; PX.41 labels the tail branch target, slot, take, then load target and value; overflow, BREAK and SYSCALL are recorded as covered by the vectors and only address errors as not covered.

## Task Commits

1. **Task 1: Tracer - the parser and step_one carry the ADDU slice end to end** - `a1fb65f` (feat)
2. **Task 2: The ALU, shift, logic, set and LUI files and the empty and truncated controls** - `ce82c2d` (test)
3. **Task 3: The CPU page and the three preparation corrections** - `a575393` (docs)
4. **Portability fix: parse allow lines without sscanf and strcpy** - `3117722` (refactor)

**Plan metadata:** the docs commit that adds this summary, STATE.md and ROADMAP.md follows.

The tracer feedback gate ran at end of phase mode with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^vectors\.r3000\.sample$'` exited 0 before Task 2 started, so expansion went ahead.

## Files Created/Modified

- `src/cpu_exec.inc` - the executor: `cpu_step`, the access front-ends `cpu_fetch`, `cpu_read`, `cpu_write`, the replay log access, register write with load cancel, the ALU class
- `src/cpu.h` - the log and cycle structs, the delay latch fields, the `schway__cpu_step_one` declaration
- `src/cpu.c` - latch reset, the five new state fields with bounds, `schway__cpu_step_one`
- `src/internal.h` - the cpu layout assertion now pins the offset of the non-state log pointer
- `tests/vectors/vectors.c` - the harness
- `tests/vectors/tests.cmake`, `tests/vectors/inventory.txt` - the executable and eleven tests
- `docs/hardware/cpu.md` - the CPU page
- `.planning/preparation/CONFORMANCE.md`, `.planning/preparation/PSXDEV-EXTRACTS.md` - the corrections

## Decisions Made

- See `key-decisions` above. The state format number and behaviour revision stay at 2: state format 2 was introduced by plan 02-02 in this same phase and has not been released.
- A load into `r0` is a pending load like any other (the vectors have 20 such states), so `no load` is register 32, not 0.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing Critical] Two extra self tests for the allow-line rules**
- **Found during:** Task 2
- **Issue:** The harness reads `--allow` lines and treats a stale line as an error (a D-21 rule), but the plan's test list has no test that exercises either path, so the rule would ship untested.
- **Fix:** Added `vectors.r3000.allow.self_test` (a line covering the one failing case produced by `--flip-expected r5` exits 0) and `vectors.r3000.allow_stale.self_test` (the same line without the flip is an error), with their two inventory lines.
- **Files modified:** `tests/vectors/tests.cmake`, `tests/vectors/inventory.txt`
- **Verification:** `ctest -L vectors` exit 0, 11 tests
- **Committed in:** `ce82c2d`

**2. [Rule 3 - Blocking] The cpu size assertion cannot name a total size once the struct holds a pointer**
- **Found during:** Task 1
- **Issue:** `sizeof(struct schway__cpu) == 268` is false after the latches and the log pointer, and the pointer's size differs on 32-bit and 64-bit targets.
- **Fix:** The assertion is now `offsetof(struct schway__cpu, log) == 288`, which pins the state part exactly on every target.
- **Files modified:** `src/internal.h`
- **Verification:** all lanes that build locally (ci, dev); the assertion compiles on the 64-bit host
- **Committed in:** `a1fb65f`

**3. [Rule 3 - Blocking] The replay front-ends cpu_read and cpu_write have no caller yet**
- **Found during:** Task 1
- **Issue:** The plan names them among this plan's symbols, but loads and stores are implemented later, so plain `static` functions would trip `-Wunused-function` under warnings-as-errors.
- **Fix:** Declared them `static inline`, which neither GCC nor Clang flags when unused. The build is clean with Apple clang; the GCC and MSVC lanes run in hosted CI.
- **Files modified:** `src/cpu_exec.inc`
- **Verification:** `cmake --workflow --preset ci` and `dev` exit 0
- **Committed in:** `a1fb65f`

**4. [Rule 1 - Bug] The sample parser's early header-count check hid the truncated path**
- **Found during:** Task 2
- **Issue:** A first draft refused the 1,000-byte file with a header-count message before reaching the per-slice length check, so the truncated control would not have exercised the slice-length rule.
- **Fix:** Removed the early check; the 1,000-byte file now reports `truncated slice`.
- **Files modified:** `tests/vectors/vectors.c`
- **Verification:** `vectors.r3000.truncated.self_test` passes on the `truncated` branch
- **Committed in:** `ce82c2d`

**5. [Rule 2 - Missing Critical] Deprecated C library calls removed from the harness**
- **Found during:** after Task 3
- **Issue:** `sscanf` and `strcpy` are marked deprecated on MSVC, and the presets make warnings fatal.
- **Fix:** A small tokenizer and `memcpy` of already length-checked strings.
- **Files modified:** `tests/vectors/vectors.c`
- **Verification:** `cmake --workflow --preset ci` exit 0, allow tests pass
- **Committed in:** `3117722`

**6. Scope notes (no rule)**
- Task 1's commit already contains all 21 opcode handlers, since the decode switch was written once; Task 2 changed only the tests and the truncated-file rule.
- PX.41 also lost its claim of an opcode block after the cycles (plan 02-04 measured none), CONFORMANCE's CF.01 row and CF.25 arithmetic follow the new numbers, and the 1.1a exceptions row now says which exceptions the vectors do take.

---

**Total deviations:** 5 auto-fixed (1 bug, 2 missing critical, 2 blocking), plus scope notes.
**Impact on plan:** All fixes are small and local; the plan's tests, artifacts and symbols are all present.

## Issues Encountered

- `cmake --workflow --preset asan` needs `clang-21`, which this host does not have, so the sanitizer lane ran as a local one-off: `cc -fsanitize=address,undefined -fno-sanitize-recover=undefined` on `tests/vectors/vectors.c` and `src/*.c`, sample run exit 0, truncated file exit 1.
- The `dev` preset prints deprecation warnings from the pinned armips sources (`sprintf`); they are third-party and did not fail the build.
- A probing call of `gsd-tools query windows.append` created `.planning/WINDOWS.md`; the untracked file was deleted, nothing was recorded or committed.

## Known Stubs

- `src/cpu_exec.inc`: an opcode the executor does not decode yet (loads, stores, branches, multiply and divide, COP0, COP2 and the trapping adds) latches a reserved-instruction first-fault record and retires like a no-op. The later plans of this phase decode them. Nothing reaches this path from a host until the run loop lands, and the vector runs only select decoded files.
- Not tested for lack of a way to write NUL bytes from CMake: the rejections for a name over 50 bytes, a cycle count over 8, a case count over 1,000, a non-zero high dword, a non-zero R0 and a long file. Only the empty, short-cut and truncated files and `--expect` have negative controls.

## Threat Flags

None - the harness reads local files only and the executor adds no network, auth or trust-boundary surface.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plans 02-07 onward can add opcode classes to `src/cpu_exec.inc` and extend `--only` and `--expect` in `tests/vectors/tests.cmake`; the harness is the one parser later plans reuse.
- Branch plans set `branch_slot`, `branch_take` and `branch_target` where `cpu_step` now clears them (the comment at the epilogue marks the place), and load plans set the pending load there too.
- The container parser reads the sample; a plan that runs the full upstream files needs a raw mode for the file without the container header.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*

## Self-Check: PASSED

- FOUND: src/cpu_exec.inc, tests/vectors/vectors.c, docs/hardware/cpu.md, src/cpu.h, src/cpu.c
- FOUND commits: a1fb65f, ce82c2d, a575393, 3117722 (ancestors of HEAD)
