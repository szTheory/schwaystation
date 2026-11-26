# Phase 16 ship correction review

Reviewed the uncommitted ship correction on `phase/16-patches-cheats-compatibility` at `203e7e6544f115343a0b394328f7fd465d4b4a2a`:

- `.github/workflows/ci.yml`
- `.github/workflows/nightly.yml`
- `.planning/REQUIREMENTS.md`
- `tests/cmake/workflows.cmake`
- `tests/hygiene/tests.cmake`

The failed hosted preflight exposed a stale 3,901-line limit in both workflow commands. The current producer measured 4,692 before the added regression assertion and 4,693 after it. Both hosted preflights and the hygiene budget now use 4,693; the workflow wiring test checks the `changes` job in CI and the `fuzz` job in Nightly. The requirements text records the approved 4,694 ceiling and the measured ship ratchet.

No correctness, security, or maintainability findings remain in this correction. The focused workflow, line-budget, and diff-whitespace checks passed locally. See `16-LOCAL-EVIDENCE.md` for commands, exit statuses, and hosted run results.
