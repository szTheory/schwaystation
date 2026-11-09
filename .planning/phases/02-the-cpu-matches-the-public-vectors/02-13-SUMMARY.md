---
phase: 02-the-cpu-matches-the-public-vectors
plan: 13
subsystem: runner
tags: [runner, probe, ps-exe, md5, sha256, rcheevos]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: schway_probe_exe, the one PS-EXE validator and schway_exe_info (plan 02-08)
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: hello.exe assembled by the pinned armips on every lane (plan 02-09)
provides:
  - schwaystation-run --probe FILE, a 19-key key=value report of a PS-EXE with no instance and no frame
  - runner/md5.c and runner/md5.h, RFC 1321 MD5 seeded from a same-author permissive source
  - runner/probe.c and runner/probe.h, schway__probe_print
  - fixtures.probe.hello, exe.probe_refusals, exe.probe_usage and exe.md5_rfc1321
  - the Probe section of docs/runner.md and the widened exit 1 text
affects: [02-14, 02-17, 02-25]

actuals:
  tokens: 8900
  tasks: 3
  commits: 3

plan_head_before: f94bf4ce8a6ea321450a8dbb1921e6a5768fe41e
plan_head_after: de0deddf04d116a6d7828ec7c36c173cc7d16f77
commits: 3

tech-stack:
  added: []
  patterns:
    - "the runner reports on a file through the core's public validator and never creates an instance"
    - "a script-mode check lives in the same tests.cmake that registers it, takes the runner and the emulator as -D values, and reads standard output and standard error separately"
    - "a hostile file name is shown with every byte outside printable ASCII as ?, so it cannot add a report line"

key-files:
  created:
    - runner/probe.c
    - runner/probe.h
    - runner/md5.c
    - runner/md5.h
  modified:
    - runner/main.c
    - runner/CMakeLists.txt
    - runner/exit_status.h
    - tests/exe/exe_tests.c
    - tests/exe/tests.cmake
    - tests/exe/inventory.txt
    - tests/fixtures/tests.cmake
    - tests/fixtures/inventory.txt
    - docs/runner.md

key-decisions:
  - "rcheevos_md5 is MD5 over the first hash_len (t_size + 0800h) bytes of the file. The rule is rc_hash_psx in rcheevos src/hash/rc_hash_disc.c at dfdc64ea035c: line 965 sets size to the header's t_size plus 2048, and lines 971 to 974 hash the file's first size bytes. That function also prepends the boot-file name taken from the disc's SYSTEM.CNF; a bare file has no boot-file name, so nothing is prepended, which makes the value equal an independent MD5 of the image for an image whose text is its whole body"
  - "A file the validator refuses, an empty file, an unreadable file and a missing file all exit 1 with one line on standard error naming the file; nothing reaches standard output. A file longer than 0800h + 2 MiB is read one byte past the limit so the validator sees it as too long instead of a valid cut"
  - "--probe is valid only as the whole command line: exactly one file argument that does not begin with a dash, so --probe --hash and --probe FILE --hash are both usage errors and an option can never be taken for a file name"
  - "lint is printed as eight hex digits like the header values, since it is a bit set"

requirements-completed: [CPU-05]

coverage:
  - id: D1
    description: "schwaystation-run --probe prints the 19 keys for hello.exe in order, with the size and the SHA-256 of the file, the licence area north_america, the region guess ntsc-u and the entry point 80010100h, and exits 0"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^fixtures\\.probe\\.hello$' --no-tests=error (exit 0)"
        status: pass
      - kind: integration
        ref: "test \"$(build/ci/schwaystation-run --probe build/ci/fixtures/hello.exe | grep -cE '^[a-z0-9_]+=')\" -ge 19 (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: "rcheevos_md5 for hello.exe equals CMake's file(MD5) of the image, and the MD5 code passes the RFC 1321 appendix A.5 messages whole and a byte at a time"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "tests/fixtures/tests.cmake case probe (fixtures.probe.hello compares rcheevos_md5 with file(MD5), exit 0)"
        status: pass
      - kind: unit
        ref: "tests/exe/exe_tests.c#case_md5_rfc1321 (exe.md5_rfc1321, exit 0)"
        status: pass
    human_judgment: false
  - id: D3
    description: "A bad signature, a header cut short, a rejected entry point, an empty file, a file past the size limit and a missing file each exit 1 with schwaystation-run, the file and a reason on standard error and nothing on standard output; the valid file exits 0"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^exe\\.probe_refusals$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: "--probe with --frames, --hash, --png, --bench or --region, with no file, with two files or with another option in the file's place exits 2 with the usage text, writes no file and never opens the named file"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^exe\\.probe_usage$' --no-tests=error (exit 0)"
        status: pass
    human_judgment: false
  - id: D5
    description: "The probe creates no instance and runs no frame: runner/probe.c does not name the creation function and reaches the core only through schway_probe_exe"
    requirement: "CPU-05"
    verification:
      - kind: other
        ref: "grep -q 'schway_create' runner/probe.c; test $? -eq 1 (exit 0)"
        status: pass
    human_judgment: false
  - id: D6
    description: "docs/runner.md lists --probe in the synopsis, options and examples, has a Probe section with every key, the MD5 rule and the exit statuses, and the exit 1 text matches what --help prints"
    requirement: "CPU-05"
    verification:
      - kind: integration
        ref: "ctest --test-dir build/ci -R '^docs\\.(budgets|api_names)$' --no-tests=error (exit 0)"
        status: pass
      - kind: other
        ref: "grep -q 'rcheevos_md5' docs/runner.md (exit 0)"
        status: pass
    human_judgment: false
---

# Phase 2 Plan 13: The runner's --probe Summary

**`schwaystation-run --probe FILE` reports a PS-EXE's identity (licence area, region guess, name, size, SHA-256, the rcheevos MD5 and the ten header fields) from the core's own validator, with no instance and no frame.**

## Performance

- **Duration:** about 13 min
- **Completed:** 2026-11-09
- **Tasks:** 3 (one tracer, two auto)
- **Files:** 13 (4 created, 9 modified)

## Accomplishments

- The tracer: `--probe` on the assembled `hello.exe` prints the 19 keys in the specified order. `fixtures.probe.hello` compares the size and the MD5 with CMake's own `file(SIZE)` and `file(MD5)`, so the numbers are checked against a second implementation. For `hello.exe` the size is 4096, the SHA-256 is `0fb2d384...4c54` and the rcheevos MD5 is `8b8ee904ef19b9d3ac5ad3905ce09280`, matching macOS `md5`.
- `runner/md5.c` is the owned RFC 1321 code from the same author's earlier runner, renamed to `schway__md5_*`, with a hex helper added. `exe.md5_rfc1321` runs the seven appendix A.5 messages whole and a byte at a time.
- Refusals and usage errors are tested. `schwaystation-test-exe write DIR` writes a good file and five bad ones (signature, truncated header, entry point 0, empty, one byte over the limit); the script adds a missing file. Every refusal exits 1 with one line on standard error naming the file and a reason, and nothing on standard output. `--probe` with any other option, no file or two files exits 2 before the file is opened.
- `docs/runner.md` has `--probe` in the synopsis, options and examples and a Probe section: each key, the MD5 rule and the exit statuses. The page still says the release loads no BIOS and no disc.

## The rcheevos rule

Read at `dfdc64ea035c` (RetroAchievements/rcheevos, MIT): `src/hash/rc_hash_disc.c`, function `rc_hash_psx`. Line 965 sets `size` to the header's 32-bit value at offset 1Ch plus 2048, and lines 971 to 974 start the MD5, append the boot-file name and hash the first `size` bytes of the executable. A bare file has no boot-file name, so the probe hashes the same first `t_size + 0800h` bytes with no prefix. The disc-only prefix is documented on the runner page.

## Task Commits

1. **Task 1: tracer, `--probe` on hello.exe** - `5e05851` (feat)
2. **Task 2: refusals, usage errors, MD5 known answers** - `67472fb` (test)
3. **Task 3: the runner page's probe section** - `de0dedd` (docs)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] The exit table's exit 1 text**
- **Found during:** Task 1
- **Issue:** `runner/exit_status.h`, which `--help` prints and `docs/runner.md` repeats, described exit 1 as "a file could not be written, memory ran out or the core failed"; a file that is not a valid PS-EXE now also exits 1, so the table would have been wrong.
- **Fix:** Widened the text in the header and the page together; the plan's file list did not name `exit_status.h`.
- **Files modified:** `runner/exit_status.h`, `docs/runner.md`
- **Commit:** `5e05851`, `de0dedd`

**2. [Rule 3 - Blocking] A hex helper and a sixth bad file**
- **Found during:** Tasks 1 and 2
- **Issue:** The seed has no hex writer (the SHA-256 module has one), and the plan's five bad files did not include a header the validator refuses on its fields.
- **Fix:** Added `schway__md5_hex` beside `schway__sha256_hex`, and a `no_entry.exe` (entry point 0) to the written files.
- **Files modified:** `runner/md5.c`, `runner/md5.h`, `tests/exe/exe_tests.c`
- **Commit:** `5e05851`, `67472fb`

**Total deviations:** 2 auto-fixed (1 Rule 2, 1 Rule 3). **Impact:** none on scope; both keep the table and the tests true to the behaviour.

## Issues Encountered

None. The first full `cmake --workflow --preset ci` after each task passed, and the hygiene preset passed (21 tests) after the last commit.

## Threat Flags

| Flag | File | Description |
|------|------|-------------|
| threat_flag: file_read | runner/probe.c | The runner now opens and reads a path named on the command line. The read is bounded to 0800h + 2 MiB + 1 bytes, the file name is shown with non-printable bytes replaced, and the core's validator judges the bytes before any field is printed. |

## Known Stubs

None.

## Next Phase Readiness

Plan 02-14 and later plans can rely on `--probe` and on `schway__probe_print`; `--expect` and the exit codes 3 and above are still to come. The exit table gains rows only when the runner first returns them.

## Self-Check: PASSED

- Created files present: `runner/probe.c`, `runner/probe.h`, `runner/md5.c`, `runner/md5.h`.
- Commits reachable from HEAD: `5e05851`, `67472fb`, `de0dedd`.
- Plan verification: `cmake --workflow --preset ci` (exit 0); `ctest --test-dir build/ci -R '^fixtures\.probe\.hello$'` (exit 0); `ctest --test-dir build/ci -L exe` (exit 0, 12 tests); `ctest --test-dir build/ci -L runner` (exit 0); `ctest --test-dir build/ci -R '^docs\.(budgets|api_names)$'` (exit 0); `cmake --workflow --preset hygiene` (exit 0).
