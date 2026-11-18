---
phase: 01-a-test-frame-in-retroarch
verified: 2026-11-18T10:23:45Z
status: passed
score: 5/5 roadmap success criteria verified
tested_revision: 35a19da726e9fed0b9c0e4dca8c418aca29dbaff
tested_tree: 058ac8996b00488ce9bd79092b4b750ae4d33214
runtime: "Codex, OpenGSD 1.20.0, execute-phase stale-resume verification"
model: "GPT-6 (session; exact model ID not exposed)"
covered_files:
  - .clang-format
  - .gitattributes
  - .github/workflows/ci.yml
  - .github/workflows/release.yml
  - .gitignore
  - .planning/phases/01-a-test-frame-in-retroarch/01-01-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-01-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-02-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-02-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-03-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-03-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-04-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-04-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-05-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-05-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-06-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-06-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-07-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-07-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-08-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-08-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-09-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-09-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-10-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-10-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-11-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-11-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-12-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-12-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-13-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-13-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-14-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-14-SUMMARY.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-15-PLAN.md
  - .planning/phases/01-a-test-frame-in-retroarch/01-15-SUMMARY.md
  - .release-please-manifest.json
  - AGENTS.md
  - CMakeLists.txt
  - CMakePresets.json
  - README.md
  - cmake/SchwaystationTests.cmake
  - cmake/packaging.cmake
  - cmake/version.h.in
  - docs/embedding.md
  - docs/maintaining.md
  - docs/runner.md
  - docs/testing.md
  - host/xrgb8888.c
  - host/xrgb8888.h
  - include/schwaystation.h
  - libretro/CMakeLists.txt
  - libretro/libretro.c
  - libretro/libretro.h
  - libretro/schwaystation_libretro.info.in
  - release-please-config.json
  - runner/CMakeLists.txt
  - runner/main.c
  - runner/png.c
  - runner/sha256.c
  - runner/sha256.h
  - scripts/hygiene.sh
  - scripts/publish.sh
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/state.c
  - src/state.h
  - src/testcard.c
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v1.c
  - tests/cmake/abi.cmake
  - tests/cmake/api_version.cmake
  - tests/cmake/archive_contents.cmake
  - tests/cmake/budgets.cmake
  - tests/cmake/check_junit.cmake
  - tests/cmake/hashes.cmake
  - tests/cmake/install_line.cmake
  - tests/cmake/inventory.cmake
  - tests/cmake/line_budget.cmake
  - tests/cmake/no_float.cmake
  - tests/cmake/no_skip.cmake
  - tests/cmake/perf_compare.cmake
  - tests/cmake/revision.cmake
  - tests/cmake/workflows.cmake
  - tests/consumer/CMakeLists.txt
  - tests/core/tests.cmake
  - tests/embed/CMakeLists.txt
  - tests/header/header.c
  - tests/header/header.cpp
  - tests/hygiene/inventory.txt
  - tests/hygiene/tests.cmake
  - tests/install/inventory.txt
  - tests/install/tests.cmake
  - tests/inventory/ci.txt
  - tests/inventory/nofp.txt
  - tests/inventory/wasm32.txt
  - tests/libretro/host.c
  - tests/libretro/inventory.txt
  - tests/libretro/tests.cmake
  - tests/nofp/inventory.txt
  - tests/nofp/tests.cmake
  - tests/perf/inventory.txt
  - tests/perf/tests.cmake
  - tests/policy/inventory.txt
  - tests/policy/tests.cmake
  - tests/release/inventory.txt
  - tests/release/tests.cmake
  - tests/retroarch/run_retroarch.cmake
  - tests/retroarch/test.cfg.in
  - tests/retroarch/verify.py
  - tests/runner/inventory.txt
  - tests/runner/tests.cmake
  - tests/testcard_pins.cmake
  - tests/tests.cmake
  - version.txt
covered_digest: "v4:sha256:31c1df3cfd755008c3401ebbf2d9661774b7575e148263e8e13053d236cc1e3d"
behavior_unverified: 0
overrides_applied: 0
---

# Phase 01: A test frame in RetroArch — Verification Report

**Phase Goal:** Merging a pull request builds, tests, attests and publishes the library, runner and libretro core on every platform; one install line puts the core into RetroArch on a Mac, where it shows a test card that hashes identically on eight lanes with no BIOS and no disc.

**Verified:** 2026-11-18T10:23:45Z

**Status:** passed
**Re-verification:** This refresh followed the execute-phase stale-report route. The previous report had no gaps section, so this was an initial goal-backward verification, not gap closure.

The prior verification had gone stale because its source fingerprint predated later phase work. This report uses current source and plan files. No tests were run locally; the owner supplied successful hosted CI results for the exact current main source tree, and the actual release workflow is also verified below.

## Goal Achievement

### Observable Truths

| # | Roadmap success criterion | Status | Evidence |
|---|---|---|---|
| 1 | All six platform builds configure, build, test and package all three deliverables; the CI inventory runs exactly once without skips; installed C/C++, and embedded consumers build. | VERIFIED | All six matrix legs and the CI-required gate passed on the exact current source tree. Each leg ran its platform workflow preset and the regular/release JUnit inventory checks. See the release-PR CI row below. |
| 2 | The no-BIOS/no-disc test card hash is identical across eight lanes; the in-process libretro frame matches the runner conversion and AV info is correct. | VERIFIED | The exact-tree CI run passed the eight-lane hash-equality job and both libretro host tests. The runner hash and test-card pin tests are in the selected CI inventory. |
| 3 | The README install line installs on macOS arm64; RetroArch's five-frame screenshot matches the runner PNG pixel-for-pixel and the one-byte control fails. | VERIFIED | The exact-tree CI run passed RetroArch and its screenshot comparison. The subsequent release workflow passed the latest-release README install-line job on macOS. |
| 4 | Pull-request rehearsal produces 18 version-free archives, checksums and attestations, then cleans up; merge publishes the same set automatically. | VERIFIED | Rehearsal and cleanup passed in a release-input PR run with unchanged workflow and publish scripts. The current-main release workflow then published v0.6.0 with 18 ZIPs plus SHA256SUMS and passed attestation/download verification and latest-install validation. |
| 5 | ABI, hygiene and policy gates reject their negative controls; benchmark fields, per-PR Cachegrind measurement, zero post-load allocations and host-clock isolation are enforced. | VERIFIED | Exact-tree CI passed the ABI, hygiene, policy, performance, no-float and sanitizer jobs, including registered negative-control tests. Core allocation and clock invariants are covered by named tests in the all-platform inventory. |

**Score:** 5/5 roadmap success criteria verified; no behavior-unverified truths.

### Hosted CI Evidence

| Purpose | CI command or job | Result | Run |
|---|---|---|---|
| Exact-tree pull-request verification. PR head 15525817, tree 058ac8996b00488ce9bd79092b4b750ae4d33214, identical to current origin/main. Linux/macOS ran cmake --workflow --preset ci; Windows ran cmake --workflow --preset ci-msvc. The six build/package legs, both JUnit inventory checks, eight-lane hash equality, RetroArch, performance, sanitizer, no-float, big-endian and wasm32 jobs all succeeded. Rehearsal/cleanup were skipped because this release-please PR had no release-input changes. | Exit 0 for all required jobs | [Release PR CI #44253419074](https://github.com/szTheory/schwaystation/actions/runs/44253419074) |
| Main push CI. Linux/macOS ran cmake --workflow --preset ci; Windows ran cmake --workflow --preset ci-msvc. All six build/package legs, tests, hash equality, RetroArch and CI-required gate succeeded. This earlier push tree differs from current main because release metadata changed afterward; it is supplemental evidence. | Exit 0 for all required jobs | [Main CI #44253375502](https://github.com/szTheory/schwaystation/actions/runs/44253375502) |
| Release-input PR rehearsal. The rehearsal steps scripts/publish.sh prepare, artifact upload, attestation, scripts/publish.sh attested, prerelease publication and scripts/publish.sh verify succeeded; cleanup removed the rehearsal release. The workflow and publish scripts have no source changes between that run and current main. | rehearsal and cleanup jobs exit 0 | [Rehearsal CI #44246614386](https://github.com/szTheory/schwaystation/actions/runs/44246614386) |
| Actual release on current main 35a19da. The release workflow built all platform artifacts, verified attestations and checksums, published v0.6.0, and ran the README latest-release install line on macOS. The release contains 18 ZIP assets plus SHA256SUMS. | release workflow, publish and install-line-latest jobs exit 0 | [Release #44262491280](https://github.com/szTheory/schwaystation/actions/runs/44262491280) |

These are hosted results, not local test executions. The exact-tree PR run proves the current build and test source; the release workflow provides the merge-and-publish evidence that the PR run cannot provide.

### Required Artifacts and Wiring

| Artifact | Evidence in current code | Status |
|---|---|---|
| Core instance, frame and test card: src/instance.c, src/frame.c, src/testcard.c | Instance-owned integer state; bounded frame runner redraws the card into VRAM; the public video getter exposes the frame and pitch. Test-card pins and core tests are wired into CI. | VERIFIED |
| Runner and host pixel conversion: runner/main.c, runner/png.c, host/xrgb8888.c | The runner creates the real core, hashes metadata and pixel bytes, and exports PNG; shared conversion is exercised against known answers. | VERIFIED |
| Libretro adapter: libretro/libretro.c | No-content load creates the library instance; each callback advances it and sends the core's video/audio to the frontend. In-process host tests check frame bytes and AV information. | VERIFIED |
| Build, inventory and packages: CMakeLists.txt, CMakePresets.json, cmake/, tests/inventory/ | Three deliverables share the real core; inventories and JUnit checks enforce test selection; installed C/C++ consumers and add_subdirectory embedding are built in CI. | VERIFIED |
| RetroArch end-to-end path: tests/retroarch/, README.md, .github/workflows/ci.yml | CI installs the built core, launches the pinned RetroArch host with no content, and compares its captured pixels with the runner output; the negative control is registered. | VERIFIED |
| Release and provenance: scripts/publish.sh, .github/workflows/release.yml, release-please-config.json, libretro/libretro.h | Workflow builds, attests, verifies, publishes and checks the latest install; the release archive set and notices are checked. Vendored interface provenance is documented. | VERIFIED |

### Data-Flow Trace

| Rendered or hashed value | Source and path | Status |
|---|---|---|
| Test-card pixel | src/testcard.c computes pixels from integer x, y and frame inputs; src/frame.c redraws VRAM; schway_get_video exposes the buffer. | FLOWING |
| Runner hash and PNG | runner/main.c consumes the core video and metadata; runner/png.c serializes those pixels; host/xrgb8888.c provides the libretro pixel conversion. | FLOWING |
| RetroArch screenshot | libretro/libretro.c advances the core and submits converted video through the frontend callback; tests/retroarch/verify.py compares captured output with the runner reference. | FLOWING |
| Eight-lane equality | Each lane runs the real runner and emits hashes.txt; tests/cmake/hashes.cmake compares the eight files and checks the expected count. | FLOWING |

No rendered or hashed acceptance value terminates in a static response or mock.

### Plans, Must-Haves and Requirements

All 15 Phase 01 plans and summaries were read. Their frontmatter declares 137 observable truths, 51 artifact checks and 31 key links. Current code and wiring were checked; exact-tree CI exercises the declared behavior.

Two artifact checks and three key-link checks report literal-pattern mismatches because their original snapshots were superseded by later planned changes, not because implementations are missing:

- The plan's initial public API version 1 and release manifest version 0.0.0 have since advanced to API v7 and release v0.6.0. API v1 remains covered, and api_version.cmake selects the current vN test.
- The API link is generated as v plus the current version plus .c, rather than a hard-coded v1.c.
- The hygiene/policy ceiling was measured and ratcheted from 1,500 to 1,392 lines at ship; current REQUIREMENTS.md and CI enforce 1,392.
- The required hash lane count advanced from seven to eight in the phase's numbered CI tail; current requirements and the eight-lane hash-equality job use eight.

These historical literals do not contradict the current roadmap contract. All FRAME-01 through FRAME-09 IDs declared in plans are covered; there are no orphaned phase requirements.

| Plan | Requirement IDs | Code and verification trace |
|---|---|---|
| 01 | FRAME-01, FRAME-02 | Build skeleton, test inventory, runner and test-card foundations. |
| 02 | FRAME-03, FRAME-05, FRAME-06 | Libretro interface, hygiene and release controls. |
| 03 | FRAME-02, FRAME-09 | Core/state determinism, allocation and clock checks. |
| 04 | FRAME-02, FRAME-07 | Runner hashes, PNG and pixel conversion. |
| 05 | FRAME-04, FRAME-08, FRAME-09 | ABI, headers, no-float and policy controls. |
| 06 | FRAME-04, FRAME-05, FRAME-06 | Hygiene, archive and release gates. |
| 07 | FRAME-01, FRAME-04, FRAME-05, FRAME-09 | Performance, docs budgets, no-skip policy and workflow checks. |
| 08 | FRAME-08 | Installed consumers and embedded-library build. |
| 09 | FRAME-05, FRAME-09 | Runner benchmark and revision guard. |
| 10 | FRAME-03, FRAME-07 | Libretro adapter, host tests and frontend path. |
| 11 | FRAME-01, FRAME-06, FRAME-07 | Package archives and README installation. |
| 12 | FRAME-01, FRAME-02, FRAME-05, FRAME-07, FRAME-09 | Platform matrix, hash equality and RetroArch end-to-end job. |
| 13 | FRAME-06, FRAME-07, FRAME-09 | Rehearsal, cleanup, release and nightly workflows. |
| 14 | FRAME-01 through FRAME-09 | Hosted end-to-end validation and measured job costs. |
| 15 | FRAME-01, FRAME-05 | Timeout calibration, bounded follow-ups and repository ruleset. |

| Requirement | Verification | human_judgment |
|---|---|---|
| FRAME-01: six-platform build, test, package and exact test inventory | All six exact-tree CI legs and inventory checks passed; installed and embedded consumers are in the tested workflow. | false |
| FRAME-02: deterministic no-BIOS/no-disc test-card hash across eight lanes | Eight-lane hash equality passed; test-card pin and libretro-host checks passed. | false |
| FRAME-03: libretro frame and AV contract | Both in-process host tests passed in CI. | false |
| FRAME-04: ABI, imports, globals, no-float, headers and docs budgets | ABI, no-float and related gates passed in CI. | false |
| FRAME-05: hygiene, policy, pins, negative controls and size ceiling | Hygiene and policy jobs passed; current 1,392-line gate is enforced. | false |
| FRAME-06: attested release artifacts and cleanup, then automatic publication | Rehearsal/cleanup and actual v0.6.0 release runs passed; 18 archives plus checksums were published. | false |
| FRAME-07: macOS install and pixel-exact RetroArch smoke | RetroArch screenshot comparison and latest-release install-line jobs passed. | false |
| FRAME-08: installed C/C++ consumers and add_subdirectory embedding | All six platform package/build legs passed. | false |
| FRAME-09: benchmark, per-PR Cachegrind, no allocations, no host clock and state round trip | Performance, allocation, clock and state checks are in the passing CI inventory; performance job passed. | false |

### Behavioral Spot-Checks

| Behavior | Command or job | Result | Status |
|---|---|---|---|
| Six-platform workflow and exact inventory | cmake --workflow --preset ci; ci-msvc on Windows; exact-tree CI run #44253419074 | All required build, JUnit and CI-required jobs succeeded. | PASS |
| Eight-lane hash identity and libretro callback video | hash-equality and libretro.host jobs in #44253419074 | Both jobs succeeded on the exact current source tree. | PASS |
| RetroArch pixels and latest-release installation | retroarch job in #44253419074; install-line-latest in #44262491280 | Both succeeded; the release install targets the published core. | PASS |
| Rehearsal and cleanup | rehearsal and cleanup jobs in #44246614386 | Both succeeded; the tested workflow and scripts are unchanged in current main. | PASS |
| Actual publication and attestation | Release workflow #44262491280 | Published 18 archives and SHA256SUMS with verified attestations. | PASS |

No local tests or servers were started; the table records the exact hosted behavioral evidence supplied for this verification.

### Probe Execution

No plan-declared probe or conventional scripts/*/tests/probe-*.sh probe exists for this phase. Not applicable.

### Test Quality and Anti-Patterns

The exact CI inventory checks ensure selected tests run once and none are silently skipped. Test-card expected values are fixed pins from an independent one-off model, rather than values generated by the implementation under test. Synthetic package/archive writes are controlled fixtures and negative controls. The relevant phase files contain no unreferenced TBD, FIXME or XXX debt markers and no empty implementation stubs. TODO comments in the vendored libretro interface and mentions of placeholder fixture archives describe upstream declarations or deliberate test fixtures; they do not reach product behavior.

Decision coverage checked against 01-CONTEXT.md is 29/29 honored. No code review blocker or unclosed review fix was found in the refreshed source scope.

### Human Verification Required

None. The project rule assigns verification to automation; the install, RetroArch screenshot, archive contents, hash equality and negative controls are all exercised in hosted workflows. No owner action is needed for this phase.

### Gaps Summary

No gaps remain. The phase goal is achieved by the exact-tree CI evidence plus the successful current-main release and latest-install workflow. The older verification's stale fingerprint and superseded literal snapshots did not reveal missing implementation.

---

_Verified: 2026-11-18T10:23:45Z_
_Verifier: gsd-verifier_
