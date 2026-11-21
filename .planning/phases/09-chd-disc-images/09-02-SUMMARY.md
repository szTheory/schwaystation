---
phase: 09-chd-disc-images
plan: "02"
subsystem: media
tags: [CHD, libchdr, CMake, CTest, bounds]
requires: []
provides:
  - Opt-in static C-only libchdr closure for host disc images
  - Private bounded CHD open/close and normalized TOC/sector callback interface
  - Enabled and disabled malformed-header test selector
affects: [Phase 09 CHD metadata and parity, Phase 09 release packaging]
actuals:
  tokens: 8549
  tasks: 2
  commits: 2
tech-stack:
  added: [libchdr at 47d794151b2b64f6aba60304e6480f4f5a2bc24b]
  patterns:
    - Digest-verified build-tree source fetch is conditional on the host image target and the explicit CHD option
    - Raw header geometry and allocation bounds are checked before the pinned reader opens the file
key-files:
  created:
    - cmake/Libchdr.cmake
    - host/disc_chd.c
    - host/disc_chd.h
    - tests/disc/chd.c
  modified:
    - CMakeLists.txt
    - cmake/DiscTargets.cmake
    - pins.txt
    - PROVENANCE.md
    - tests/disc/tests.cmake
    - tests/disc/inventory.txt
key-decisions:
  - Pin the libchdr commit archive by SHA-256 and build only its static C closure with bundled codecs.
  - Cap CHD file and logical sizes at 1 GiB, hunk size at 16 MiB, and hunk count at 65536 before libchdr opens a file.
patterns-established:
  - Keep the private reader opaque and host-only; normalize future metadata projection through schway_toc and the existing sector callback.
requirements-completed: []
coverage:
  - id: D1
    description: The approved libchdr source builds as a static C-only host dependency when enabled and is absent from the CHD-off and core-only configure graphs.
    requirement: CHD-01
    verification:
      - kind: integration
        ref: "cmake --preset ci -DSCHWAYSTATION_CHD=ON; cmake --build --preset ci --target schwaystation_disc_image"
        status: pass
      - kind: other
        ref: "Fresh CHD-on build confirmed no C++ compiler, bundled codec cache settings, and a fresh CHD-off/core-only configure confirmed no libchdr source directory."
        status: pass
    human_judgment: false
  - id: D2
    description: Empty, truncated, wrong-magic, unsupported-version, over-limit and overflowing CHD headers fail with explicit status and no returned handle; close accepts NULL.
    requirement: MEDIA-02
    verification:
      - kind: unit
        ref: "ctest --preset ci --no-tests=error -R '^disc[.]image[.]chd_headers$' --output-on-failure"
        status: pass
      - kind: unit
        ref: "ctest --test-dir build/chd-off --no-tests=error -R '^disc[.]image[.]chd_headers$' --output-on-failure"
        status: pass
    human_judgment: false
  - id: D3
    description: The exact source archive and all compiled codec license choices are recorded for the later notice packaging plan.
    requirement: CHD-01
    verification:
      - kind: other
        ref: "gh api commit-pin lookup; shasum -a 256 on the fetched libchdr archive; git diff --check origin/main...HEAD"
        status: pass
    human_judgment: false
duration: 27min
completed: 2026-11-20
status: complete
---

# Phase 9 Plan 09-02 Summary

The optional CHD dependency now builds from a verified libchdr archive, with hostile header sizes rejected before the reader allocates from them.

## Performance

- **Duration:** about 27 minutes from the last saved Plan 09-01 state; the exact task start was not recorded.
- **Started:** 2026-11-20T03:57:56Z (last saved plan boundary; approximate)
- **Completed:** 2026-11-20T04:24:46Z
- **Tasks:** 2
- **Files modified:** 10
- **Runtime/model:** Codex with OpenGSD 1.20.0; primary GPT-6 model ID is not exposed. `workflow.use_worktrees=false` kept execution sequential and inline.

## Accomplishments

- Added `SCHWAYSTATION_CHD`, default OFF. The enabled host image helper fetches the exact libchdr commit archive with SHA-256 validation and builds bundled zlib/miniz, zstd, LZMA and the C dr_flac backend as static C targets. System codecs, optional micro-flac, upstream tests and LZMA assembly are disabled. No CHD code enters the core or a core-only configure.
- Corrected the provenance audit: libchdr and its public header have separate BSD-3 holders; miniz and dr_flac use their selected public-domain grants; zstd uses the BSD-3 terms from the exact 1.5.7 license. The archive digest is `d2f1756ba5dced374ea1288943fecee92fcab2351ea99add5c726e42d3561957`.
- Added a private opaque reader interface with signatures `schway__disc_chd_open(const char *, schway__disc_chd **, schway_disc_image_error *)`, `schway__disc_chd_close(schway__disc_chd *)`, `schway__disc_chd_toc(const schway__disc_chd *, schway_toc *)`, and `schway__disc_chd_read_sector(void *, uint32_t, uint8_t[2352], uint8_t[96])`. The TOC and sector callbacks fail closed pending the metadata/padding projection in Plan 09-05; this plan tests header admission only.
- Added the sorted `disc.image.chd_headers` selector for both configurations. It checks empty/truncated files, invalid magic, an unsupported version, oversized logical geometry, legacy arithmetic overflow, the hunk-count ceiling, explicit error status, failure-atomic output and NULL-safe close.

## Task Commits

1. **Task 1: Admit and configure the exact static reader closure** - `d2b0f31` (`feat(09-02): wire pinned CHD dependency`)
2. **Task 2: Exercise bounded header rejection through the private backend** - `e63727f` (`feat(09-02): add bounded CHD header backend`)

## Files Created/Modified

- `cmake/Libchdr.cmake` - exact archive fetch, digest check and static C-only dependency options.
- `host/disc_chd.c` and `host/disc_chd.h` - bounded opaque host reader and normalized callback declarations.
- `tests/disc/chd.c` - enabled malformed-header cases and disabled-feature assertion.
- `CMakeLists.txt`, `cmake/DiscTargets.cmake`, `pins.txt`, `PROVENANCE.md`, `tests/disc/tests.cmake`, and `tests/disc/inventory.txt` - option, host-only dependency graph, source/licence provenance and test registration.

## Commands and Results

| Command | Exit | Result |
|---|---:|---|
| `gh api repos/rtissera/libchdr/commits/47d794151b2b64f6aba60304e6480f4f5a2bc24b --jq .sha` | 0 | The exact approved commit resolves publicly. |
| `shasum -a 256 /private/tmp/schwaystation-libchdr-09-02/libchdr.tar.gz` | 0 | The fetched archive matches the pin recorded in `pins.txt` and `PROVENANCE.md`. |
| Fresh CHD-on configure/build in `build/chd-on`, followed by cache checks | 0 | Built the helper and its 24 C steps without enabling a C++ compiler; bundled codecs are selected and libchdr tests are off. |
| `cmake --preset ci -DSCHWAYSTATION_CHD=ON && cmake --build --preset ci --target schwaystation-test-disc-chd && ctest --preset ci --no-tests=error -R '^disc[.]image[.]chd_headers$' --output-on-failure` | 0 | Enabled malformed-header selector passed 1/1. |
| Fresh CHD-off configure/build and the same selector under `build/chd-off` | 0 | Disabled-feature assertion passed 1/1 and no libchdr source directory was fetched. |
| Core-only CHD-on configure and `test ! -e build/chd-core/pins/libchdr` | 0 | The core-only graph did not fetch or link the host dependency. |
| `LC_ALL=C sort -cu tests/disc/inventory.txt && git diff --check origin/main...HEAD` | 0 | Inventory and committed diff checks passed. |

No hosted CI or `act` run applied to this plan slice; the focused local configuration and target cover its changes without running unrelated suites. No hosted URL applies.

## Decisions Made

The pinned CMake project declares CMake 3.10 policies, where `option()` clears normal variables and would silently restore unwanted codec/test defaults. The integration now force-sets the exact closure options in the CHD-enabled cache. A fresh configure confirms the intended options and emits no policy warnings.

## Deviations from Plan

None. The TOC and sector callback implementations intentionally remain fail-closed until Plan 09-05, which owns the validated CHD track metadata and storage-padding projection.

## Issues Encountered

- The first CHD-enabled configure exposed the old `option()` policy behavior; it was fixed before accepting the fresh static C-only build.
- The initial CHD-off CTest compile found enabled-only helpers unused under `-Werror`; they are now conditionally compiled, and both ON and OFF selectors pass.
- One exploratory CHD-off configure disabled libretro while using the full CI inventory, so inventory validation correctly stopped configure. The retry matched CI's host options and passed; the partial build files remain ignored under `build/`.

## User Setup Required

None.

## Next Phase Readiness

Plan 09-02 is complete. Continue with the next ready Phase 9 plan. The run-local fixture generator still has an unresolved complete-license/build-closure gate; do not build or use it until that audit passes. Later plans must populate the private TOC and sector callbacks before parity work.

---
*Phase: 09-chd-disc-images*
*Completed: 2026-11-20*

## Self-Check

All plan tasks, focused enabled and disabled checks, and summary coverage metadata passed.
