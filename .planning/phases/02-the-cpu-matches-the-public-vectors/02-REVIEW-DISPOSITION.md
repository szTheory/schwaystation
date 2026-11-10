---
phase: 02
review: 02-REVIEW.md
titles: json
findings:
  - id: WR-01
    severity: warning
    disposition: open
    title: "User-mode (SR.KUc = 1) instruction fetch from KSEG0, KSEG1 or the BIOS window is not an address error"
  - id: WR-02
    severity: warning
    disposition: open
    title: "State load accepts a non-zero `r0` and arbitrary values in COP0 bits the CPU never produces"
  - id: WR-03
    severity: warning
    disposition: open
    title: "A bus error while refilling later words of an I-cache line overwrites a still-valid cached word with 0"
  - id: WR-04
    severity: warning
    disposition: open
    title: "The exception-loop heuristic reports a guest that is making progress as a first fault"
  - id: WR-05
    severity: warning
    disposition: open
    title: "The runner prints guest TTY bytes unfiltered, so a hostile executable can drive the user's terminal"
  - id: WR-06
    severity: warning
    disposition: open
    title: "`docs/hardware/cpu.md` says the CPU does not take interrupts"
  - id: IN-01
    severity: info
    disposition: open
    title: "The interrupt entry re-fetches the word at EPC from the bus, with side effects, and ignores the I-cache"
  - id: IN-02
    severity: info
    disposition: open
    title: "`print_hash` does not bound the video height, unlike `write_png`"
  - id: IN-03
    severity: info
    disposition: open
    title: "Vector harness reads an uninitialised cycle record in two malformed-file paths and ORs overlapping store chunks"
  - id: IN-04
    severity: info
    disposition: open
    title: "The `nofp` job's 1-minute timeout leaves no margin for runner start-up"
  - id: IN-05
    severity: info
    disposition: open
    title: "`tools/sst_slice.py` writes a container whose name field is not 16 bytes if a file stem is longer"
  - id: IN-06
    severity: info
    disposition: open
    title: "`schway_load_exe` refuses only `frame != 0`, so it can replace RAM under a CPU that has already run"
  - id: IN-07
    severity: info
    disposition: open
    title: "SWL and SWR reach the bus as separate 4/2/1-byte writes, each queued and charged on its own, and the vectors do not check that"
open: 13
total: 13
recorded: 2026-11-10T00:52:41.840Z
---
# Phase 02: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| WR-01 | warning | open | - |
| WR-02 | warning | open | - |
| WR-03 | warning | open | - |
| WR-04 | warning | open | - |
| WR-05 | warning | open | - |
| WR-06 | warning | open | - |
| IN-01 | info | open | - |
| IN-02 | info | open | - |
| IN-03 | info | open | - |
| IN-04 | info | open | - |
| IN-05 | info | open | - |
| IN-06 | info | open | - |
| IN-07 | info | open | - |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
