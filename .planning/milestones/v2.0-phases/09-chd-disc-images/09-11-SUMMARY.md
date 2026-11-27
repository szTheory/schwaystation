---
phase: 09-chd-disc-images
plan: 11
subsystem: ci
tags: [CHD, CMake, GitHub Actions, act, fuzzing, release]
requires:
  - phase: 09-13
    provides: CHD ON/OFF package and notice validation
  - phase: 09-15
    provides: External native CHD generator handoff for cross builds
provides:
  - CHD-enabled required CI/package presets and per-job cross-lane fixture generation
  - Bounded CHD sanitizer fuzz campaign with an owned baseline fixture
  - Fast CI preflight and documented CI/CD cost review, including measured Main/Release disposition
affects: [Phase 9 final verification, Phase 10 provider integration]
actuals:
  tokens: 7947
  tasks: 3
  commits: 3
tech-stack:
  added: []
  patterns:
    - Generate platform-specific CHD provenance beside each job's build tree and keep it outside fixture byte hashes.
    - Configure the nightly fuzz tree once for owned-fixture generation, then build the remaining targets from that tree.
key-files:
  created:
    - .planning/phases/09-chd-disc-images/09-11-SUMMARY.md
    - .planning/todos/completed/050-release-ci-artifact-reuse.md
  modified:
    - CMakePresets.json
    - .github/workflows/ci.yml
    - .github/workflows/nightly.yml
    - tests/cmake/workflows.cmake
    - docs/maintaining.md
key-decisions:
  - Keep the Main and Release CI runs separate; measured outputs differ across all 18 product archives and serializing them adds 24m25s to the observed release duration.
  - The first CHD-enabled hosted WASI module measured 859,379 bytes; update the exact bound from the valid producer output and verify the new bound on a green exact-head run.
  - Use Act dry runs for route evidence only; the CI classifier outputs and hosted integrations still require GitHub Actions.
patterns-established:
  - Run cheap source, pin, timeout, inventory, budget and route checks in the existing changes job before dependent CI work.
  - Keep disc bytes inside each job and avoid a second CMake configure when a workflow has already configured its tree.
requirements-completed:
  - CHD-01
  - MEDIA-02
  - COMPAT-02
coverage:
  - id: D1
    description: Required CI and package presets enable CHD with native or handed-off native fixture generation, while preserving eight-lane byte comparison and exact-release-SHA gates.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "cmake -DCASE=disc_registration -DROOT=. -P tests/cmake/workflows.cmake; cmake --list-presets, --list-presets=build, --list-presets=test and --list-presets=workflow; act pull_request --dryrun for ci.yml; all exited 0."
        status: pass
      - kind: integration
        ref: "Run 44827711416 built and validated the CHD-enabled WASI module at 859379 bytes, then failed the previous CHD-off 608540-byte bound. Run 44830485927 confirmed the Windows CRT-header collision fix was incomplete; the final CMake include-scope correction has local compile-graph evidence, with hosted Windows/WASI/eight-lane verification pending. https://github.com/szTheory/schwaystation/actions/runs/44827711416; https://github.com/szTheory/schwaystation/actions/runs/44830485927"
        status: unknown
    human_judgment: false
  - id: D2
    description: CI and nightly run the reusable cheap preflight, and maintainer docs describe route limits and measured CI/CD efficiency review.
    requirement: MEDIA-02
    verification:
      - kind: other
        ref: "Workflow pin, timeout, route, inventory, diff and line-budget preflights exited 0; 3,901/3,901-line cap; act pull_request --dryrun for ci.yml and nightly.yml exited 0."
        status: pass
    human_judgment: false
  - id: D3
    description: Nightly fuzz wiring adds a bounded CHD campaign with 12 checked seeds and maximum input length 16,384 bytes while retaining prior campaigns.
    requirement: CHD-01
    verification:
      - kind: other
        ref: "disc_registration route selector and nightly act dry run exited 0; direct hosted sanitizer campaign remains pending final exact-head verification."
        status: pass
      - kind: integration
        ref: "Run 44830486029 on exact head 83a27a4 passed the owned fixture build and bounded campaigns, including the CHD target, after dependency installation was moved before compilation. https://github.com/szTheory/schwaystation/actions/runs/44830486029"
        status: pass
    human_judgment: false
duration: not measured
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 11: CHD CI and release wiring Summary

**Required CI lanes now build and test CHD from job-local fixtures, and nightly runs a bounded CHD fuzz campaign.**

## Performance

- **Duration:** Not measured; the inherited executor did not preserve a reliable start timestamp or active-time log.
- **Completed:** 2026-11-20 19:46 UTC.
- **Tasks:** 3.
- **Implementation files:** 7, including the completed artifact-reuse review.
- **Iteration cost:** Five cheap preflight groups completed in under 0.13 seconds each; the CMake classifier selector passed in 0.06 seconds. The Act dry runs completed together in about 15 seconds. No full suite or hosted run was repeated for closeout.
- **Efficiency improvement:** Nightly configures the fuzz tree to create its owned fixture, then builds the fuzz preset directly. Replacing the second `cmake --workflow --preset fuzz` with `cmake --build --preset fuzz` avoids repeating configure; a time saving was not measured because the local clang-21 fuzz toolchain is unavailable.

## Accomplishments

- Enabled CHD in the applicable CI and package presets while keeping the standalone library preset CHD-off and the native generator separate for BE32/WASI. Each cross job copies a host-native generator, creates its own fixture and manifest, builds the target, and runs its existing test inventory. The release workflow remains unchanged.
- Added the existing eight-file hash-equality and lane route assertions to the updated static selector, without growing the gate beyond its 3,901-line limit.
- Added the CHD fuzz corpus to the existing nightly sanitizer campaign: 12 replay seeds, 16,384-byte maximum input, and the unchanged 60-second campaign. Existing fuzz campaigns remain present.
- Added source, pin, timeout, sorted inventory, budget and route preflight to the existing CI changes job and nightly fuzz job. Updated maintainer instructions to explain Act’s classifier limitation, hosted-only checks, and the evidence-based CI/CD review.
- Closed todo 050 with the measured conclusion to retain separate Main and Release gates. Release exact-SHA validation, all 18 archive names, checksum/attestation verification and the install check remain wired.

## Verification

| Check | Exit | Result |
|---|---:|---|
| Action pins, workflow timeouts, route selector, sorted inventories, line budget, preset listings and diff checks | 0 | All pass; the budget is 3,901/3,901 lines. The route selector checks the eight-file hash gate and edited BE32/WASI/nightly routes. |
| `cmake --preset ci`; `ctest --preset ci --no-tests=error -R '^ci[.]classify_policy$' --output-on-failure` | 0 | CHD and native tool options were enabled; classifier selector passed 1/1 in about 0.06 seconds. |
| `act pull_request --dryrun -W .github/workflows/ci.yml` with all hosted labels mapped to `ubuntu:latest` | 0 | Act simulated changes, hygiene, title and required aggregate only. It does not execute the classifier, resolve its outputs, or prove dependent matrix jobs. |
| `act pull_request --dryrun -W .github/workflows/nightly.yml` with all hosted labels mapped to `ubuntu:latest` | 0 | New preflight, generator, fixture and bounded campaign steps were routable; no workflow step executed. Both successful dry runs used `XDG_CACHE_HOME=/tmp/schwaystation-act-cache`. |
| Release workflow review | 0 | No `release.yml` diff. The release CI call still uses the Release Please SHA; publish retains its 18-archive, checksum and attestation validation; `install-line-latest` remains wired. |

No real Act job was started. The available Act image is ARM64 on this host and lacks CMake; local clang-21/wasm-ld and Windows/MSVC are absent. Exact-head run 44827711416 measured the valid CHD-enabled WASI module at 859,379 bytes, then failed the old 608,540-byte CHD-off bound; Windows x64 and arm64 failed because the pinned C++ sources reached MSVC `<limits>` before its floating-point macros were defined. Its two Windows failures and the WASI failure were confirmed before canceling the remaining known-failing tail. The run consumed 1,684 raw runner-seconds (28.1 runner-minutes) over 268 seconds wall time. Nightly run 44827711388 passed vectors and all four benchmark jobs but failed its fuzz job when the owned fixture generator lacked FFmpeg development packages; it consumed 351 raw runner-seconds (5.9 runner-minutes) over 85 seconds wall time. Raw runner time is not billing-weighted cost.

The follow-up sets the WASI bound to the measured CHD-enabled size and installs the sanitizer/FFmpeg development packages once before expensive Nightly compilation. Its first Windows repair force-included `float.h`, but hosted compile commands showed the name resolving to vendored FLAC's private header. The current reviewed correction confines FLAC private/protected include paths to FLAC C sources; local compile commands show all 21 pinned C++ sources exclude them, all 17 FLAC C sources retain them, and the CHD tool rebuilt 85/85 actions. The second attempt's exact-head Nightly passed the bounded sanitizer campaign. Four benchmarks and the full-vector job, unrelated to CHD, consumed 322 raw runner-seconds (5.4 runner-minutes); the measured router remains assigned to Phase 10 todo 051. The next exact-head hosted rerun remains required for Windows, WASI, all eight parity lanes and the corrected source graph. Release permissions, attestations and publication remain ship-time hosted evidence.

## Task Commits

1. **Task 1: Route identical checked CHD bytes to every parity lane** — `0fc7f50`.
2. **Task 2: Preserve release guarantees and add recurring fuzz coverage** — `6e60d85`.
3. **Task 3: Check workflow routing locally before hosted verification** — Act dry runs and evidence recorded above.

Follow-up diagnostic correction: `82606fc` makes the shared workflow selector name its active case in pass and failure messages. The plan was present before implementation; this closeout adds the summary and updates STATE/ROADMAP.

## Deviations and Issues

- The first Act invocation used the protected default cache path and exited 1 before completing the dry run. Setting `XDG_CACHE_HOME=/tmp/schwaystation-act-cache` fixed the environment issue; both final dry runs exited 0.
- The route selector printed `spu_registration` even when invoked as `disc_registration`. Its existing diagnostics now use `${CASE}`; both modes passed afterward with no budget increase.
- The hosted WASI measurement and all eight hosted parity lanes remain pending by plan. The size bound was not loosened to compensate for the missing local toolchain.

## Next Plan Readiness

Plan 09-11 is complete. Plan 09-12 is next and adds runner/libretro integration. Plan 09-14 completes owned-guest parity and ledger evidence. Then review all Phase 9 source before final local and hosted verification; no owner UAT is required.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*
