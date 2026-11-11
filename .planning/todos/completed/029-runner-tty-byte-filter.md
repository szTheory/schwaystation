---
title: "Filter control bytes out of the runner's guest TTY output"
status: complete
priority: P2
source: "phase 02 code review, finding WR-05"
created: 2026-11-10
theme: runner
bound_step: "phase 4, plan 10"
---

## Goal

With `--tty`, the runner prints every byte outside printable ASCII and tab as `?`, so a hostile executable cannot
move the cursor, set the window title or overwrite the `tty ` prefix on a person's terminal.

## Context

- Review finding: `tty_emit` in `runner/main.c` writes the guest's bytes unfiltered, while `runner/probe.c` already
  replaces every byte outside printable ASCII so a hostile name cannot add a line or a control sequence. Line
  structure is safe (the prefix is re-added per line), so machine parsers are unaffected.
- The fix is the `put_text` filter plus tab (`c >= 0x20 && c < 0x7F` or `c == 0x09`, else `?`).
- Bound to phase 4: the first phase-4 plan that edits `runner/main.c` (the `--save-dir` flag is the first planned
  caller; name the plan here when phase 4 is planned).

## Acceptance Criteria

- A runner test feeds an executable that writes an escape byte and a carriage return and sees `?` in their place.
- `docs/runner.md` states that non-printable bytes print as `?`.
- The test runs in `cmake --workflow --preset ci`, and no planning ID appears in the code it adds.

## Resolution

Completed by phase 4, plan 10. `tty_drain` replaces every byte outside printable ASCII and tab with `?` before
the runner appends it to a line. `runner.tty_sanitize` checks ESC, CR, tab, printable ASCII, DEL and a high byte;
`cmake --workflow --preset ci` passed. The behavior is wired into the runner's TTY drain path. User-facing wording
is assigned to phase 4, plan 32, which owns `docs/runner.md`.
