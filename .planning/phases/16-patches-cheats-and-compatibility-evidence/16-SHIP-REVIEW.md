# Phase 16 ship correction review

Reviewed the uncommitted ship correction on `phase/16-patches-cheats-compatibility` at `203e7e6544f115343a0b394328f7fd465d4b4a2a`:

- `.github/workflows/ci.yml`
- `.github/workflows/nightly.yml`
- `.planning/REQUIREMENTS.md`
- `tests/cmake/workflows.cmake`
- `tests/hygiene/tests.cmake`

The failed hosted preflight exposed a stale 3,901-line limit in both workflow commands. The current producer measured 4,692 before the added regression assertion and 4,693 after it. Both hosted preflights and the hygiene budget now use 4,693; the workflow wiring test checks the `changes` job in CI and the `fuzz` job in Nightly. The requirements text records the approved 4,694 ceiling and the measured ship ratchet.

No correctness, security, or maintainability findings remain in this correction. The focused workflow, line-budget, and diff-whitespace checks passed locally. See `16-LOCAL-EVIDENCE.md` for commands, exit statuses, and hosted run results.

## Follow-up: source and timeout corrections

Reviewed the follow-up diff based on `f3d544967e6efcc2936e3dea5837ad4ef084cf7f`:

- `runner/main.c` — the frame loop now skips cheat-table validation when the parsed cheat count is zero. Configured cheats still use the same per-frame application and failure path.
- `tests/disc/patch.c` — non-literal BPS actions now receive a valid unused byte pointer. The fixture macro copies bytes only for literal actions, so this only avoids GCC's `-Werror=nonnull` diagnostic and does not change the generated patch.
- `tests/hygiene/tests.cmake` — `hygiene.tree` has a 180-second timeout, above the measured 101.02-second standalone scan and 75.99-second hygiene-workflow scan.

No findings remain. Focused patch/cheat regressions passed locally; the GCC 15 patch build and final hosted performance, WASI-size, hygiene, and platform results are recorded in `16-LOCAL-EVIDENCE.md` after they complete.
