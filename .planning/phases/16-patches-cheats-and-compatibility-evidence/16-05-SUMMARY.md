---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "05"
subsystem: libretro
tags:
  - libretro
  - savestates
  - cheats
requires:
  - phase: 16-04
    provides: Canonical bounded host cheat table and core state validation
provides:
  - Versioned libretro state envelope containing canonical cheat configuration
  - Atomic rejection of invalid host envelopes and core state payloads
affects:
  - Phase 16 cheat and compatibility verification
actuals:
  tokens: 8158
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 3d19212fec1794284941793b9102a8b6c3db6c6e
plan_head_after: 113f231776987900b3442cba6b1abb5e4d2c2c57
tech-stack:
  added: []
  patterns:
    - Fixed-size little-endian versioned state envelope with CRC-32/ISO-HDLC
    - Candidate validation before publishing host configuration
key-files:
  created: []
  modified:
    - libretro/libretro.c
    - tests/libretro/host.c
    - docs/libretro.md
key-decisions:
  - Keep the existing core state format inside a host-only envelope with canonical cheat bytes.
  - Validate the complete envelope and cheat candidate before applying core state or publishing host cheats.
patterns-established:
  - Libretro state has a 32-byte SCHL v1 header, canonical SCHT cheat payload, then core state bytes.
  - Rejection tests compare complete pre- and post-restore core and host state.
requirements-completed:
  - CHEAT-01
coverage:
  - id: D1
    description: Libretro state preserves normalized cheat definitions, order, and enable state across continuation and rejects malformed states atomically.
    requirement: CHEAT-01
    verification:
      - kind: integration
        ref: "cmake --preset ci; build libretro and core state test targets; CTest selectors libretro.cheats.apply, libretro.cheats.controls, libretro.cheats.state, core.state.round_trip, core.state.rejects"
        status: pass
    human_judgment: false
duration: 33min 19sec
completed: 2026-11-26
status: complete
runtime: Codex / OpenGSD 1.23.0
model: gsd-executor gpt-6-luna
---

# Phase 16 Plan 05: Serialize libretro cheat configuration atomically Summary

**Libretro savestates now preserve the canonical cheat table and reject malformed envelopes without partial mutation.**

## Performance

- **Duration:** 33 minutes 19 seconds (measured; the planned 20–28 minute window was exceeded by the final test and commit work)
- **Started:** 2026-11-26T03:27:42Z
- **Completed:** 2026-11-26T04:01:01Z
- **Tasks:** 2
- **Files modified:** 3
- **Model:** gsd-executor, gpt-6-luna

## Accomplishments

- Wrapped existing core state bytes with a fixed-size, versioned `SCHL` envelope containing canonical `SCHT` cheat bytes.
- Validated header fields, exact component lengths, reserved fields, checksum, and the full cheat table before core or host mutation.
- Added continuation, zero- and maximum-table, ordering, and malformed-state isolation coverage; documented the format and its limits.

## Task Commits

1. **Task 1 RED: cheat state behavior tests** - `1993191` (`test`)
2. **Task 1 GREEN: serialize libretro cheat state** - `4fc0a28` (`feat`)
3. **Task 2: validate envelope and continuation** - `113f231` (`test`, includes documentation)

The summary was committed separately after its self-check.

## Files Modified

- `libretro/libretro.c` - serializes and restores the host envelope atomically.
- `tests/libretro/host.c` - tests state identity, continuation, capacity boundaries, and rejection isolation.
- `docs/libretro.md` - specifies the envelope header, checksum, restore behavior, and version compatibility.

## Decisions Made

- Kept the existing core state format unchanged inside a libretro-specific envelope.
- Stored canonical cheat bytes before core bytes and publish the decoded candidate only after successful core state application.
- Kept the envelope scoped to cheat continuation identity; disc-state behavior and `.info` metadata remain unchanged.

## Verification Sequence

1. RED evidence: `ctest --preset ci --no-tests=error -R '^libretro[.]cheats[.]state$' --output-on-failure` exited **8** on the new assertions for serialized cheat identity and restored continuation, before the production implementation.
2. Task 1 tracer verification exited **0**, with `libretro.cheats.state` passing **1/1**. The exact command was:

   ```sh
   set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(libretro[.]cheats[.]state)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("libretro.cheats.state".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-libretro-card-test && ctest --preset ci --no-tests=error -R '^(libretro[.]cheats[.]state)$' --output-on-failure
   ```

3. Task 2 final plan verification exited **0**. CMake configuration and all three requested build targets succeeded; selector preflight found no missing or duplicate selectors; CTest passed **5/5**. The exact command was:

   ```sh
   set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(libretro[.]cheats[.]apply|libretro[.]cheats[.]controls|libretro[.]cheats[.]state|core[.]state[.]round_trip|core[.]state[.]rejects)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("libretro.cheats.apply libretro.cheats.controls libretro.cheats.state core.state.round_trip core.state.rejects".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-libretro-host schwaystation-libretro-card-test schwaystation-test-state && ctest --preset ci --no-tests=error -R '^(libretro[.]cheats[.]apply|libretro[.]cheats[.]controls|libretro[.]cheats[.]state|core[.]state[.]round_trip|core[.]state[.]rejects)$' --output-on-failure
   ```

4. `LC_ALL=C sort -cu tests/libretro/inventory.txt` exited **0**. `ctest --preset ci --no-tests=error -R '^docs[.]budgets$' --output-on-failure` exited **0**, **1/1**. `git diff --check HEAD` passed.
5. Supplementary regressions: `ctest --preset ci --no-tests=error -R '^(libretro[.]host|libretro[.]input[.]movies|libretro[.]input[.]controls|libretro[.]content[.]profile)$' --output-on-failure` and the separate `ctest --preset ci --no-tests=error -R '^libretro[.]input[.]movies$' --output-on-failure` were run. The host and controls selectors passed, and the standalone movies selector passed **1/1**. The optional profile selector was rerun with `ctest --preset ci --no-tests=error -R '^libretro[.]content[.]profile$' --output-on-failure` and exited **8** because this isolated build lacks generated `build/ci/openbios/openbios-fast.bin`; its `disc.media.preflight` selector passed. This selector is outside this plan's required set.

## Deviations from Plan

None in the implementation. The optional profile regression could not run to completion because its generated OpenBIOS fixture was absent from this worktree's build directory.

## Issues Encountered

- The sandbox initially denied writes to the shared Git worktree index. The authorized task commit succeeded after retrying with elevated filesystem access; repository hooks ran normally.
- `libretro.content.profile` could not load its expected generated OpenBIOS fixture, as noted above. The five plan selectors all passed.

## User Setup Required

None.

## Next Phase Readiness

The plan's cheat serialization and atomic rejection behavior is implemented and locally verified. The phase orchestrator can continue with its review and remaining plan work.

---
*Plan: 16-05*
*Completed: 2026-11-26*

## Self-Check: PASSED

- Summary file exists at the plan output path.
- Task commits `1993191`, `4fc0a28`, and `113f231` are ancestors of the current worktree HEAD.
