---
phase: 06-discs-and-fmv
plan: "22"
subsystem: sdl2-example-host
tags: [example, sdl2, installed-package, disc-provider]
requirements-completed: []
coverage:
  - id: DISC-04-SDL-EXAMPLE
    description: "A small SDL2 host compiles against the installed CMake package and uses the public disc callbacks for CUE, ISO and M3U content, with keyboard pad and lid/swap controls."
    requirement: DISC-04
    verification:
      kind: integration
      ref: "cmake --preset ci; cmake --build --preset ci --target schwaystation; cmake --install build/ci --prefix build/ci --component library; cmake -S examples/sdl2 -B build/example-sdl2 -DCMAKE_PREFIX_PATH=\"$PWD/build/ci\" -Dschwaystation_DIR=\"$PWD/build/ci/lib/cmake/schwaystation\"; cmake --build build/example-sdl2 — all corrected steps exit 0 and the example target compiles. No hosted run URL yet."
      status: pass
    human_judgment: false
  - id: DISC-04-SDL-ISOLATION
    description: "SDL window/audio/input code stays in examples/sdl2; product targets and installed library package remain independent of SDL2."
    requirement: DISC-04
    verification:
      kind: other
      ref: "Source/CMake inspection and successful installed-package build. Existing tests/cmake/archive_contents.cmake enforces an exact release member list; no example source was added to package targets. No app runtime test is required by 06-22-PLAN."
      status: pass
    human_judgment: false
metrics:
  duration: "Wall interval from the 04:19 UTC checkpoint through 04:35 UTC; active time was not measured."
  tasks: 2
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable)."
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 22: Optional SDL2 example

Added a small SDL2 host that links the installed `schwaystation::schwaystation` package, loads a user-supplied BIOS
and CUE/ISO/M3U image, displays native frames and queues audio, and maps keyboard input plus F1 lid and F2 playlist
swap controls to the public API. It compiles the existing host disc and pixel-conversion helpers as example-only
source. The helper is not installed; the library, runner and libretro core remain the only deliverables.

## Verification

- `cmake --preset ci`, `cmake --build --preset ci --target schwaystation`, and `cmake --install build/ci --prefix build/ci --component library` — exit 0.
- The first standalone CMake configure used only the prefix and failed because the generated build-tree
  `schwaystationConfig.cmake` at the prefix root shadows the installed package config under
  `lib/cmake/schwaystation/`. Setting `schwaystation_DIR` to that installed config fixed discovery; the corrected
  configure and `cmake --build build/example-sdl2` both exited 0.
- `actionlint` exited 0 with the existing `ubuntu-26.04` and `ubuntu-26.04-arm` runner-label messages ignored; the
  local actionlint version does not recognize those current hosted labels. No other workflow diagnostics remained.
- `git diff --check` — exit 0. The existing archive checker compares exact package members, so no separate
  example-archive gate was added.
- The example was compiled only, not launched or runtime-tested, as the plan requires. The CI build step is wired
  to the existing Linux x64 matrix job and uses the system SDL2-compatible development package only when `pkg-config`
  cannot find SDL2. Hosted execution remains pending the phase release; no hosted URL applies yet.

The installed-prefix preflight found a build-tree versus installed-config collision quickly. Future installed-package
examples built from inside a CMake build prefix should specify the installed package config directory when that prefix
also contains a generated build-tree config file.

## Remaining phase acceptance

Aggregate DISC-04 acceptance remains pending the final phase integration and hosted RetroArch checks. The example
does not claim physical-console equivalence or save-state support; its host-supplied disc identity is unavailable.

## Next

Continue with Plan 06-20: replay hostile media seeds and register the fuzz targets.
