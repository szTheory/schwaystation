---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "16"
subsystem: compatibility
tags: [Redump, CTest, libretro, documentation]
requirements-completed: [COMPAT-01, COMPAT-03, PATCH-01, CHEAT-01]
coverage:
  - id: D1
    description: The generated report reconciles the pinned inventory and summarizes current required capability results.
    requirement: COMPAT-01
    verification:
      - kind: integration
        ref: policy.compat_doc_fresh and build-local compatibility report
        status: pass
    human_judgment: false
  - id: D2
    description: The report leaves boot-to-play gaps explicit and describes supported patch and cheat predicates.
    requirement: COMPAT-03
    verification:
      - kind: integration
        ref: compat.capabilities.coverage, compat.capabilities.results and compat.features.ratchet
        status: pass
    human_judgment: false
  - id: D3
    description: Public API and host documentation describe the tested patch and cheat behavior.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: abi.api_v11, runner.cheats.cli, runner.cheats.hash and libretro.cheats.state
        status: pass
    human_judgment: false
  - id: D4
    description: Historical patch feature rows name the exercised formats and validation predicates.
    requirement: PATCH-01
    verification:
      - kind: integration
        ref: disc.patch.ppf1, disc.patch.ppf2, disc.patch.ppf3, disc.patch.bps and runner.patch.cli
        status: pass
    human_judgment: false
metrics:
  completed: 2026-11-26
  duration: 5m
commits: 1
plan_head_before: d4511ed92eaff1608b22dbd3b29182b623065788
plan_head_after: 899db26f05af3cf1e9b2695b5659cb2f010b68ba
status: complete
---

# Phase 16 Plan 16 Summary

The public report reconciles 13,407 pinned inventory entries and shows which capabilities still await evidence. Historical patch and cheat rows cite executed test selectors. The checked-in report, `docs/compatibility.md`, regenerates via `scripts/compat-doc.sh`. The optional build-local report imports current completed CTest results through the same evaluator used by packaging.

The public documentation now records API v11, state format 21 and behavior revision 21, the libretro cheat claim, and cached DAT validation. The evaluator's result comparison was simplified to make room for the new report and controls within the approved check budget.

## Checks run

- `ctest --preset ci --no-tests=error -R '^policy[.]compat_doc_fresh$' --output-on-failure` exited 8 after adding the inventory and missing-provider assertions and before implementing the generator. The same selector passed after implementation.
- `cmake --preset ci` and the targeted API, runner, patch and libretro build exited 0. The eleven targeted producers passed before their results were added to the historical ledger.
- The combined capability, historical feature, generated document, API, patch and cheat selection passed all eighteen tests with CTest exit 0. A build-local report from that CTest JUnit recorded four patch and two cheat passes, with eight provider rows unknown. The temporary report was removed.
- `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check`, `git diff --check`, `sh -n scripts/compat-doc.sh`, and the focused hygiene budget test exited 0. The measured repository-check total was 4,690 lines.

Runtime: Codex coordinator with OpenGSD 1.23.0. The primary model ID was unavailable.

## Self-Check: PASSED

The published report and its supporting checks are accounted for in the task commit.
