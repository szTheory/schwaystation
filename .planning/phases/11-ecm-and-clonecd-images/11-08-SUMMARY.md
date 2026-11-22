---
phase: 11-ecm-and-clonecd-images
plan: "08"
subsystem: runner
tags: [ECM, CloneCD, disc-probe, CMake, CTest]
requires:
  - phase: 11 plan 07
    provides: generated ECM and CloneCD fixtures, manifests, and parity-gated preflights
provides:
  - runner probe and guest paths for direct ECM, ECM CUE children, M3U CUE entries, and CloneCD
  - source-versus-reader guest and save identity checks with matching subcode semantics
  - direct and nested malformed-media diagnostics while retaining PBP and CHD policy refusals
affects: [Phase 11 plans 09-12, runner documentation]
actuals:
  tokens: 8286
  tasks: 2
  commits: 4
  plan_head_before: 0dff03597f3ad1cbbece87b192c0c042ca5e6903
  plan_head_after: cdcd1b37d3e3b1f5d0f59748223669d84e035e09
tech-stack:
  added: []
  patterns:
    - CUEs with ECM children use the shared image reader; ordinary raw CUEs retain their existing parser
    - runner identity tests compare media only when their SUB inputs match
key-files:
  created: []
  modified:
    - host/disc_probe.c
    - tests/runner/tests.cmake
    - docs/runner.md
    - docs/discs.md
key-decisions:
  - Route CUEs that name ECM files through the shared image reader and leave ordinary CUE handling unchanged.
  - Compare CCD save identity against the same IMG with optional SUB absent; keep the generated SUB parity and guest checks on the full CCD fixture.
requirements-completed: []
coverage:
  - id: D1
    description: Runner probe, M3U, bounded guest output, and save identity cover supported ECM paths.
    requirement: ECM-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^runner[.]disc[.]legacy$|^runner[.]disc[.]unsupported$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: Runner probe and guest loading cover supported CloneCD paths and malformed-media refusals.
    requirement: CCD-01
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^runner[.]disc[.]legacy$|^runner[.]disc[.]unsupported$' --output-on-failure"
        status: pass
    human_judgment: false
duration: 33m32s
completed: 2026-11-22
status: complete
---

# Phase 11 Plan 08: Runner/probe and content identity Summary

**The runner now probes and loads fixture-supported ECM and CloneCD media, with identity checks that account for optional SUB data.**

## Performance

- **Duration:** 33m32s through final targeted verification; the implementation and test-oracle diagnosis exceeded the 30-minute target.
- **Started:** 2026-11-22T16:00:47Z
- **Final verification completed:** 2026-11-22T16:34:19Z
- **Tasks:** 2
- **Files modified:** 4
- **Measured diff:** 33,146 characters, or 8,286 chars/4 estimate tokens; four task commits.
- **Runtime/model:** OpenGSD 1.20.0 Codex executor; configured `gpt-6-luna` xhigh. The primary runtime model identifier was not exposed.
- **Bottleneck:** Diagnosing the CUE-to-shared-reader route and correcting the CCD save-identity oracle for different SUB bytes.

## Accomplishments

- Routed direct ECM and CCD probes through the shared disc image reader. CUE sheets with ECM children now use that reader too; ordinary raw CUE handling remains unchanged.
- Added direct, case-insensitive, M3U, bounded guest-output, and separate-save identity checks for ECM and CloneCD paths.
- Replaced obsolete planned-version refusals with malformed ECM/CCD checks for truncation, invalid EDC, missing IMG, unsupported CCD version/session, canonical child escape, and unproven ECM 2336 mapping. Existing PBP and CHD refusals remain.
- Documented runner entry points and the supported CloneCD descriptor, companion, and SUB subset.

## Verification Evidence

- `LC_ALL=C sort -cu tests/runner/inventory.txt` — exit 0.
- Fixture extent/count preflight — exit 0; ECM source: 64 sectors; CCD IMG: 222 sectors; SUB: 222 × 96 bytes; source, requested, and produced counts matched the manifests.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` — exit 0, 3,898/3,901 lines.
- `cmake --preset ci -DSCHWAYSTATION_CHD=OFF -DSCHWAYSTATION_BUILD_CHD_TOOL=OFF` — exit 0.
- `cmake --build --preset ci --target schwaystation-run schwaystation-run-test schwaystation-legacy-disc-fixtures` — exit 0.
- `ctest --preset ci --no-tests=error -R '^runner[.]disc[.]legacy$|^runner[.]disc[.]unsupported$' --output-on-failure --output-junit tests/plan-11-08-final.xml` — exit 0, 4/4 tests passed in 19.30 seconds, including the legacy preflight and parity setup tests.
- `git diff --check HEAD` — exit 0. No full suite, Docker, Act execution, or hosted CI was needed for this plan.

## Task Commits

1. **Task 1 test slice:** `23d1c48` — direct ECM source identity probe check.
2. **Task 1 route:** `c7126f8` — route ECM and CCD through the shared probe.
3. **Task 1 expansion and CUE route:** `2123375` — verify runner paths and use the shared reader for ECM CUEs.
4. **Task 2 malformed-media and docs:** `cdcd1b3` — cover malformed legacy media and supported-subset documentation.

**Plan metadata:** committed after summary and state updates.

## TDD Gate Compliance

- The direct ECM test was committed first. The focused CTest selector then exited 8 because the source CUE passed while direct ECM still returned the obsolete unsupported-format result. `gsd_run check tdd-red-evidence` classified the evidence as `RED_EVIDENCE_OK` for `runner.disc.legacy`.
- The shared direct-media route was committed, and the tracer selector passed 3/3 with the required fixture setup tests. Expanded cases exposed that ECM CUEs still entered the raw-file parser; the shared-reader CUE route and a zero-initialized manifest fix were included in the later Task 1 commit.
- A CCD save-identity comparison against a source CUE without SUB was an invalid oracle because the identity includes SUB bytes. The test now compares a no-SUB CCD copy to the matching IMG-backed CUE, while the original CCD fixture still checks guest output and generated SUB parity. No invalid-oracle failure is presented as a product RED.
- Task 2 did not manufacture a RED; its final malformed-media and retained-policy selector passed.

## Decisions Made

- Use the shared media reader only for CUEs that name ECM files, preserving the existing raw CUE path.
- Treat a missing SUB as zero subcode for the save-identity parity case; a CCD with generated SUB has a different content identity and is checked separately.
- Keep the full CTest setup chain for the runner legacy test, so source counts and provider parity pass before guest execution.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Route ECM CUEs through the shared image reader**
- **Found during:** Task 1 expansion
- **Issue:** The host probe's raw CUE parser treated compressed ECM child bytes as raw track data.
- **Fix:** Detect ECM children and route those CUEs through `schway_disc_image_open`; retain the prior raw CUE path.
- **Files modified:** `host/disc_probe.c`
- **Verification:** Final legacy selector passed 3/3 including setup fixtures and ECM-CUE source parity.
- **Committed in:** `2123375`.

**2. [Rule 1 - Bug] Initialize the CUE manifest before scanning ECM children**
- **Found during:** Task 1 expansion
- **Issue:** The first helper version called the parser with an uninitialized manifest, so even a raw source CUE failed metadata validation.
- **Fix:** Zero-initialize the manifest before parsing.
- **Files modified:** `host/disc_probe.c`
- **Verification:** Rebuilt the two runner targets and the final legacy selector passed.
- **Committed in:** `2123375`.

**3. [Rule 1 - Test oracle] Match CCD SUB semantics in save-identity comparison**
- **Found during:** Task 1 expansion
- **Issue:** The raw source CUE omitted SUB while the CCD fixture includes generated SUB; the two media therefore have different save identities.
- **Fix:** Compare the matching IMG-backed CUE with an otherwise identical CCD copy without optional SUB; retain the real SUB fixture for guest and provider parity.
- **Files modified:** `tests/runner/tests.cmake`
- **Verification:** Final legacy selector passed 3/3.
- **Committed in:** `2123375`.

**4. [Rule 3 - Blocking test wiring] Pass Python to the malformed-media runner test**
- **Found during:** Task 2 verification
- **Issue:** The new runtime ECM mutations needed Python, but the CTest registration did not pass the configured interpreter.
- **Fix:** Pass `Python3_EXECUTABLE` into `runner.disc.unsupported`.
- **Files modified:** `tests/runner/tests.cmake`
- **Verification:** Final unsupported selector passed 1/1; combined final selector passed 4/4.
- **Committed in:** `cdcd1b3`.

**Total deviations:** 4 focused fixes (two implementation, two test wiring/oracle corrections). No planned behavior case was removed.
**Impact on plan:** All local plan checks passed. The 30-minute target was exceeded during diagnosis; implementation was complete, so no remaining scope was split.

## Issues Encountered

- The first CUE escape assertion expected `missing or escaping FILE child`; the actual stable diagnostic was `unsafe FILE child path`, and the test was updated to match it.
- No source review finding remained open. No external setup, Docker, hosted CI, or owner action was required.

## Next Plan Readiness

Plan 11-09 is dependency-ready. Phase-level code review and final verification remain with the orchestrator; ECM-01 and CCD-01 are not marked complete from these local checks alone. No hosted CI run is active.

## Self-Check: PASSED

- Summary file exists and has no trailing whitespace; tracked changes pass `git diff --check`.
- All four task commits are ancestors of the plan head.
- No personal paths or email addresses were found in the summary, runner docs, disc docs, or runner tests.
