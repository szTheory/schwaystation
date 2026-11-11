---
phase: 04-games-play-in-retroarch
plan: "22"
subsystem: testing
tags: [openbios, linux-ci, sha256, wasi, be32]
requires:
  - phase: "04-21"
    provides: Release archive firmware scanning and CI hygiene contracts.
  - phase: "04-38"
    provides: BIOS identity and digest handling.
provides:
  - A pinned Linux-only OpenBIOS build checked against its measured image size and SHA-256.
  - Gating synthetic digest-match and corrupted-copy refusal cases.
  - Correct emulator forwarding and WASI filesystem setup for runner card checks.
affects: [GAME-05, "04-39", "04-40"]
actuals:
  tokens: 4545
  tasks: 2
  commits: 7
plan_head_before: 23ef5f993922c82dd89fcab89fbd7cfa3c936965
plan_head_after: 522a48fd10e2b9b8a2820cfb21a4d8aa2f179167
tech-stack:
  added: [pinned Linux mipsel GCC 12.3.0 build tool]
  patterns: [measured output digest with corrupted-copy control]
key-files:
  created:
    - cmake/OpenBIOS.cmake
    - tests/built/expected.txt
    - tests/built/inventory.txt
    - tests/built/tests.cmake
  modified:
    - .github/workflows/ci.yml
    - CMakePresets.json
    - docs/testing.md
    - THIRD-PARTY-NOTICES.md
    - pins.txt
    - host/cards.c
    - tests/card/card_tests.c
    - tests/runner/tests.cmake
    - tests/inventory/asan.txt
    - tests/inventory/wasm32.txt
key-decisions:
  - "Build the upstream-supported OPENBIOS_SHELL=shell target with the pinned source and Linux toolchain."
  - "Keep OpenBIOS bytes in the Linux runner's temporary directory; commit only the measured SHA-256."
  - "Set be32 to seven minutes after the 433-test run exceeded the old five-minute bound on two long QEMU tests."
patterns-established:
  - "The Linux OpenBIOS lane checks 524288 bytes and a pinned SHA-256; a one-byte corruption must be refused."
requirements-completed: []
coverage:
  - id: D1
    description: The pinned Linux build produces the expected 524288-byte OpenBIOS image and digest.
    requirement: GAME-05
    verification:
      - kind: integration
        ref: "CI run 43069874480, Linux x64 job 130694615653, head 522a48fd10e2b9b8a2820cfb21a4d8aa2f179167"
        status: pass
    human_judgment: false
  - id: D2
    description: The digest consumer accepts the expected image and rejects a corrupted copy.
    requirement: GAME-05
    verification:
      - kind: unit
        ref: "cmake --workflow --preset ci: built.openbios.digest and built.openbios.digest_corrupt"
        status: pass
    human_judgment: false
duration: 106min
completed: 2026-11-11
status: complete
---

# Phase 04 Plan 22: Pinned OpenBIOS build summary

**Linux CI builds the pinned nugget OpenBIOS image with GCC 12.3.0 and rejects any output that differs from its measured 512 KiB SHA-256.**

## Performance

- **Duration:** 106 minutes, including hosted-run recovery and exact-head CI.
- **Started:** 2026-11-11T16:12:50Z, the first plan commit.
- **Completed:** 2026-11-11T17:58:21Z, after exact-head `CI required` passed at 17:43:32Z.
- **Tasks:** 2.
- **Files modified:** 20.

## Accomplishments

- CI fetched nugget commit `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9` and verified the pinned Linux GCC 12.3.0 archive (`228f031a25cf2687d8845fd1421f625bafc211fa27da428e458e80d030a726f8`). The build used upstream's supported `OPENBIOS_SHELL=shell` option.
- The real Linux build produced 524288 bytes with SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8`. `tests/built/expected.txt` contains that digest; the firmware bytes stayed in the runner's temporary directory.
- The registered `built.openbios.digest` and `built.openbios.digest_corrupt` tests passed, including the one-byte-corrupted negative control.
- Fixed C17 host declarations, Windows input-file newlines, WASI cwd preopen and large test-only stack buffers, and the nested runner emulator forwarding. Corrected the emulator being prepended twice in three runner card cases.
- Increased the be32 job bound to seven minutes based on the expanded inventory's measured runtime. All 433 tests completed in the hosted run.

## Task Commits

- Task 1 implementation and measured digest: `2c2daf9`, `cf35cfc`, `1053862`, `5c72a0e`, `3c98240`.
- Task 2 documentation and WASI wiring: `8170554`.
- Hosted be32 and runner-card corrections: `522a48f`.

## Verification

- `cmake --build --preset ci --target schwaystation-run schwaystation-test-run-stop && ctest --test-dir build/ci -R '^runner\.cards\.' --no-tests=error --output-on-failure` — exit 0; 3/3 runner card tests passed.
- `cmake --workflow --preset ci` — exit 0; 466 regular tests and 5 release tests passed.
- `cmake --workflow --preset hygiene` — exit 0; 23 checks passed.
- Hosted CI run [43069874480](https://github.com/szTheory/schwaystation/actions/runs/43069874480), captured as `OPENBIOS_RUN_ID=43069874480`, used exact head `522a48fd10e2b9b8a2820cfb21a4d8aa2f179167` and finished with `CI required` success. Its Linux x64 job [130694615653](https://github.com/szTheory/schwaystation/actions/runs/43069874480/job/130694615653) ran `sh scripts/fetch-pin.sh gcc-mipsel-none-elf-linux "$archive_dir"`, checked the ZIP, built with `make -C "$source_dir/openbios" -j2 BUILD=Release BOARD=psx BOOT=rom OPENBIOS_SHELL=shell`, and checked the output with `cmake -DOPENBIOS_INPUT=... -DEXPECTED_FILE=tests/built/expected.txt -P cmake/OpenBIOS.cmake`; exit 0. The log reports the expected size and SHA-256.
- Hosted be32 job [130694615418](https://github.com/szTheory/schwaystation/actions/runs/43069874480/job/130694615418) passed 433/433 tests in 6m09s. `runner.hashes_file` took 83.55s and `fixtures.pad_counter.movies` took 268.31s. The three runner-card tests passed in 0.66s, 0.06s, and 0.06s.
- Hosted WASI job 130694615476 passed; the Linux x64 suite also recorded both digest tests passing.
- `shasum -a 256 /private/tmp/schwaystation-gcc-mipsel-none-elf-12.3.0-linux.zip` exited 0 with the pinned GCC SHA-256. GitHub API returned the pinned nugget commit and its MIT root license; `openbios/kernel/alloc.c`, `openbios/main/main.c`, `openbios/shell/shell.c`, `shell/main.c`, and `common/psxlibc/fastmemset.s` at that commit carry MIT headers.

## Decisions Made

- Use the upstream-supported C `shell` target. It meets the inspected OpenBIOS shell goal with the pinned toolchain and does not require a custom standard-library overlay.
- Keep the OpenBIOS output ephemeral and use the expected digest to identify the tested build.
- Keep GAME-05 pending: the later same-run transfer, runner boot, and cross-platform shell-hash plans still need to prove the rest of that requirement.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] Fixed host and test harness issues exposed by supported platform lanes.**
- **Found during:** Task 1 and hosted verification.
- **Issue:** C17 declarations and Windows-specific file handling prevented the host from building consistently; Windows line endings changed generated input movies; WASI tests lacked a working-directory preopen and nested emulator forwarding; large test-local card buffers exceeded the WASI stack; three runner cases prepended Wasmtime twice.
- **Fix:** Added the required POSIX feature declaration and Windows sharing-mode open, wrote test inputs with Unix newlines, preopened the test directory, forwarded the existing emulator list, moved large test scratch to heap allocations, and kept the emulator prefix at the shared invocation boundary.
- **Files modified:** `host/cards.c`, `CMakePresets.json`, `tests/card/card_tests.c`, `tests/runner/tests.cmake`.
- **Verification:** Focused card/runner tests passed locally; WASI and be32 inventories passed in run 43069874480.
- **Committed in:** `cf35cfc`, `1053862`, `8170554`, `522a48f`.

**2. [Rule 3 - Blocking] Raised the be32 timeout to fit its expanded, measured suite.**
- **Found during:** Hosted verification.
- **Issue:** The old five-minute bound was measured against 208 tests. The 433-test run was cancelled at five minutes while `fixtures.pad_counter.movies` and `runner.hashes_file` were still running.
- **Fix:** Set the be32 job timeout to seven minutes, preserving all tests and assertions.
- **Files modified:** `.github/workflows/ci.yml`.
- **Verification:** The exact-head be32 job completed all 433 tests in 6m09s.
- **Committed in:** `522a48f`.

**Total deviations:** 2 blocking fixes.
**Impact on plan:** Required platform and execution-bound corrections only; no tests or inventory rows were removed.

## Issues Encountered

- An earlier hosted run encountered Ubuntu DNS failures and the next successful run exposed WASI and be32 test harness defects. The fixes above were verified on the final exact head; run 43069874480 passed the full required matrix.
- The GCC archive verification command and source commit were public and pinned; no local OpenBIOS firmware file was added to the repository or the hosted artifact.
- The now-resolved `04-22-BLOCKED.md` checkpoint was removed and its final run evidence is recorded in this summary.

## Next Phase Readiness

- The expected image digest and `OPENBIOS_SHELL=shell` identity are ready for dependent plans 04-39 and 04-40.
- GAME-05 remains pending until the later plans verify same-run transfer, runner boot to the shell, and matching frame hashes on applicable platforms.

---
*Phase: 04-games-play-in-retroarch*
*Completed: 2026-11-11*

## Self-Check: PASSED

- Summary and OpenBIOS digest implementation files exist.
- All seven plan commits are ancestors of the current head.
- The exact-head hosted run is successful and its Linux x64 log contains the expected size and digest.
