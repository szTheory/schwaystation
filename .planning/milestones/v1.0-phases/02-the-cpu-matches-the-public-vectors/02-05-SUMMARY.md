---
phase: 02-the-cpu-matches-the-public-vectors
plan: 05
subsystem: testing
tags: [armips, fixtures, ps-exe, externalproject, sha256, hashes]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: per-directory test inventories, the SCHWAYSTATION_FIXTURES option and the gate budget (plans 02-01, 02-03)
provides:
  - cmake/Armips.cmake, a digest-checked armips download, a host-native ExternalProject build, and a script mode that runs the digest check alone
  - the armips row in pins.txt (full 40-character archive URL and SHA-256)
  - tests/fixtures/src/header.inc, the 2048-byte PS-EXE header every fixture emits, and hello.asm, the first fixture
  - schway__fixture(), SCHWAYSTATION_ARMIPS, SCHWAYSTATION_FIXTURES_DIR and the targets schwaystation-armips and schwaystation-fixtures
  - fixtures.hello.header and fixtures.armips_pin.self_test
  - tests/inventory/asan.txt, the asan and perf presets that never build armips
  - fixture lines in hashes.txt for the eight-lane comparison
affects: [02-06, 02-09, 02-10, 02-27]

actuals:
  tokens: 3500
  tasks: 3
  commits: 3

plan_head_before: 8ff16f0c55b13f13394e8b531cbeb15b99338c46
plan_head_after: 0879fc8e54d179737edb541e4cfbe4e14380cdae
commits: 3

tech-stack:
  added: [armips 62adab4ef30d (MIT, build tool, downloaded at configure, never committed)]
  patterns:
    - "a fixture is an armips source under tests/fixtures/src that includes header.inc first and ends with .align 0x800, the label SW_END and .close; schway__fixture(name source) assembles it into the build tree"
    - "the pinned tool is fetched by file(DOWNLOAD ... EXPECTED_HASH) and built by a host-native ExternalProject that inherits none of the parent's toolchain, deployment target, C compiler setting or warnings-as-errors"
    - "lanes that need no armips are selected out, not skipped: asan.txt omits the fixtures directory and the asan preset sets SCHWAYSTATION_FIXTURES OFF"

key-files:
  created:
    - cmake/Armips.cmake
    - tests/fixtures/src/header.inc
    - tests/fixtures/src/hello.asm
    - tests/inventory/asan.txt
  modified:
    - pins.txt
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - CMakePresets.json
    - .github/workflows/ci.yml
    - tests/cmake/hashes.cmake
    - tests/runner/tests.cmake

key-decisions:
  - "armips builds with the target armips-bin (its executable target; the target named armips is the static library)"
  - "the image path reaches armips as -strequ IMAGE, not OUTPUT, because OUTPUT inside an add_custom_command COMMAND is parsed as that command's own keyword"
  - "header.inc opens the file with .create IMAGE, 0 and switches to the load address with .headersize 0x80010000 - 0x800; a .org to 80010000h from header size 0 would pad the file by two gigabytes"
  - "PC0 and GP0 are armips labels defaulted by .ifndef and overridden with -definelabel, so a fixture can move its entry point or set gp0"
  - "the digest preamble line is printed before file(DOWNLOAD) because a hash mismatch is a fatal error raised inside file(DOWNLOAD) before STATUS can be read"

requirements-completed: [CPU-04]

coverage:
  - id: D1
    description: "cmake/Armips.cmake downloads the commit archive named by the armips row of pins.txt with EXPECTED_HASH, extracts it, and builds a host-native ExternalProject that inherits no parent toolchain, deployment target, cl setting or warnings-as-errors"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "cmake --workflow --preset ci (exit 0; configure downloads and builds armips, 83 tests pass)"
        status: pass
      - kind: other
        ref: "grep -qE '^armips 62adab4ef30d https://github.com/Kingcom/armips/archive/[0-9a-f]{40}[.]tar[.]gz [0-9a-f]{64}$' pins.txt (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "hello.exe assembles into the build tree and starts with PS-X EXE, is a whole number of 2048-byte sectors above one sector, and carries t_size equal to the size minus 2048"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures[.]hello[.]header$' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "t=$(git ls-files -- '*.exe' build) && test -z \"$t\" (exit 0; no assembled image is tracked)"
        status: pass
    human_judgment: false
  - id: D3
    description: "a wrong SHA-256 on a local file:// URL fails the digest check with exit 1 and a message naming armips and the SHA-256 check, through expect_fail.cmake, with no network"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures[.]armips_pin[.]self_test$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "the asan lane selects every ci directory but fixtures through tests/inventory/asan.txt and SCHWAYSTATION_FIXTURES OFF, and the perf preset registers no tests, so neither builds armips"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "cmake --preset asan -B <scratch> -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ (exit 0; the inventory check passes with no fixtures tests)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset perf (exit 0; no CTest file, no armips-build)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0; action pins and timeouts hold)"
        status: pass
    human_judgment: false
  - id: D5
    description: "runner.hashes_file appends one fixture line per assembled file, sorted by name, to hashes.txt when fixtures are on"
    requirement: "CPU-04"
    verification:
      - kind: integration
        ref: "grep -qE '^fixture hello[.]exe sha256 [0-9a-f]{64}$' build/ci/hashes.txt (exit 0)"
        status: pass
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^runner[.]hashes_(file|compare[.]eight)$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false

duration: 11min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 05: Pinned armips and the first fixture Summary

**A digest-checked armips built for the host at configure assembles hello.exe, a 4096-byte PS-EXE with a checked header, whose SHA-256 joins hashes.txt, while the asan, perf, hygiene and nofp lanes never build the assembler**

## Performance

- **Duration:** 11 min
- **Started:** 2026-11-09T19:02:00Z
- **Completed:** 2026-11-09T19:13:00Z
- **Tasks:** 3
- **Files modified:** 11 (4 created, 7 modified)

## Accomplishments

- armips at `62adab4ef30d` comes from one `pins.txt` row (full 40-character archive URL). `cmake/Armips.cmake` downloads it with `file(DOWNLOAD ... EXPECTED_HASH SHA256=...)`, extracts it with `file(ARCHIVE_EXTRACT)` and builds the `armips-bin` target in a host `ExternalProject` with `-DARMIPS_USE_STD_FILESYSTEM=ON -DARMIPS_LIBRARY_ONLY=OFF` and, on a macOS host, `CMAKE_OSX_DEPLOYMENT_TARGET=10.15`. The fresh ci workflow, including the download and the build, ran in about 20 s.
- `header.inc` emits the 2048-byte header; `hello.asm` is a 256-byte mailbox (`SWMB`, version 1, status 80h, count 0, fixture id 1) and `j hang` / `nop` at 80010100h. `hello.exe` is 4096 bytes with `t_size` 800h. `schway__fixture(name source ...)` is the one call later fixture plans make.
- `fixtures.hello.header` reads magic, size and `t_size` from the bytes (a 4000-byte truncation fails it). `fixtures.armips_pin.self_test` runs the digest check against a `file://` URL with 64 zeros and passes on exit 1 and `armips:.*SHA-256`; the same script exits 0 for the real digest of the file.
- `tests/inventory/asan.txt` is the ci selection without `fixtures`; the asan preset also sets `SCHWAYSTATION_FIXTURES OFF`, the perf preset sets `SCHWAYSTATION_BUILD_TESTS OFF`, and the asan job's "Every test ran" step reads `asan.txt`.
- `runner.hashes_file` appends `fixture hello.exe sha256 0fb2d38424b999a3f228cfb8ff2007195f5d4be6d4730544bd31f1992a2c4c54` after the PNG line.

## Measurements

- **armips archive digest:** `f7a65102febb4811a336a06e6d93c7e84cc86548182d54e1b1b4e2bffe562f56`, 230,419 bytes, downloaded from the full-SHA URL (`62adab4ef30da765f5cf22a451eb08a59c54dc8b`, confirmed with `gh api repos/Kingcom/armips/commits/62adab4ef30d -q .sha`). It equals PX.50's 12-character-URL digest, so GitHub serves one archive for both forms.
- **Gate-line total:** `cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l` is 1402 (limit 1520; `policy.gate_budget` passes against its pinned 1392 ceiling).
- **hello.exe SHA-256:** `0fb2d38424b999a3f228cfb8ff2007195f5d4be6d4730544bd31f1992a2c4c54` on this host (macOS arm64); the eight-lane comparison of it is plan 02-09's hosted evidence.
- **Seed commit lines:**
  - `d341d04` feat(02-05): build the pinned armips and assemble hello.exe, checked by a header test
  - `0565733` feat(02-05): negative control for the armips pin and lanes that skip armips
  - `0879fc8` test(02-05): add the assembled fixtures' digests to hashes.txt

## Task Commits

1. **Task 1: Tracer - armips at its pin assembles hello.exe, checked by a CTest in the ci workflow** - `d341d04` (feat)
2. **Task 2: The pin's negative control and the lanes that skip armips** - `0565733` (feat)
3. **Task 3: Fixture digests in hashes.txt** - `0879fc8` (test)

**Plan metadata:** the docs commit that carries this summary (docs: complete plan)

The tracer's `<verify>` (the ci workflow, then `fixtures.hello.header`) was re-run end to end after Task 1 and passed; expansion into Tasks 2 and 3 followed.

## Files Created/Modified

- `cmake/Armips.cmake` - verified download, host `ExternalProject`, script-mode digest check
- `pins.txt` - the `armips` row
- `tests/fixtures/src/header.inc`, `tests/fixtures/src/hello.asm` - header and first fixture sources
- `tests/fixtures/tests.cmake` - `header` script case, `schway__fixture`, the two tests
- `tests/fixtures/inventory.txt` - the two tests
- `tests/inventory/asan.txt` - the asan selection
- `CMakePresets.json`, `.github/workflows/ci.yml` - asan inventory and `SCHWAYSTATION_FIXTURES OFF`, perf without tests, the asan job's check
- `tests/cmake/hashes.cmake`, `tests/runner/tests.cmake` - fixture digest lines

## Decisions Made

See `key-decisions` above. The two that later plans must know: fixtures name their output as `IMAGE` (not `OUTPUT`), and `header.inc` expects the fixture to end with `.align 0x800`, the label `SW_END` and `.close`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking] The armips output name `OUTPUT` collides with an `add_custom_command` keyword**
- **Found during:** Task 1
- **Issue:** `-strequ OUTPUT <path>` inside `COMMAND` is parsed as a second `OUTPUT` section, so ninja reported the image defined as an output multiple times.
- **Fix:** the name is `IMAGE` in `header.inc` (`.create IMAGE, 0`), in `schway__fixture` and in the usage comment.
- **Files modified:** `tests/fixtures/src/header.inc`, `tests/fixtures/tests.cmake`
- **Commit:** `d341d04`

**2. [Rule 1 - Bug] `.create OUTPUT, 0` followed by `.org 80010000h` would pad the file by two gigabytes**
- **Found during:** Task 1 (read of the armips documentation before writing `header.inc`)
- **Issue:** `.create` takes a header size (memory address minus file offset); `.org` moves only the memory address, so an `.org` from header size 0 to 80010000h places the next byte at file offset 80010000h.
- **Fix:** the header is written at header size 0 and `.headersize 0x80010000 - 0x800` then maps file offset 800h to 80010000h. PC0 and GP0 are armips labels (`.ifndef` defaults, `-definelabel` overrides) rather than `equ`, because `.ifdef` sees labels.
- **Files modified:** `tests/fixtures/src/header.inc`
- **Commit:** `d341d04`

**3. [Rule 1 - Bug] A digest mismatch is fatal inside `file(DOWNLOAD)`, so the planned message cannot be printed**
- **Found during:** Task 2 (the self test failed on the first run: output named `file DOWNLOAD HASH mismatch` and no `armips:`)
- **Issue:** with `EXPECTED_HASH`, a mismatch raises a CMake error before `STATUS` is read. The planned message `armips: <url>: download or SHA-256 check failed: <status>` is reached only for other failures, such as an unreachable URL.
- **Fix:** both modes print `armips: <url>: downloading, expecting SHA-256 <hex>` before the download, so the self test's `armips:.*SHA-256` matches in the same output as the fatal error and exit 1. The planned message stays for the non-digest failures. The test would also pass for any other exit-1 failure of that script on a local file that exists; the all-zero digest on pins.txt, which does exist, leaves a mismatch as the only failure that file can have.
- **Files modified:** `cmake/Armips.cmake`
- **Commit:** `0565733`

**4. [Rule 3 - Blocking] The armips executable target is `armips-bin`**
- **Found during:** Task 1
- **Issue:** the plan names `armips` as the built target in the sense of the executable; in the tarball's CMakeLists the target `armips` is the static library and the executable target is `armips-bin` with output name `armips`.
- **Fix:** `BUILD_COMMAND cmake --build <BINARY_DIR> --target armips-bin`; `SCHWAYSTATION_ARMIPS` is `armips-build/armips${CMAKE_HOST_EXECUTABLE_SUFFIX}` under the build directory.
- **Files modified:** `cmake/Armips.cmake`
- **Commit:** `d341d04`

---

**Total deviations:** 4 auto-fixed (2 Rule 1, 2 Rule 3)
**Impact on plan:** each one is forced by how armips or CMake behaves; truths and acceptance criteria hold as written.

## Issues Encountered

- A first draft of the `header` test compared the magic against nine bytes of hex and compared `t_size` with an arithmetic expression inside `if()`; both failed at once and were fixed before the commit.
- `hygiene.tree` and `hygiene.no_bios_bytes` timed out at 60 s on one hygiene run (this host was also finishing the ci build) and passed on the next (43 s each). Both scan every commit and predate this plan; nothing here adds to them.
- The asan job was checked with the preset's cache variables on a scratch build directory using the local Apple clang instead of clang-21; the sanitized build itself runs on the hosted lane.

## Known Stubs

None.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: network-fetch | cmake/Armips.cmake | configure downloads a tarball from github.com; the pinned SHA-256 is checked by `file(DOWNLOAD)` before the archive is extracted, and a wrong digest fails configure (covered by `fixtures.armips_pin.self_test`) |

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 02-06 and later fixture plans call `schway__fixture(<name> <source.asm>)` in `tests/fixtures/tests.cmake` before the `add_custom_target` line, and their images join `hashes.txt` with no further change.
- Hosted evidence for armips on all six native legs plus `be32` and `wasm32`, the eight-file comparison of the fixture line, and the CONFORMANCE section 2 and PX.50 submodule corrections (D-49) belong to plan 02-09.

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*

## Self-Check: PASSED

Created files and the three task commits exist; the plan-level verification (ci workflow, hygiene workflow, hashes.txt line, gate total 1402 of 1520) passed.
