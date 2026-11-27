---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "15"
subsystem: compatibility
tags: [CMake, CTest, CI, protected-base]
requirements-completed: [COMPAT-03]
coverage:
  - id: D1
    description: Main CI supplies protected feature and capability rows to the compatibility check.
    requirement: COMPAT-03
    verification:
      - kind: integration
        ref: compat.ratchet.protected_controls and workflow registration
        status: pass
    human_judgment: false
  - id: D2
    description: Historical feature protection and current package evidence use separate inputs.
    requirement: COMPAT-03
    verification:
      - kind: integration
        ref: compat.features.ratchet and compat.capabilities.claims
        status: pass
    human_judgment: false
metrics:
  completed: 2026-11-26
  duration: 4m
commits: 1
plan_head_before: dbb223df2cd948046fe896c4c92efbfdf80c4283
plan_head_after: b4577bcf907423a940de5b50916a58572015b21a
status: complete
---

# Phase 16 Plan 15 Summary

Main CI checks the historical feature ledger against the protected PR base and rejects removed or changed required capability rows. The package importer still derives current capability results from the completed CTest report. The historical comparison reuses the old judge's two input positions with the same historical file to preserve its protected-base interface.

Created or changed checks in `tests/cmake/workflows.cmake`, `scripts/compat-ratchet.sh`, `tests/compat/tests.cmake` and `tests/cmake/Capabilities.cmake` were exercised through the same Main CI registrations.

## Checks run

- `cmake -DCASE=disc_registration -DROOT="$PWD" -P tests/cmake/workflows.cmake` exited 1 for the missing protected checkout paths once the workflow check's condition was corrected. After adding those paths and matching the existing BE32/WASI step order, it exited 0.
- `sh -n scripts/compat-ratchet.sh`, `git diff --check`, `cmake --preset ci` and the exact selector preflight exited 0. The focused compatibility, libretro claims and documentation CTest selection passed with all ten selectors. Negative controls exercised a protected passing feature regression and a removed required matrix row.
- The focused `compat.features.ratchet` CTest command with `COMPAT_BASE_DIR` and `COMPAT_BASE_SHA` set to a temporary sparse checkout of local `origin/main` exited 0. The temporary checkout was removed.
- `cmake --preset hygiene` and `ctest --preset hygiene --no-tests=error -R '^policy[.]gate_budget$' --output-on-failure` exited 0. The measured count was 4,691 against the approved 4,694-line ceiling.
- `act --dryrun --no-cache-server --action-cache-path "$task_cache" pull_request --container-architecture linux/arm64 -P ubuntu-26.04=node:24-bookworm -P ubuntu-26.04-arm=node:24-bookworm -W .github/workflows/ci.yml -j build` exited 0 and reached PR change classification. Dry-run cannot produce the classifier outputs that drive the hosted build matrix. Earlier attempts hit first-run image selection and a sandboxed default cache path. The temporary cache was removed and no containers were created.

Runtime: Codex coordinator with OpenGSD 1.23.0. The primary model ID was unavailable. Hosted PR checkout permissions, routing and runner behavior remain for phase verification.

## Self-Check: PASSED

The production commit and all planned checks are accounted for.
