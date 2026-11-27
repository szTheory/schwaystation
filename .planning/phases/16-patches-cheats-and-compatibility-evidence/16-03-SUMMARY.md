---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "03"
subsystem: host-cheats
tags: [GameShark, host, C17, deterministic-state]
requires:
  - phase: "16-02"
    provides:
      - checked public RAM write through API v11
provides:
  - bounded shared host cheat parser and checked-write consumer
  - fixed-size canonical cheat state encoding and validation
affects: [runner-cheats, libretro-cheats, phase-16-cheat-state]
actuals:
  tokens: 12331
  tasks: 3
  commits: 6
tech-stack:
  added: []
  patterns:
    - transactional text replacement
    - fixed-layout little-endian host state
key-files:
  created:
    - host/cheats.c
    - host/cheats.h
    - docs/cheats.md
  modified:
    - CMakeLists.txt
    - tests/runner/cheats.c
key-decisions:
  - "Keep cheat parsing, bounds, and enabled state in the shared host layer; apply only through schway_write_ram."
  - "Use a 16,656-byte canonical format with fixed entry and record slots, explicit little-endian fields, and zeroed reserved bytes."
patterns-established:
  - "Parse a whole replacement into a candidate and publish it only after validating the complete table."
  - "Encode host state field by field instead of serializing C structure padding."
requirements-completed:
  - CHEAT-01
coverage:
  - id: cheat-parser-checked-write
    description: The shared parser accepts only bounded 30 byte and 80 halfword writes and applies enabled entries through the checked RAM API.
    requirement: CHEAT-01
    verification:
      - kind: unit
        ref: "runner.cheats.parser, runner.cheats.bounds, runner.cheats.order (final consolidated CTest selector run)"
        status: pass
      - kind: unit
        ref: "abi.api_v11 (final consolidated CTest selector run)"
        status: pass
    human_judgment: false
  - id: cheat-canonical-state
    description: Canonical cheat bytes are stable across equivalent text, preserve order and enabled state, and reject malformed envelopes without publishing them.
    requirement: CHEAT-01
    verification:
      - kind: unit
        ref: "runner.cheats.order canonical encode/decode assertions (final consolidated CTest selector run)"
        status: pass
      - kind: unit
        ref: "docs.budgets (final consolidated CTest selector run)"
        status: pass
    human_judgment: false
duration: 31m
completed: 2026-11-26
status: complete
plan_head_before: 542580e5ca0770820647aab45b75722b484713f0
plan_head_after: 1602992574984ac980cee67ae11c9beb2afb2cfc
---

# Phase 16 Plan 03: Shared Cheat Parser and State Summary

**The shared host layer parses bounded GameShark byte and halfword writes, applies them through API v11, and serializes validated cheat state as canonical little-endian bytes.**

## Performance

- **Duration:** 31 minutes
- **Started:** 2026-11-26T01:34:47Z
- **Completed:** 2026-11-26T02:05:41Z
- **Runtime/model:** Codex GSD executor; configured executor model `gpt-6-luna`
- **Tasks:** 3
- **Files modified:** 5
- **Task commits:** 6 measured from the plan base to the final implementation commit

## Accomplishments

- Added a shared host cheat table with 32 entry indices, 64 writes per entry, 1,024 text bytes per entry, and 256 total normalized writes.
- Implemented transactional parsing for `30` byte writes and `80` aligned halfword writes, strict physical main-RAM bounds, ordered application, and no-op behavior for disabled entries.
- Added a fixed 16,656-byte canonical state format. Decoding validates all counts, records, reserved bytes, sizes, and RAM bounds before replacing the destination table.
- Documented the accepted grammar, limits, ordering, disable behavior, and state format; linked the shared consumer into `schwaystation-host`.

## Task Commits

1. **Task 1: Wire one host cheat through the checked operation** — `fe805bb` RED, `5b07b5e` GREEN
2. **Task 2: Parse and apply the complete write-only subset** — `cfab666` RED, `85bcb5f` GREEN
3. **Task 3: Validate canonical host cheat state** — `e16114b` RED, `1602992` GREEN

## TDD Execution Evidence

Each task's RED command was captured as fresh CTest JUnit, semantically inspected, and accepted by `check tdd-red-evidence` as `RED_EVIDENCE_OK`; each GREEN implementation then passed the task's focused checks.

| Task | RED command | RED exit / duration | RED evidence check |
|---|---|---:|---|
| 1 | `ctest --preset ci --no-tests=error -R '^runner[.]cheats[.]parser$' --output-on-failure --output-junit /private/tmp/schwaystation-16-03-task1-red.xml` | 8 / 0.01 s | `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence .gsd/tdd-evidence/16-03/task1-red.json --raw` — exit 0 |
| 2 | `ctest --preset ci --no-tests=error -R '^runner[.]cheats[.]parser$' --output-on-failure --output-junit /private/tmp/schwaystation-16-03-task2-red.xml` | 8 / 0.02 s | `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence .gsd/tdd-evidence/16-03/task2-red.json --raw` — exit 0 |
| 3 | `ctest --preset ci --no-tests=error -R '^runner[.]cheats[.]order$' --output-on-failure --output-junit /private/tmp/schwaystation-16-03-task3-red.xml` | 8 / 0.02 s | `node "$HOME/.codex/gsd-core/bin/gsd-tools.cjs" check tdd-red-evidence .gsd/tdd-evidence/16-03/task3-red.json --raw` — exit 0 |

The RED evidence records and command reports were kept under ignored worktree evidence storage or `/private/tmp`; no logs or runtime bytes were committed.

## Verification Evidence

All commands ran from the worktree root. The final consolidated check also reran the released memory-read baseline.

| Command | Exit | Duration / result |
|---|---:|---|
| `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]cheats[.]parser|abi[.]api_v11)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("runner.cheats.parser abi.api_v11".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-cheats schwaystation-test-api-v11 && ctest --preset ci --no-tests=error -R '^(runner[.]cheats[.]parser|abi[.]api_v11)$' --output-on-failure` | 0 | Task 1 GREEN; CTest 2/2 in 0.15 s |
| `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]cheats[.]parser|runner[.]cheats[.]bounds|runner[.]cheats[.]order|docs[.]budgets)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("runner.cheats.parser runner.cheats.bounds runner.cheats.order docs.budgets".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-cheats && ctest --preset ci --no-tests=error -R '^(runner[.]cheats[.]parser|runner[.]cheats[.]bounds|runner[.]cheats[.]order|docs[.]budgets)$' --output-on-failure` | 0 | Task 2 corrected GREEN; CTest 4/4 in 0.02 s |
| `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]cheats[.]parser|runner[.]cheats[.]bounds|runner[.]cheats[.]order|docs[.]budgets)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("runner.cheats.parser runner.cheats.bounds runner.cheats.order docs.budgets".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-cheats && ctest --preset ci --no-tests=error -R '^(runner[.]cheats[.]parser|runner[.]cheats[.]bounds|runner[.]cheats[.]order|docs[.]budgets)$' --output-on-failure` | 0 | Task 3 GREEN; CTest 4/4 in 0.02 s |
| `set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(runner[.]cheats[.](memory_read|parser|bounds|order)|abi[.]api_v11|docs[.]budgets)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("runner.cheats.memory_read runner.cheats.parser runner.cheats.bounds runner.cheats.order abi.api_v11 docs.budgets".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-cheats schwaystation-test-api-v11 && ctest --preset ci --no-tests=error -R '^(runner[.]cheats[.](memory_read|parser|bounds|order)|abi[.]api_v11|docs[.]budgets)$' --output-on-failure` | 0 | Final CTest 6/6 in 0.17 s; build reported no work |

## Files Created/Modified

- `host/cheats.h` — bounded table, write record, and parse/apply/validate/encode/decode interfaces.
- `host/cheats.c` — transactional parser, full-table validation, checked RAM application, and canonical state codec.
- `tests/runner/cheats.c` — parser, cap, bounds, ordering, rejection atomicity, and canonical state assertions.
- `docs/cheats.md` — supported syntax, limits, application semantics, and canonical format.
- `CMakeLists.txt` — links the shared parser into `schwaystation-host`.

## Decisions Made

- Interpreted the six-digit address field strictly as a physical offset in `0..0x1FFFFF`; unsupported aliases are rejected, not masked.
- Counted configured writes across all entries, including disabled entries, toward the 256-write cap so serialized storage and worst-case work stay fixed.
- Used a format version and fixed slots with reserved-zero validation so state bytes do not depend on compiler layout.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test assertion] Corrected the overlapping RAM boundary expectation**
- **Found during:** Task 2 (Parse and apply the complete write-only subset)
- **Issue:** The initial boundary assertion expected byte `0x5A` at `0x1FFFFF`, but the preceding halfword write also covered that byte and correctly left `0xBE` there.
- **Fix:** Ordered the final byte write after the halfword and asserted the resulting halfword `0x5AEF`.
- **Files modified:** `tests/runner/cheats.c`
- **Verification:** The parser, bounds, order, and docs-budget selectors passed 4/4 after the correction.
- **Committed in:** `85bcb5f` (part of the Task 2 implementation commit)

**Total deviations:** 1 auto-fixed (Rule 1 test assertion)
**Impact on plan:** The correction aligned the boundary test with last-write-wins semantics; no production scope changed.

## Issues Encountered

- The sandbox made the worktree's shared `.git/worktrees` metadata read-only, preventing the plan ledger and worktree sentinel from being created there. The exact starting HEAD was recorded in `/private/tmp/schwaystation-plan-head-before-16-03`; each commit independently checked the fixed worktree root and protected-branch status. Normal commits ran with configured hooks after scoped Git-metadata escalation.
- One task verification initially failed on the overlapping boundary assertion documented above; the corrected test and full focused selection passed.
- End-to-end plan time was 31 minutes, just over the 30-minute split threshold; implementation and focused checks finished before summary and hook closeout extended elapsed time.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

The runner and libretro work can now consume the same parser and canonical table through `host/cheats.h`. The phase orchestrator still owns its source review, broad local workflow, and hosted checks. No owner action is needed for this plan.

## Self-Check: PASSED

- All five owned files exist and the worktree was clean after the implementation commits.
- All six task commits are ancestors of the recorded final task head.
- The final focused selectors passed 6/6, including API v11 and the released memory-read baseline.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*
