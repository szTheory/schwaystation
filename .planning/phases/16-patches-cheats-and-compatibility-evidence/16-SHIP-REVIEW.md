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

## Windows test-host and WASI replay correction review

Reviewed the follow-up corrections prompted by CI [45889677135](https://github.com/szTheory/schwaystation/actions/runs/45889677135):

- `tests/libretro/host.c` — `core_size` now has a defined zero fallback before the conditional load path, satisfying MSVC's `/W4 /WX` analysis without changing the valid loaded-size path.
- `tests/fuzz/tests.cmake` — the 256 KiB linker stack applies only to the WASI PPF regression replay executable. Its nested source/target buffers and baseline patch exceed WASI's 128 KiB default; the previously trapping replay now passes under pinned Wasmtime.

No correctness, security, or maintainability findings remain. The targeted native host build and WASI PPF regression passed locally. The corrected hosted Windows matrix and full CI/Nightly results remain pending.

## WASI stack and Windows hash correction review

Reviewed the changes prompted by CI run [45892261219](https://github.com/szTheory/schwaystation/actions/runs/45892261219):

- `cmake/toolchains/wasm32-wasi.cmake` raises the stack baseline to 256 KiB after the required CCD runner path trapped with the prior 128 KiB stack. The per-target 256 KiB overrides are removed because the toolchain now provides the same tested baseline to every WASI executable.
- `scripts/library.sh` selects `sha256sum` or `shasum -a 256`, matching the existing cross-platform pin-fetching pattern. The three native library compatibility tests passed.
- `quality.json` lowers the WASM floor to the measured local result of 1,014,617 bytes and retains 1,014,665 as the latest exact hosted ceiling until the revised hosted module is measured.

The pinned Wasmtime 48.0.7 local run passed `runner.disc.legacy`, all 14 selected patch/PPF/cheat tests, and the local WASM size check against the revised floor. The exact changed-head hosted WASM producer and Windows matrix remain required ship checks.
