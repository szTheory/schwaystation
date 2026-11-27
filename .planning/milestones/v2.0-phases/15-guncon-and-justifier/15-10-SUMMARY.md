---
phase: 15-guncon-and-justifier
plan: "10"
subsystem: quality-and-documentation
tags: [guncon, justifier, compatibility-evidence, quality-bounds]
requires:
  - phase: 15-08
    provides:
      - Capability-gated LIGHTGUN and POINTER input mappings with direct-port topology limits
  - phase: 15-09
    provides:
      - Serializable in-flight gun polls and GPU beam events
provides:
  - Five synthetic gun feature rows tied to passing selectors
  - Generated compatibility guidance distinguishing synthetic predicates from media identity evidence
  - Exact state, WASI and Cachegrind quality measurements for the current Phase 15 source
affects: [phase-15-review, phase-15-verification, compatibility-report]
actuals:
  tokens: 6114
  tasks: 2
  commits: 2
commits: 2
plan_head_before: 807a310e1151f5d9483c71fcb30cf9632b68331f
plan_head_after: 25943c2dd94aa662e481abeb8961f9630bacb81a
tech-stack:
  added: []
  patterns:
    - Compatibility feature rows cite only exercised synthetic selectors.
    - Absolute quality bounds follow measured producers while relative performance limits stay fixed.
key-files:
  created:
    - .planning/phases/15-guncon-and-justifier/15-10-SUMMARY.md
    - .planning/phases/15-guncon-and-justifier/deferred-items.md
  modified:
    - tests/compat/features.csv
    - tests/compat/tests.cmake
    - scripts/compat-doc.sh
    - docs/compatibility.md
    - quality.json
    - .planning/preparation/PSX-PERIPHERALS-AND-SAVES.md
    - .planning/preparation/PSX-GPU-AND-GTE.md
key-decisions:
  - Synthetic gun results do not imply retail-game compatibility or calibrated physical sensitivity.
  - Exact size and instructions-per-frame bounds track current producer measurements; the 100-basis-point relative limit is unchanged.
  - The OpenBIOS shell hash regression is deferred to phase review because it predates Plan 15-10's owned changes.
requirements-completed:
  - GUN-01
  - GUN-02
  - SIO-05
coverage:
  - id: D1
    description: GunCon, Justifier, beam sampling and libretro lightgun behavior are recorded as synthetic evidence backed by passing tests.
    requirement: GUN-01
    verification:
      kind: unit
      ref: ctest --preset ci --no-tests=error -R '^(gpu[.]gun[.]sample|gpu[.]gun[.]geometry|gpu[.]gun[.]irq_order|gpu[.]gun[.]state|sio[.]gun[.]latch|sio[.]gun[.]guncon|sio[.]gun[.]justifier|sio[.]gun[.]state|libretro[.]input[.]lightgun|libretro[.]input[.]pointer|libretro[.]input[.]gun_unavailable|libretro[.]input[.]multitap)$' --output-on-failure
      status: pass
    human_judgment: false
  - id: D2
    description: The generated compatibility page separates title serial/hash results from synthetic feature predicates and makes no retail or calibration claim.
    requirement: GUN-02
    verification:
      kind: other
      ref: sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check
      status: pass
    human_judgment: false
  - id: D3
    description: State bytes, linked WASI size and affected Cachegrind metrics are measured and compared against exact bounds; baseline comparisons stay within the unchanged regression limit.
    requirement: SIO-05
    verification:
      kind: other
      ref: "See Verification: state.stable_bytes, size.wasm, and Cachegrind producer commands and comparisons."
      status: pass
    human_judgment: false
  - id: D4
    description: The focused compatibility CTest selection reaches a failing built.openbios.shell fixture setup on the Phase 15 source head, despite all seven selected compatibility and documentation tests passing.
    requirement: GUN-01
    verification:
      kind: integration
      ref: ctest --preset ci --no-tests=error -R '^(compat[.]features[.]preflight|compat[.]features[.]controls|compat[.]features[.]ratchet|compat[.]ratchet[.]rows|policy[.]compat_doc_fresh|docs[.]api_names|docs[.]budgets)$' --output-on-failure
      status: fail
    human_judgment: false
duration: ~60min
completed: 2026-11-25
status: complete
---

# Phase 15 Plan 10: Synthetic gun evidence and quality handoff summary

Five tested synthetic gun predicates are now in the compatibility feature ledger, the generated report explains how that evidence differs from media-identity results, and the active quality bounds track measured state, WASI and Cachegrind producers. The final handoff includes one Phase 15 source regression that needs review before the phase can pass.

## Performance

- **Duration:** about 60 minutes wall time; the exact handoff start was not separately recorded.
- **Start reference:** immediately after the Plan 15-09 closeout recorded in STATE.md at 2026-11-25T15:05Z.
- **Completed:** 2026-11-25.
- **Tasks:** 2/2 execution tasks complete.
- **Plan task commits:** 2.

The main cost was setting up a task-named Ubuntu 26.04 x86_64 producer container and measuring both the Phase 15 head and its merge base. The container's CMake 4.2.3 could not parse this repository's CMakePresets schema version 11, so the WASI build used the same `wasm32-chd` cache settings through direct `cmake -S/-B` configuration with tests enabled. The required compiler/linker versions were Clang/LLD 21.1.8. `policy.quality_metrics` took 45.81 seconds; no broad workflow, hosted CI, Act run or fuzz campaign ran in this plan.

## Accomplishments

- Added GunCon, Justifier, LIGHTGUN, sensor and state rows to `tests/compat/features.csv`, with exact selector references and matching ratchet expectations. The retail media ledger is unchanged.
- Regenerated `docs/compatibility.md` and clarified that Results are media-identity evidence tied to serials/hashes while Feature evidence contains synthetic test predicates. The report explicitly avoids retail-playability and calibrated-hardware claims.
- Audited controller classes and identifiers, the 228-byte API prefix and 24-byte controller stride, direct-port-only topology, threshold 48, and the public mappings in `docs/controllers.md`, `docs/hardware/sio.md` and `docs/hardware/gpu.md`; those public mapping files already matched implementation, so no extra edits were needed.
- Replaced the stale per-pixel timestamp proposal in preparation notes with the delivered bounded beam-event model and pointers to public GPU/SIO behavior.
- Re-measured quality producers. `quality.json` now records the exact current-head outputs, while the relative Cachegrind regression limit remains 100 basis points.

## Verification

Cheap checks completed before the quality producers:

- `cmake --preset ci` — exit 0.
- `git diff --check` — exit 0.
- `LC_ALL=C sort -cu tests/compat/inventory.txt` and `awk 'NR > 1' tests/compat/features.csv | LC_ALL=C sort -c` — exit 0.
- `scripts/compat-ratchet.sh tests/compat/features.csv tests/compat/features.csv '' tests/compat/allow.txt 2026-11-25 features` — exit 0.
- `cmake -DFEATURES=tests/compat/features.csv -DMODE=features-preflight -P tests/cmake/compat-ratchet.cmake` — exit 0.
- Counted repository-check sources — 3,901/3,901 lines.
- `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md --check` — exit 0.

Targeted behavior and policy checks:

- The 12-selector gun and direct-port topology command in coverage D1 passed 12/12 in 0.09 seconds.
- `cmake --build --preset ci --target schwaystation-test-state` — exit 0 (no work required).
- `ctest --test-dir build/ci --no-tests=error -R '^(policy[.]fixture_budget|docs[.]budgets|policy[.]quality_metrics|abi[.]api_version|core[.]state[.]stable_bytes)$' --output-on-failure` — exit 0, 5/5 passed in 46.05 seconds. The focused `policy.quality_metrics` test took 45.81 seconds.
- `build/ci/tests/schwaystation-test-state state.stable_bytes` — exit 0, `state_bytes=3757754`; `cmake -DQUALITY=quality.json -DMETRIC=state.bytes -DMEASURED=3757754 -P tests/cmake/quality.cmake` — exit 0.
- On an x86_64 Ubuntu 26.04 container, with tests registered, CHD enabled, and Clang/LLD 21.1.8, the linked WASI runner measured 956,794 bytes. `cmake -DQUALITY=quality.json -DMETRIC=size.wasm -DARTIFACT=build/phase15-10-src/build/wasm32/schwaystation-run -P tests/cmake/quality.cmake` passed after updating the exact bound. The first comparison against the prior 947,969-byte pin failed as stale; no value was guessed or padded.
- With GCC 15.2.0 and Valgrind 3.26.0, `env -i "$valgrind" --tool=cachegrind --cache-sim=no ...` used the CI frame windows (testcard 60/360, GPU motion 10/60). Current-head results were 1,649,769 and 138,440,983 Ir/frame. The same producer on merge base `f08fc08d40a1ebcb42708e0a31be3111e3eed711` measured 1,649,208 and 138,364,065; the respective changes are about 3.4bp and 5.6bp, below 100bp. Exact-bound comparisons passed after updating `quality.json`.

One required focused CTest command initially returned exit 8: its seven selected compatibility/documentation tests passed, but the fixture setup `built.openbios.shell` failed the 240-frame BIOS hash. It produced `1b819a89fbb391d76011340ee7a762e7583b878e18a79d8f12d4300dbdd79552` on the Phase 15 head; an isolated build from merge base `f08fc08d40a1ebcb42708e0a31be3111e3eed711` produced the checked hash `df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372`. The cause was resolved during phase review: commit `d7ab38e` limits frame-start GPU synchronization to direct-gun configurations while retaining mode-zero gun sensing. The exact OpenBIOS hash and focused OpenBIOS/GPU/state/SIO regression set then passed; see `15-REVIEW-FIX.md`.

All bounded local commands completed. The task-named containers and images, the two temporary source trees and the temporary source archive were removed; the pre-existing arm64 `ubuntu:26.04` image tag was restored. No task container, volume, network, fuzzer or child process remains.

## Decisions Made

- Compatibility feature rows describe only synthetic predicates and passing tests; no retail serials, ROM identities or measured physical-gun calibration were added.
- Absolute quality bounds follow the measured current-head producer outputs. The 100-basis-point relative Cachegrind gate and the existing workload fingerprint were not changed.
- The OpenBIOS shell fixture was outside this plan's files, so its initial failure was handed to phase review and fixed there as recorded above.

## Deviations from Plan

- Updated the existing `scripts/compat-doc.sh` generator so the required evidence distinction appears in generated output, and extended `tests/compat/tests.cmake`'s protected expected rows alongside the ledger. These are necessary to keep the generated document and ledger ratchet accurate.
- CMake 4.2.3 in the minimal Ubuntu container did not support the project's preset schema version 11. The exact Clang/LLD versions and cache settings were used with a direct configure command; preset execution remains a hosted confirmation.
- The focused compatibility CTest selection exposed the OpenBIOS fixture failure described above. It is an open phase issue, not a passing result.

## Review Follow-up

- The deferred OpenBIOS hash regression is resolved; the malformed multitap save-state finding is also resolved. See [`15-REVIEW-FIX.md`](15-REVIEW-FIX.md) and [`deferred-items.md`](deferred-items.md). Final phase verification remains with the orchestrator.

## Self-Check: PASSED

- The summary and deferred-item files exist.
- Task commits `d7ee26e` and `25943c2` are ancestors of the current plan head.
- `git diff --check` passed before metadata closeout.
