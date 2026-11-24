---
phase: 14-multitap-and-special-controllers
fixed_at: 2026-11-24T13:25:02Z
review_path: .planning/phases/14-multitap-and-special-controllers/14-REVIEW.md
iteration: 1
findings_in_scope: 4
fixed: 4
skipped: 0
status: all_fixed
---

# Phase 14: Code Review Fix Report

- **Fixed at:** 2026-11-24T13:25:02Z
- **Source review:** `.planning/phases/14-multitap-and-special-controllers/14-REVIEW.md`
- **Iteration:** 1

**Summary:**
- Findings in scope: 4
- Fixed: 4
- Skipped: 0

## Fixed Issues

### CR-01: Accepted multitap state can reach an out-of-range shift

- **Files modified:** `src/sio.c`, `tests/core/test_state.c`
- **Commit:** `cc6eeb1`
- **Applied fix:** State validation now checks each transaction mode's live terminal index, including long reply 34, short reply 2, and pad reply 8. Added checksummed malformed states for every mode and verified rejected imports remain atomic.

### CR-02: Runner input movies stop controlling socket A when a tap is enabled

- **Files modified:** `runner/main.c`, `tests/runner/tests.cmake`
- **Commit:** `b88bc11`
- **Applied fix:** When a tap is enabled, the runner copies the existing physical-port device, buttons, axes and analog button into socket A before `schway_set_input`. The runner movie regression confirms legacy p1 input changes socket-A state with a tap enabled; B–D remain unpopulated.

### WR-01: SIO documentation still describes only two card slots

- **Files modified:** `docs/hardware/sio.md`
- **Commit:** `ee834d8`
- **Applied fix:** Documented the eight port/socket card endpoints, socket address range with taps enabled, and the existing socket-A address when taps are disabled.

### WR-02: Runner save-directory option still promises two card files

- **Files modified:** `docs/runner.md`
- **Commit:** `ea7ab0b`
- **Applied fix:** Updated `--save-dir` to describe loading and saving all eight card sockets.

## Verification

Verification ran in the main checkout because `workflow.use_worktrees` is `false`. No isolated worktree was created. All commands ran from the repository root; no hosted run was requested, so there is no run URL.

**Tree status:** All four fix commits are present, with no modified or staged tracked files. This report is the only untracked file and is left for the parent workflow to commit.

- `git diff --check` — exit 0.
- `LC_ALL=C sort -cu tests/core/inventory.txt` — exit 0.
- `LC_ALL=C sort -cu tests/runner/inventory.txt` — exit 0.
- `cmake --preset ci` — exit 0.
- `ctest --preset ci --show-only=json-v1 -R '^(core[.]state[.]rejects|sio[.]tap[.]history_edges|runner[.]inputs[.](script|controls))$' | python3 -c 'import json,sys; actual={t["name"] for t in json.load(sys.stdin)["tests"]}; required={"core.state.rejects","sio.tap.history_edges","runner.inputs.script","runner.inputs.controls"}; print("enumerated:", ", ".join(sorted(actual))); missing=required-actual; print("missing:", ", ".join(sorted(missing))); sys.exit(bool(missing))'` — exit 0; all four selectors were present.
- `ctest --preset ci --no-tests=error -R '^fixtures[.]stub_drift$' --output-on-failure` — exit 0; the committed fixture matches its source.
- `cmake --build --preset ci --target schwaystation-test-state schwaystation-test-sio schwaystation-run` — exit 0.
- `ctest --preset ci --no-tests=error -R '^(core[.]state[.]rejects|sio[.]tap[.]history_edges|runner[.]inputs[.](script|controls))$' --output-on-failure` — exit 0; all four targeted tests passed.
- Exploratory `LC_ALL=C sort -cu tests/fixtures/manifest.txt` — exit 1 because the existing manifest is not alphabetically ordered. The manifest was not modified; the project fixture consistency test passed.

## Skipped Issues

None.

---

- _Fixed: 2026-11-24T13:25:02Z_
- _Fixer: the agent (gsd-code-fixer)_
- _Iteration: 1_
