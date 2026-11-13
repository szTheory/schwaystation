---
phase: 05-the-sound-matches-the-console
plan: "04"
subsystem: testing
tags:
  - cmake
  - ctest
  - ps1-tests
  - spu
  - provenance
requires:
  - phase: 05-the-sound-matches-the-console
    plan: "01"
    provides: Bounded SPU frame and PCM test surfaces
provides:
  - Identity-checked, build-local SPU memory-transfer executable and hardware log
  - Strict seven-case hardware-log matcher and malformed-report controls
affects:
  - 05-16
  - 05-05
  - SOUND-01
tech-stack:
  added: []
  patterns:
    - Verify pinned archive, member, and log digests before admitting hardware evidence
    - Match exact ordered hardware case lines, successful exit, and completion marker
key-files:
  created:
    - cmake/SPUReferences.cmake
    - tests/spu/reference.c
  modified:
    - pins.txt
    - tests/spu/tests.cmake
    - tests/spu/inventory.txt
    - PROVENANCE.md
    - ASSET_POLICY.md
key-decisions:
  - Keep the build-158 test executable in build storage because it contains MPL-2.0 runtime bytes.
  - Treat the seven transfer lines as transfer/status/timing evidence and not as a PCM oracle.
  - Require exact case order, exit code zero, and the final Done. marker before accepting a transcript.
patterns-established:
  - Asset fetches verify both the pinned archive and the selected member before use.
  - Hardware logs are accepted only by exact line matching with explicit negative controls.
requirements-completed:
  - SOUND-01
commits: 3
plan_head_before: 99c966ef80d57755b5317e4648d3bf318a8bd0ed
plan_head_after: ba7b109e0aced9727e94a38d135dc01d74bf76a1
actuals:
  tokens: 7883
  tasks: 2
  commits: 3
duration: "under 40 minutes; exact start time was not persisted"
completed: 2026-11-13
status: complete
coverage:
  - id: D1
    description: Pinned transfer executable and log are checked and prepared under build storage.
    requirement: SOUND-01
    verification:
      - kind: integration
        ref: "cmake --preset ci (exit 0); ctest --test-dir build/ci -R '^spu\\.reference\\.(fetch_controls|log)$' --no-tests=error --output-on-failure (exit 0)"
        status: pass
    human_judgment: false
  - id: D2
    description: Only a complete, ordered seven-case transcript with a successful process exit is accepted.
    requirement: SOUND-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^spu\\.reference\\.controls$' --no-tests=error --output-on-failure (exit 0)"
        status: pass
    human_judgment: false
---

# Phase 5 Plan 4: Trace pinned assets through identity checks and seven-case parsing Summary

**Build-local ps1-tests transfer assets are digest-checked, and the matcher accepts only complete ordered hardware cases.**

## Performance

- **Duration:** Under 40 minutes; the exact plan start time was not persisted. The preceding state update was at 2026-11-13T22:11:07Z.
- **Completed:** 2026-11-13
- **Tasks:** 2
- **Files modified:** 7
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh

## Accomplishments

- Verified the public ps1-tests commit and release before fetching. Recorded source file identities, the release member digest, archive identity, and log digest in `pins.txt`; fetched bytes remain in build storage.
- Added a gated CMake target and tests for archive/member/log identity and exact seven-case matching. The matcher rejects missing, duplicate, reordered, failing, incomplete, nonzero-exit, and PCM-like inputs; it accepts CRLF and TTY-prefixed lines.
- Added a bounded first-upload guest runner path for later DMA integration. It does not treat its intentionally incomplete early transcript as a full seven-case pass.
- Documented per-file provenance and the fetch-only executable boundary.

## Task Commits

1. **Task 1: Trace pinned assets through identity checks and seven-case parsing**
   - RED: `e2549af` — `test(05-04): add failing SPU reference matcher control`
   - GREEN: `127431e` — `feat(05-04): prepare pinned SPU transfer oracle`
2. **Task 2: Document the oracle boundary and fetch-only provenance**
   - `ba7b109` — `test(05-04): reject failed and incomplete SPU reports`

The plan commit count and head range above are measured from the persisted plan ledger. The later metadata commit records this summary and state updates.

## Verification

- `cmake --preset ci` — exit 0; configured and verified the pinned build-158 archive, executable member, and source log.
- `cmake --build --preset ci --target schwaystation-test-spu-reference` — exit 0.
- `ctest --test-dir build/ci -R '^spu\\.reference\\.(controls|fetch_controls|log)$' --no-tests=error --output-on-failure` — exit 0; 3/3 focused tests passed.
- RED evidence: `ctest --test-dir build/ci -R '^spu.reference.controls$' --no-tests=error --output-on-failure --output-junit "$PWD/build/ci/tdd/05-04-task1-red.xml"` (repo-root-normalized report path) failed at the intended valid-transcript assertion (exit 8). The JUnit report and evidence record are under ignored build storage. `gsd_run check tdd-red-evidence build/ci/tdd/05-04-task1-red.json --raw` returned `RED_EVIDENCE_OK`; the named target ran and failed for the planned behavior.
- Public identity checks: `gh api` for the pinned commit and `build-158` release succeeded before fetching. No hosted run is required for this plan.

The seven hardware lines cover memory-transfer, status, and timing behavior only. They do not provide PCM evidence or complete SOUND-01; other plan slices still own the remaining requirement behavior.

## Files Created/Modified

- `cmake/SPUReferences.cmake` — fetches and verifies the pinned transfer assets and registers the focused controls.
- `tests/spu/reference.c` — implements exact log matching, rejection controls, identity checks, and the bounded first-upload runner.
- `pins.txt` — records the public commit, source files, archive, executable member, and log identities.
- `tests/spu/tests.cmake`, `tests/spu/inventory.txt` — register the gated reference checks and sorted inventory entries.
- `PROVENANCE.md`, `ASSET_POLICY.md` — document source screening, licensing, and build-only executable handling.

## Decisions Made

- The full hardware log is the reference for its seven named transfer cases only; it cannot grade PCM output.
- The release executable remains fetch-only because it contains MPL-2.0 runtime bytes. No fetched asset is committed or shipped.
- The current plan prepares the `--first-upload` path, while later DMA work owns running it against completed guest behavior.

## Deviations from Plan

None — the planned asset preparation, matcher controls, and provenance work were completed within scope.

## Issues Encountered

- Strict project warnings exposed an unused pinned-log fixture during implementation; it is now exercised as the positive matcher control. The bounded file reader also closes the stream on every early error path. Both fixes stayed within the planned test and fetch work.

## User Setup Required

None — no external service configuration is required.

## Next Phase Readiness

The pinned oracle is ready for the ordered Phase 5 consumers. This plan does not ship Phase 5 or start Phase 6; the phase verification boundary remains in force.

---
*Phase: 05-the-sound-matches-the-console*
*Completed: 2026-11-13*

## Self-Check: PASSED

- SUMMARY.md exists at the required phase path.
- RED, GREEN, and Task 2 commits are ancestors of the current plan head; the ledger measures 3 plan commits from `99c966e` through `ba7b109e0aced9727e94a38d135dc01d74bf76a1`.
- The coverage classifier accepts both deliverables as fully automated; the targeted CTest run passed 3/3.
- The RED evidence validator returned `RED_EVIDENCE_OK`, and no stub or placeholder text was introduced by this plan.
