---
phase: 08-per-game-card-saves
plan: "10"
subsystem: libretro-saves
tags: [memory-cards, libretro, RetroArch, CI]
requires:
  - phase: 08-09
    provides:
      - In-process card persistence and the registered two-process RetroArch scenario
provides:
  - Fresh-process proof that both libretro card slots resume across separate RetroArch launches
  - A grounded true libretro_saves capability claim
affects: [phase-08-verification, phase-08-ship, libretro-saves]
actuals:
  tokens: 6463
  tasks: 2
  commits: 9
tech-stack:
  added: []
  patterns:
    - Gate public host capability claims on independent local and exact-head hosted evidence
    - Keep the generated metadata refusal self-test valid for either template boolean
key-files:
  created:
    - .planning/phases/08-per-game-card-saves/08-10-SUMMARY.md
  modified:
    - .github/workflows/ci.yml
    - CMakePresets.json
    - libretro/schwaystation_libretro.info.in
    - tests/cmake/info_claims_self_test.cmake
    - tests/cmake/workflows.cmake
key-decisions:
  - Keep libretro_saves false until both exact-head RetroArch round trips passed, then enable it.
  - Use act for locally reproducible workflow jobs only when it adds coverage beyond the existing local suite; preserve hosted-only gates.
requirements-completed: [SAVE-04]
coverage:
  - id: SAVE-04-host-claim
    description: The generated libretro metadata advertises saves and the checker requires both persistence and control registrations.
    requirement: SAVE-04
    verification:
      kind: integration
      ref: "cmake --preset ci && rg -q '^libretro_saves = \"true\"$' build/ci/libretro/schwaystation_libretro.info && ctest --preset ci --no-tests=error -R '^libretro[.](info[.]claims([.]self_test)?|cards[.](controls|persistence))$' --output-on-failure (exit 0, 4/4)"
      status: pass
    human_judgment: false
  - id: SAVE-04-fresh-process
    description: Two fresh RetroArch launches each ran 120 frames and resumed both 128 KiB card slots, with counters advancing from 01/02 to 02/03.
    requirement: SAVE-04
    verification:
      kind: e2e
      ref: "Exact-head hosted runs https://github.com/szTheory/schwaystation/actions/runs/44604545904 and https://github.com/szTheory/schwaystation/actions/runs/44618451958; required card step passed in both; replay artifacts https://github.com/szTheory/schwaystation/actions/runs/44604545904/artifacts/14571695496 and https://github.com/szTheory/schwaystation/actions/runs/44618451958/artifacts/14576843021."
      status: pass
    human_judgment: false
  - id: SAVE-04-cross-platform
    description: The full required CI matrix and deterministic hash comparison passed on the claim-false baseline and the claim-true implementation head.
    requirement: SAVE-04
    verification:
      kind: integration
      ref: "https://github.com/szTheory/schwaystation/actions/runs/44604545904 (head f52ef90b7653c1f172f3141a85080751e6a306c5, success); https://github.com/szTheory/schwaystation/actions/runs/44618451958 (head d4463e798bcb23b3b059a6844908616e526a9b20, success)."
      status: pass
    human_judgment: false
duration: "at least 224min; active time not measured"
completed: 2026-11-19
status: complete
---

# Phase 8 Plan 08-10 Summary: Verified libretro save claim

**RetroArch resumed both libretro card slots across fresh processes, so the core now advertises its verified save support.**

## Performance

- **Observed wall interval:** At least 224 minutes from the 2026-11-19T18:47:21Z STATE checkpoint through the second exact-head CI roll-up at 22:31:22Z; active time was not measured.
- **Tasks:** 2.
- **Plan commits:** 9, including the preflight and workflow repairs, the self-test repair, and the claim change.
- **Runtime/model:** Codex/OpenGSD session; GPT-6, exact deployed model and CLI minor version unavailable.
- The local full `cmake --workflow --preset ci` completed with exit 0: 621/621 CTests and 5/5 release checks. The first hosted run took about 76m34s; the claim-head run took about 66m53s. BE32 was the critical path in both.

## Accomplishments

- Kept `libretro_saves` false through the claim-false exact-head run, then enabled it only after the required RetroArch step and local in-process card selectors passed.
- Confirmed both slots resumed through two independent RetroArch processes on each hosted head. Each process ran 120 frames; the `.srm` and `.1.mcr` images were 131,072 bytes, with counters advancing `01`/`02` then `02`/`03`.
- The claim-true exact-head run passed the complete required CI matrix and byte-identical cross-platform hash check. Its BE32 job completed in about 65m39s, compared with about 76m on the preceding run.
- Moved the exact stable-state size preflight before the full Linux x64 build and test job. In the claim-head run it completed in 9 seconds before the 4m03s Linux x64 build/test/package step. The report-only perf job skipped on manual dispatch without a control commit; PR gating remains unchanged.
- Added discoverable `act` guidance in `AGENTS.md` and `.planning/AUTONOMOUS.md` (commit `d0c0bb1`): use compatible jobs when they add local coverage, and keep hosted-only runners and integrations gated.
- Updated the claim self-test to derive its baseline from generated metadata so malformed and missing-inventory cases still run when the shipped claim is true. The final repository-check line count remains exactly 3,901.

## Acceptance Evidence

| Check | Result | Evidence |
| --- | --- | --- |
| Local integrated suite with the claim false | Exit 0; 621/621 CTests and 5/5 release tests | `cmake --workflow --preset ci` |
| Claim-true configure and focused selectors | Exit 0; 4/4 passed | `cmake --preset ci && rg -q '^libretro_saves = "true"$' build/ci/libretro/schwaystation_libretro.info && ctest --preset ci --no-tests=error -R '^libretro[.](info[.]claims([.]self_test)?|cards[.](controls|persistence))$' --output-on-failure` |
| Cheap repository consistency checks | Exit 0; whitespace clean and the repository-check ceiling is met | `git diff --check`; `cmake -DROOT="$PWD" '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=3901 -P tests/cmake/line_budget.cmake` |
| Claim-false fresh-process hosted proof | Exit 0; exact head and required card step passed | [Run 44604545904](https://github.com/szTheory/schwaystation/actions/runs/44604545904), head `f52ef90b7653c1f172f3141a85080751e6a306c5`; [replay artifact](https://github.com/szTheory/schwaystation/actions/runs/44604545904/artifacts/14571695496) |
| Claim-true fresh-process hosted proof | Exit 0; exact head, full required matrix, hash equality and required card step passed | [Run 44618451958](https://github.com/szTheory/schwaystation/actions/runs/44618451958), head `d4463e798bcb23b3b059a6844908616e526a9b20`; [replay artifact](https://github.com/szTheory/schwaystation/actions/runs/44618451958/artifacts/14576843021) |
| Phase 8 source review | Clean; no confirmed findings in the save-claim change | [08-REVIEW.md](./08-REVIEW.md) |

The local full suite preceded the final one-line workflow-test consolidation; the claim-head hosted run exercised that consolidation and completed green. After the save self-test was made baseline-independent, the complete plan selector command above was rerun and passed 4/4.

## Task Commits

1. **Task 1: Prepare and prove the claim-false implementation** — `5c89899`, `5f94440`, `f703de4`, `9cd833c`, `ebb4710`, `bfba329`, `f52ef90`.
2. **Task 2: Repair the metadata self-test and enable the verified claim** — `5789c16`, `d4463e7`.

**Additional user-requested CI guidance:** `d0c0bb1`.

## Deviations and Issues Encountered

- The first claim-true selector run exposed a self-test assumption that `libretro_saves` was always false. The self-test now derives the present boolean value before constructing its malformed case; the original `cmake --preset ci` and generated-info assertion had already passed, and the fixed combined command passed 4/4.
- The first repair draft raised the counted repository-check budget to 3,905 lines. It was compressed before verification and commit; the final count is 3,901/3,901.
- An early hosted baseline run found the size ceiling 308 bytes below the measured stable state and cancelled its report-only perf job at the old five-minute timeout. The producer metric was corrected to the measured 3,755,328 bytes, the cheap size preflight now runs before full Linux tests, and the perf allowance is 10 minutes. The repaired exact-head run passed.
- `act` and Docker are installed locally. No duplicate `act` full-suite run was started because the local full CMake workflow had already passed and the exact-head hosted matrix was already running; the repository guidance now requires `act` when it adds locally reproducible coverage, not as a redundant rerun or replacement for hosted-only checks.

## Next Phase Readiness

Phase 8 implementation and SAVE-04 evidence are complete. Proceed with normal phase verification and ship; after the merged PR, green main CI, and automatic release are confirmed, begin Phase 9.

---
*Phase: 08-per-game-card-saves*
*Plan: 08-10*
*Completed: 2026-11-19*
