---
phase: 01-a-test-frame-in-retroarch
verified: '2026-10-29T18:27:06Z'
status: passed
score: 5/5 pre-merge observable truths verified
tested_revision: 384ee9d6983b700c15c0c52c3801df9ca3a4dc86
runtime: Codex, OpenGSD 1.19.0, inline verification
model: GPT-6 (session; exact model ID not exposed)
covered_files:
- .clang-format
- .env.example
- .gitattributes
- .githooks/pre-commit
- .githooks/pre-push
- .github/rulesets/main.json
- .github/workflows/ci.yml
- .github/workflows/nightly.yml
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
- .planning/phases/01-a-test-frame-in-retroarch/01-REVIEW-DISPOSITION.md
- .planning/phases/01-a-test-frame-in-retroarch/01-REVIEW-FIX.md
- .planning/phases/01-a-test-frame-in-retroarch/01-UAT.md
- .release-please-manifest.json
- AGENTS.md
- ASSET_POLICY.md
- CLAUDE.md
- CMakeLists.txt
- CMakePresets.json
- LICENSE
- PROVENANCE.md
- README.md
- THIRD-PARTY-NOTICES.md
- cmake/SchwaystationTests.cmake
- cmake/packaging.cmake
- cmake/schwaystation.pc.in
- cmake/schwaystationConfig.cmake.in
- cmake/toolchains/ppc-linux-gnu.cmake
- cmake/toolchains/wasm32-wasi.cmake
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
- pins.txt
- release-please-config.json
- runner/CMakeLists.txt
- runner/bench.c
- runner/bench.h
- runner/clock.c
- runner/clock.h
- runner/crc32.c
- runner/crc32.h
- runner/exit_status.h
- runner/main.c
- runner/png.c
- runner/png.h
- runner/rss.c
- runner/rss.h
- runner/sha256.c
- runner/sha256.h
- runner/stop.c
- runner/stop.h
- scripts/apply-repo-settings.sh
- scripts/check-title.sh
- scripts/classify-changes.sh
- scripts/fetch-pin.sh
- scripts/hygiene.sh
- scripts/publish.sh
- src/crc32.c
- src/frame.c
- src/instance.c
- src/internal.h
- src/state.c
- src/state.h
- src/testcard.c
- tests/abi/clock_fixture.c
- tests/abi/global_fixture.c
- tests/abi/inventory.txt
- tests/abi/libc_fixture.c
- tests/abi/symbol_fixture.c
- tests/abi/tests.cmake
- tests/alloc.h
- tests/api/v1.c
- tests/check.h
- tests/cmake/abi.cmake
- tests/cmake/api_version.cmake
- tests/cmake/archive_contents.cmake
- tests/cmake/budgets.cmake
- tests/cmake/check_junit.cmake
- tests/cmake/expect_fail.cmake
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
- tests/consumer/main.c
- tests/consumer/main.cpp
- tests/core/inventory.txt
- tests/core/test_core.c
- tests/core/test_state.c
- tests/core/tests.cmake
- tests/embed/CMakeLists.txt
- tests/header/header.c
- tests/header/header.cpp
- tests/hygiene/fixtures/titles.txt
- tests/hygiene/inventory.txt
- tests/hygiene/tests.cmake
- tests/install/inventory.txt
- tests/install/tests.cmake
- tests/inventory/ci.txt
- tests/inventory/hygiene.txt
- tests/inventory/nofp.txt
- tests/inventory/wasm32.txt
- tests/libretro/host.c
- tests/libretro/inventory.txt
- tests/libretro/tests.cmake
- tests/nofp/inventory.txt
- tests/nofp/nofp_fixture.c
- tests/nofp/tests.cmake
- tests/perf/allow.txt
- tests/perf/inventory.txt
- tests/perf/tests.cmake
- tests/policy/inventory.txt
- tests/policy/tests.cmake
- tests/release/inventory.txt
- tests/release/tests.cmake
- tests/retroarch/bmp.c
- tests/retroarch/compare.c
- tests/retroarch/flip.cmake
- tests/retroarch/run_retroarch.cmake
- tests/retroarch/test.cfg.in
- tests/runner/inventory.txt
- tests/runner/test_png.c
- tests/runner/test_runner.c
- tests/runner/tests.cmake
- tests/testcard_pins.cmake
- tests/tests.cmake
- version.txt
covered_digest: v4:sha256:daf756c05b93eb39c08af285441857c6efe35c456110d02791d8460bd127be29
behavior_unverified: 0
overrides_applied: 0
---

# Phase 01: A test frame in RetroArch — Verification

The phase's pre-merge implementation is verified. Its real release and the
latest-download install are confirmed by the separately invoked ship step,
as AGENTS rules 6, 9 and 10 require. This report does not claim a release exists.

Verification ran inline under the invoked verify-work workflow's missing-report
recovery path. The existing code review and fix disposition were read first;
no new source review or extra GSD step ran. AGENTS rule 6 supersedes routine
owner checkpoints. No owner test or consent remains outstanding.

## Observable Truths

| # | Truth | Status | Behavioral evidence |
|---|---|---|---|
| 1 | The library, runner and libretro core build, test and package on all six platforms; installed C/C++ and embedded C clients work. | Verified | Current hosted CI run 40929433476; every build leg's regular and release JUnit inventories pass. Local CI exits 0: 78 regular tests, two archive tests, three archives. |
| 2 | The moving test card and metadata hash identically on eight lanes; libretro delivers the reference frame, timing and audio. | Verified | Current hash-equality checks seven required files; wasm32 separately equals Linux x64. Both in-process libretro cases and converter known answers pass. |
| 3 | The README install line installs the checked core; RetroArch 1.22.2 displays exactly the runner's frame 5 and rejects the altered reference. | Verified | Current hosted retroarch job; local isolated install and tamper checks exit 0. The generated frame-5 PNG was viewed against the documented bands and counter. |
| 4 | The prerelease rehearsal exercises 18 archives, checksums, attestations and cleanup, with the production release flow wired for merge. | Verified before merge | Hosted run 39460774025 rehearsal and cleanup pass; logs confirm 18 archives, 19 assets and verified downloads, then deletion. Publishing and workflow inputs have no change since that tested revision. Real publication is checked at ship. |
| 5 | ABI/hygiene/policy controls, bench output, allocation-free frames, host-clock isolation and state round trips work, including the review fix. | Verified | Current CI, both nofp jobs, sanitizer and be32 lanes; local hygiene exits 0 with 21 tests. The new core.state.run_bounds test passes on current hosted builds, including portable lanes. |

Score: 5/5 pre-merge observable truths, zero unexercised behaviors in this phase's scope.
The current hosted run is https://github.com/szTheory/schwaystation/actions/runs/40929433476.
The rehearsal evidence is https://github.com/szTheory/schwaystation/actions/runs/39460774025.

## Artifacts and Connections

| Artifact or path | Substantive implementation and connection | Exercised by |
|---|---|---|
| src/instance.c, src/frame.c, src/testcard.c | Caller allocator creates one instance; each bounded run advances exact ticks, queues silent pairs and redraws VRAM; get_video exposes the native display through its pitch. | Core API, allocation, isolation, redraw, video tuple/pitch and audio tests. |
| src/state.c, src/state.h, src/internal.h | One field walk writes explicit little-endian values; validate precedes apply; shared inclusive counter bounds are checked before running mutates state or output. | State round trip, stable bytes, reject/record sweep, visitor and run_bounds tests. |
| runner/main.c, runner/png.c, host/xrgb8888.c | CLI creates/runs the real core, serializes tuple and native pixels to the hash, and converts current display rows to a PNG; timing stays in the runner. | Pinned region/frame hashes, decoded PNG pixels, converter known answers, usage/error/signal/bench tests. |
| libretro/libretro.c | No-content load creates the same library instance; retro_run polls options, advances it, converts its actual video and drains its audio into callbacks. Region changes recreate the instance and update AV info. | libretro.host and libretro.host.pal exercise both audio and core-option interfaces, state restore, reset and region transitions. |
| CMakeLists.txt, CMakePresets.json, cmake/, tests/inventory/ | The three deliverables link the real core; per-directory inventories select tests; configure and JUnit checks enforce the selected names and labels. Package consumers use installed headers and archive. | Current local and hosted workflows; install/embed consumers; archive tests. |
| tests/retroarch/, pins.txt, .github/workflows/ci.yml | The SHA-checked real frontend loads the built module in an isolated home and compares its screenshot with the built runner; a changed pixel proves the compare fails. | Current hosted retroarch job; 293 e2e lines, within 300. |
| scripts/publish.sh, .github/workflows/release.yml, release-please-config.json | The rehearsal prepares/uploads/attests/verifies/deletes; the production workflow uses the App, CI on the release SHA, publication by draft ID and latest-install check. | Successful hosted rehearsal and signer/ref negative controls; production wiring read against docs. |
| libretro/libretro.h, PROVENANCE.md, THIRD-PARTY-NOTICES.md | Vendored header matches the recorded SHA-256; all three seed/header commit APIs return successfully; notices are packaged. | Digest/API readbacks and release.archive_contents. |

All 15 plan commit windows reconcile exactly to their recorded counts, and each
recorded end is an ancestor of the tested branch. Every declared artifact exists
with content, and link endpoints and patterns were inspected. One plan's literal
v1.c link is generated as v${version}.c in api_version.cmake; the header selects
version 1, and the passing API coverage and negative control exercise that link.
The one-off literal-pattern assertion was corrected; no product fix was needed.

## Requirement Coverage

| Requirement | Source plans | Verification |
|---|---|---|
| FRAME-01 | 01, 08, 11, 12, 14, 15 | Six hosted builds, installed/embedded consumers, exact JUnit inventories; local ci exit 0. |
| FRAME-02 | 01, 03, 04, 12, 14 | Moving-card pins, native LE tuple/pixels, seven-file required equality plus identical report-only wasm32. |
| FRAME-03 | 03, 10, 14 | Both libretro host tests compare actual callback video/audio and exact AV rates; real frontend smoke. |
| FRAME-04 | 03, 05, 06, 07, 14 | ABI namespace/globals/imports and negative controls, C17/C++17 header checks, nofp, no_float and document budgets. |
| FRAME-05 | 06, 07, 09, 12, 14, 15 | Hygiene, policy and revision checks and controls; 1392 gate lines under 1500. Ship ratchets the limit. |
| FRAME-06 | 02, 06, 11, 13, 14 | Archive members/notices, 18-archive rehearsal, attestations/downloads, cleanup and unchanged production wiring. Merge-only publication belongs to ship. |
| FRAME-07 | 04, 11, 12, 13, 14 | Isolated README install and tamper refusal; current RetroArch frame-5 pixel equality and changed-pixel control; e2e budget. Latest URL awaits ship. |
| FRAME-08 | 05, 08, 14 | Installed C/C++ consumers, C-only embedding and every public API name exercised. |
| FRAME-09 | 03, 05, 07, 09, 13, 14 | Bench fields, allocator and clock isolation, visitor/state tests, current Cachegrind control and historical PR/base NO_BASE report; nightly success and rolling issue closure. |

The phase-1 policy intentionally keeps wasm32 outside the required seven-file
comparison and performance in report mode. Their promotion/arming remains the
separate post-release CI tail. Neither is an unresolved UAT gap.

## Evidence and Limits

The companion 01-UAT.md preserves every one of the 114 structured summary
coverage entries plus three current checks: 117 passes, zero issues, no pending,
blocked or skipped rows. It records commands, exit statuses and run URLs; raw logs
and generated images remain outside the tracked tree.

- cmake --workflow --preset ci: exit 0 (78 regular and two archive tests).
- cmake --workflow --preset hygiene: exit 0 (21 tests).
- All three local check_junit.cmake invocations: exit 0; 78, 2 and 21 tests ran once each.
- README install_line.cmake, normally and with FLIP=1: both exit 0; the altered archive's shell line exits 1 and installs no core.
- scripts/apply-repo-settings.sh --check: exit 0, no drift; read-only ruleset query confirms active CI required. The owner's earlier admin action was not repeated.
- The current hosted result and logs were read with gh run view: exit 0; successful CI required, all six builds, asan, both nofp jobs, be32, wasm32, hashes and RetroArch. Rehearsal/title skip only because this was workflow_dispatch; the earlier PR run proves those jobs.
- The historical rehearsal log and job results were read with gh run view: exit 0. The good nightly and closed rolling issue were also confirmed read-only, exit 0.

Attempt 1 of the current run hit the Windows ARM64 three-minute job timeout
after every build/test/package/upload step had succeeded. Its check annotation
was read before the affected job and downstream checks were rerun. Attempt 2
passes. No assertion, timeout or workflow was changed to obtain that result.
Todo 024 carries the timing sample into the existing phase-1 CI tail.

The actual tested revision is recorded in frontmatter; later changes in this
verification step are planning evidence only. Ship must obtain CI on its own PR
head and confirm the real release. No retail BIOS, game, executable, state file
or owner hardware is needed to verify this phase.

## Human Verification and Gaps

None. Documentation and image judgment were performed by the agent. Historical
coverage's invalid pending/not_run statuses were retained and resolved using
later-plan and hosted evidence, not silently auto-passed or sent to the owner.
The existing CR-01 review finding is fixed with its regression. WR-01 remains a
tooling seed (todo 023, phase 2 plan 1); todos 019–022 retain their existing
dispositions. No second review or gap round was introduced.

Next command: `$gsd-ship 1`. Stop after this verification step.
