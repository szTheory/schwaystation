---
phase: 02-the-cpu-matches-the-public-vectors
plan: 04
subsystem: testing
tags: [vectors, singlesteptests, fixtures, manifest, third-party-notice, python]

requires:
  - phase: 02-the-cpu-matches-the-public-vectors
    provides: per-directory test inventories and the gate budget (plans 02-01, 02-03)
provides:
  - tests/vectors/r3000/sst-sample.bin, the first 100 verbatim records of each of the 55 upstream instruction files in one container (5,500 cases, 2,395,390 bytes)
  - tools/sst_slice.py, the standard-library maintainer slicer that CI never runs
  - tests/vectors/r3000/allow.txt holding only its format comment
  - the sample's row in tests/fixtures/manifest.txt, its .gitignore exemption and its MIT notice
affects: [02-05, 02-06, 02-07, 02-27]

actuals:
  tokens: 1100
  tasks: 2
  commits: 2

plan_head_before: 692971a61990c34f5f10e595250041720543ddc5
plan_head_after: 0c8802649eeb5b73b41f2b235af72eec58e81a7e
commits: 2

tech-stack:
  added: []
  patterns:
    - "the committed vector sample is a container of verbatim upstream slices, so one C parser serves the sample and the nightly full files and cmp against upstream proves no transcoding bug"
    - "container layout, little-endian: u16 slice count, then per slice a 16-byte NUL-padded name (the file stem), a u32 byte length and the slice (u32 case count patched to 100, then 100 records)"

key-files:
  created:
    - tools/sst_slice.py
    - tests/vectors/r3000/sst-sample.bin
    - tests/vectors/r3000/allow.txt
    - tests/fixtures/manifest.txt
  modified:
    - .gitignore
    - THIRD-PARTY-NOTICES.md

key-decisions:
  - "The container carries the case count inside each slice (patched to 100) and a u32 slice length before it, so the harness parser reads a slice exactly as it reads a full upstream file"
  - "A record is 403 bytes up to its cycle count, then u32 cycles and 24 bytes per cycle; the upstream files carry no opcode block after the cycles (all 55 files parse to exactly 1,000 records with at most 3 cycles)"

requirements-completed: [CPU-01]

coverage:
  - id: D1
    description: "sst-sample.bin holds a u16 slice count of 55, then per slice a 16-byte name, a u32 length and the upstream bytes of the file's first 100 records with the count patched to 100, in byte-sorted name order, under 4 MB"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "test $(wc -c < tests/vectors/r3000/sst-sample.bin) -lt 4194304 && sh scripts/hygiene.sh --tree (exit 0)"
        status: pass
      - kind: other
        ref: "python3 -I tools/sst_slice.py DIR tests/vectors/r3000/sst-sample.bin (exit 0; 55 slices, 2395390 bytes)"
        status: pass
    human_judgment: false
  - id: D2
    description: "each slice after its first four bytes equals the same prefix of its upstream blob byte for byte, and the 55 upstream blobs match their tree SHA-1 and size at pin 51e227447d0b"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "scratch script extracting each slice and its upstream prefix with tail and head then cmp -n (exit 0; prefix ok 55 slices)"
        status: pass
      - kind: other
        ref: "git hash-object of each blob fetched with gh api against the tree listing (55 of 55 equal)"
        status: pass
    human_judgment: false
  - id: D3
    description: "the sample is exempted in .gitignore, listed in tests/fixtures/manifest.txt with licence MIT and its sha256, noticed in THIRD-PARTY-NOTICES.md, and allow.txt holds only its format comment"
    requirement: "CPU-01"
    verification:
      - kind: other
        ref: "grep -q tests/vectors/r3000/sst-sample.bin tests/fixtures/manifest.txt (exit 0) and git check-ignore -q on the sample (exit 1)"
        status: pass
      - kind: integration
        ref: "cmake --workflow --preset hygiene (exit 0, 21 of 21 tests passed)"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-11-09
status: complete
---

# Phase 2 Plan 04: Public vector sample Summary

**The first 100 verbatim records of each of the 55 SingleStepTests/r3000 files at pin 51e227447d0b, committed as one 2,395,390-byte container with a 40-line standard-library slicer, a manifest row and an MIT notice.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-11-09T18:57:00Z
- **Completed:** 2026-11-09T19:02:00Z
- **Tasks:** 2
- **Files modified:** 6 (4 created, 2 edited)

## Accomplishments

- `tests/vectors/r3000/sst-sample.bin`: 55 slices in byte-sorted name order (ADD through XORI), 5,500 cases, 2,395,390 bytes (the 2,394,288 bytes of records measured in planning, plus the 2-byte count and 55 name-and-length headers of 20 bytes each). SHA-256 `d6ef9a9ce978c487d2065c60c7312959a8bb6ddbd67b19c8465d1e28557d5ffc`.
- `tools/sst_slice.py`: walks 100 records per file using the transcoder's layout to find the prefix length, patches the count to 100 and writes the container; run as `python3 -I tools/sst_slice.py DIR tests/vectors/r3000/sst-sample.bin`.
- The tree scan admits the binary through its manifest row; `.gitattributes` already marked it `binary` (plan 02-03).
- The upstream MIT notice, with the commit, the file's SHA-256 and the one change (count 1,000 to 100), joins `THIRD-PARTY-NOTICES.md`.

## Task Commits

1. **Task 1: Tracer - fetch, slice and commit the sample** - `a29a064` (test)
2. **Task 2: The notice** - `0c88026` (docs)

**Plan metadata:** the commit that adds this summary, STATE.md and ROADMAP.md follows (docs).

The tracer's feedback gate (automation-only `<verify>`, default `end-of-phase` mode) re-ran green before the notice task: the size test and `sh scripts/hygiene.sh --tree` exited 0.

## Files Created/Modified

- `tools/sst_slice.py` - maintainer slicer, standard library only, never run by CI
- `tests/vectors/r3000/sst-sample.bin` - the committed 5,500-case sample
- `tests/vectors/r3000/allow.txt` - format comment only, `YYYY-MM-DD <file> <case-hex> <upstream-issue-URL>`
- `tests/fixtures/manifest.txt` - header comment and the sample's row (path, source, pin `51e227447d0b`, MIT, sha256, notice)
- `.gitignore` - `!/tests/vectors/r3000/sst-sample.bin` after the `*.bin` rule
- `THIRD-PARTY-NOTICES.md` - SingleStepTests/r3000 section

## Evidence

- Fetch: `gh api repos/SingleStepTests/r3000/git/trees/51e227447d0b?recursive=1` listed 55 `v1/*.json.bin` blobs; each `gh api repos/SingleStepTests/r3000/git/blobs/<sha> -H 'Accept: application/vnd.github.raw'` result matched its tree SHA-1 and size under `git hash-object` (55 of 55). Blobs stayed in the scratch area.
- Layout check (scratch, `python3 -I`): every upstream file parses as 1,000 records ending exactly at end of file, at most 3 cycles per record.
- Slicer: `python3 -I tools/sst_slice.py <scratch blobs> tests/vectors/r3000/sst-sample.bin` exit 0, "55 slices, 2395390 bytes".
- Prefix check: a scratch shell script extracted each slice after its first four bytes and the same-length upstream prefix with `tail -c +N | head -c LEN`, then `cmp -n LEN a b`; exit 0, "prefix ok: 55 slices", and the script's final offset equalled the file size. BSD `cmp -i skip1:skip2 -n` mis-reported EOF on macOS, so the plan's "`cmp -n` on extracted slices" was run with extracted files.
- Verify: `test "$(wc -c < tests/vectors/r3000/sst-sample.bin)" -lt 4194304 && sh scripts/hygiene.sh --tree` exit 0; `cmake --workflow --preset hygiene` exit 0 (21 of 21 tests).

## Decisions Made

- The slice keeps the upstream leading u32 (patched to 100) so one parser reads a slice and a full file alike; the container's u32 length is the slice's total bytes including that count.
- The record layout has no trailing opcode block (the planning extract PX.41 lists one; `transcode_json.py` never reads it and no file carries bytes for it).

## Deviations from Plan

None - plan executed exactly as written. (`.gitattributes` needed no edit: plan 02-03 had already added the `binary` line for the sample.)

## Issues Encountered

- The first fetch loop used `path` as a shell variable name, which zsh ties to `PATH`; renamed and re-run. Nothing was written outside the scratch area.

## Known Stubs

None.

## Threat Flags

None - no network endpoint, auth path or trust-boundary file access added; the slicer reads local files only.

## Next Phase Readiness

- Plan 02-05 onward can parse `sst-sample.bin` with the layout above; the harness parser, the zero-case guard (expected count 5,500) and the `pins.txt` blob rows belong to later plans.
- `allow.txt` is empty by design; an unmatched line is a harness error (D-21).

## Self-Check: PASSED

- FOUND: tools/sst_slice.py, tests/vectors/r3000/sst-sample.bin, tests/vectors/r3000/allow.txt, tests/fixtures/manifest.txt
- FOUND commits: a29a064, 0c88026 (ancestors of HEAD)

---
*Phase: 02-the-cpu-matches-the-public-vectors*
*Completed: 2026-11-09*
