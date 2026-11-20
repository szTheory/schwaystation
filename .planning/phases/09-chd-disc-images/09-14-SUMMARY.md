---
phase: 09-chd-disc-images
plan: 14
subsystem: testing
tags: [CHD, CTest, compatibility, documentation]
requires:
  - phase: 09-06
    provides: Whole-source CUE/CHD sector, subcode and identity parity
  - phase: 09-12
    provides: Runner and libretro CHD integration
  - phase: 09-13
    provides: CHD ON/OFF package and notice checks
provides:
  - Bounded owned boot, read, audio and FMV guest cases through CHD callbacks
  - CHD feature evidence spanning provider, runner, libretro, package and guest checks
  - Current CHD subset, bounds, pregap, subcode and error behavior in the disc guide
affects: [Phase 9 source review and final verification, future media providers]
actuals:
  tokens: 3159
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 43dc950b0b7df58f02b6931f8741dada146acf2a
plan_head_after: 938cca6f09514c51a4a211a13f2f577edfcce9fc
tech-stack:
  added: []
  patterns:
    - Reuse the same bounded guest cases and expected pins through alternate disc providers.
    - Use CTest setup fixtures to finish structural, parity and feature-key checks before guests.
key-files:
  created:
    - .planning/phases/09-chd-disc-images/09-14-SUMMARY.md
  modified:
    - tests/disc/fixture.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - tests/compat/features.csv
    - tests/compat/tests.cmake
    - docs/discs.md
    - docs/compatibility.md
key-decisions:
  - Use chd-boot/read/audio/fmv aliases to reuse the existing guest paths, expected outputs and emulated-time bounds.
  - Pass the local CHD feature row based on provider, host, package and guest evidence; keep hosted lane, hash and fuzz evidence unknown.
patterns-established:
  - Run the full-source parity and feature-key fixtures before any CHD guest mode.
requirements-completed:
  - MEDIA-01
  - MEDIA-02
  - CHD-01
  - COMPAT-02
coverage:
  - id: D1
    description: The owned CUE and CHD sources pass manifest, track geometry and logical sector-count preflights before guest execution.
    requirement: MEDIA-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^disc[.](media[.]|image[.]chd_|fixture[.]chd)' --output-on-failure; media preflight passed before guest tests."
        status: pass
    human_judgment: false
  - id: D2
    description: CHD decoding returns the same TOC, all 222 sectors in both read orders, absent-subcode bytes and content-based identity as CUE.
    requirement: MEDIA-01
    verification:
      - kind: integration
        ref: "disc.image.chd_parity in the focused selector above; 222 input sectors and 222 produced sectors, exit 0."
        status: pass
    human_judgment: false
  - id: D3
    description: The bounded owned boot, read, audio and FMV guests pass through CHD with the existing CUE expectations and completion bounds.
    requirement: CHD-01
    verification:
      - kind: e2e
        ref: "disc.fixture.chd.boot/read/audio/fmv in the focused selector above; all four passed, including the CUE FMV predicate setup."
        status: pass
    human_judgment: false
  - id: D4
    description: The compatibility ledger records only the locally demonstrated CHD provider, host, package and guest predicate, with its protected ratchet intact.
    requirement: COMPAT-02
    verification:
      - kind: integration
        ref: "ctest --preset ci --no-tests=error -R '^compat[.]features[.]' --output-on-failure; 2/2 passed."
        status: pass
    human_judgment: false
  - id: D5
    description: The disc guide matches the implemented CHD metadata subset, sector mapping, bounds and error statuses.
    requirement: MEDIA-01
    verification:
      - kind: other
        ref: "Cross-checked docs/discs.md against host/disc_chd_metadata.c, host/disc_chd.c and host/disc_image.h; compatibility doc freshness passed."
        status: pass
    human_judgment: false
duration: 22m12s
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 14: Owned CHD guest parity and evidence Summary

**The owned boot, read, audio and FMV guests now exercise CHD with the same CUE expectations and time bounds.**

## Performance

- **Duration:** 22m12s.
- **Started:** 2026-11-20T20:42:42Z.
- **Completed:** 2026-11-20T21:04:54Z.
- **Tasks:** 2.
- **Files modified:** 7 source, test, ledger and documentation files.
- **Execution model:** Codex/OpenGSD 1.20.0; executor gpt-6-luna xhigh.

## Accomplishments

- Added four CTest cases that run the existing bounded guest paths against `disc_boot.chd`. The boot, read, XA/CD-DA audio and FMV pins and completion bounds are shared unchanged with the CUE cases.
- Made the media manifest/geometry check, whole-source CUE/CHD parity test and feature-key preflight CTest setup dependencies of every new CHD guest. The FMV case also waits for the existing CUE FMV predicate producer.
- Updated the CHD feature row with local provider, runner, libretro, package and guest evidence. The compatibility ratchet and generated compatibility page now agree with that result.
- Corrected the disc guide's stale statement that runner/libretro selection and ON/OFF packaging were future work. It now documents current version, mode, pregap, subcode, limit and error behavior.

## Source, Decoder and Guest Results

| Evidence layer | Result |
| --- | --- |
| Source coverage | The media preflight accepted the owned source digests, the 64-sector data track, the 150-frame audio pregap plus 8 audio program sectors, and the 222-sector logical geometry. |
| Decoder output | `disc.image.chd_parity` compared 222 sectors in forward and reverse order, equal TOCs, guarded sector buffers, zero absent-subcode output and matching content-based identity. Input and produced counts were both 222. |
| Guest completion | `disc.fixture.chd.boot`, `.read`, `.audio` and `.fmv` all passed with the existing expected output pins and bounded frame counts. |

## Verification

- Cheap preflights passed before any guest attempt: `LC_ALL=C sort -cu tests/disc/inventory.txt` (exit 0) and `cmake -DFEATURES=$PWD/tests/compat/features.csv -DMODE=features-preflight -P tests/cmake/compat-ratchet.cmake` (exit 0).
- The new boot test's RED run executed media, parity and feature setup successfully, then rejected the not-yet-supported `chd-boot` mode before guest emulation (CTest exit 8). The alias implementation made all four CHD guest cases pass.
- Initial CHD guest selector: `ctest --preset ci --no-tests=error -R '^disc[.]fixture[.]chd[.]' --output-on-failure`; 8/8 including its setup tests, exit 0, 11.10s wall time.
- Plan verifier: `LC_ALL=C sort -cu tests/disc/inventory.txt && cmake --preset ci -DSCHWAYSTATION_CHD=ON && cmake --build --preset ci --target schwaystation-test-disc-fixture schwaystation-chd-fixtures && ctest --preset ci --no-tests=error -R '^disc[.](media[.]|image[.]chd_|fixture[.]chd)' --output-on-failure`; 14/14, exit 0, 10.71s. The required tracer feedback rerun also passed 14/14, exit 0, 11.45s; Ninja had no rebuild work.
- Feature verification: `ctest --preset ci --no-tests=error -R '^compat[.]features[.]' --output-on-failure`; 2/2, exit 0, 0.58s. `policy.compat_doc_fresh` passed 1/1 in 0.35s, and `COMPAT_TODAY=2026-11-20 sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` exited 0.
- `git diff --check origin/main...HEAD`, `git diff --check HEAD`, and the sorted inventory check exited 0. The local CI build remains configured with `SCHWAYSTATION_CHD=ON`.
- Iteration check: these focused selectors ran all four guest cases in parallel after the setup fixtures; the complete selector took about 11 seconds wall time against 23–25 seconds summed test process time. No workflow changed, so no hosted CI or `act` run was started; no safe local optimization that preserves every check was evidenced.

## Task Commits

1. **Task 1: Run the bounded owned guest through the CHD provider** — `1d48028` (test cases), `ab91147` (CHD mode aliases).
2. **Task 2: Record truthful feature evidence and complete the review handoff** — `938cca6`.

Three task commits were measured from plan base `43dc950b0b7df58f02b6931f8741dada146acf2a` through `938cca6f09514c51a4a211a13f2f577edfcce9fc`.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 3 - Blocking issue] Updated the compatibility ratchet's expected feature row and generated page.**
- **Found during:** Task 2.
- **Issue:** The pre-existing ratchet fixture expected the old unknown row exactly, and `docs/compatibility.md` is generated from the feature ledger.
- **Fix:** Updated the expected result and regenerated the compatibility page with the evidence-backed pass row.
- **Files modified:** `tests/compat/tests.cmake`, `docs/compatibility.md`, `tests/compat/features.csv`.
- **Verification:** Feature preflight, feature ratchet, generated-page freshness test and generator `--check` all passed.
- **Committed in:** `938cca6`.

**Total deviations:** 1 auto-fixed integration issue. The implementation stayed within the plan's test, evidence and documentation scope.

## Remaining Phase Verification

The hosted eight-lane exact-source parity/hash checks and malformed-input fuzz run remain **unknown** until Phase 9 final verification. The actual WASI module size and its quality bound also remain pending for that verification. The local feature row claims only the provider/host/package/guest facts exercised above. Source review is next; this plan did not run code review, broad Phase 9 tests or hosted CI.

## Issues Encountered

The sandbox rejected writing the required plan-head ledger under `.git`. The baseline SHA was retained in the ignored `build/ci/gsd-plan-head-before-09-14` file instead; commit count and head were measured from that file. No repository history was rewritten.

## User Setup Required

None. No owner action is needed before the Phase 9 source review and final verification.

## Next Phase Readiness

Plan 09-14 is complete. All Phase 9 source is ready for code review, followed by final local and hosted verification. Hosted-only results remain unknown until those checks pass.

## Self-Check: PASSED

The summary file exists, all three Plan 09-14 task commits are ancestors of HEAD, and the measured plan count is three commits from the persisted baseline SHA.
