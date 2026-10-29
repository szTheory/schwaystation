---
phase: 01-a-test-frame-in-retroarch
plan: 04
subsystem: runner, testing
tags: [c17, cmake, ctest, sha256, png, xrgb8888, usage, hashes-txt]

requires:
  - phase: 01-01
    provides: "schwaystation-run --frames N --hash, schwaystation-host converter, schway_add_test, expect_fail.cmake, per-directory inventories"
provides:
  - "Pins SCHWAYSTATION_TESTCARD_SHA256_FRAME5, _NTSC_J, _PAL and SCHWAYSTATION_TESTCARD_PNG_SHA256 in tests/testcard_pins.cmake"
  - "tests/cmake/hashes.cmake: MODE=write (build/<preset>/hashes.txt, five LF lines in a fixed order) and MODE=compare (COUNT and byte checks)"
  - "runner/png.{c,h}, runner/crc32.{c,h}: stored-deflate RGB PNG writer"
  - "schwaystation-run --png N:FILE through schway_host_xrgb8888_frame"
  - "schwaystation-test-runner with cases png, png_file, xrgb8888"
  - "runner.usage.* refusals (exit 2, usage text, no file written)"
affects: [01-09, 01-12, 01-14]

actuals:
  tokens: 11464
  tasks: 3
  commits: 3
plan_head_before: 59da2c0298ec675129e5f69e406cf9b7388a36d9
plan_head_after: 86e3479d6a02bc76c0725a4fe8bdbdabcb3fd72d

tech-stack:
  added: []
  patterns:
    - "tests/runner/tests.cmake doubles as a script (cmake -DCASE=... -P) for multi-command runner checks"
    - "Usage refusals call refuse(), which include()s tests/cmake/expect_fail.cmake with CODE 2 and REGEX usage per command"
    - "A digest is pinned only after a decode of the same file proved every pixel"

key-files:
  created:
    - runner/png.c
    - runner/png.h
    - runner/crc32.c
    - runner/crc32.h
    - tests/cmake/hashes.cmake
    - tests/runner/test_png.c
  modified:
    - runner/main.c
    - runner/CMakeLists.txt
    - tests/runner/tests.cmake
    - tests/runner/inventory.txt
    - tests/testcard_pins.cmake

key-decisions:
  - "hashes.txt lines carry the region first: 'ntsc-u frame 5 sha256 <hex>', 'ntsc-u frame 60 ...', 'ntsc-j frame 60 ...', 'pal frame 60 ...', then 'ntsc-u png 5 sha256 <hex>'; the hex is field 5 of every line. The NTSC-U runs pass --region ntsc-u explicitly"
  - "--png may be given once in this release ('give --png once' is a usage error); N is parsed by the same 1..1000000 rule as --frames and must not exceed --frames"
  - "An unwritable --png path exits 1 ('cannot write'), removing a partial file; its test and exit-table wording belong to plan 01-09 (runner.exit.failure)"
  - "runner.png_file compares decoded pixels with schway_host_xrgb8888_pixel(schway__testcard_pixel(x, y, 5)); the converter's rule is pinned separately by runner.xrgb8888's known answers, and the PNG digest is also matched by an independent Python model"
  - "Task 3 needed no runner/main.c change: plan 01-01's parser already refused every listed command line; the six usage tests pin it"

requirements-completed: [FRAME-02, FRAME-07]

coverage:
  - id: D1
    description: "Frame 5, NTSC-J frame 60 and PAL frame 60 hashes equal pins from the independent model, and the three frame-60 pins differ pairwise (configure check)"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hash.frame5"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hash.ntsc_j"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hash.pal"
        status: pass
      - kind: other
        ref: "python3 -I card_model.py (scratchpad one-off, not committed) exit 0; values equal the pins"
        status: pass
    human_judgment: false
  - id: D2
    description: "runner.hashes_file writes build/<preset>/hashes.txt with five LF-only lines in the fixed order, no two sharing a hash"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hashes_file"
        status: pass
      - kind: other
        ref: "od -c build/ci/hashes.txt | grep -c '\\r' printed 0; unique field-5 count equals line count, exit 0"
        status: pass
    human_judgment: false
  - id: D3
    description: "The compare mode fails on two files one hex digit apart and on fewer files than COUNT, each with its named message"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hashes_compare.byte.self_test"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.hashes_compare.count.self_test"
        status: pass
    human_judgment: false
  - id: D4
    description: "The shared converter matches its known answers (0, 1, 15, 16, 31 to 0, 8, 123, 132, 255 per channel; 0x7FFF to 0x00FFFFFF; 0x8000 to 0), the mask-twin band converts equal to the grey band while all 310 word pairs differ, and the frame converter honours both pitches"
    requirement: FRAME-07
    verification:
      - kind: unit
        ref: "tests/runner/tests.cmake#runner.xrgb8888"
        status: pass
    human_judgment: false
  - id: D5
    description: "The PNG encoder's chunk CRCs, Adler-32 and stored blocks pass an independent parse, and its size refusals hold"
    requirement: FRAME-07
    verification:
      - kind: unit
        ref: "tests/runner/tests.cmake#runner.png"
        status: pass
    human_judgment: false
  - id: D6
    description: "--frames 5 --png 5:FILE decodes pixel for pixel to the converted card frame 5, then matches SCHWAYSTATION_TESTCARD_PNG_SHA256"
    requirement: FRAME-07
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.png_file"
        status: pass
      - kind: other
        ref: "python3 -I png_model.py (scratchpad one-off, not committed) exit 0 printed the same digest 62ddd3f2..."
        status: pass
      - kind: other
        ref: "Agent viewed build/ci/card5.png: card bands, row strip, grey and mask-twin bands identical, counter reads 101"
        status: pass
    human_judgment: false
  - id: D7
    description: "Every malformed command line exits 2 with a usage message and writes nothing"
    requirement: FRAME-02
    verification:
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.no_args"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.unknown"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.positional"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.frame_range"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.png_syntax"
        status: pass
      - kind: integration
        ref: "tests/runner/tests.cmake#runner.usage.region"
        status: pass
    human_judgment: false
  - id: D8
    description: "No planning ID and no seed name in code or tests; gate-line ceiling for hashes.cmake held"
    verification:
      - kind: other
        ref: "planning-ID git grep over runner host tests/runner tests/cmake/hashes.cmake tests/testcard_pins.cmake; test $? -eq 1 exit 0; cruft grep a=1 b=1 exit 0; hashes.cmake 55 lines <= 55 exit 0"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-10-20
status: complete
---

# Phase 1 Plan 04: The runner as frame oracle Summary

**`schwaystation-run` now prints pinned hashes for three regions and two frames, writes a five-line LF `hashes.txt` every lane can upload and compare, and writes the frame-5 reference PNG through the shared XRGB8888 converter. The converter is pinned by known answers and the PNG by a pixel-by-pixel decode before its digest. Every malformed command line exits 2.**

## Performance

- **Duration:** 6 min (wall time, 2026-10-20T03:22:45Z to 03:28Z, SUMMARY included)
- **Tasks:** 3
- **Files modified:** 11

## Accomplishments

- Pins computed independently first: `python3 -I card_model.py` (scratchpad, not committed) exit 0 printed NTSC-U frame 5 `c3409c28...`, NTSC-J frame 60 `547d307f...` and PAL frame 60 `8b8f3d0a...`. The built runner printed the same values in `runner.hash.frame5`, `runner.hash.ntsc_j` and `runner.hash.pal`.
- `hashes.cmake` writes `ntsc-u frame 5`, `ntsc-u frame 60`, `ntsc-j frame 60`, `pal frame 60` and `ntsc-u png 5` lines through `file(CONFIGURE ... NEWLINE_STYLE UNIX)` (55 lines, at its ceiling). The compare mode keeps the seed's COUNT and byte checks.
- `--png N:FILE` converts the frame with `schway_host_xrgb8888_frame`, drops the X byte and writes one stored-deflate PNG in a single `fwrite`.
- **PNG pin order:** (1) built the runner and `schwaystation-test-runner`; (2) `build/ci/schwaystation-run --frames 5 --png 5:build/ci/card5.png` exited 0; (3) `build/ci/tests/schwaystation-test-runner png_file build/ci/card5.png` exited 0, so every pixel decoded to the converted card; (4) only then did `cmake -E sha256sum` give `62ddd3f20a21a7bd48b21d0b7bb843e239236d1668af312f1259f03b33491ea1`, which was pinned. An independent `python3 -I png_model.py` (card rules, bit-replication widening, stored deflate built by hand, `zlib.crc32`/`adler32`) exited 0 with the same digest.

## Task Commits

1. **Task 1: Tracer - region and frame pins through hashes.txt and the compare mode** - `ca88b3f` (test)
2. **Task 2: PNG writer, `--png N:FILE` and the converter known answers** - `3906b82` (feat)
3. **Task 3: The runner's usage contract** - `86e3479` (test)

## Evidence (command, exit status; run from the repository root)

- `cmake --workflow --preset ci`: exit 0, 39 of 39 tests passed (final run after Task 3).
- `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake`: exit 0 (39 tests ran once each).
- `ctest --test-dir build/ci -R '^runner\.(hash|hashes)' --output-on-failure`: exit 0; `test "$(grep -c ' sha256 ' build/ci/hashes.txt)" -ge 4`: exit 0.
- `test "$(sort -u build/ci/hashes.txt | cut -d' ' -f5 | sort -u | wc -l)" -eq "$(wc -l < build/ci/hashes.txt)"`: exit 0; `od -c build/ci/hashes.txt | grep -c '\\r'`: printed 0.
- Tracer gate: the Task 1 `<verify>` re-run passed before any Task 2 work.
- `ctest --test-dir build/ci -R '^runner\.(png|png_file|xrgb8888)$' --output-on-failure`: exit 0.
- RED for Task 2: `cmake -DRUN=build/ci/schwaystation-run -DCASE=png_file -DOUT=build/ci/tests/card.png -DDECODE=none -DEXPECTED=none -P tests/runner/tests.cmake` against the pre-change runner: exit 1, the target assertion "expected exit 0" failing on "unknown option". After the change: GREEN.
- Negative control: one byte flipped at offset 1000 of a copy of the PNG made `schwaystation-test-runner png_file` report the IDAT CRC and Adler-32 mismatches (not committed).
- `build/ci/schwaystation-run --frames 5 --png 5:build/ci/card5.png`: exit 0; `file build/ci/card5.png`: "PNG image data, 320 x 240, 8-bit/color RGB".
- `grep -E -c 'float|double' host/xrgb8888.c runner/png.c`: 0 and 0.
- `build/ci/schwaystation-run --frames 0 --hash; test $? -eq 2`: exit 0. `grep -c '^runner\.usage\.' tests/runner/inventory.txt`: 6.
- Planning-ID scan: `git grep --untracked -nE -e 'D-[0-9]{2}' -e 'FRAME-[0-9]' -e 'XFER-[0-9]' -e 'DEC\.[0-9]' -- runner host tests/runner tests/cmake/hashes.cmake tests/testcard_pins.cmake; test $? -eq 1`: exit 0.
- Cruft grep (same pathspecs as plan 01-01): a=1, b=1, `test "$a" -eq 1 && test "$b" -eq 1` exit 0.
- Gate-line ceiling: `test "$(cat tests/cmake/hashes.cmake | wc -l)" -le 55`: exit 0 (55 lines). `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/* | tail -1`: 870 total.
- `sh scripts/hygiene.sh --tree`: exit 0.
- Lanes `be32`, `wasm32`, `nofp` and `asan` were not run (their tools are absent on this host; plan 01-14's hosted run exercises them). Every runner and test-program command goes through `CMAKE_CROSSCOMPILING_EMULATOR`.

## Files Created/Modified

- `runner/png.c`, `runner/png.h` - stored-deflate RGB PNG writer and Adler-32 (`schway__png_size`, `schway__png_write`, `schway__png_adler32`).
- `runner/crc32.c`, `runner/crc32.h` - `schway__runner_crc32` for chunk CRCs.
- `runner/main.c` - `--png N:FILE`, `write_png`, synopsis line.
- `runner/CMakeLists.txt` - adds `png.c` and `crc32.c`.
- `tests/cmake/hashes.cmake` - write and compare modes.
- `tests/runner/test_png.c` - `schwaystation-test-runner` (png, png_file, xrgb8888).
- `tests/runner/tests.cmake`, `tests/runner/inventory.txt` - 18 runner tests registered and listed.
- `tests/testcard_pins.cmake` - four new pins, the 64-hex loop and the pairwise-distinct check.

## Decisions Made

See `key-decisions` in the frontmatter. In short: hashes.txt lines put the region first; `--png` is given once; an unwritable path exits 1, and plan 01-09 owns its test; the converter and the PNG are pinned independently.

## Deviations from Plan

None. Plan executed as written. Two notes:
- `runner.png` and `runner.xrgb8888` also go through `CMAKE_CROSSCOMPILING_EMULATOR` (the seed ran the test program directly), so be32 and wasm32 run their own guest binary as the plan's context requires.
- The xrgb8888 case also checks `0xFFFF` and the frame converter's input and output pitches. These go beyond the plan's minimum.

## Issues Encountered

None. `host/xrgb8888.c` passed its known answers unchanged.

## Known Stubs

None.

## Requirement Status

`requirements-completed` lists this plan's share of FRAME-02 and FRAME-07. The REQUIREMENTS.md checkboxes stay open, as they did after plans 01-01 and 01-03. FRAME-02 still needs the eight-lane `hash-equality` job (plans 01-12 and 01-14). FRAME-07 still needs the install line and the hosted RetroArch compare (plans 01-12 and 01-14).

## Next Phase Readiness

- Plan 01-09 adds `runner.exit.failure` for the unwritable `--png` path (already exit 1) and the exit-table text.
- Plan 01-12's RetroArch compare reads `schwaystation-run --frames 5 --png 5:FILE`; CI uploads `build/<preset>/hashes.txt`.

## Self-Check: PASSED

- Files: runner/png.c, runner/png.h, runner/crc32.c, runner/crc32.h, tests/cmake/hashes.cmake and tests/runner/test_png.c are present.
- Commits: ca88b3f, 3906b82 and 86e3479 are ancestors of HEAD.
