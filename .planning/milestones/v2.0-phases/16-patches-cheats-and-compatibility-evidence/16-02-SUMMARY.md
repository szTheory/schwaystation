---
phase: 16-patches-cheats-and-compatibility-evidence
plan: "02"
subsystem: api
tags: [api, ram, ctest, compatibility]
requires:
  - phase: 16-01
    provides:
      - ABI test inventory and test registration skeleton
      - Released API v10 caller baseline
provides:
  - Checked API v11 writes to physical main RAM
  - Frozen caller coverage for the released v10 input layout
affects:
  - Phase 16 host cheat consumers
  - Future C API callers
actuals:
  tokens: 5904
  tasks: 2
  commits: 3
commits: 3
plan_head_before: 1e5a01c92911d39bfbf550b316ddc9789a3f767e
plan_head_after: 5b0c69664b705eaa6e001f56c51d54f1ed159541
tech-stack:
  added: []
  patterns:
    - Validate all write arguments before mutating instance-owned RAM
    - Keep a versioned API caller for every additive public name
key-files:
  created:
    - tests/api/v11.c
  modified:
    - include/schwaystation.h
    - src/instance.c
    - tests/api/v9.c
    - tests/api/v10.c
    - tests/abi/tests.cmake
    - tests/abi/inventory.txt
    - docs/embedding.md
key-decisions:
  - Preserve the library ABI/SOVERSION and all input struct layouts for this additive API version.
  - Limit writes to aligned byte or halfword values in physical 2 MiB main RAM.
requirements-completed:
  - CHEAT-01
coverage:
  - id: D1
    description: Hosts can write checked little-endian byte and halfword values to physical main RAM without advancing ticks.
    requirement: CHEAT-01
    verification:
      - kind: unit
        ref: tests/api/v11.c check_ram_write_api via abi.api_v11
        status: pass
    human_judgment: false
  - id: D2
    description: Released API v9 and v10 input callers preserve their layouts and behavior under API v11.
    requirement: CHEAT-01
    verification:
      - kind: unit
        ref: abi.api_v9
        status: pass
      - kind: unit
        ref: abi.api_v10
        status: pass
      - kind: unit
        ref: abi.api_v11
        status: pass
    human_judgment: false
metrics:
  duration: "16m 41s"
  completed: 2026-11-26
status: complete
---

# Phase 16 Plan 02: Checked RAM Writes and API v11 Summary

**API v11 exposes checked, tick-neutral writes to physical main RAM while retaining the released v9 and v10 input callers.**

## Performance

- **Runtime/model:** Codex; configured executor model `gpt-6-luna` (runtime model ID not exposed).
- **Duration:** 16m 41s.
- **Started:** 2026-11-26T00:46:24Z.
- **Completed:** 2026-11-26T01:03:05Z.
- **Tasks:** 2/2.
- **Files modified:** 8.

## Accomplishments

- Added `schway_write_ram` to API v11 for checked byte and halfword writes to physical main RAM.
- Added caller checks for little-endian values, final-byte/halfword boundaries, invalid arguments, full serialized-state equality on rejection, and unchanged instance ticks on accepted writes.
- Registered an API v11 caller that retains the complete released v10 gun-extended input layout and frame behavior; relaxed only the older callers' current-version checks to lower bounds.
- Documented the offset, width, alignment, range, value, rejection, and no-tick contract in the embedding guide.

## Task Commits

1. **Task 1: Register the released caller for the additive API** — `6bb67e7` (`feat`).
2. **Task 2 RED: Assert API v11 RAM-write contract** — `22ce083` (`test`).
3. **Task 2 GREEN: Add checked RAM-write operation** — `5b0c696` (`feat`).

## Files Created/Modified

- `include/schwaystation.h` — advances `SCHWAY_API_VERSION` to 11 and declares `schway_write_ram`.
- `src/instance.c` — validates the full request before directly storing little-endian RAM bytes.
- `tests/api/v11.c` — freezes the v10 input layout and tests the new API operation.
- `tests/api/v9.c`, `tests/api/v10.c` — preserve prior input layout and behavior assertions under the newer API version.
- `tests/abi/tests.cmake`, `tests/abi/inventory.txt` — register the new portable caller and selector.
- `docs/embedding.md` — documents the new public operation.

## Execution and Verification Evidence

The isolated worktree started clean at `1e5a01c92911d39bfbf550b316ddc9789a3f767e` on `worktree-agent-p16-02-20261126`. The execution order was:

1. **Task 1 preflight:** `git diff --check HEAD && LC_ALL=C sort -cu tests/abi/inventory.txt` — exit 0.
2. **Task 1 verification:** the following command configured the CI preset, checked both selectors, built the v10/v11 callers, and ran them. Exit 0; 2/2 tests passed in 0.02s. CMake reported 7.3s for the initial configure.

   ```sh
   set -o pipefail && git diff --check HEAD && LC_ALL=C sort -cu tests/abi/inventory.txt && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(abi[.]api_v10|abi[.]api_v11)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("abi.api_v10 abi.api_v11".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-api-v10 schwaystation-test-api-v11 && ctest --preset ci --no-tests=error -R '^(abi[.]api_v10|abi[.]api_v11)$' --output-on-failure
   ```

3. **Tracer feedback gate:** repeated the same Task 1 verification before expanding; exit 0, 2/2 passed in 0.03s.
4. **Task 2 RED:** built the current caller against API v10, then ran the new v11 selector. The target compiled and failed at the intended runtime assertion (`10 != 11`), exit 8, 0.02s. Its JUnit report passed `gsd-tools check tdd-red-evidence` with `RED_EVIDENCE_OK`; the ignored evidence record is `.gsd/tdd-evidence/16-02/red.json`. The temporary report was removed after validation.

   ```sh
   ctest --preset ci --no-tests=error -R '^abi[.]api_v11$' --output-on-failure --output-junit /private/tmp/schwaystation-16-02-red.xml
   ```

5. **Task 2 GREEN verification:** the full focused command below exited 0. Selector preflight found all four names, all v9/v10/v11/ABI-version tests passed (4/4), and CTest reported 0.15s total test time.

   ```sh
   set -o pipefail && git diff --check HEAD && cmake --preset ci && ctest --preset ci --show-only=json-v1 -R '^(abi[.]api_v9|abi[.]api_v10|abi[.]api_v11|abi[.]api_version)$' | python3 -c 'import json,sys; names=[t["name"] for t in json.load(sys.stdin)["tests"]]; required=set("abi.api_v9 abi.api_v10 abi.api_v11 abi.api_version".split()); missing=required-set(names); print("missing selectors:",sorted(missing)); sys.exit(bool(missing) or len(names)!=len(set(names)))' && cmake --build --preset ci --target schwaystation-test-api-v9 schwaystation-test-api-v10 schwaystation-test-api-v11 && ctest --preset ci --no-tests=error -R '^(abi[.]api_v9|abi[.]api_v10|abi[.]api_v11|abi[.]api_version)$' --output-on-failure
   ```

The three task commits passed `git show --check`; the final source diff passed `git diff --check`. No code review or broad local/hosted workflow ran in this plan; the phase orchestrator owns review and reviewed-head verification. No human verification is required for these API behaviors.

## Decisions Made

- Followed settled D-04: increment the public API version to 11, keep struct layouts and the separate library ABI/SOVERSION unchanged, and keep parsing/configuration outside the core.
- Accepted only widths 1 and 2; halfwords are aligned and writes must stay within physical 2 MiB RAM. Invalid requests return `SCHWAY_STATUS_ERR_ARGUMENT` before any store.

## TDD Cycle

- **RED:** `abi.api_v11` failed on the planned runtime version assertion while API v10 was still present. The report showed one executed target assertion, not a compile, discovery, or fixture failure; the GSD classifier returned `RED_EVIDENCE_OK`.
- **GREEN:** API v11, the declaration, implementation, and behavior tests were added. The focused v9/v10/v11/API-version set passed 4/4.
- **REFACTOR:** None needed.

## Deviations from Plan

None. The task-level TDD cycle produced separate test and feature commits. The first sandboxed Git write was denied because worktree Git metadata is read-only in the default sandbox; the same normal commits then succeeded with scoped elevated access and repository hooks enabled.

## Issues Encountered

No implementation or verification blockers remain. The initial Git metadata write restriction was resolved without bypassing hooks or changing the branch.

## Next Phase Readiness

Plan 16-03 can use `schway_write_ram` for the planned host-side cheat adapter. It remains responsible for host parsing, ordering, and configuration behavior.

## Self-Check: PASSED

- Summary file exists at the planned phase path.
- Task commits `6bb67e7`, `22ce083`, and `5b0c696` are ancestors of the plan head recorded above.
- All plan-owned changes are committed; no STATE.md or ROADMAP.md edits were made in this worktree.

---
*Phase: 16-patches-cheats-and-compatibility-evidence*
*Completed: 2026-11-26*
