---
phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
plan: 07
subsystem: gte
tags:
  - gte
  - mvmva
  - colour-commands
  - vectors
  - quirks

requires:
  - phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers
    provides: the integer numerics, the divide, the shared helpers and the first seven commands (plan 26)
provides:
  - MVMVA with the matrix, vector and translation fields, the reserved fourth matrix and the far-colour translation
  - DPCS, DPCT, INTPL, NCS, NCT, NCDS, NCDT, NCCS, NCCT, CDP, CC, DCPL, GPF and GPL on shared colour-FIFO and blend helpers
  - all 22 documented commands computing, and a number outside them changing no register
  - gte.vectors.ps1tests replaying all 1,150 cases with a histogram check, and three negative controls
  - directed tests for the far colour, the reserved matrix, IR3 with lm, H sign, saturation at the end of a sum, RTPT's IR0 and the unlisted-number no-op
  - the completed GTE page
affects:
  - 03-13
  - 03-10
  - 03-11
  - phase-3 plans that write the reference model or run the GTE under the CPU

actuals:
  tokens: 12200
  tasks: 3
  commits: 3

plan_head_before: d96621799dabe553ffee596c470233049af0a4a7
plan_head_after: 8dc7d952429a87bdccdd89597b00d4d619eb25b5
commits: 3

tech-stack:
  added: []
  patterns:
    - "a matrix is read out once into nine signed values, and one matrix-times-vector helper serves the projection, MVMVA and the lighting commands"
    - "a colour blend with the far colour is one helper (saturate the far-colour difference without lm, then add IR0 times it with lm) shared by DCPL, CDP and the NCDx commands"
    - "a directed quirk test lists only the registers a command should change, and every other register must read as it did before the command"

key-files:
  created: []
  modified:
    - src/gte.c
    - tests/gte/gte_tests.c
    - tests/gte/tests.cmake
    - tests/gte/inventory.txt
    - docs/hardware/gte.md

key-decisions:
  - "The seed's mirrors of 00h and 1Ah are not copied: every number outside the 22 hits the default case, which changes no register and costs 5 cycles; the page says this is the engine's choice, not a hardware claim"
  - "The histogram check counts the whole file, not the cases a run selects with --ops, so it holds the container to 50 register-only cases and 50 for each of the 22 numbers in every run that asks for it"
  - "--flip counts cases from 1, as the failure messages do, and inverts bit 0 of output register 0 of that case in memory, so the flipped control is independent of which command the case runs"
  - "The matrix helper takes the nine elements as signed values and an optional three-word translation, replacing the packed-word element reader; the projection and OP use it too, with no change to their results"

patterns-established:
  - "Pattern: expect_fail controls for a harness live beside its main test in the directory's own tests.cmake, with names prefixed by the directory so two include()d files do not share variables"

requirements-completed: []

coverage:
  - id: D1
    description: "MVMVA with every matrix, vector and translation field matches its 50 hardware cases, with the far-colour translation and the reserved matrix"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gte\\.vectors\\.part1$' after Task 1 (--expect 450, 0 failures, exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "All 22 documented commands match all 1,150 hardware cases, with a histogram of 50 register-only cases and 50 per command"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gte\\.vectors\\.ps1tests$' (--expect 1150 --histogram, 0 failures, exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "The replay cannot pass on an empty file, a truncated file or a file with one expected bit altered"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^gte\\.vectors\\.(empty|truncated|flipped)\\.self_test$' (exit 0; a one-comparison mutation of the colour clamp in the engine failed gte.vectors.ps1tests alone)"
        status: pass
    human_judgment: false
  - id: D4
    description: "The far colour, the reserved matrix, IR3 with lm, H sign, saturation at the end of a sum and RTPT's IR0 each have a directed test with expected values worked by hand"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.quirk\\.(fc|mx3|ir3_lm|h_sign|clip_end|rtpt_ir0)$' (exit 0; mutations of the far-colour branch, the reserved matrix sign, the IR3 lm bound and the RTPT last-vertex choice each failed their test)"
        status: pass
    human_judgment: false
  - id: D5
    description: "Every command number outside the 22 leaves all 64 registers unchanged, with sf and lm both ways"
    requirement: "GTE-01"
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^gte\\.unlisted_noop$' (42 numbers times 4 flag patterns, exit 0; a mutation of the default case failed it)"
        status: pass
    human_judgment: false
  - id: D6
    description: "The GTE page lists every command with its cycles, the colour FIFO, the quirks and the no-op rule, without a planning identifier"
    requirement: "GTE-01"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (docs tests among 298, exit 0); grep -qE planning-id pattern over docs/hardware/gte.md src/gte.c tests/gte/gte_tests.c exits 1 as required"
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-11-10
status: complete
---

# Phase 3 Plan 07: The remaining GTE commands, the full 1,150-case replay and the quirk tests Summary

**MVMVA (every matrix, vector and translation field, the reserved matrix, the far-colour translation) and the fourteen lighting, colour and interpolation commands complete the GTE: all 22 commands match all 1,150 hardware cases on the first run, with a histogram check, three negative controls and seven directed tests for the quirks the cases pin thinly**

## Performance

- **Duration:** 9 min
- **Started:** 2026-11-10T06:23:00Z
- **Completed:** 2026-11-10T06:32:00Z (code, tests and docs; the close-out commit follows)
- **Tasks:** 3
- **Files modified:** 5 outside `.planning`

## Accomplishments

- `src/gte.c` gained `load_matrix`, `matrix_multiply` and `set_mac_ir_vec` (the projection and OP now use them too), the colour FIFO push, the far-colour blend, and `mvmva`, `normal_colour`, `colour_matrix`, `dpc`, `intpl` and `gp`. The switch dispatches every one of the 22 documented numbers to a computing command; the default case changes no register.
- The tracer, MVMVA, matched its 50 cases on the first run (450 replayed with the other seven commands). The fourteen colour-group commands then matched with all 1,150 cases replaying clean on the first run, so no port fix against the seed was needed.
- The `vectors` case gained `--histogram` (exit 1 unless the file holds 50 register-only cases and 50 for each of the 22 numbers) and `--flip N`. `gte.vectors.part1` became `gte.vectors.ps1tests` (`--expect 1150 --histogram`, TIMEOUT 60), with `gte.vectors.empty.self_test`, `gte.vectors.truncated.self_test` (an empty file and the first 1,000 bytes, built at configure time) and `gte.vectors.flipped.self_test` (`--flip 600`), each through `expect_fail.cmake` with the lane's emulator in the command.
- Seven directed tests, each with its inputs and expected values worked by hand in a comment: `gte.quirk.fc` (MAC1 = 200 while IR1's flag follows the full 50100), `gte.quirk.mx3` (5330, 180, 300), `gte.quirk.ir3_lm` (lm = 1 clears IR3 to 0 yet leaves bit 22 clear, FLAG 81060000h; lm = 0 gives 80060000h and SX2 = FFF6h), `gte.quirk.h_sign` (reads FFFF8000h, divides to 10000h, SXY2 = FFCE0064h), `gte.quirk.clip_end` (partial products 3FFF0001h and -3FFE8002h sum to 7FFFh without a flag; the next row's 8000h flags IR2), `gte.quirk.rtpt_ir0` (IR0 = 400h from the third vertex, not 80h or 100h) and `gte.unlisted_noop` (42 numbers times 4 flag patterns). Each lists only the registers that should change; the harness requires the other registers to read as before.
- `docs/hardware/gte.md` now has a command table with every command, the colour FIFO and blend rules, the MVMVA fields, a quirks section naming each test, the no-op rule, and the hardware-cases section describing the full replay and its controls.

## Task Commits

1. **Task 1: Tracer - MVMVA with every matrix, vector and translation field matches its 50 hardware cases** - `a5ef197` (feat)
2. **Task 2: The lighting, colour and interpolation commands and all 1,150 cases with their controls** - `09f2e8e` (feat)
3. **Task 3: Directed quirk tests, the unlisted no-op, and the completed page** - `8dc7d95` (test)

**Plan metadata:** the docs commit that carries this summary, STATE.md and ROADMAP.md.

The tracer gate ran with an automated-only verify: `cmake --workflow --preset ci` and `ctest -R '^gte\.vectors\.part1$'` were re-run after the tracer commit, both exited 0, and the expansion task went ahead.

## Files Created/Modified

- `src/gte.c` - the matrix helpers, MVMVA, the colour commands and the complete dispatch
- `tests/gte/gte_tests.c` - `--histogram`, `--flip`, the quirk harness, seven new cases and the usage text
- `tests/gte/tests.cmake`, `tests/gte/inventory.txt` - the renamed replay, three controls and seven new tests with `Fault prevented:` comments
- `docs/hardware/gte.md` - the command table, the colour FIFO, the quirks and the no-op rule

## Decisions Made

See `key-decisions` above.

## Deviations from Plan

None - plan executed exactly as written.

One detail the plan left open: the quirk tests are separate command-line arguments (`quirk_fc`, `quirk_mx3`, ...) like the existing cases, rather than a `quirk NAME` pair; this changes no registered name.

## Issues Encountered

- Not run locally: the `asan` and `be32` lanes need clang-21, which this host lacks; hosted CI runs them. The new code uses fixed-width integers, widening multiplies on `int64_t`, `sar64` and no signed shift, and `gte.regs.sweep` already drives every command number on edge-value registers for the sanitizer lane. The empty and truncated controls run through the lane's emulator command, as the CPU vectors' controls do; the wasm32 lane was not run locally either.
- A first mutation of the RTPT test (marking the first vertex `last` while leaving the third `last` as well) did not fail it, because the third vertex's write overrides IR0 anyway; the mutation was redone with the first vertex alone as `last`, and the test failed as it should (IR0 = 80h, expected 400h).

## Known Stubs

None. The fifteen commands the previous plan left uncomputed are now implemented, and no empty or placeholder value flows to a result.

## Threat Flags

None. No endpoint, auth path, file access or schema at a trust boundary is added. The vectors case reads the file its caller names, as before, and the new quirk cases read none.

## Evidence (commands and exit statuses)

- `cmake --workflow --preset ci`: exit 0 (298 tests ran once each after Task 3)
- `ctest --test-dir build/ci -R '^gte\.vectors\.part1$' --no-tests=error --output-on-failure` after Task 1: exit 0
- `grep -q -- '--expect 450' tests/gte/tests.cmake` at the Task 1 commit: exit 0
- `build/ci/tests/schwaystation-test-gte vectors tests/vectors/gte/ps1tests-1150.bin --expect 1150 --histogram`: exit 0 (1150 cases, 0 failures)
- `ctest --test-dir build/ci -L '^gte$' --no-tests=error --output-on-failure`: exit 0 (22 of 22)
- `ctest --test-dir build/ci -R '^gte\.vectors\.(ps1tests|empty\.self_test|truncated\.self_test|flipped\.self_test)$' --no-tests=error`: exit 0
- `grep -q 'gte.vectors.part1' tests/gte/inventory.txt; test $? -eq 1`: exit 0
- `ctest --test-dir build/ci -R '^gte\.(quirk\.(fc|mx3|ir3_lm|h_sign|clip_end|rtpt_ir0)|unlisted_noop)$' --no-tests=error`: exit 0
- `grep -qE 'D-[0-9]{2}|GTE-0[0-9]|PX\.[0-9]|DEC\.[0-9]|XFER-' docs/hardware/gte.md src/gte.c tests/gte/gte_tests.c`: exit 1, as required
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0
- `cmake --workflow --preset hygiene`: exit 0
- Mutation checks (each reverted afterwards): the colour clamp bound changed by one failed `gte.vectors.ps1tests` on one case; the far-colour branch bypassed, the reserved matrix sign flipped, the IR3 lm bound changed, the first vertex marked last in RTPT, and a register written in the default case each failed exactly their directed test.

## Next Phase Readiness

All 22 commands now compute, so plan 03-13's reference model can be differenced against a complete engine and the GTE COP2 plans can run any command. The one place the numerics may still differ from a 44-bit clamp, the IR clamp taking the low 32 bits of the shifted MAC when `sf` is clear (recorded in plan 03-26), is unchanged and is where the differential would show it.

## Self-Check: PASSED

- Modified files exist: `src/gte.c`, `tests/gte/gte_tests.c`, `tests/gte/tests.cmake`, `tests/gte/inventory.txt`, `docs/hardware/gte.md`.
- Commits `a5ef197`, `09f2e8e` and `8dc7d95` are ancestors of HEAD; the plan measured 3 commits from the recorded base `d966217`.
- Every task's acceptance commands exited as the plan says; `cmake --workflow --preset ci` and `cmake --workflow --preset hygiene` pass.

---
*Phase: 03-the-gte-matches-the-hardware-cases-dma-and-timers*
*Completed: 2026-11-10*
