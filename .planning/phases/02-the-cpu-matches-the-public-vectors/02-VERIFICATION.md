---
phase: 02-the-cpu-matches-the-public-vectors
verified: 2026-11-10T01:15:00Z
status: passed
score: 5/5 must-haves verified
covered_files: [".github/workflows/ci.yml", ".github/workflows/nightly.yml", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-01-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-01-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-02-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-02-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-03-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-03-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-04-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-04-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-05-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-05-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-06-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-06-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-07-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-07-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-08-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-08-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-09-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-09-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-10-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-10-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-11-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-11-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-12-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-12-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-13-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-13-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-14-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-14-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-15-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-15-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-16-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-16-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-17-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-17-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-18-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-18-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-19-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-19-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-20-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-20-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-21-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-21-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-22-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-22-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-23-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-23-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-24-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-24-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-25-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-25-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-26-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-26-SUMMARY.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-27-PLAN.md", ".planning/phases/02-the-cpu-matches-the-public-vectors/02-27-SUMMARY.md", "cmake/Armips.cmake", "include/schwaystation.h", "pins.txt", "runner/mailbox.c", "runner/main.c", "runner/probe.c", "runner/trace.c", "src/bus.c", "src/cop0.c", "src/cpu.c", "src/cpu_exec.inc", "src/exe.c", "src/frame.c", "src/irq.c", "src/stub_kernel.c", "tests/bus/bus_tests.c", "tests/bus/cache_tests.c", "tests/cpu/delay_tests.c", "tests/cpu/exception_tests.c", "tests/cpu/timing_tests.c", "tests/diag/expected.csv", "tests/fixtures/src/exc_matrix.asm", "tests/vectors/r3000/sst-sample.bin", "tests/vectors/tests.cmake", "tests/vectors/vectors.c"]
covered_digest: "v4:sha256:c91e4df1a6b5ea96bfe5750ce75d89da668e354f7ef9fa19a4606a41564d96c6"
behavior_unverified: 0
overrides_applied: 0
advisory:
  - finding: "WR-01: user-mode (SR.KUc = 1) instruction fetch from KSEG0/KSEG1/BIOS is not an address error; the data path checks KUc (src/cpu_exec.inc:421), the fetch path does not"
    category: other
    reason: "Real and confirmed in code (the only KUc test in src/cpu_exec.inc outside the data check is the COP0-usable test at line 879). No success criterion or CPU-0x text names user-mode fetch; vectors force SR to 0. Belongs to the review-fix step with a cpu.exception.* test and a cpu.md line."
    evidence_status: "code read; no failing test named by any requirement"
  - finding: "WR-03: an I-cache refill that bus-errors on a later word writes 0 into a word whose valid bit is still set (src/cpu.c:124-132 with src/bus.c:539-541)"
    category: other
    reason: "Real and confirmed in code. Needs a guest to shrink RAM_SIZE or unmap a window under a partly valid line; CPU-03's listed I-cache behaviours (fill to line end, per-word valid bits, isolation) pass in bus.icache_*. Review-fix step."
    evidence_status: "code read"
  - finding: "WR-04: the exception-loop heuristic latches a first fault after 1,000 consecutive same-EPC same-ExcCode exceptions even if the handler makes progress (src/cop0.c:60-75)"
    category: other
    reason: "Real, bounded to the first-fault report and the runner's default stop-on-fault; emulation results and the core's own run are unaffected. CPU-05 asks that an exception loop be reported and fault_exception_loop.exe is; the false-positive shape is undocumented. Review-fix step (document or tighten)."
    evidence_status: "code read; fault_exception_loop fixture passes"
  - finding: "WR-02 (state load accepts non-zero r0 and unreachable COP0 bits), WR-05 (runner TTY bytes unfiltered), WR-06 (cpu.md says the CPU does not take interrupts), IN-01 to IN-07"
    category: other
    reason: "None defeats a roadmap success criterion or a CPU-01..05 requirement. WR-06 is a stale doc sentence; WR-05 is terminal hygiene for a hostile EXE; IN-03/IN-05 are test and tooling code (seeds under rule 7). All 13 dispositions are open in 02-REVIEW-DISPOSITION.md for the review-fix step."
    evidence_status: "02-REVIEW.md"
---

# Phase 2: The CPU matches the public vectors Verification Report

**Phase Goal:** The R3000A matches the public vectors before any game runs, with COP0 exceptions, bus decode, memory control, the I-cache and interrupt registers; PS-EXEs load and run under the own stub kernel, and the probe and first-fault trace diagnose any EXE
**Verified:** 2026-11-10T01:15:00Z
**Status:** passed
**Re-verification:** No, initial verification

Method: the SUMMARY claims were not used as evidence. Every row below was checked by running the commands in this
working tree (head `cb6e0fa`; code unchanged since `a89637d`), by reading the code, or by querying the hosted runs
through `gh run view` and `gh api` (read-only).

## Goal Achievement

### Observable Truths (roadmap success criteria)

| # | Truth | Status | Evidence |
| - | ----- | ------ | -------- |
| 1 | `ctest -L vectors` passes all 5,500 sample cases on registers, delay latches and every bus cycle; a zero-case run exits 1; nightly `vectors-full` passes 55,000 at the pin | VERIFIED | `ctest --test-dir build/ci -L vectors`: 13 of 13 passed. Test 178 prints `vectors: 5500 cases, 0 failures` (and `rule (a) handled 310 cases, rule (b) handled 400 cases`, both rules named and documented in `docs/hardware/cpu.md` lines 215-220; `tests/vectors/r3000/allow.txt` has no entries, so nothing is skipped). `vectors.r3000.empty.self_test` and `truncated.self_test` pass (expect exit 1). Six `flip.*` self-tests (r5, epc, tar, cause, branch_target, load_value) prove the comparison is not vacuous. `tests/nightly/tests.cmake:38` runs `--expect 55000` on the pinned files fetched by blob SHA; hosted nightly run 42728718880 is `success` with `vectors-full` and `fuzz` both `success`. Nightly head `a91d542` differs from the code head `a89637d` by no file under `src runner include tests cmake tools` (git diff empty). |
| 2 | Exception, delay-slot and MULT/DIV timing tests pass; `exc_matrix` `mailbox@30` records match `expected.csv` rows | VERIFIED | `ctest -L cpu`: 57 of 57 passed, including `cpu.delay.*` (load_delay, write_cancels_pending, failed_load_untouched, load_before_irq, lwl_lwr_interlock, mfc0_delay), `cpu.timing.*` (mult_6, mult_9, mult_13, multu_raw, div_36, divu_36, early_read_stalls, replace_while_busy, mtlo_mthi_stamp_zero, survives_exception) and `cpu.exception.*`. I ran `schwaystation-run --exe build/ci/fixtures/exc_matrix.exe --no-stop-on-fault --frames 30 --stop-on mailbox`: `mailbox SWMB status=00 records=30`, and the 30 values equal the 30 `exc_matrix` rows of `tests/diag/expected.csv` (all rows `source=doc`; `tests/diag/allow.txt` is empty). `fixtures.exc_matrix` passes. |
| 3 | `ctest -L bus` passes on physical decode, RAM mirrors, memory-control costs, I-cache with isolation, write queue, scratchpad, I_STAT/I_MASK | VERIFIED | `ctest -L bus`: 17 of 17 passed: ram_mirrors, memctl_costs, scratchpad, bios_window, exp_regions, kseg2, irq_edge, irq_line, irq_cause_bit, icache_fill, icache_valid_bits, icache_isolation, write_queue_depth, write_queue_drain, write_queue_exception, write_queue_state. Physical decode is also covered by the vectors' bus cycles. |
| 4 | `--exe hello.exe --tty --stop-on mailbox` prints `tty ` lines, shows `SWMB`, exits 0; armips built on every leg at its pin assembles every fixture to one digest; `fixtures.armips_pin.self_test` fails on a wrong digest | VERIFIED | Ran it: stdout `tty hello port`, `tty hello putchar`, `mailbox SWMB status=00 records=2`; exit 0. `pins.txt:21` pins armips `62adab4ef30d` with a tarball SHA-256; `fixtures.armips_pin.self_test` passes. `build/ci/hashes.txt` carries 22 fixture digests; hosted CI run 42729199629 on `a89637d` is `success` for all 20 jobs including every `build (...)` leg, `be32` and `hash-equality`, which compares eight `hashes.txt` files byte for byte (`ci.yml:246-264`). |
| 5 | `--probe FILE` prints identity fields without emulating; `--trace first-fault` on a crafted EXE prints one `fault ...` line and a 4,096-record tail and exits 7 | VERIFIED | `--probe hello.exe` exit 0: `format`, `serial`, `licence_area`, `region_guess`, `exe_name`, `exe_size`, `sha256`, `rcheevos_md5` and the header fields (`pc0 gp0 t_addr t_size d_* b_* s_* lint`); `exe.probe_valid`, `exe.probe_refusals`, `exe.probe_usage` pass. `--exe fault_unmapped_read.exe --trace first-fault`: exit 7, 4,097 lines, exactly one `fault frame=0 line=0 dot=0 pc=0x80010128 op=0x00000000 addr=0xbf801030 width=4 dev=unmapped kind=unmapped_read` line (line 1) and 4,096 tail records. One fixture per fault class is in `hashes.txt` (unmapped fetch/read/write, misaligned fetch/read/write, reserved instruction, cop unusable, exception loop). |

**Score:** 5/5 truths verified (0 present, behavior-unverified). Behaviour-dependent truths (delay latches, write-queue
drain, exception priority, MULT/DIV stamps) each have a named passing test, so none rests on presence alone.

### Required Artifacts and Wiring

| Artifact | Expected | Status | Details |
| -------- | -------- | ------ | ------- |
| `src/cpu_exec.inc`, `src/cpu.c`, `src/cop0.c` | R3000A core, I-cache, exceptions | VERIFIED | Substantive; driven by the vectors harness and the cpu tests. |
| `src/bus.c`, `src/irq.c` | Decode, memory control, write queue, I_STAT/I_MASK | VERIFIED | Driven by `bus.*` tests and the vectors' bus cycles. |
| `src/exe.c`, `src/stub_kernel.c` | PS-EXE validator and loader, own stub kernel | VERIFIED | `fixtures.stub_drift`, `fixtures.stub.window`, `fixtures.boot.past_stub`, `fuzz.regress.exe` pass. |
| `runner/main.c`, `probe.c`, `trace.c`, `mailbox.c` | `--exe --tty --stop-on --probe --trace first-fault` | VERIFIED | All exercised directly above. |
| `tests/vectors/r3000/sst-sample.bin` | 5,500-case committed sample | VERIFIED | 2,395,390 bytes (under 4 MB); consumed by test 178. |
| `tests/diag/expected.csv`, `exc_matrix.asm` | `doc`-sourced rows | VERIFIED | 30 rows, 30 records, match. |
| `pins.txt`, `cmake/Armips.cmake` | armips pin by SHA-256 | VERIFIED | Self-test passes. |
| `.github/workflows/ci.yml`, `nightly.yml` | Lanes, `vectors-full`, `fuzz` | VERIFIED | Hosted runs green (see below). |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | -- | --- | ------ | ------- |
| vectors harness | `schway_*` step API | `tests/vectors/vectors.c` calls the core's single-step on each case | WIRED | A flipped expected field fails (six flip self-tests pass). |
| `--stop-on mailbox` | stub kernel mailbox | `runner/mailbox.c` reads `SWMB` from RAM | WIRED | `records=2` and `records=30` observed. |
| first-fault latch | runner exit 7 | `schway_get_fault` then `exit_status.h`; `runner.exit_table` | WIRED | exit 7 observed; `runner.exit_table.self_test` passes. |
| nightly `vectors-full` | pinned files | `gh api .../git/blobs/$sha` plus `git hash-object` check, `--expect 55000` | WIRED | Run 42728718880 green. |

### Data-Flow Trace (Level 4)

Not applicable beyond the above: the phase renders no UI. The runner's printed values (mailbox records, fault line,
probe fields) come from live core state, shown by the changing `exc_matrix` records and the per-fixture fault lines.

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| Full local gate | `cmake --workflow --preset ci` | exit 0; `100% tests passed out of 240` (ran in about 2 s on the built tree) | PASS |
| Vectors | `ctest --test-dir build/ci -L vectors` | 13 of 13; 5500 cases, 0 failures | PASS |
| Bus | `ctest --test-dir build/ci -L bus` | 17 of 17 | PASS |
| CPU | `ctest --test-dir build/ci -L cpu` | 57 of 57 | PASS |
| hello | `schwaystation-run --exe hello.exe --tty --stop-on mailbox` | `tty` lines, `SWMB`, exit 0 | PASS |
| exc_matrix | `schwaystation-run --exe exc_matrix.exe --no-stop-on-fault --frames 30 --stop-on mailbox` | 30 records equal the CSV | PASS |
| probe | `schwaystation-run --probe hello.exe` | identity fields, exit 0 | PASS |
| first-fault | `schwaystation-run --exe fault_unmapped_read.exe --trace first-fault` | one `fault` line, 4,096 records, exit 7 | PASS |
| Hosted CI | `gh run view 42729199629` | `success`, 20 of 20 jobs, head `a89637d` | PASS |
| Hosted nightly | `gh run view 42728718880` | `success`; `vectors-full`, `fuzz`, bench jobs, `report` | PASS |
| Determinism | `nofp` x2 and `be32` and `hash-equality` jobs in run 42729199629 | all `success` | PASS |

### Probe Execution

The phase declares no `scripts/*/tests/probe-*.sh`; its probes are the runner's `--probe` mode (covered above).
SKIPPED, no shell probes.

### Requirements Coverage

Every requirement ID in the PLAN frontmatter was cross-referenced with `.planning/REQUIREMENTS.md` (lines 30-34 and
the traceability table, lines 124-128). The union of the 27 plans' `requirements:` is exactly CPU-01..CPU-05 and
every one is claimed; REQUIREMENTS.md maps no other ID to Phase 2, so there are no orphans.

| Requirement | Source plans | Description | Status | Evidence |
| ----------- | ------------ | ----------- | ------ | -------- |
| CPU-01 | 01, 02, 03, 04, 06, 10, 12, 15, 26, 27 | Vector sample, harness, zero-case guard, nightly full | SATISFIED | Truth 1; `allow.txt` empty; sample 2.4 MB under the 4 MB cap. |
| CPU-02 | 01, 03, 10, 12, 17, 19, 25, 26, 27 | Exception, delay, timing tests; `exc_matrix` rows | SATISFIED | Truth 2. Every exception listed in the requirement (AdEL/AdES on misalignment, Ovf, RI, CpU, SYSCALL/BREAK with and without BD, IRQ masking, BEV, DBE) has a row. |
| CPU-03 | 01, 02, 03, 07, 14, 26, 27 | Bus, memory control, I-cache, write queue, scratchpad, I_STAT | SATISFIED | Truth 3. |
| CPU-04 | 01, 03, 05, 08, 09, 11, 16, 17, 18, 20, 23, 26, 27 | PS-EXE load under own stub kernel, `--tty`, `--stop-on`, armips pin | SATISFIED | Truth 4; `tests/api/v2.c` and `docs.api_names` pass. |
| CPU-05 | 01, 03, 08, 11, 13, 21, 22, 24, 26, 27 | `--probe`, `--trace first-fault`, loader fuzz | SATISFIED | Truth 5; `fuzz.regress.exe` gates on every leg and the nightly `fuzz` job is green (report-only by design, per the requirement text). |

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| (all phase code) | - | `TBD`, `FIXME`, `XXX` scan over `src runner include tests tools cmake docs scripts .github` | none | No debt markers. |
| `src/` | - | float, double, wall clock, `getenv`, `fopen` | none | Only a comment match; the rule-4 `nofp` jobs are green. |

### Code Review Findings Weighed Against the Criteria

02-REVIEW.md has 0 critical and 6 warnings, all `open`. I read the code behind the three the caller flagged and
checked each against the roadmap criteria and CPU-01..05 text:

- **WR-01 (user-mode instruction fetch)**: confirmed (`src/cpu_exec.inc:421` guards data accesses only). It does not
  defeat a criterion: the vectors run with SR forced to 0, CPU-02's exception list does not include user-mode fetch,
  and no later-phase requirement is blocked by it. Advisory; should be fixed in the review-fix step with a
  `cpu.exception.*` test and a `docs/hardware/cpu.md` sentence.
- **WR-03 (I-cache refill clobbers a valid word)**: confirmed at `src/cpu.c:124-132` and `src/bus.c:539-541`. Needs a
  guest that makes a later line word unmapped under a partly valid line. CPU-03's I-cache behaviours pass. Advisory.
- **WR-04 (exception-loop heuristic)**: confirmed at `src/cop0.c:60-75`. It can only produce a first-fault report
  (and a runner stop), never a wrong emulation result, and CPU-05 asks for exactly this heuristic. The false-positive
  shape is undocumented. Advisory; document or tighten in the review-fix step.
- WR-02, WR-05, WR-06 and IN-01..IN-07 do not touch a criterion. IN-03, IN-05 and IN-04 concern test, tooling and CI
  code and are seeds under rule 7, not requirements.

None of these is a gap; they stay in the review-fix queue and are not Phase 2 blockers.

### Human Verification Required

None. Under rule 6 the owner verifies nothing by default, and every criterion here is closed by a command, an exit
status and a hosted run URL (42729199629, 42728718880). No criterion needs the owner's own hardware or identity-bound
consent.

### Gaps Summary

No gaps. All five roadmap success criteria and all five requirements (CPU-01..CPU-05) are met in the code as it
stands: the local `ci` workflow passes 240 of 240, the direct spot-checks match the criteria's stated outputs and exit
statuses, and the hosted CI (20 of 20) and nightly (`vectors-full` over 55,000 cases, `fuzz`) runs are green on a code
tree identical to the current head. The 13 open review findings are recorded as advisory items for the review-fix step.

---

_Verified: 2026-11-10T01:15:00Z_
_Verifier: Claude (gsd-verifier)_
