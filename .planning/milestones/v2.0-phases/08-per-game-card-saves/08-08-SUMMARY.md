---
phase: 08-per-game-card-saves
plan: "08"
subsystem: runner-saves
tags: [disc-identity, playlists, memory-cards, runner]
requires:
  - phase: 08-03
    provides:
      - Executable save identity resolution and two-slot runner card paths
provides:
  - First-member serial selection for discs and M3U playlists
  - Versioned full-sector content identity when the first serial is absent or invalid
  - Frozen two-slot card paths across runner disc swaps
affects: [phase-08-verification, phase-08-ship, runner-saves]
actuals:
  tokens: 10130
  tasks: 2
  commits: 1
tech-stack:
  added: []
  patterns:
    - Hash canonical provider TOC and sectors without path or container bytes
    - Resolve card identity once before loading either card slot
key-files:
  created: []
  modified:
    - cmake/DiscFixtures.cmake
    - docs/runner.md
    - host/cards.c
    - host/save_game_identity.c
    - host/save_game_identity.h
    - runner/main.c
    - tests/card/files.c
    - tests/fixtures/disc_boot_playlist.xml
    - tests/fixtures/manifest.txt
    - tests/runner/tests.cmake
key-decisions:
  - Use the first declared member's normalized boot serial when valid; later members never replace it.
  - Use an sgi2 digest with distinct standalone and M3U media types when the first member lacks a valid serial.
  - Hash ordered TOC metadata and every provider sector, including audio and returned subcode.
patterns-established:
  - Disc identities use canonical provider output and ignore paths, M3U whitespace, and container padding.
  - Identity read errors return before either card path is applied.
requirements-completed: [SAVE-01, SAVE-03]
coverage:
  - id: SAVE-01-disc-identity
    description: First-member serial precedence and full-content fallback are stable across moved media, playlist formatting, membership, and late sector changes.
    requirement: SAVE-01
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci --no-tests=error -R '^runner[.]disc[.]load$' --output-on-failure (exit 0, 1/1)"
      status: pass
    human_judgment: false
  - id: SAVE-03-frozen-card-paths
    description: Both slots share one identity across disc swaps, and read failure creates no card files.
    requirement: SAVE-03
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci --no-tests=error -R '^(runner[.]disc[.](load|probe)|runner[.]inputs[.]disc_controls|runner[.]cards[.]controls)$' --output-on-failure (exit 0, 4/4)"
      status: pass
    human_judgment: false
  - id: SAVE-01-card-digest-path
    description: The shared card-path validator accepts the versioned sgi2 digest used for disc fallback names.
    requirement: SAVE-01
    verification:
      kind: unit
      ref: "ctest --test-dir build/ci --no-tests=error -R '^(card[.]files[.](atomic_roundtrip|atomic_failures|wrappers|wrapper_edges))$' --output-on-failure (exit 0, 4/4)"
      status: pass
    human_judgment: false
duration: "at least 27min; active time not measured"
completed: 2026-11-19
status: complete
---

# Phase 08 Plan 08 Summary: Runner disc save identities

**Runner serial naming now uses the first disc member's valid serial or a versioned digest of the complete declared media set.**

## Performance

- **Observed wall interval:** At least 27 minutes from creation of the Plan 08-08 base ledger at 2026-11-19T17:47:12Z through implementation verification and summary preparation. This is a lower bound; active time was not measured.
- **Tasks:** 2.
- **Files modified:** 10.
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; primary GPT-6 session, exact deployed model ID unavailable; single-writer execution.

## Accomplishments

- Disc serial mode probes only the first declared member. Its normalized serial wins; if absent or invalid, the resolver hashes the whole declared media set, even when a later member has a serial.
- The `sgi2-` digest frames standalone images separately from M3U playlists, includes member and track order plus fixed-endian TOC fields, then streams every provider sector from index 0 through leadout. Paths and playlist formatting do not enter the digest.
- Both card slots use the identity resolved before card loading, and an identity-sector read error stops before card paths or files are created.
- Updated the runner guide and added owned fixtures and integration checks for serial precedence, full-sector fallback, late audio changes, moved/renamed media, M3U whitespace and membership, two-slot naming, and swap stability.

## Acceptance Evidence

| Check | Result | Evidence |
| --- | --- | --- |
| Input, manifest, and diff preflight | Exit 0; fixture hash matched its manifest entry; targeted CTest selectors were nonempty | `git diff --check`; shasum -a 256 tests/fixtures/disc_boot_playlist.xml; `ctest --test-dir build/ci -N -R '^(runner[.]disc[.](load|probe)|runner[.]inputs[.]disc_controls|runner[.]cards[.]controls)$'` |
| Configure and focused build | Exit 0; runner, test runner, card-file test, and disc fixtures built | `cmake --preset ci`; `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-test-card-files schwaystation-disc-fixtures` |
| Runner integration | Exit 0; 4/4 passed in 1.80 seconds | `ctest --test-dir build/ci --no-tests=error -R '^(runner[.]disc[.](load|probe)|runner[.]inputs[.]disc_controls|runner[.]cards[.]controls)$' --output-on-failure` |
| Card paths and CLI documentation | Exit 0; 5/5 passed | `ctest --test-dir build/ci --no-tests=error -R '^(runner[.]exit_table|card[.]files[.](atomic_roundtrip|atomic_failures|wrappers|wrapper_edges))$' --output-on-failure` |
| Combined focused regressions | Exit 0; 9/9 passed in 1.89 seconds | `ctest --test-dir build/ci --no-tests=error -R '^(runner[.]disc[.](load|probe)|runner[.]inputs[.]disc_controls|runner[.]cards[.]controls|runner[.]exit_table|card[.]files[.](atomic_roundtrip|atomic_failures|wrappers|wrapper_edges))$' --output-on-failure` |
| Media startup cost | Exit 0; five-run medians measured +1.97 ms for 522,144 bytes and +3.29 ms for 1,220,688 bytes | One-off Python 3 `perf_counter` comparison; command below. |

```sh
python3 -c 'import pathlib,subprocess,timeit,statistics;root=pathlib.Path.cwd();run=str(root/"build/ci/schwaystation-run");bios=str(root/"build/ci/openbios/openbios-fast.bin");fixtures=root/"build/ci/fixtures";save="/private/tmp/schwaystation-identity-measure";opts=["--save-dir",save,"--memcard-names","serial"];cases=[("single",str(fixtures/"disc_boot.cue"),["disc_boot.bin","disc_boot_track02.bin"]),("playlist",str(fixtures/"disc_boot_runner.m3u"),["disc_boot.bin","disc_boot_track02.bin","disc_boot_playlist.bin","disc_boot_playlist_track02.bin"])];measure=lambda c:statistics.median(timeit.repeat(lambda:subprocess.run(c,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE,check=True),number=1,repeat=5))*1000;[print(name,sum((fixtures/file).stat().st_size for file in files),measure(base+opts)-measure(base)) for name,image,files in cases for base in [[run,"--bios",bios,"--disc",image,"--frames","1"]]]'
# Exit status: 0
```

No hosted CI run was started for this local implementation plan; no run URL applies. Plan 08-10 remains responsible for the real RetroArch save-continuation proof.

## Task Commit

The two outcomes share the identity resolver and integration fixture, so implementation and tests landed together in `2b0d935` (`feat(08-08): resolve full disc save identities`). The follow-up plan-metadata commit records this summary and state changes.

## Decisions Made

- Keep PS-EXE fallback names on the existing `sgi1-` scheme; disc fallback uses `sgi2-` so old names stay stable and media encodings do not overlap.
- Treat returned zero subcode as part of canonical provider output when a provider has no subcode.
- Give the build-time playlist fixture a valid `SLUS-12346` serial to exercise first-member precedence and later-member suppression without adding a binary fixture.

## Deviations from Plan

None. The build-time playlist fixture was adjusted to cover the plan's serial and fallback branches.

## Issues Encountered

- The initial RED run failed at the old CLI validation, as expected. The fixture builder rejected a literal `;1` XML filename, so the XML now uses its version-free name form and SYSTEM.CNF retains the ISO boot suffix.
- Focused checks caught that the new serial normalizer initially omitted its separator and that a test movie selected index 2 in a two-entry playlist; both were corrected before the final passing run.
- The new digest namespace was initially rejected by the card-path helper. It now accepts complete `sgi2-` digests while retaining `sgi1-` validation.

## Threat Flags

None. The new code reads through already-open media providers and introduces no external or privileged boundary.

## User Setup Required

None.

## Self-Check: PASSED

The summary's listed implementation files exist, commit `2b0d935` is in the current history, and the summary validator passes.

## Next Phase Readiness

Plan 08-09 can run the integrated state/save checks and review the Phase 8 source before phase verification. Plan 08-10 remains responsible for exact-head hosted RetroArch evidence and the save-capability claim.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*
