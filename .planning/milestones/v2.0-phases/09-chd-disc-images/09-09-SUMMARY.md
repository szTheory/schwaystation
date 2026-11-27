---
phase: 09-chd-disc-images
plan: "09"
subsystem: media
tags: [CHD, CMake, CTest, licensing, compatibility]
requires:
  - phase: 09-chd-disc-images
    provides: pinned CHD provider, feature ledger and protected ratchet
provides:
  - linked-provider-derived libretro extensions and license claims
  - exact notices for the pinned CHD and codec closure
  - shared date conversion with protected-base helper transport
affects: [Phase 9 package validation, Phase 10 tracked-source license scan]
actuals:
  tokens: 11985
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Frontend metadata is derived from the linked provider target.
    - Transported CMake judges carry their relative helper dependency.
key-files:
  created:
    - tests/cmake/date_days.cmake
  modified:
    - libretro/CMakeLists.txt
    - libretro/schwaystation_libretro.info.in
    - tests/cmake/info_claims.cmake
    - tests/cmake/info_claims_self_test.cmake
    - THIRD-PARTY-NOTICES.md
    - tests/cmake/compat-ratchet.cmake
    - tests/cmake/perf_compare.cmake
    - scripts/compat-ratchet.sh
    - tests/compat/tests.cmake
    - .planning/todos/pending/005-notices-gates.md
key-decisions:
  - Keep the repository-check ceiling at 3901 lines and defer the generic source-wide SPDX scanner to its first tracked-source caller.
  - Derive CHD metadata from the actual host target link closure; Plan 13 owns packaged ON/OFF proof.
patterns-established:
  - The shared date helper uses a strict-calendar policy so compatibility rejects impossible dates while perf preserves prior behavior.
  - Protected-base and test fixtures copy the helper beside transported compatibility judges that include it.
requirements-completed: []
coverage:
  - id: D1
    description: Configured libretro metadata reflects the linked CHD provider and selected license closure.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=ON && ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure"
        status: pass
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=OFF && ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Exact CHD notices and transported compatibility checks pass within the gate cap.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "ctest --preset hygiene --no-tests=error -R '^policy[.]notices([.]self_test)?$' --output-on-failure"
        status: pass
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^compat[.]ratchet[.]protected_controls$|^perf[.]gate[.]allowed$' --output-on-failure"
        status: pass
      - kind: other
        ref: "cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake"
        status: pass
    human_judgment: false
duration: "not recoverable from the inherited checkpoint"
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-09 Summary

**Libretro CHD claims now follow the linked provider, with exact dependency notices and the source-wide SPDX scan deferred to its first tracked caller.**

## Performance

- **Duration:** The inherited handoff did not preserve the plan start. The measured review-to-Task-2-commit interval was 6m15s; the wider active duration is unknown.
- **Tasks:** 2
- **Observed bottleneck:** Standard code review took several minutes; configuration and focused selectors completed in under one second. The notice selector caught a bad Applies-to path before packaging or hosted CI.

## Accomplishments

- Derived supported extensions, description and license fields from whether schwaystation_disc_image actually links chdr-static; added ON/OFF claim checks and negative controls.
- Added exact pinned libchdr and chd.h BSD-3 notices, LZMA SDK public-domain grant, miniz Unlicense, selected dr_flac public-domain grant, and zstd 1.5.7 BSD-3 notice.
- Shared date conversion between perf and compatibility gates and copied the helper with transported judges, preserving the strict compatibility invalid-date control and existing allowance behavior.
- Kept the counted gate at 3900/3901. Todo 005 now covers the generic tracked-source scan for its first Phase 10 source caller; .info license matching is checked here against the linked closure.

## Task Commits

1. **Task 1: Derive frontend claims from the compiled provider** — d336d33.
2. **Task 2: Complete CHD license notices and compatibility helper transport** — 1470276.

## Verification

| Command | Exit | Result |
|---|---:|---|
| `cmake --preset ci -DSCHWAYSTATION_CHD=ON && ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure` | 0 | 2/2 passed. |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF && ctest --preset ci --no-tests=error -R '^libretro[.]info[.]claims([.]self_test)?$' --output-on-failure` | 0 | 2/2 passed. |
| `cmake --preset hygiene && ctest --preset hygiene --no-tests=error -R '^policy[.]notices([.]self_test)?$' --output-on-failure` | 0 | 2/2 passed. |
| `cmake --preset ci && ctest --preset ci --no-tests=error -R '^compat[.]ratchet[.]protected_controls$|^perf[.]gate[.]allowed$' --output-on-failure` | 0 | Setup, protected-base and allowance checks passed 3/3 in 0.92s. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | 0 | Gate total 3900/3901. |
| `sh -n scripts/compat-ratchet.sh`; `LC_ALL=C sort -cu tests/hygiene/inventory.txt`; `LC_ALL=C sort -cu tests/libretro/inventory.txt`; `git diff --check` | 0 | Syntax, inventory and whitespace preflights passed. |
| `gh api` for the pinned libchdr license files and facebook/zstd v1.5.7 LICENSE | 0 | Pinned notice text sources resolved; no hosted URL applies. |

The code review found one blocker in the CHD suffix regex. It was corrected, confirmed in a read-only follow-up, and then exercised in both metadata configurations. No full build, broad suite, act or hosted CI run applied because this plan changed no workflow. Plan 11 retains the routing dry run and producer-path measurement; Main/Release artifact reuse remains rejected because the recorded archive digests differ.

## Scope Adjustment and Issues

The owner chose to keep the hard gate cap and defer the generic source-wide SPDX scanner. The CHD closure is fetched into the build tree and was audited in Plan 09-02; Plan 13 validates packaged ON/OFF notice bytes. Todo 005 now targets the first later tracked SPDX-bearing source and keeps the configure-time .info check out of that future work.

The initial combined hygiene command exited 8: the existing path checker treated the backticked CHD option on an Applies-to line as a filesystem path, and hygiene does not register the compatibility/perf selectors. The CHD option was moved to section prose; ctest -N identified the CI preset for compatibility/perf. The corrected selectors all passed.

## Next Phase Readiness

Plan 09-13 is dependency-ready and next. Its ON/OFF package operations remain sequential because OFF replaces the same build outputs. Plan 09-11 depends on those archive checks, then Plan 09-12 and final Plan 09-14 follow. No user action is needed.

---
Phase: 09-chd-disc-images
Completed: 2026-11-20
