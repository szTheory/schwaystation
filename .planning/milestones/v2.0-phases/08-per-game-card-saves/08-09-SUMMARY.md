---
phase: 08-per-game-card-saves
plan: "09"
subsystem: testing
tags: [memory-cards, save-states, fixtures, CI]
requires:
  - phase: 08-08
    provides:
      - Disc save identity resolution and full-sector fallback
provides:
  - Independently reproduced format 18 GPU motion state pins
  - Focused integrated state, card, runner, libretro, and guest evidence
affects: [phase-08-verification, phase-08-ship]
actuals:
  tokens: 540
  tasks: 2
  commits: 1
task-commit: 6e45e1e
tech-stack:
  added: []
  patterns:
    - Resolve exact CTest selectors and fixture inputs before targeted execution
    - Reproduce changed deterministic pins in separate build directories
key-files:
  created:
    - .planning/phases/08-per-game-card-saves/08-09-REVIEW.md
  modified:
    - docs/testing.md
    - tests/fixtures/tests.cmake
key-decisions:
  - Update only the two saved-state hashes changed by format 18 card protocol fields; retain all rendered-frame pins.
  - Include abi.api_v4 in the focused checks because it explicitly verifies card bytes and generations stay outside ordinary states.
patterns-established:
  - Cheap inventory, budget, selector, manifest, metadata, and workflow checks precede broader verification.
requirements-completed: [SAVE-01, SAVE-02, SAVE-03, SAVE-05]
coverage:
  - id: SAVE-01-disc-identity
    description: Runner disc identity tests cover first-member precedence, full-content fallback, media moves, playlist changes, and swaps.
    requirement: SAVE-01
    verification:
      kind: integration
      ref: "ctest --preset ci --no-tests=error -R '^(abi[.]api_v4|card[.]|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))' --output-on-failure (exit 0, 24/24)"
      status: pass
    human_judgment: false
  - id: SAVE-02-card-file-validation
    description: Raw-card and wrapper import/export checks retain atomic failure behavior.
    requirement: SAVE-02
    verification:
      kind: unit
      ref: "ctest --preset ci --no-tests=error -R '^(abi[.]api_v4|card[.]|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))' --output-on-failure (exit 0, 24/24)"
      status: pass
    human_judgment: false
  - id: SAVE-03-two-slot-persistence
    description: Runner card paths, both slots, and fresh-process persistence remain isolated and resumable.
    requirement: SAVE-03
    verification:
      kind: integration
      ref: "ctest --preset ci --no-tests=error -R '^(abi[.]api_v4|card[.]|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))' --output-on-failure (exit 0, 24/24)"
      status: pass
    human_judgment: false
  - id: SAVE-04-libretro-local-path
    description: In-process libretro card persistence and capability mapping pass; the required real-host continuation proof remains in Plan 08-10.
    requirement: SAVE-04
    verification:
      kind: integration
      ref: "ctest --preset ci --no-tests=error -R '^(abi[.]api_v4|card[.]|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))' --output-on-failure (exit 0, 24/24)"
      status: pass
    human_judgment: false
  - id: SAVE-05-pocketstation-storage
    description: The documented PocketStation card protocol and owned guest fixture pass their focused checks.
    requirement: SAVE-05
    verification:
      kind: integration
      ref: "ctest --preset ci --no-tests=error -R '^(abi[.]api_v4|card[.]|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))' --output-on-failure (exit 0, 24/24)"
      status: pass
    human_judgment: false
duration: "at least 20min; active time not measured"
completed: 2026-11-19
status: complete
---

# Phase 08 Plan 09 Summary: State and save integration evidence

**Format 18's card protocol state now has reproducible GPU motion pins, and the focused save integration checks pass.**

## Performance

- **Wall interval:** At least 20 minutes from the 2026-11-19T18:22:20Z state checkpoint through focused verification and closeout; active time was not measured.
- **Tasks:** 2.
- **Files changed:** Two implementation/documentation files; one review artifact was added.
- **Runtime/model:** Codex/OpenGSD CLI 1.20.0; primary GPT-6 session, exact deployed model ID unavailable.

## Accomplishments

- Reproduced the frame-150 state hash as d6bef00aeadcaf9ae91354e316dcecb6c1ccbc7be74e59ba777d4496cb950ace and the frame-180 state hash as 9e06757cdd4f4d233ec23c331a0e466b1c44339ef7f5d72fc82a2cc9ea529873 in both build/ci and build/ci-local-4. The old literals came from the earlier state format; only these two state pins changed.
- Kept all GPU motion frame pins unchanged. The format 18 fields add each card slot's pending-error latch; card bytes and generation counters remain host-owned and outside ordinary states.
- Added the API v4 card-field exclusion test to this focused run, alongside old-format rejection, card wrappers, runner persistence, libretro metadata and save behavior, and both owned guest fixtures.
- Reviewed the 08-07 and 08-08 implementation changes in context. The follow-up report records no open findings.

## Evidence

| Check | Result | Command or evidence |
| --- | --- | --- |
| Inventory and budget preflight | Exit 0; inventories ordered, repository checks 3901/3901 lines, RetroArch harness 300/300 lines | LC_ALL=C sort -cu tests/card/inventory.txt && LC_ALL=C sort -cu tests/fixtures/inventory.txt; cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake; cmake -DROOT=. '-DPATTERNS=tests/retroarch/*' -DLIMIT=300 -P tests/cmake/line_budget.cmake |
| Fixture manifest | Exit 0; playlist XML hash matches its manifest row | shasum -a 256 tests/fixtures/disc_boot_playlist.xml; result 41e12b61409f968de0afc3c62b4b7edfadef12ed9e2979d06cf01d04cf875868 |
| Workflow wiring | Exit 0; required guest upload/download and existing RetroArch job path are connected | cmake -DROOT=. -DCASE=disc_registration -P tests/cmake/workflows.cmake |
| CI configure and selector preflight | Exit 0; exact state selection 14 tests and save selection 24 tests; both guest executables exist | cmake --preset ci; ctest --preset ci --no-tests=error -N -R '^(core[.]state[.]|fixtures[.]gpu_motion[.])'; ctest --preset ci --no-tests=error -N -R '^(abi[.]api_v4|card[.]|policy[.]media_registration|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))'; test -s build/ci/fixtures/memcard_roundtrip.exe && test -s build/ci/fixtures/pocketstation_storage.exe |
| Initial state-pin run | Exit 8; 12/14 passed, and only the two format 18 state pins were stale | cmake --build --preset ci --target schwaystation-test-state schwaystation-run schwaystation-fixtures && ctest --preset ci --no-tests=error -R '^(core[.]state[.]|fixtures[.]gpu_motion[.])' --output-on-failure |
| Independent hash reproduction | Exit 0; both build directories produced identical state hashes at frames 150 and 180 | build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 150 --hash-state; build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 180 --hash-state; build/ci-local-4/schwaystation-run --exe build/ci-local-4/fixtures/gpu_motion.exe --frames 150 --hash-state; build/ci-local-4/schwaystation-run --exe build/ci-local-4/fixtures/gpu_motion.exe --frames 180 --hash-state |
| State and frame regressions after pin update | Exit 0; 14/14 passed in 7.14 seconds; second build's two state pins passed 2/2 in 1.15 seconds | ctest --preset ci --no-tests=error -R '^(core[.]state[.]|fixtures[.]gpu_motion[.])' --output-on-failure; ctest --test-dir build/ci-local-4 --no-tests=error -R '^fixtures[.]gpu_motion[.]state[.](150|180)$' --output-on-failure |
| Focused save integration | Exit 0; 24/24 passed in 2.97 seconds | ctest --preset ci --no-tests=error -R '^(abi[.]api_v4|card[.]|runner[.]cards[.]|runner[.]disc[.]load|libretro[.](cards|info)[.]|fixtures[.](pocketstation_storage|memcard_roundtrip[.]))' --output-on-failure |
| Source review and diff hygiene | Exit 0; no findings, and git diff --check is clean | 08-09-REVIEW.md; git diff --check |

No hosted run was started for this plan, so no hosted run URL applies. Plan 08-10 owns full local CI and exact-head hosted RetroArch proof.

## CI and release efficiency

The focused 24-test selector took 2.97 seconds total. The new RetroArch card check reuses the existing macOS job and guest artifact; the wiring adds no job or matrix leg. Two separate RetroArch processes are necessary to prove persistence across a restart. Its hosted duration remains unmeasured and will be checked during and after the exact-head run in Plan 08-10. No cache, concurrency, or coverage change is justified by the current measurements.

## Task Commit

The two state pins and their testing documentation landed together in 6e45e1e (test(08-09): refresh format 18 state pins). The review and planning metadata are recorded in the plan closeout commit.

## Deviations from Plan

The focused selector also ran abi.api_v4 because inspection showed that test explicitly guards against card bytes and generation counters appearing in ordinary state fields. No production implementation changed.

## Issues Encountered

The first state selector exposed stale frame-150 and frame-180 hashes. Their replacements matched in two independent local build directories, and the updated selector passed without changing any frame hash or other expected device output.

## Threat Flags

None. This plan changes deterministic fixture expectations and user-facing testing documentation only.

## Next Plan Readiness

Plan 08-10 can run the approved full local CI workflow, then push the reviewed branch and inspect the exact-head hosted RetroArch card step. Keep libretro_saves false until that host check passes.

## Self-Check: PASSED

The summary and review artifact exist, the task commit is in history, and the GSD summary validator passes.

---
*Phase: 08-per-game-card-saves*
*Completed: 2026-11-19*
