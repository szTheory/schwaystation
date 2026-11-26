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

## Hosted-CI correction review

Reviewed the follow-up corrections based on `25ba6bb749c968018e23618df35951a12c092645`:

- `.github/workflows/nightly.yml` — the fuzz timeout increases from 15 to 18 minutes based on run 45885388337, where 12 of 13 bounded 61-second campaigns completed before the job limit.
- `examples/sdl2/CMakeLists.txt` — the installed-library example now compiles the patch/BPS providers and their CRC/SHA helpers, and adds the runner's private include directory. The local installed-consumer build passed.
- `host/disc_bps.c` — only MSVC uses `tmpfile_s()` to avoid its deprecation warning under `/WX`; WASI's exclusive unlinked staging and other platforms' `tmpfile()` path are unchanged.
- `quality.json` — exact ceilings follow the successful hosted perf and WASI producers from run 45885388357; their relative regression and size checks passed.
- `tests/disc/patch.c` — the malformed two-byte record now has two readable fixture bytes, so the test reaches the intended source-bound rejection without an out-of-bounds test read. The seven focused cases passed in normal and ASan builds.

No correctness, security, or maintainability findings remain in this correction. Windows compilation and the complete CI/Nightly results remain pending a fresh hosted run. Commands and current hosted outcomes are recorded in `16-LOCAL-EVIDENCE.md`.

## MSVC inventory file-open correction review

Reviewed `host/dat_inventory.c` after CI run [45888744986](https://github.com/szTheory/schwaystation/actions/runs/45888744986) exposed MSVC C4996 for `fopen()` in the private report writer. The new helper uses `fopen_s()` under MSVC for both DAT reads and exclusive report creation, restores its error code to `errno` for the existing `EEXIST` retry, and leaves other C libraries on `fopen()`. The compatibility inventory tests passed after this change. Microsoft's CRT documentation confirms `fopen_s()` accepts the C11 exclusive-create `x` mode: [Microsoft Learn](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-s-wfopen-s?view=msvc-170).

No findings remain in this correction. The hosted Windows build and full CI/Nightly validation must run again on the next head.
