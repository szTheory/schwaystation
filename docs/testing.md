# Testing

Every check CI runs, a contributor can run with one command. This page covers that command, the lanes, the test
inventories, how to add a test, the negative controls, the budgets and the performance gate.

## The entry point

```sh
cmake --workflow --preset ci
```

It needs CMake 4.3 or later (the presets' minimum; the library alone configures with 3.25), Ninja and a C and
C++ compiler. The workflow configures into `build/ci` (Release, warnings as errors, macOS deployment target 10.13),
builds, and runs every test except those labelled `release`, writing `build/ci/junit.xml`. The `release` tests
check the packaged archives and run after the package step, with `ctest --preset ci-release`.

`ctest --test-dir build/ci -R <regex> --output-on-failure` reruns part of the suite after a build;
`ctest --test-dir build/ci -L <label>` selects by label.

**Reconfigure after each commit.** Some checks read git's list of tracked files, and the runner's revision is
embedded when CMake configures. A new file is seen only once git tracks it, and `build.revision_matches_head`
fails when the embedded revision is not `git rev-parse HEAD`. After committing, rerun the whole workflow preset,
which reconfigures, rather than `ctest` alone.

## Lanes

Each lane is one workflow preset, `cmake --workflow --preset <name>`, building into `build/<name>`.

| Preset | What it proves | Inventory | Tests run |
|---|---|---|---|
| `dev` | A Debug build passes, for day-to-day work. | `ci` | all but `release` |
| `ci` | The workflow above. Every Linux and macOS leg runs it. | `ci` | all but `release` |
| `ci-msvc` | The same with MSVC and the static C runtime. | `ci` | all but `release` |
| `hygiene` | The repository is clean: tree scan, BIOS and executable bytes, the integer-only text scan, titles, the publish script and the gate-line budget. | `hygiene` | `hygiene` |
| `asan` | No memory or undefined-behaviour fault, with clang-21's AddressSanitizer and UndefinedBehaviorSanitizer; any finding stops the test. | `ci` | all but `abi` and `release`, whose binary-shape checks the instrumentation changes |
| `nofp` | The core uses no floating point: GCC's `-mgeneral-regs-only` rejects any floating-point type in it. | `nofp` | `abi` |
| `be32` | The same hashes on a big-endian, 32-bit target with unsigned `char`: 32-bit PowerPC (`cmake/toolchains/ppc-linux-gnu.cmake`) under `qemu-ppc`. | `ci` | `portable` |
| `wasm32` | The same hashes in WebAssembly: clang-21 for `wasm32-wasi` (`cmake/toolchains/wasm32-wasi.cmake`) under the pinned wasmtime. WASI cannot load a libretro core, so this lane builds none. | `wasm32` | `portable` |
| `perf` | A Release build for the instructions-per-frame job. Configure and build only. | `ci` | none |

`SCHWAYSTATION_TIMEOUT_SCALE` multiplies every test timeout: 3 for `asan`, 8 for `be32` and 5 for `wasm32`.

### Lanes that need tools only CI has

`dev`, `ci` and `hygiene` run on any Linux or macOS host with the tools above. The others need:

- `nofp`: GCC itself, since configure stops for any other compiler (on macOS, `gcc` is clang);
- `asan`: `clang-21` with its sanitizer runtimes;
- `be32`: a PowerPC cross compiler and `qemu-ppc`;
- `wasm32`: clang-21, lld-21, `wasi-libc`, the wasm32 compiler runtime, and wasmtime in `build/pins/wasmtime`;
- the `perf` job: Valgrind (Cachegrind);
- the RetroArch end-to-end job: a pinned RetroArch on a macOS runner.

On Ubuntu these come from the distribution's packages. Where a host lacks them, CI's run of that lane is the
evidence; the workflow is the same command.

## The inventories

The tests a lane registers are listed in two levels of plain text:

- `tests/inventory/<preset>.txt` selects directories, one per line (`ci.txt`, `hygiene.txt`, `nofp.txt`,
  `wasm32.txt`); lines starting with `#` are comments.
- `tests/<dir>/inventory.txt` lists that directory's tests, one `name<TAB>labels` line each, labels sorted and
  comma-separated, lines sorted in byte order.

Configure fails when the registered tests, with their labels, differ in either direction from the union of the
selected directories' inventories (`cmake/SchwaystationTests.cmake`, reading them through
`tests/cmake/inventory.cmake`). The message lists each test that is listed but not registered and each that is
registered but not listed. A test cannot be dropped, renamed or registered on one platform only without its
inventory changing in the same commit, and two pull requests adding tests to different directories never touch
the same file.

| Label | Tests |
|---|---|
| `core` | The library's behaviour: the API contract, sizes, allocation, faults, state, the frame and the test card. |
| `portable` | Tests that hold on every platform, including the frame hashes; the emulated lanes run these. |
| `abi` | The public header compiled alone as C17 and C++17, the API test, and the library's symbols. |
| `runner` | `schwaystation-run` options, output, exit statuses and the PNG writer. |
| `libretro` | The libretro core in an in-process host and its `.info` file. |
| `install` | The library installed and found by an outside project. |
| `perf` | Allocation-free frames and the instructions-per-frame gate's own logic. |
| `policy` | Rules about the repository, the documents and the tests themselves. |
| `release` | The packaged archives, run after the package step. |
| `hygiene` | The hygiene lane's checks. |

### The JUnit check

A zero exit from CTest does not prove that each test ran. After each test step CI runs, for example:

```sh
cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release \
  -P tests/cmake/check_junit.cmake
```

It fails unless every selected inventory test whose labels match `LABELS` and not `EXCLUDE` appears exactly once,
ran, and did not fail, error or skip. `policy.no_skip` fails when any test file sets a skip return code, a skip
regular expression or disables a test, so no test is ever conditional.

## Adding a test

1. Register it in its directory's `tests/<dir>/tests.cmake`:

   ```cmake
   schway_add_test(NAME runner.example
     COMMAND schwaystation-run --frames 1
     LABELS runner TIMEOUT 10)
   ```

   `NAME`, `COMMAND`, `LABELS` and `TIMEOUT` are required; `PASS_REGEX` makes the test pass only when its output
   matches, and `COST` orders long tests first. A comment above it names the fault the test prevents.
2. Add `runner.example<TAB>runner` to `tests/<dir>/inventory.txt` at its sorted place.
3. Run `cmake --workflow --preset ci`. If configure fails, its message names the difference between what was
   registered and what the inventories list.

A new directory also needs a line in each `tests/inventory/<preset>.txt` that should run it, and an `include()` in
`tests/tests.cmake`. Every lane registers the same tests for its inventory; a check that only one platform can
perform runs there as a CI step instead.

## Negative controls

A check that cannot fail proves nothing, so each repository check has a self test that feeds it a known fault.
`tests/cmake/expect_fail.cmake` runs a command and passes only when it exits with exactly the expected code and its
output matches the expected regular expression; a crash or a missing file does not count as the expected failure.
A test never uses CTest's bare `WILL_FAIL`, which passes on any failure at all.

```cmake
schway_add_test(NAME policy.no_skip.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=CMakeLists.txt:1: ..."
    -P tests/cmake/expect_fail.cmake
  LABELS policy TIMEOUT 10)
```

`CMD` is a CMake list; replace its `;` with `$<SEMICOLON>` before passing it. Fixtures that would trip the
repository's own scans (a home directory path, an executable signature, a skip property) are built in pieces or
written into the build tree when CMake configures, so no tracked file holds them.

## Budgets

**Documents.** `docs.budgets` keeps `README.md` under 16,384 bytes, `include/schwaystation.h` under 40,960 bytes and
each `docs/*.md` page under 61,440 bytes, each strictly below its limit, and names every file at or over it with
its size. Run it by hand with `cmake -DROOT=. -P tests/cmake/budgets.cmake`.

**Gate code.** Repository-check code is CMake `-P` scripts or POSIX sh. `policy.gate_budget` (hygiene lane) counts
every line of `scripts/*.sh`, `tests/cmake/*.cmake` and `.githooks/*` against a fixed limit set in
`tests/hygiene/tests.cmake`; the limit is an absolute line count, lowered when a release ships. Check the total with
`wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1`. Gate logic stays in those files: it is not moved
into `tests/<dir>/tests.cmake`, `cmake/` or workflow YAML to fit.

**End-to-end code.** Everything under `tests/retroarch` stays within 300 lines in total; `policy.e2e_budget`
counts them with the same script, `tests/cmake/line_budget.cmake`.

## Performance

**Instructions per frame.** CI's `perf` job counts the instructions `schwaystation-run` executes per frame of the
test card under Cachegrind: Ir per frame = (Ir at 360 frames − Ir at 60) / 300, in integers. It measures the pull
request's head once and its base twice, then `tests/cmake/perf_compare.cmake` prints one line:

```text
perf testcard head=<n> base=<n> delta_bp=<n> verdict=PASS
```

`delta_bp` is the change in basis points. `PASS` means at most 1.00 % slower (exactly +1.00 % passes); `FAIL` more
than that or more than the allow file permits; `UNSTABLE` the two base measurements differ; `NO_BASE` the base has
no `perf` preset. An empty or non-numeric measurement is an input error, never a pass.

**Report mode.** `SCHWAYSTATION_PERF_MODE` is `report` or `gate`. In `report` the script prints the verdict and
exits 0 whatever it is; in `gate`, `FAIL`, `UNSTABLE` and `NO_BASE` exit 1. The job runs in report mode until a
separate `ci:` pull request arms it.

**Exceptions.** A pull request that must be slower adds a line to `tests/perf/allow.txt`:
`<fixture> <max % × 100> <expiry YYYY-MM-DD> <reason>`. It raises that fixture's limit through its expiry, at most
60 days away, and is removed after the pull request merges.

The `perf.gate.*` tests feed the comparison its edge cases on synthetic measurements, so they run on every lane
without Valgrind: identical runs, exactly +1.00 %, +1.01 %, an allowed and an expired exception, unstable bases, no
base, a missing measurement and an unset mode.

## CI jobs

<!-- The CI workflow's jobs, the roll-up check and how to reproduce a red job are described here when the workflow lands. -->
