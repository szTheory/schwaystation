---
phase: 02-the-cpu-matches-the-public-vectors
verified: 2026-11-18T10:41:44Z
status: passed
score: 5/5 must-haves verified
covered_files:
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-01-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-01-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-02-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-02-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-03-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-03-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-04-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-04-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-05-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-05-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-06-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-06-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-07-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-07-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-08-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-08-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-09-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-09-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-10-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-10-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-11-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-11-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-12-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-12-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-13-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-13-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-14-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-14-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-15-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-15-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-16-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-16-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-17-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-17-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-18-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-18-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-19-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-19-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-20-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-20-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-21-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-21-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-22-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-22-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-23-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-23-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-24-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-24-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-25-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-25-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-26-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-26-SUMMARY.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-27-PLAN.md
  - .planning/phases/02-the-cpu-matches-the-public-vectors/02-27-SUMMARY.md
  - CMakePresets.json
  - cmake/Armips.cmake
  - pins.txt
  - runner/mailbox.c
  - runner/main.c
  - runner/probe.c
  - runner/trace.c
  - src/bus.c
  - src/cop0.c
  - src/cpu.c
  - src/cpu_exec.inc
  - src/exe.c
  - src/irq.c
  - src/stub_kernel.c
  - tests/bus/bus_tests.c
  - tests/bus/cache_tests.c
  - tests/cmake/hashes.cmake
  - tests/cpu/delay_tests.c
  - tests/cpu/exception_tests.c
  - tests/cpu/timing_tests.c
  - tests/diag/expected.csv
  - tests/exe/exe_tests.c
  - tests/fixtures/src/exc_matrix.asm
  - tests/fixtures/tests.cmake
  - tests/fuzz/exe.c
  - tests/fuzz/tests.cmake
  - tests/inventory/ci.txt
  - tests/nightly/tests.cmake
  - tests/runner/tests.cmake
  - tests/vectors/r3000/sst-sample.bin
  - tests/vectors/tests.cmake
  - tests/vectors/vectors.c
covered_digest: "v4:sha256:92e3ce9b54a7924cc4e8dbf897b2e5bf8b85d481755397c633d7412b83da7947"
behavior_unverified: 0
overrides_applied: 0
advisory:
  - finding: "The exception-loop heuristic can flag 1,000 consecutive exceptions with the same EPC and ExcCode even when a handler makes progress."
    category: other
    reason: "Historical code-review observation; this false-positive edge case is outside CPU-01..CPU-05. The required exception-loop diagnostic is exercised by the fixture."
    evidence_status: "code read; no failing acceptance test"
---

# Phase 2: The CPU matches the public vectors — Verification Report

**Phase Goal:** The R3000A matches the public vectors before any game runs, with COP0 exceptions, bus decode, memory control, the I-cache and interrupt registers; PS-EXEs load and run under the own stub kernel, and the probe and first-fault trace diagnose any EXE.
**Verified:** 2026-11-18T10:41:44Z
**Status:** passed

The previous report had no `gaps:` section, so this is a fresh verification against current code. I checked the 27 plan/summary pairs, current `REQUIREMENTS.md`, roadmap criteria, source and test wiring. The plan requirement union is exactly CPU-01 through CPU-05, and those are the only requirements mapped to Phase 2. All 170 summary coverage entries keep `human_judgment: false`. No local tests were run; hosted results below are the regression evidence.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|---|---|---|
| 1 | The 5,500-case committed vector sample and zero-case guard pass on every CI lane; nightly runs all 55,000 pinned cases. | VERIFIED | Current CI 44253419074 is green across six build legs, BE32, WASM32, and `CI required`; its `Every test ran` checks passed. Nightly 44233700921 reports `vectors-full` success. The CPU, vector harness/data, and nightly/fuzz inputs checked for this criterion have no file delta between that nightly head and current main. |
| 2 | COP0 exceptions, delay slots, MULT/DIV timing, and `exc_matrix` records match `expected.csv`. | VERIFIED | Current CI passes the registered `cpu.exception.*`, `cpu.delay.*`, `cpu.timing.*`, and `fixtures.exc_matrix` checks. The fixture judge wires mailbox records to the CSV rows in `tests/fixtures/tests.cmake`. |
| 3 | Bus decode, mirrors, memory-control costs, I-cache/isolation, write queue, scratchpad, and I_STAT/I_MASK work. | VERIFIED | Current CI passes the registered bus and cache tests in `tests/bus/tests.cmake`; the tests exercise `src/bus.c`, `src/irq.c`, and the CPU fetch/cache path. |
| 4 | `hello.exe` runs under the own stub, prints TTY and mailbox output, exits 0; armips and its digest guard work on every lane. | VERIFIED | Current CI passes the `fixtures.hello.criterion4`, fixture assembly, and `fixtures.armips_pin.self_test` checks across the build lanes. Wiring is in `runner/main.c`, `runner/mailbox.c`, `src/exe.c`, and `src/stub_kernel.c`. |
| 5 | PS-EXE probe output is produced without emulation; first-fault trace prints a fault and 4,096-record tail, then exits 7. | VERIFIED | Current CI passes the registered probe, trace, and per-fault fixture checks, including `fixtures.trace.criterion5`; `runner/probe.c` calls the core validator and `runner/trace.c` formats the trace returned by the core. |

**Score:** 5/5 truths verified (0 behavior-unverified).

### Hosted Evidence

| Check | Command / result | Evidence |
|---|---|---|
| Current CI | `cmake --workflow --preset ci` — exit 0; all six build legs, BE32, WASM32, `CI required`, and `hash-equality` succeeded. The run head is `15525817`; its tree `058ac899` is the current `main` tree at `35a19da`. | [CI run 44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) |
| Main CI | Passed. | [Main CI run 44253375502](https://github.com/szTheory/schwaystation/actions/runs/44253375502) |
| Nightly CPU regressions | `vectors-full` and `fuzz` both succeeded on head `7d24bed`; relevant CPU, vector, loader-fuzz, and nightly files are unchanged in current `main`. | [Nightly run 44233700921](https://github.com/szTheory/schwaystation/actions/runs/44233700921) |

## Requirements Coverage

| Requirement | Source plans | Status | Evidence |
|---|---|---|---|
| CPU-01 | 01, 02, 03, 04, 06, 10, 12, 15, 26, 27 | SATISFIED | Truth 1; current CI plus successful `vectors-full` nightly. |
| CPU-02 | 01, 03, 10, 12, 17, 19, 25, 26, 27 | SATISFIED | Truth 2; registered CPU and exception-matrix checks pass in current CI. |
| CPU-03 | 01, 02, 03, 07, 14, 26, 27 | SATISFIED | Truth 3; registered bus/cache checks pass in current CI. |
| CPU-04 | 01, 03, 05, 08, 09, 11, 16, 17, 18, 20, 23, 26, 27 | SATISFIED | Truth 4; runner, loader, stub, mailbox, and assembler checks pass in current CI. |
| CPU-05 | 01, 03, 08, 11, 13, 21, 22, 24, 26, 27 | SATISFIED | Truth 5; probe/trace checks pass in current CI and fuzz succeeds in nightly. |

No additional requirement is mapped to Phase 2 in `REQUIREMENTS.md`; no plan requirement is orphaned.

## Required Artifacts and Key Links

| Area | Evidence in current code | Status |
|---|---|---|
| CPU and COP0 | `src/cpu.c`, `src/cpu_exec.inc`, `src/cop0.c`; named vector, exception, delay, and timing tests. | VERIFIED |
| Bus and interrupts | `src/bus.c`, `src/irq.c`; `tests/bus/bus_tests.c` and `tests/bus/cache_tests.c`. | VERIFIED |
| PS-EXE and runner | `src/exe.c`, `src/stub_kernel.c`, `runner/main.c`, `runner/mailbox.c`, `runner/probe.c`, `runner/trace.c`; fixture and runner tests. | VERIFIED |
| Public-vector and exception inputs | `tests/vectors/r3000/sst-sample.bin`, `tests/fixtures/src/exc_matrix.asm`, `tests/diag/expected.csv`; CI/nightly registrations. | VERIFIED |

| From | To | Via | Status |
|---|---|---|---|
| Vector harness | CPU single-step API | `tests/vectors/vectors.c` invokes the core stepper with each public case. | WIRED |
| CI inventory | CPU, bus, vector, EXE, fuzz, fixture tests | `tests/inventory/ci.txt` and per-directory `tests.cmake`; hosted JUnit inventory checks. | WIRED |
| Exception fixture | Expected records | Runner mailbox output compared to `tests/diag/expected.csv`. | WIRED |
| Probe and first-fault CLI | Core EXE validator and trace ring | Runner calls public core APIs and formats their returned data. | WIRED |

### Behavioral Evidence

All behavior checks were run by the hosted CI/nightly workflows listed above. This refresh ran no local tests or application processes. There are no user-only verification items.

### Probe Execution

No shell probe script is declared for this phase. The `--probe` CLI behavior is covered by the registered probe and fixture tests in CI.

### Anti-Patterns and Historical Review Note

No unresolved `TBD`, `FIXME`, or `XXX` debt marker was found in the phase implementation areas. Two observations in the prior report are fixed in current code: user-mode instruction fetch is checked in `src/cpu_exec.inc`, and a failed later-word I-cache refill preserves previously valid words in `src/cpu.c`. The exception-loop heuristic advisory in frontmatter remains non-blocking; it does not invalidate the required loop diagnostic.

## Human Verification Required

None. All five success criteria have automated evidence, consistent with the project's automation-first verification policy.

## Gaps Summary

No gaps. All roadmap success criteria and CPU-01 through CPU-05 are verified against current code and hosted evidence.

---

_Verified: 2026-11-18T10:41:44Z_
_Verifier: gsd-verifier_
