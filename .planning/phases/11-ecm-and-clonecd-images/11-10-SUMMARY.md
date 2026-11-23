---
phase: 11-ecm-and-clonecd-images
plan: "10"
subsystem: testing
tags: [libFuzzer, ECM, CloneCD, CMake, CTest, ASan, UBSan]
requires:
  - phase: 11-07
    provides: owned legacy fixtures, fixture preflights and shared replay conventions
provides:
  - ECM and CCD production-reader replay targets with named malformed and valid inputs
  - Sanitizer campaign target and preset wiring for ECM and CCD
  - Seed writer rejection when a positive fixture cannot produce a nonempty seed
affects: [phase-11, fuzzing, disc-image-testing]
actuals:
  tokens: 8867
  tasks: 2
  commits: 2
  plan_head_before: 13798a0d9b5c67b67045b7c6abd4a1bb4d305da2
  plan_head_after: 2aee10a4607ac332c6b263e881f7f29137e27a2c
tech-stack:
  added: []
  patterns:
    - Use production parser replay binaries to exercise the same named seed generators used by sanitizer campaigns.
    - Reject empty seed generation before opening a corpus output file.
key-files:
  created:
    - tests/fuzz/ecm.c
    - tests/fuzz/ccd.c
  modified:
    - tests/fuzz/tests.cmake
    - tests/fuzz/inventory.txt
    - tests/fuzz/replay_main.h
    - CMakePresets.json
key-decisions:
  - Do not mark ECM-01 or CCD-01 complete until the remaining phase evidence and final verification exist.
  - Preserve the ignored build/fuzz cache and corpora; use a separate build root because its cache points at an old checkout.
  - Use a disposable local Linux container to verify sanitizer campaign binaries because the host Apple Clang lacks libFuzzer.
patterns-established:
  - Report named seed count, per-seed names, cap and positive fixture closure from seed-writing mode.
requirements-completed: []
coverage:
  - id: D1
    description: ECM valid and malformed seeds replay through the production dispatcher and direct reader with bounded reads and retained-reader checks.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.](ecm|ccd)$' --output-on-failure; exit 0, 4/4"
        status: pass
    human_judgment: false
  - id: D2
    description: CCD descriptor, companion and SUB-transpose seeds replay through the production dispatcher and direct reader.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.](ecm|ccd)$' --output-on-failure; exit 0, 4/4"
        status: pass
    human_judgment: false
  - id: D3
    description: ECM and CCD campaign targets share their replay source and production reader; seed output and fixture closure are inspectable.
    verification:
      - kind: other
        ref: "Both replay --write commands produced 10 exact named seeds within reported caps; missing positive fixture probes exited 1 without writing corpus files."
        status: pass
      - kind: other
        ref: "Clang 21 ASan/UBSan configure and build of both campaign targets, both replay targets, and legacy fixture target in disposable Ubuntu 26.04 container; exit 0, 104 Ninja steps."
        status: pass
      - kind: other
        ref: "Both sanitizer campaigns loaded ten named seeds and exited 0 with -runs=1; no timed campaign run."
        status: pass
    human_judgment: false
duration: 1h 10m 25s
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 10: Production ECM and CCD fuzz replay Summary

ECM and CCD replay and sanitizer campaign targets exercise the same named inputs through the production readers, with bounded seeded smoke runs on the Linux libFuzzer runtime.

## Performance

- **Duration:** 1h 10m 25s wall time
- **Started:** 2026-11-22T17:16:28Z
- **Completed:** 2026-11-22T18:26:53Z
- **Tasks:** 2 of 2 complete
- **Files changed:** 6
- **Runtime/model:** Codex with OpenGSD 1.20.0; configured executor model `gpt-6-luna`; primary runtime model ID unavailable
- **Bottleneck:** Provisioning and configuring the disposable Linux toolchain took most elapsed time; the initial CMake attempt also exposed a missing `curl` executable, installed only inside the task container.

## Accomplishments

- Added ten named ECM seeds and ten named CCD seeds covering valid inputs, malformed metadata/records, extents, bounds, companion failures and SUB transposition.
- Registered portable replay tests with the existing legacy fixture preflight/parity dependencies and added the ECM/CCD targets to the fuzz preset while preserving all 18 prior targets.
- Added seed metadata output and fixed the replay writer to fail on a zero-length seed before creating a corpus file. Strengthened the CCD scope-escape seed with a valid outside SUB companion.

## Task Commits

1. **Task 1: Replay hostile encoded records and descriptors through production readers** - `12008bd` (feat)
2. **Task 2: Compile matching sanitizer campaigns and prove seed preflight** - `2aee10a` (feat)

The plan ledger records base `13798a0d9b5c67b67045b7c6abd4a1bb4d305da2`; two task commits were measured through `2aee10a4607ac332c6b263e881f7f29137e27a2c`.

## Verification Evidence

| Command or check | Result |
| --- | --- |
| `LC_ALL=C sort -cu tests/fuzz/inventory.txt` | Exit 0 |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` | Exit 0, 3,899/3,901 lines |
| `python3 -m json.tool CMakePresets.json` and fuzz-target comparison | Exit 0; 18 prior targets preserved, four ECM/CCD targets added |
| `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` | Exit 0 |
| `cmake --build --preset ci --target schwaystation-fuzz-ecm-replay schwaystation-fuzz-ccd-replay schwaystation-test-disc-legacy schwaystation-legacy-disc-fixtures` | Exit 0 |
| `ctest --preset ci --no-tests=error -R '^fuzz[.]regress[.](ecm|ccd)$' --output-on-failure` | Exit 0; fixture preflight, parity and both replay tests passed 4/4 in 0.32s |
| Alternate fuzz configure with `/usr/bin/clang` and `/usr/bin/clang++` | Exit 0 in 11.5s; used a separate build root to preserve the stale ignored `build/fuzz` artifacts |
| Direct sanitizer-instrumented ECM and CCD replay binaries | Exit 0; both reported 10 inputs |
| Both replay binaries with `--write` | Exit 0; each wrote ten exact named seeds. ECM max file was 2,097,158 bytes under reported cap 2,099,201; CCD max was 886 bytes under cap 65,541. |
| Missing positive-fixture probes for both seed writers | Exit 1 as expected; neither wrote a corpus file |
| Missing corpus-input probes for both replay binaries | Nonzero as expected |
| Clang 21 ASan/UBSan configure and build of both campaign targets, both replay targets and legacy fixture target in a disposable Ubuntu 26.04 container | Exit 0; 104 Ninja steps. Explicit CMake settings matched the fuzz preset because the ignored local `CMakeUserPresets.json` uses schema 11. |
| `schwaystation-fuzz-ecm-replay --write /tmp/plan11-10-ecm-corpus` and `schwaystation-fuzz-ccd-replay --write /tmp/plan11-10-ccd-corpus` in the container | Exit 0; both formats wrote exactly ten named seeds with fixture closure and caps reported. |
| `schwaystation-fuzz-ecm -runs=1 /tmp/plan11-10-ecm-corpus` and `schwaystation-fuzz-ccd -runs=1 /tmp/plan11-10-ccd-corpus` in the container | Exit 0; both loaded ten seeds and completed bounded smoke runs; no timed campaign was run. |
| `git diff --check 13798a0..HEAD`, inventory sort and 3901-line budget | Exit 0; 3,899/3,901 lines |

The explicit container build used the tracked fuzz preset's effective settings:

```sh
cmake -S . -B /tmp/schwaystation-plan11-10-fuzz-build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_COMPILE_WARNING_AS_ERROR=ON -DCMAKE_C_COMPILER=clang-21 -DCMAKE_CXX_COMPILER=clang++-21 "-DCMAKE_C_FLAGS=-O1 -g -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer" "-DCMAKE_CXX_FLAGS=-O1 -g -fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined -DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address,undefined -DCMAKE_MODULE_LINKER_FLAGS=-fsanitize=address,undefined -DSCHWAYSTATION_TIMEOUT_SCALE=3 -DSCHWAYSTATION_INVENTORY=asan -DSCHWAYSTATION_FIXTURES=ON -DSCHWAYSTATION_FUZZ=ON -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF -DSCHWAYSTATION_CHD_TOOL_EXECUTABLE=/tmp/schwaystation-chd-tool-build/tools/schwaystation-chd-tool
cmake --build /tmp/schwaystation-plan11-10-fuzz-build --target schwaystation-fuzz-ecm schwaystation-fuzz-ccd schwaystation-fuzz-ecm-replay schwaystation-fuzz-ccd-replay schwaystation-legacy-disc-fixtures
```

The host's Apple Clang cannot link libFuzzer (`libclang_rt.fuzzer_osx.a` is absent). To complete the campaign build gate without changing the project toolchain, the source was copied read-only into a named disposable local Ubuntu 26.04 container. Clang 21, its Fuzzer runtime, CMake/Ninja and fixture prerequisites were installed only in that container. Its CMake 4.2.3 did not accept the ignored local `CMakeUserPresets.json` schema 11, so the tracked fuzz preset's settings were passed explicitly. The actual CHD tool was built in-container and supplied as the prebuilt tool prerequisite. Campaign and replay targets linked, and bounded seeded smoke runs passed. The named container was removed afterward; no image, volume, or other Docker resource was retained.

An additional focused CTest attempt under the sanitizer fuzz configuration failed in its inherited `disc.image.legacy_preflight` fixture setup with an ASan heap-buffer-overflow in `tests/disc/legacy.c:replace_manifest_field`; the dependent parity and replay tests did not run in that attempt. The current CI-preset selector above passed all four tests, and no unrelated fixture code was changed.

Act 0.2.87 parsed the Nightly fuzz job and an explicit `ubuntu-26.04=ubuntu:26.04` dry run routed the jobs, but Act exited before job steps while trying to clone `actions/upload-artifact` into a sandbox-blocked action-cache path. The dry run did not start a container or pull an image. The separately authorized local container build established Clang 21/libFuzzer campaign coverage. No hosted CI was started.

## TDD Gate Compliance

- **Task 1 RED:** No production parser defect was established before the replay harness work; existing ECM/CCD parser tests already cover the format behavior. Per the executor's direction, no failing selector/build was manufactured. Task 1 therefore did not have a valid parser RED before its implementation commit.
- **Task 2 RED:** A real missing-fixture probe found both `--write` commands returned success and created empty/partial seed files. The writer now rejects zero-length seed builds before opening files. The same probes now exit 1 and write no corpus files. This was a direct executable check, not a canonical test-runner RED record or separate test commit.
- **GREEN:** The focused CI selector passed 4/4; direct sanitized replay, seed writing, cap/count/name checks, missing-fixture rejection, Linux campaign linking and one-run seeded sanitizer smoke checks passed.
- **REFACTOR:** Source review found the CCD escape sentinel could be rejected as short even if an external file had been opened. The sentinel now uses the valid generated SUB file, so following the escape would make the otherwise-valid descriptor succeed and trip the expected rejection. The focused CI selector passed after this correction.

## Deviations from Plan

### Auto-fixed issues

1. **Rule 2 - Missing critical validation:** The existing replay writer treated zero-length generated seeds as successful corpus files. It now reports seed-build failure and exits nonzero without opening the file. Verified by hiding the generated positive fixture in a reversible build-only probe for both formats.
2. **Plan correction:** Replaced the CCD escape-case outside sentinel with a valid SUB fixture so an accidental external open cannot pass merely by rejecting a short file.

### Execution deviations

- The plan's strict Task 1 RED/GREEN order was not followed because no parser defect was demonstrated and the owner directed that no RED be manufactured. The Task 2 missing-fixture behavior did have a real failing probe before its correction; the actual order is recorded above.
- The standard ignored fuzz build root was preserved because its cache points at an old `/work` source tree. Apple Clang compiled campaign objects but lacks its libFuzzer runtime. The disposable Linux container used a fresh `/tmp` binary directory and explicit settings matching the tracked fuzz preset; this passed both campaign links and bounded seeded smoke checks.
- The sanitizer fuzz CTest's shared fixture setup exposed an ASan issue in unchanged `tests/disc/legacy.c`; the CI-preset focused selector passed. This unrelated issue remains out of scope for Plan 10.
- The ASan bounds issue is recorded in `deferred-items.md` for Plan 11-12 to triage if its Nightly/ASan wiring exercises the affected fixture setup.
- No 60-second campaign, broad suite or hosted CI was run.

**Total deviations:** Two scoped correctness adjustments, documented TDD ordering adjustments, and a disposable container toolchain to complete the planned campaign build.
**Impact:** Both tasks are complete. The focused CI selector, campaign builds and bounded seeded smoke checks pass. One unrelated ASan fixture-preflight issue is documented for subsequent plan triage.

## Next Step

Plan 11-11 is dependency-ready. During phase closeout, triage the separate ASan failure in `tests/disc/legacy.c:replace_manifest_field` if the Nightly/ASan wiring exercises that fixture preflight.

## Self-Check: PASSED

- Summary file exists.
- Task commits `12008bd` and `2aee10a` are ancestors of HEAD.

---
*Phase: 11-ecm-and-clonecd-images*
*Plan: 10*
*Recorded: 2026-11-22*
