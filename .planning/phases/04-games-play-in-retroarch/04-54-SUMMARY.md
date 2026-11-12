---
phase: 04-games-play-in-retroarch
plan: "54"
subsystem: testing
tags: [cmake, portability, inventory, emulator]
requires:
  - phase: 04-games-play-in-retroarch
    provides: Existing CI inventory, compatibility harness and metadata/probe callers
provides:
  - Strict-compiler-safe controller movie and portable read-only probe opening
  - Compatibility registration based on the selected inventory
  - Full emulator argument forwarding for compatibility helpers and library runners
affects: [04-55, 04-29, phase-04-hosted-verification]
actuals:
  tokens: 3856
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 2618185b24c770393b77afb13ab9d83bbf3136c2
plan_head_after: 476691714166dc42eda50d1877db4f74a9e7b6bc
post_closeout_followup:
  parent_head: 4eeaab0f9bb87247076b31eda46cc1065a587d7e
  commit: cdf99341a9f0c0d3cc9038fc5bff2a33c4e9e00b
  diff_bytes: 7450
tech-stack:
  added: []
  patterns:
    - Private fopen_s/fopen read helper
    - Selected-inventory test registration
    - Quoted build-tree POSIX emulator launchers
key-files:
  created: []
  modified:
    - tests/libretro/host.c
    - runner/probe.c
    - tests/tests.cmake
    - tests/compat/tests.cmake
    - scripts/library.sh
    - docs/testing.md
    - AGENTS.md
key-decisions:
  - Local prerequisite success does not close cross-platform requirement acceptance; the combined hosted run after 04-55 remains required.
  - The local OpenBIOS test input was reused only after the repository validator accepted its size and pinned digest.
patterns-established:
  - Emulator arguments are shell-quoted individually and runner arguments pass through with "$@".
  - Compatibility cases follow the selected inventory without changing CI, ASan, WASI or BE32 inventory definitions.
requirements-completed: []
coverage:
  - id: GAME-03
    description: Controller-movie and metadata-probe consumers are repaired for strict compilers; host-matrix acceptance remains pending.
    requirement: GAME-03
    verification:
      - kind: unit
        ref: "cmake --preset ci; focused host/probe build and seven compat.probe cases"
        status: pass
      - kind: integration
        ref: "Combined hosted CI after 04-55; Windows/GCC lanes"
        status: unknown
    human_judgment: false
  - id: GAME-06
    description: Compatibility registration follows the chosen inventory; emulator prefixes and native CMake drive paths reach only approved build-tree runners, including `.exe`; Windows and BE32 acceptance remain pending.
    requirement: GAME-06
    verification:
      - kind: integration
        ref: "compat.library.controls/synthetic_result and full ci-local-4 workflow, including compat ratchet"
        status: pass
      - kind: integration
        ref: "Focused library controls: `.exe` runner, relative root/runner refusal and `../` input refusal"
        status: pass
      - kind: integration
        ref: "Combined hosted Windows CI after 04-55; drive-rooted CMake paths and PE executable launch"
        status: unknown
      - kind: integration
        ref: "Combined hosted CI after 04-55; BE32 compatibility consumers"
        status: unknown
    human_judgment: false
  - id: GAME-07
    description: Existing libretro host movie checks pass locally; the integrated RetroArch session remains part of the open 04-29 acceptance.
    requirement: GAME-07
    verification:
      - kind: integration
        ref: "ci-local-4: libretro.host and libretro.input.movies"
        status: pass
      - kind: e2e
        ref: "Hosted RetroArch session after 04-55; 04-29 checkpoint remains open"
        status: unknown
    human_judgment: false
duration: 44 min
completed: 2026-11-12
status: complete
---

# Phase 04 Plan 54: Hosted Portability Prerequisites Summary

Strict compiler consumers, selected compatibility inventories and emulator forwarding now pass the complete local CI workflow; platform acceptance remains assigned to the next combined hosted run.

## Performance

- **Duration:** 44 min
- **Started:** 2026-11-12T18:09:35Z
- **Completed:** 2026-11-12T18:54:00Z
- **Tasks:** 2
- **Files modified:** 7, including the durable workflow-recovery notes in `AGENTS.md`
- **Runtime/models:** Codex/OpenGSD, sequential shared checkout; executor gpt-6-luna xhigh; planner gpt-6.1-sol high; independent checker gpt-6-luna xhigh

## Accomplishments

- Made the controller movie's pressed count explicitly unsigned and routed all four read-only probe opens through a private `fopen_s`/`fopen` helper without changing failure or cleanup behavior.
- Registered compatibility tests only when the selected inventory lists `compat`. Forwarded decoded emulator arguments to existing raw helpers and generated quoted shell launchers for test and production runners, preserving the single-runner library interface.
- Added a post-closeout Windows path fix: normalize drive-rooted CMake paths using the POSIX shell's `cd`/`pwd`, accept only the existing build-tree runner locations with optional `.exe`, and preserve production executable suffixes in the ordinary-mode mirror.
- Reused the repo-validated OpenBIOS image at the exact local test input path. The full four-job CI workflow, package step and release tests completed successfully.
- Recorded durable recovery guidance: use command/session evidence and repository snapshots before interrupting quiet executors, and align temporary CMake binary directories with expected fixture paths.

## Task Commits

1. **Task 1: Restore strict compiler portability** — `8665f1b` (`fix`)
2. **Task 2: Register compatibility by inventory and forward emulator arguments** — `4766917` (`fix`)

The later Windows path follow-up is recorded separately because Plan 55 commits are between the original measured Plan 54 ledger boundary and that commit.

## Post-closeout Follow-up

- Commit: `cdf9934` (`fix(04-54): accept native Windows library test paths`), parent `4eeaab0` (completed Plan 55 head), 7,450-byte diff.
- Native Windows CMake paths are canonicalized through shell `cd`/`pwd`; the runner allowlist remains restricted to the same build-tree targets, with `.exe` added. The ordinary-mode mirror keeps the actual executable filename, and the default local runner checks the `.exe` form when the extensionless form is unavailable.
- Added existing-case controls for an `.exe`-suffixed runner, relative prepared root, relative runner and parent-directory escape.
- Focused command: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-library schwaystation-run-test schwaystation-run && ctest --test-dir build/ci --parallel 4 -R '^compat[.]library[.](controls|synthetic_result)$' --no-tests=error --output-on-failure` — exit 0; 2/2 cases passed. The drive-letter filesystem branch remains pending hosted Windows CI.

## Deviations from Plan

- The first complete local workflow exited 8 at 508/509 because the expected local OpenBIOS input was absent. The existing `build/ci/openbios/openbios.bin` passed `cmake/OpenBIOS.cmake` (524,288 bytes and the pinned SHA-256); copying it to `build/ci-local-4/openbios/openbios.bin` allowed the existing shell test to generate a fresh tuple. The subsequent complete workflow exited 0. The temporary ignored `CMakeUserPresets.json` was removed.
- Local ASan configuration could not be established: the preset compiler `clang-21` is unavailable; retrying with the installed Apple Clang reached generation but hit the pre-existing missing `schwaystation-fixtures` target while fixtures were disabled. The WASI preset likewise requires unavailable `clang-21`. No inventory entries or fixture policy were changed; the combined hosted run remains the source of these lane results.

## Evidence

- Task 1 command: `cmake --preset ci && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-test-probe schwaystation-run schwaystation-run-test && ctest --test-dir build/ci --parallel 4 -R '^compat[.]probe[.](sha1_vectors|synthetic_identity|full_identity|malformed|cue_modes|cue_edges|iso_edges)$' --no-tests=error --output-on-failure` — exit 0; seven probe cases passed.
- Task 2 native configure/build and focused library cases passed. Positive multi-argument `cmake -E env` forwarding passed for direct helpers and library test/ordinary mirror runners, including an argument with a space. Deliberately failing prefixes propagated nonzero status through all three call paths. Compatibility tests passed 2/2; probe tests passed 7/7.
- Checked input command: `cmake -DOPENBIOS_INPUT=build/ci-local-4/openbios/openbios.bin -DEXPECTED_FILE=tests/built/expected.txt -P cmake/OpenBIOS.cmake` — exit 0; size and digest matched the repository pin.
- Final local closeout: `cmake --workflow --preset ci-local-4` — exit 0; 509/509 CI tests passed, three ZIP packages were produced, and 5/5 release tests passed. CTest reported about 60 seconds of test time. The test-generated shell tuple enabled the compatibility ratchet row.
- `cmake --workflow --preset hygiene` was run once after source changes. Its current `build/hygiene/Testing/Temporary/LastTest.log` ends normally with all 23 checks passed. The outer workflow exit was not captured because its tool output was truncated; it is recorded as unconfirmed, not claimed as exit 0. No repeat was made solely to recover that output.

## Deferred Validation

- Cross-platform compiler and inventory claims await the combined normal hosted run after 04-55. Do not infer MSVC, GCC cross-compiler, ASan, WASI or BE32 acceptance from this local run.
- The follow-up's native POSIX test proves `.exe` suffix and path-refusal controls, not Windows filesystem behavior; hosted Windows x64/arm64 validation remains required.
- 04-29 remains incomplete: its two distinct RetroArch frames are still mandatory, and the local LaunchServices denial continues to prohibit a local RetroArch run. This plan does not close or weaken that acceptance.

## User Setup Required

None. Hosted CI dispatch remains with the coordinator after 04-55.

## Self-Check: PASSED

- Summary file exists.
- Task commits `8665f1b` and `4766917` are ancestors of HEAD.
- Persisted plan ledger measures 2 commits from `2618185b24c770393b77afb13ab9d83bbf3136c2` through `476691714166dc42eda50d1877db4f74a9e7b6bc`.
- Follow-up commit `cdf9934` is a child of the recorded post-55 base `4eeaab0`; current phase ordinal and open 04-29 checkpoint are unchanged.
