---
phase: 06-discs-and-fmv
plan: "18"
subsystem: libretro-disc-control
tags: [libretro, disc-control, m3u, audio]
actuals:
  tasks: 2
  files: 14
  duration: "Active time not measured reliably after the owner-reported 22-hour wall pause."
requirements-completed: []
coverage:
  - id: DISC-04-LIBRETRO-EXT
    description: "The extended disk interface loads CUE/ISO/M3U content, honors initial image selection, updates playlist entries, reports paths and labels, and changes the guest-visible TOC after a lid event."
    requirement: DISC-04
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host($|\\.)' --no-tests=error --output-on-failure (exit 0, 4/4; 39.95 seconds); no hosted run URL applies."
      status: pass
    human_judgment: false
  - id: DISC-04-NO-MEDIA
    description: "An empty TOC removes the attached medium while the lid is open; the tray closes and the guest receives INT5 with GetTN no-media error 80h."
    requirement: DISC-04
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^(core\\.disc_api|libretro\\.host\\.disc_control)$' --no-tests=error --output-on-failure (exit 0, 2/2)."
      status: pass
    human_judgment: false
  - id: DISC-02-NATIVE-DISC-PCM
    description: "The complete owned XA/CD-DA disc stream matches runner PCM byte-for-byte through both libretro callback modes for NTSC-U and PAL; steady-state cadence, reset, reload, mute and AV timing are checked."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.host($|\\.)' --no-tests=error --output-on-failure (exit 0, 4/4; includes libretro.host.disc_audio for both regions and callback modes)."
      status: pass
    human_judgment: false
  - id: DISC-04-INFO-CLAIMS
    description: "The .info extensions and disk_control claim are tied to the matching registered host tests, with negative controls for missing inventory coverage and changed extensions."
    requirement: DISC-04
    verification:
      kind: integration
      ref: "ctest --test-dir build/ci -R '^libretro\\.info\\.claims($|\\.)' --no-tests=error --output-on-failure (exit 0, 2/2); repository checks remain at 2700/2700 lines."
      status: pass
    human_judgment: false
metrics:
  duration: "At least 1h09m wall from the previous state checkpoint through final host verification; active time is unavailable."
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); standard gsd-code-reviewer."
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 18: Libretro disc control and PCM

The libretro core now opens supported disc content with the system BIOS, exposes the extended disk-control callbacks,
and keeps the runner and libretro's XA/CD-DA output byte-identical. A frontend can also remove the current medium,
close the empty tray, and receive the expected no-media response in the guest.

## Work completed

- Added extended disk control for CUE, ISO, and M3U content, including initial image selection, path and label
  queries, add/replace operations, lid state, and selected-image swaps. The provider remains alive until its core
  callbacks are detached.
- Allowed `schway_swap_disc` with a valid empty TOC to clear the cached TOC, callback pointers, and media identity
  while preserving the open lid. The adapter uses this for libretro's remove-media sentinel and selected playlist
  entry removal.
- Added OpenBIOS loading from the frontend system directory, updated the `.info` extension and disk-control claims,
  and documented content and swapping in `docs/libretro.md`.
- Extended the owned guest and host to prove eject INT5, changed TOC, no-media GetTN error, callback validation,
  and runner/libretro audio parity for both batch and single-sample callbacks in NTSC-U and PAL.
- The first audio probe showed BIOS startup callbacks can split one sample period across adjacent frames; the test
  still compares every PCM byte and applies frame cadence assertions after startup settles. The broad host run then
  caught and corrected stale extension expectations in the existing baseline tests.
- Registered the Plan 19 vendored checks as specified. Their implementation and execution belong to Plan 19.

## Verification

- `git diff --check`, sorted inventory check, `cmake --preset ci`, and fixture manifest digest comparison — exit 0.
- `cmake --build --preset ci --target schwaystation_libretro schwaystation-test-libretro-host schwaystation-run schwaystation-test-disc-api schwaystation-disc-fixtures` — exit 0.
- `ctest --test-dir build/ci -R '^(core\\.disc_api|libretro\\.host\\.disc_control)$' --no-tests=error --output-on-failure` — exit 0, 2/2.
- `ctest --test-dir build/ci -R '^libretro\\.host\\.disc_audio$' --no-tests=error --output-on-failure` — exit 0, 1/1.
- `ctest --test-dir build/ci -R '^libretro\\.host($|\\.)' --no-tests=error --output-on-failure` — exit 0, 4/4 in 39.95 seconds.
- `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=2700 -P tests/cmake/line_budget.cmake` — exit 0, exactly 2700/2700 lines.
- `ctest --test-dir build/ci -R '^libretro\\.info\\.claims($|\\.)' --no-tests=error --output-on-failure` — exit 0, 2/2.
- Standard code review — clean after fixing the no-media removal path.
- No hosted run URL applies. No physical PlayStation capture was needed; the owner has no time for one now.

## Remaining phase acceptance

The independent disc MD5 oracle and vendored-file failure control are next in Plan 19. Aggregate Phase 6 verification
remains pending; this summary does not claim DISC-02 or DISC-04 complete across the milestone.

## Next

Continue with Plan 06-19: vendor and verify the independent disc-hash implementation.
