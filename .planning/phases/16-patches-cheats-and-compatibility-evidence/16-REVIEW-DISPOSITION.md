---
phase: 16
review: 16-REVIEW.md
titles: json
findings:
  - id: CR-01
    severity: critical
    disposition: fixed
    title: "BPS staging prevents the WASI runner from compiling"
  - id: CR-02
    severity: critical
    disposition: fixed
    title: "WASI CCD path resolution traps in the 128 KiB runner stack"
  - id: WR-01
    severity: warning
    disposition: fixed
    title: "Nightly fuzz workflow runs targets omitted from its build preset"
  - id: WR-02
    severity: warning
    disposition: fixed
    title: "Capability importer attributes an unrelated JUnit failure to every selector"
  - id: WR-03
    severity: warning
    disposition: fixed
    title: "Windows library compatibility checks depend on `shasum`"
  - id: WR-04
    severity: warning
    disposition: fixed
    title: "BE32 inventory tests invoke the target runner without its emulator"
  - id: WR-05
    severity: warning
    disposition: fixed
    title: "BE32 inventory controls invoke the test helper without its emulator"
open: 0
total: 7
recorded: 2026-11-26T22:12:30.350Z
---

# Phase 16: Code Review Disposition

| Finding | Severity | Disposition | Source |
|---------|----------|-------------|--------|
| CR-01 | critical | fixed | 16-REVIEW-FIX.md |
| CR-02 | critical | fixed | 256 KiB WASI stack baseline; `runner.disc.legacy` passed under Wasmtime 48.0.7 |
| WR-01 | warning | fixed | 16-REVIEW-FIX.md |
| WR-02 | warning | fixed | 16-REVIEW-FIX.md |
| WR-03 | warning | fixed | `sha256sum` / `shasum` fallback; all three native library compatibility tests passed |
| WR-04 | warning | fixed | `16-REVIEW.md` and `16-LOCAL-EVIDENCE.md`; changed-head hosted BE32 validation is pending |
| WR-05 | warning | fixed | `16-REVIEW.md` and `16-LOCAL-EVIDENCE.md`; the controls helper now uses the decoded emulator |

Dispositions: `open` (recorded, not yet triaged), `fixed`, `skipped`, `deferred`.
Set `deferred` by hand and put the reason in the Source cell; both are preserved. A `|` in the reason is kept as prose and escaped on the next run.
Re-running the gate keeps every row it can. A row the current review no longer reports is kept and its Source cell flagged, so a finding does not leave this record silently. ONE exception: when a finding id is REUSED by a different finding, the earlier decision cannot keep a row — the id is taken — and it is dropped. A RECORDED decision (anything but `open`) is named on the console when that happens; a row still at `open` is replaced silently, because `open` records no decision to lose.
