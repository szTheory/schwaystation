---
phase: 02
review: 02-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: deferred
    todo: "025"
    bound_to: "phase 4, the first plan that edits src/cpu_exec.inc"
    title: "User-mode (SR.KUc = 1) instruction fetch from KSEG0, KSEG1 or the BIOS window is not an address error"
  - id: WR-02
    severity: warning
    disposition: deferred
    todo: "026"
    bound_to: "phase 4, the first plan that edits src/cpu.c or the CPU part of the state walk"
    title: "State load accepts a non-zero `r0` and arbitrary values in COP0 bits the CPU never produces"
  - id: WR-03
    severity: warning
    disposition: deferred
    todo: "027"
    bound_to: "phase 4, the first plan that edits src/cpu.c"
    title: "A bus error while refilling later words of an I-cache line overwrites a still-valid cached word with 0"
  - id: WR-04
    severity: warning
    disposition: deferred
    todo: "028"
    bound_to: "phase 4, the first plan that edits src/cop0.c"
    title: "The exception-loop heuristic reports a guest that is making progress as a first fault"
  - id: WR-05
    severity: warning
    disposition: deferred
    todo: "029"
    bound_to: "phase 4, the first plan that edits runner/main.c"
    title: "The runner prints guest TTY bytes unfiltered, so a hostile executable can drive the user's terminal"
  - id: WR-06
    severity: warning
    disposition: fixed
    todo: "030"
    fixed_by: "phase 3, plan 21"
    title: "`docs/hardware/cpu.md` says the CPU does not take interrupts"
  - id: IN-01
    severity: info
    disposition: fixed
    todo: "031"
    fixed_by: "phase 3, plan 06"
    title: "The interrupt entry re-fetches the word at EPC from the bus, with side effects, and ignores the I-cache"
  - id: IN-02
    severity: info
    disposition: deferred
    todo: "032"
    bound_to: "phase 4, the first plan that makes the display range guest-controlled"
    title: "`print_hash` does not bound the video height, unlike `write_png`"
  - id: IN-03
    severity: info
    disposition: deferred
    todo: "033"
    bound_to: "SEED-007, a tooling seed"
    title: "Vector harness reads an uninitialised cycle record in two malformed-file paths and ORs overlapping store chunks"
  - id: IN-04
    severity: info
    disposition: deferred
    todo: "034"
    bound_to: "phase 4, the first plan that edits .github/workflows/ci.yml, else SEED-007"
    title: "The `nofp` job's 1-minute timeout leaves no margin for runner start-up"
  - id: IN-05
    severity: info
    disposition: deferred
    todo: "035"
    bound_to: "SEED-007, a tooling seed"
    title: "`tools/sst_slice.py` writes a container whose name field is not 16 bytes if a file stem is longer"
  - id: IN-06
    severity: info
    disposition: deferred
    todo: "036"
    bound_to: "phase 4, the first plan that edits src/exe.c"
    title: "`schway_load_exe` refuses only `frame != 0`, so it can replace RAM under a CPU that has already run"
  - id: IN-07
    severity: info
    disposition: deferred
    todo: "037"
    bound_to: "phase 4, the first plan that adds a width-sensitive GPU register"
    title: "SWL and SWR reach the bus as separate 4/2/1-byte writes, each queued and charged on its own, and the vectors do not check that"
open: 0
fixed: 2
deferred: 11
total: 13
recorded: 2026-11-10T00:52:41.840Z
triaged: 2026-11-10
---
# Phase 02: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | deferred to phase 4 (first plan that edits `src/cpu_exec.inc`) | todo 025 |
| WR-02 | warning | deferred to phase 4 (first plan that edits `src/cpu.c` or the CPU state walk) | todo 026 |
| WR-03 | warning | deferred to phase 4 (first plan that edits `src/cpu.c`) | todo 027 |
| WR-04 | warning | deferred to phase 4 (first plan that edits `src/cop0.c`) | todo 028 |
| WR-05 | warning | deferred to phase 4 (first plan that edits `runner/main.c`) | todo 029 |
| WR-06 | warning | fixed by phase 3, plan 21 | todo 030 (completed) |
| IN-01 | info | fixed by phase 3, plan 06 (the interrupt entry peeks without side effects) | todo 031 (completed) |
| IN-02 | info | deferred to phase 4 (the plan that makes the display range guest-controlled) | todo 032 |
| IN-03 | info | deferred to SEED-007 (tooling seed) | todo 033 |
| IN-04 | info | deferred to phase 4 (first plan that edits `ci.yml`), else SEED-007 | todo 034 |
| IN-05 | info | deferred to SEED-007 (tooling seed) | todo 035 |
| IN-06 | info | deferred to phase 4 (first plan that edits `src/exe.c`) | todo 036 |
| IN-07 | info | deferred to phase 4 (first plan that adds a width-sensitive GPU register) | todo 037 |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`. Todo files live in
`.planning/todos/pending/` (deferred) and `.planning/todos/completed/` (fixed). Phase 4 is unplanned, so each phase 4
binding names the first plan that edits the file the finding names; the todo is re-bound to that plan's number when
phase 4 is planned.
