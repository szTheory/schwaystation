---
phase: 07-faster-and-more-trustworthy-ci
plan: "07"
subsystem: ci
tags: [quality-json, cachegrind, wasm, state, cmake]
requires:
  - phase: "07-01"
    provides: registered quality policy controls and counted repository-check budget
  - phase: "07-03"
    provides: sourced deterministic quality measurements and CI settings audit
  - phase: "07-06"
    provides: measured workflow timeouts and release metadata routing
provides:
  - single cited quality registry with Cachegrind, state-byte, and critical-path bounds
  - production CI comparisons for Cachegrind, the linked WASI module, and state serialization
  - focused production-path controls for missing, malformed, and over-bound measurements
affects:
  - Phase 7 ship evidence
tech-stack:
  added: []
  patterns:
    - Mark production measurement blocks and execute them with focused fixture controls
    - Report actual bytes from the existing state stability test and feed that output to the shared comparator
key-files:
  created: []
  modified:
    - quality.json
    - .github/workflows/ci.yml
    - tests/cmake/quality.cmake
    - tests/cmake/quality_controls.cmake
    - tests/core/test_state.c
    - tests/hygiene/tests.cmake
    - .planning/REQUIREMENTS.md
    - .planning/ROADMAP.md
    - .planning/STATE.md
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-07-PLAN.md
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-08-PLAN.md
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-10-PLAN.md
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-CONTEXT.md
    - .planning/phases/07-faster-and-more-trustworthy-ci/07-RESEARCH.md
key-decisions:
  - Keep size.wasm measured but unbounded until the focused Ubuntu 26 Release lane produces the actual linked module size.
  - Preserve the prior Cachegrind base and relative comparison while adding the sourced absolute bound.
metrics:
  duration: 43min
  completed: 2026-11-18
actuals:
  tokens: 12177
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 8112e0c9b2256033f16def6d0a9903e059cb628c
plan_head_after: 102558fe7fa07dd52c2ef54c5f97f3a6c32deab2
requirements-completed: []
coverage:
  - id: D1
    description: Fresh Cachegrind totals reach the shared comparator while the existing base and relative performance gate remains active.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure (exit 0)"
        status: pass
      - kind: other
        ref: "quality.json cited rationale and matched Plan 02 critical-path ceiling/workload hash (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: The linked WASI artifact is passed to the comparator with path, regular-file, and WebAssembly magic checks; its size is reported without a guessed bound.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: "policy.quality_metrics production-block controls: valid unbounded module, missing file, bad magic, and one-byte-over bounded fixture (exit 0)"
        status: pass
      - kind: other
        ref: "Ubuntu 26 Release module measurement and first size.wasm bound are reserved for ship"
        status: pending-ship
    human_judgment: false
  - id: D3
    description: The existing stable_bytes case reports the actual serialized state length and CI rejects missing, malformed, duplicate, or over-bound output.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^(core[.]state[.]stable_bytes|policy[.]quality_metrics)$' --output-on-failure (exit 0; 2/2 passed)"
        status: pass
      - kind: other
        ref: "Production-style block measured state_bytes=3755020 and quality.cmake accepted the checked-in state.bytes bound (exit 0)"
        status: pass
    human_judgment: false
  - id: D4
    description: The dated process-budget correction is cited in quality.json and the unchanged counted globs remain under the temporary ceiling.
    requirement: TUNE-05
    verification:
      - kind: integration
        ref: "ctest --test-dir build/hygiene -R '^policy[.]gate_budget$' -V (exit 0; 3509/3900)"
        status: pass
      - kind: other
        ref: "git diff --check (exit 0)"
        status: pass
    human_judgment: false
status: complete
---

# Phase 7 Plan 7: Enforce measured quality outputs

**The quality registry now receives real instruction counts, linked Wasm bytes, and serialized state bytes through the existing CI producers.**

## Accomplishments

- Added a dated, cited process-budget rationale to `quality.json`, sourced deterministic metric bounds, and copied the critical-path ceiling and workload hash from the matching Plan 02 cohort.
- Connected both head Cachegrind measurements to the shared comparator while retaining the original base measurements and relative performance comparison.
- Added a production WASI artifact check that measures the linked Release runner, validates its path and Wasm magic, and reports its size. The registry deliberately keeps this one metric unbounded until the matching Ubuntu 26 Release module is measured at ship.
- Made `state.stable_bytes` print its actual serialized length after the existing assertions. The Linux x64 CI producer requires exactly one positive measurement and sends it to the same comparator.
- Added focused controls that execute the production measurement blocks with valid, missing, malformed, duplicate, bad-magic, and one-byte-over inputs.
- Kept the counted repository-check gate at 3,509 of the corrected 3,900-line ceiling; the separate 300-line RetroArch limit and existing assertions remain unchanged.

## TDD RED/GREEN Evidence

- **RED:** Commit `599c8f9` added the Task 2 production-block controls before implementation. The focused `policy.quality_metrics` run exited 8 because the planned `QUALITY_WASM_COMPARE_BEGIN/END` production block was absent. The target test ran and failed on that intended assertion; no syntax, discovery, setup, or fixture error occurred. The unchanged JUnit report and evidence record are under `build/phase07/07-07-red.xml` and `build/phase07/07-07-red-evidence.json`. `gsd_run check tdd-red-evidence` returned `RED_EVIDENCE_OK`.
- **GREEN:** Commit `102558f` added the linked Wasm and state-byte production paths. The focused CTest pair passed. The real local stable state test reported 3,755,020 bytes and the comparator accepted the checked-in state bound.
- **REFACTOR:** None needed.

## Verification

- `cmake --preset ci` — exit 0.
- `ctest --preset ci --no-tests=error -R '^policy[.]quality_metrics$' --output-on-failure` — exit 0; the registry, Cachegrind production block, and metric failure controls passed.
- `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake` — exit 0.
- `jq -e '.reasons | any(startswith("2026-11-18") and test("3,?133") and test("3,?900"))' quality.json` — exit 0.
- `cmake --build build/ci --target schwaystation-test-state` — exit 0.
- `ctest --preset ci --no-tests=error -R '^(core[.]state[.]stable_bytes|policy[.]quality_metrics)$' --output-on-failure` — exit 0; 2/2 passed.
- The production-style state measurement command — exit 0; printed `state_bytes=3755020`, accepted by `quality.cmake`.
- `cmake --preset hygiene` — exit 0.
- `ctest --test-dir build/hygiene -R '^policy[.]gate_budget$' -V` — exit 0; 3,509/3,900 counted lines.
- `git diff --check` — exit 0.
- No hosted workflow, whole suite, or guest test was run.

## Deferred WASI Measurement

The linked module was absent from the matching historical artifact; only the `hashes-wasm32` artifact was available, so it cannot supply a byte count. `cmake --build build/wasm32 --target help` exited 1 because the existing build tree had no `build.ninja`. Two isolated local setup attempts did not reach CMake configuration or module production:

1. The Ubuntu 26 Docker setup with pinned Clang 21.1.8, `wasi-libc`, and compiler runtime package versions failed package resolution: `libopencl-clang-21-dev` version `1:21.1.8-6ubuntu1` was unavailable. The process status was not retained; apt-get exit 100 is inferred.
2. The retry omitted the unresolved `clang++-21` package request, installed the compiler/runtime packages, and verified the official CMake 4.4.5 download. GNU tar then failed to create its extraction directories with `Function not implemented`. The process status was not retained; GNU tar exit 2 is inferred.

No project files were changed by either setup attempt. Further local emulation was stopped. At ship, the Ubuntu 26 WASI lane is the source of truth: capture the module size, add that measured value as the checked-in bound, and run the final changed head. Until then `TUNE-05` remains pending because its Wasm-size portion is not yet bounded.

## Deviations from Plan

**1. Made the generated state-test fixture executable.** The first GREEN control run showed that the shell fixture could not be launched as the production test binary. Added CMake `file(CHMOD ...)` for that generated fixture and reran the focused policy test successfully. No production behavior changed.

## Task Commits

- `961ee85` — `feat(07-07): enforce measured quality bounds`
- `599c8f9` — `test(07-07): add failing Wasm and state metric controls`
- `102558f` — `feat(07-07): measure WASI and state quality outputs`

## Self-Check: PASSED

- The summary exists at `.planning/phases/07-faster-and-more-trustworthy-ci/07-07-SUMMARY.md`.
- All three task commits are ancestors of the current plan head `102558f`.
- The RED evidence classifier returned `RED_EVIDENCE_OK`; Task 2 passed its focused GREEN checks.
