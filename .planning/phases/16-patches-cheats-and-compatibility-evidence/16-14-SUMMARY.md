---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "14"
subsystem: compatibility
tags: [libretro, CMake, CTest, packaging]
requires:
  - phase: 16-05
    provides: Libretro cheat configuration and state.
  - phase: 16-11
    provides: Patch and cheat test coverage.
  - phase: 16-13
    provides: Pinned compatibility inventory.
provides:
  - Required capability matrix with a current-build result evaluator.
  - Experimental metadata and supported cheat claims in the staged libretro info file.
affects: [compatibility, libretro, packaging]
key-files:
  created:
    - tests/cmake/Capabilities.cmake
    - tests/cmake/capabilities_controls.cmake
    - tests/compat/required-capabilities.csv
  modified:
    - libretro/CMakeLists.txt
    - libretro/schwaystation_libretro.info.in
    - tests/cmake/info_claims.cmake
    - tests/cmake/info_claims_self_test.cmake
    - tests/cmake/workflows.cmake
    - tests/compat/tests.cmake
    - tests/hygiene/tests.cmake
    - .planning/REQUIREMENTS.md
requirements-completed: [COMPAT-03, CHEAT-01]
coverage:
  - id: D1
    description: The required capability matrix covers providers, profiles, peripheral classes, card persistence, patches and cheats.
    requirement: COMPAT-03
    verification:
      - kind: integration
        ref: compat.capabilities.coverage
        status: pass
    human_judgment: false
  - id: D2
    description: The evaluator and staging hook keep the experimental marker when current required evidence is incomplete or fails.
    requirement: COMPAT-03
    verification:
      - kind: integration
        ref: compat.capabilities.results and compat.capabilities.claims
        status: pass
    human_judgment: false
  - id: D3
    description: Staged libretro metadata exposes the tested GameShark cheat subset.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: libretro.info.claims and libretro.cheats.state
        status: pass
    human_judgment: false
metrics:
  completed: 2026-11-26
  duration: unavailable across interrupted sessions
commits: 1
plan_head_before: 608a8b9bb5158b055aec3d4106b7e3913cc06587
plan_head_after: 9787c08bd72e27caac85f77d894abb2d1a3e5f13
status: complete
---

# Phase 16 Plan 14 Summary

The libretro package derives its experimental marker from required capability results and advertises the supported cheat subset. There are 51 matrix rows. The provider and profile boot-to-play selectors are absent, so the staged package remains experimental.

## Verification evidence

- `git diff --cached --check`, `LC_ALL=C sort -c tests/compat/inventory.txt`, and `cmake -DMODE=capabilities -DROOT="$PWD" -P tests/cmake/workflows.cmake` exited 0. Both package presets place tests before installation.
- `cmake --preset ci` and `cmake --build --preset ci --target schwaystation_libretro schwaystation-test-libretro-host schwaystation-libretro-card-test` exited 0.
- `ctest --preset ci --no-tests=error -R '^(compat[.]capabilities[.](coverage|results|claims)|libretro[.]info[.]claims([.]self_test)?|libretro[.]cheats[.]state|docs[.]budgets)$' --output-on-failure` exited 0. The seven selected tests and the compatibility setup fixture passed. CTest wrote a complete single `testsuite` report in a temporary directory.
- `cmake --install build/ci --component libretro --prefix "$stage"` and the staged `info_claims.cmake` check exited 0. The staging prefix was removed after the check.
- `cmake --preset hygiene` and `ctest --preset hygiene --no-tests=error -R '^policy[.]gate_budget([.]self_test)?$' --output-on-failure` exited 0. The declared inventory measures 4,693 lines under the owner-approved 4,694-line ceiling.

## Test and implementation order

The earlier executor's Task 1 RED evidence check exited 0 for an expected CTest exit 8. The resumption began with the evaluator, controls and metadata already drafted. Separate pre-change runs for the metadata and package-hook tasks were unavailable, so the passing draft was preserved. During resumption, the new truncated-JUnit control reproduced a false pass (CTest exit 8). Requiring the closing CTest suite element made the control pass (exit 0).

The draft used OpenGSD on Codex with a GPT-6 Luna executor. The Codex coordinator resumed and closed it in a separate session. The elapsed pause included owner review, so total active execution time could not be reconstructed. Full phase review and local and hosted verification remain downstream.

## Self-Check: PASSED

The declared files are present in the task commit, and the focused capability, metadata, package staging and budget checks passed.
