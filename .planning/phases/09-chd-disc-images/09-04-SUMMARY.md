---
phase: 09-chd-disc-images
plan: 04
subsystem: media
tags: [chd, fixture, preflight, determinism]
requires:
  - Opt-in pinned CHD fixture tool from Plan 09-03
provides:
  - Reproducible build-directory CHD from the owned two-track CUE fixture
  - Run-local source/tool/output manifest and bounded malformed-fixture controls
affects: [Phase 9 CHD metadata and parity, Phase 9 CI lane wiring]
actuals:
  tasks: 2
  commits: 2
tech-stack:
  added: []
  patterns:
    - Check fixture geometry and generator options before compression
    - Verify source, tool and CHD hashes before structural reads or guest tests
key-files:
  created:
    - cmake/CHDFixtures.cmake
  modified:
    - cmake/DiscFixtures.cmake
    - tools/chd-tool/main.cpp
    - tests/disc/media.c
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
    - docs/testing.md
key-decisions:
  - Keep CHD output and its manifest in the build directory; each later CI lane will generate its own checked fixture.
  - Require byte-identical output from two generations with the pinned tool settings and source bytes.
patterns-established:
  - Run cheap inventory, authored-CUE, media-input, sector-count and tool-setting checks before compression.
  - Gate guest tests on a structural fixture preflight and keep explicit malformed-fixture controls in both feature configurations.
requirements-completed: []
coverage:
  - id: CHD-01-A
    description: The owned fixture preserves its two source tracks and generates a deterministic CHD strictly below 512 KiB.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=ON -DSCHWAYSTATION_BUILD_CHD_TOOL=ON; cmake --build --preset ci --target schwaystation-test-disc-media schwaystation-chd-fixtures; focused disc.media preflight and fixture_controls CTests (exit 0). Output 5,552 bytes with SHA-256 4952c3a05ff2e2e596deed7ed2d37358a646790b890d5ed38484feec5b797569."
        status: pass
    human_judgment: false
  - id: MEDIA-02-A
    description: Cheap preflight validates source identity, generator settings, exact track counts, CHD cap and readable header before guest execution.
    requirement: MEDIA-02
    verification:
      - kind: integration
        ref: "CHD-on disc.media.preflight and disc.media.fixture_controls passed 2/2 in 0.07 seconds; inventory sort, generated-media check and diff check passed."
        status: pass
    human_judgment: false
  - id: MEDIA-02-B
    description: CHD-off builds retain no generated CHD or manifest and reject CHD at the private reader boundary.
    requirement: MEDIA-02
    verification:
      - kind: integration
        ref: "Fresh CHD-off configure/build plus disc.media.preflight and disc.media.fixture_controls passed 2/2 in 0.02 seconds."
        status: pass
    human_judgment: false
duration: 51min
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-04 Summary

The owned CUE now produces a deterministic, size-bounded CHD with a checked manifest, and both CHD-enabled and disabled builds reject malformed fixture setup before guest work.

## Performance

- **Duration:** 51 minutes from the preceding saved checkpoint; active time was not measured.
- **Started:** 2026-11-20T05:12:09Z (previous saved STATE checkpoint)
- **Completed:** 2026-11-20T06:03:28Z
- **Tasks:** 2
- **Changed paths:** 7 implementation and documentation paths, plus the plan record.
- **Runtime/model:** Codex/OpenGSD 1.20.0; exact primary GPT-6 model ID is not exposed.

## Accomplishments

- Added `schwaystation-chd-fixtures` to generate `build/ci/fixtures/disc_boot.chd` from the owned CUE and both run-local source tracks. The generator checks exact CUE layout, whole-sector inputs, 64 and 158 source sectors, 222 total sectors, and codec/hunk/worker settings before compression.
- Generated twice and required identical bytes. The output is 5,552 bytes, below the strict 524,288-byte cap. The manifest records source CUE and track hashes, generator commit/archive/binary hashes, settings, counts, output size and SHA-256. No disc bytes were committed.
- Added a structural media preflight and isolated wrong-hash, missing-track, wrong-count, oversized-output, malformed-manifest, corrupted-output and missing-output controls. CHD-off controls verify stale generated files are removed and the private CHD reader reports unsupported.
- Added build and maintainer documentation for the opt-in CHD fixture path and the cheap preflight selectors.
- Resolved the plan's artifact handoff wording against the settled asset policy: CHD bytes remain build-local, and the future CI plan requires each applicable lane to create and hash its own fixture.

## Task Commits

1. **Build one owned CUE-to-CHD fixture with a checked manifest** — `54bae5a` (`feat(09-04): generate and validate owned CHD fixture`)
2. **Reject structural fixture mistakes before decode/guest work** — `472c371` (`test(09-04): reject invalid CHD fixture handoffs`)

The separate planning correction is `b3b38c0` (`docs(09): keep CHD fixtures build-local`).

## Verification and Efficiency

- Cheap inputs first: sorted inventory, generated-media, exact CUE layout/track-size and diff checks all exited 0 before the focused builds.
- Commands:

  ```sh
  LC_ALL=C sort -cu tests/disc/inventory.txt
  python3 tools/gen_disc.py --check-media build/ci/fixtures/media
  git diff --check HEAD
  ```
- CHD-on configure/build and focused CTest selectors exited 0; `disc.media.preflight` and `disc.media.fixture_controls` passed 2/2 in 0.07 seconds. The fixture generator's repeated-output check held the output at 5,552 bytes with the SHA-256 above.
- Fresh CHD-off configure/build and the same selectors exited 0; 2/2 passed in 0.02 seconds and the generated CHD/manifest were absent.
- No guest boot, full suite, hosted CI or `act` run applied: this plan did not change workflow wiring, and its new producer/consumer path was directly covered by focused local ON/OFF configurations. No equivalent full suite was repeated for bookkeeping.
- CI and release efficiency/preflight guidance is already discoverable in `AGENTS.md` and `.planning/AUTONOMOUS.md`; it remains the standing policy for the later workflow plans.

## Deviations and Boundaries

The public image router does not dispatch CHD until a later plan. This preflight therefore opens the generated file through the private bounded CHD reader, as intended for this structural stage. No guest was run; TOC projection and CUE/CHD parity remain for Plans 09-05 and 09-06.

## Next Plan

Continue with Plan 09-05, which projects validated CHD metadata into the normalized TOC and checks LBA semantics.

## Self-Check

Both plan tasks, ON/OFF preflights and summary metadata were checked; all required local commands exited 0.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*
