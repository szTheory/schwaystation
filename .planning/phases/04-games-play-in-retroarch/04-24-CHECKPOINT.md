# Phase 04 Plan 24 — Incomplete Checkpoint

**Status:** Incomplete. The ten-column ledger/schema slice is committed and passes its focused test. The compatibility transition judge, full registered refusal coverage, generated documentation, and documentation-freshness gate remain unfinished. No `04-24-SUMMARY.md` was created, and the plan/roadmap ordinal remains unchanged.

**Budget decision resolved:** The owner subsequently approved the 2,300-line whole-phase ceiling.
The concrete requirement, configured limit and standing authorization now reflect it. The measurements
and proposal below remain historical provenance; resume the unfinished implementation and controls.

## Committed work

- `59ad2c5d589b`: TDD RED registration and missing-ledger schema assertion. Its focused CTest run exited 8 because the ledger did not exist; `gsd_run check tdd-red-evidence` accepted the named failing assertion.
- `2b8c94c31583`: schema/data GREEN subset. `tests/compat/compat_tests.c` checks ten nonempty fields, supported statuses, date shape, and duplicate serial+MD5+predicate keys. `tests/compat/ledger.csv` records only the measured `gpu_motion`, `pad_counter`, `memcard_roundtrip`, and OpenBIOS fixture results. This does not claim retail-game compatibility or oracle agreement.

The post-commit focused command `cmake --build build/ci --target schwaystation-test-compat && ctest --test-dir build/ci -R '^compat\.ratchet\.rows$' --no-tests=error --output-on-failure` exited 0 (1/1). A temporary duplicate-key ledger was rejected by `build/ci/tests/schwaystation-test-compat schema <temporary-ledger>` with exit 1. Full CI and hygiene were not run for this incomplete slice; the previous hosted run is on `b7fa360`, not this commit.

Measured fixture inputs recorded in the ledger:

| Fixture | Measurement | Registered result |
| --- | --- | --- |
| `gpu_motion` | runner MD5 `65c83347edb2a0d5a74e53d929142bb4`; frame 60 SHA-256 `20c3dcac8ba7b17ac4a6fd611164d1ca2c1aab03419f72c9ef58f04fe6d47042` | `fixtures.gpu_motion.hash.60`, exit 0 |
| `pad_counter` | runner MD5 `8bd0ea92e968e43a0b8a7ea12ba5764e`; press@60 frame 300 SHA-256 `cfcae1a726602580ac1741eb597c951cd3977f8f96a8df6be30c9ea50d9543b4` | `fixtures.pad_counter.press60`, exit 0 |
| `memcard_roundtrip` | runner MD5 `5be739d3557ba03db984cd49ba6b03c3`; second frame 120 SHA-256 `cd1c037ef800397a9c95eb23ebc5524710f55db2eca8e03c975f841f0e9232db` | `fixtures.memcard_roundtrip.persistence`, exit 0 |
| OpenBIOS shell | frame 240 SHA-256 `4d1f0ff260e2cf065ae0b7add4ba60353b2b01241dda8325309b5398ba00acec`; image SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8` | `built.openbios.shell`, exit 0 |

## Budget evidence and decision

The measured repository-check count is `wc -l scripts/*.sh tests/cmake/*.cmake .githooks/*` = **1842**, against the current **1850** ceiling. There are 8 lines of headroom. No counted repository implementation was added by this checkpoint.

Three uncommitted scratch prototypes provide a concrete, tested sizing sample. They are not production gates and omit required integration/edge coverage:

- `/private/tmp/compat-ratchet-prototype.cmake` — 176 lines; SHA-256 `1bbed3bcbe47b56e953a14f0c1f7629f87d07f15d737c69e583917ffd530311c`.
- `/private/tmp/compat-doc-prototype.sh` — 45 lines; SHA-256 `4682addba3bb0a53590be2aac1a037c208dc6de3718af0d21da4bfb43a14c16d`.
- `/private/tmp/compat-protected-base-prototype.sh` — 33 lines; SHA-256 `d4ea521bc53bd1a6b153499c1541bf02cc6bebc7e656dfc872efe741301b154e`.
- Combined prototype size: **254 lines**.
- `/private/tmp/test-compat-prototypes.py` is a scratch proof harness (SHA-256 `da275fdd258b8d891cd7ceb1f361d1b4a6b3fa02fe8f4fad6edf1402d894420b`); it is not counted implementation code.

`python3 /private/tmp/test-compat-prototypes.py` exited 0. The 221-line ratchet/docs prototypes accept the measured fixture ledger, reject pass-to-fail without an allowance, accept an allowance spanning exactly 60 days, reject a 61-day or expired allowance, and reject missing/extra fresh rows and mismatched fresh status. The docs prototype emits stable byte-sorted title rows and status/region/media counts; its `--check` mode rejects deliberately stale output and accepts regenerated output. The separate 33-line protected-base wrapper selects first introduction only when both base artifacts are absent and rejects a one-missing base. In a disposable git repository, the copied base shell adapter invoked the actual ratchet prototype: it accepted unchanged rows (exit 0), then rejected an altered pass-to-fail row after the head judge was deleted (exit 1). This proves the scratch judge under the wrapper path, not the final self-contained artifact packaging or GitHub workflow extraction.

This is **not a minimum implementation estimate**. The prototypes still omit full production field validation, CI/local row selection, every registered edge case, production protected-base packaging/workflow integration owned by 04-41, and complete docs confidence/freshness integration. The official remaining phase allocations are 90 compatibility, 40 documentation freshness, 80 local-library metadata, and 40 `.info` claims lines (250 total). The 254-line prototype sample plus the 120-line library/`.info` estimates gives **2216** counted lines from the current 1842, before omitted controls and integration. This is a partial forecast, not a proven floor. A 2250 cap would leave 34 lines beyond this sample, which may not cover the omitted work. A 2300 whole-phase ceiling would leave 84 lines; this is the current recommendation, with explicit risk that protected-judge integration or missing controls may exceed it. The actual count must be ratcheted at ship, and no counted overflow is authorized before the owner decides.

The protected-base capture and first-introduction selection are outside the 221-line ratchet/docs prototype and belong to 04-41. The 33-line scratch wrapper was exercised against the actual ratchet prototype through a copied base adapter, but it is not integrated with the production artifact or CI workflow. The owner/coordinator must resolve one whole-phase budget that preserves the named checks, or identify an equally concrete source of fault-preserving reuse. Do not reduce the plan contract or commit counted overflow. Plan 04-27 remains independently ready for the coordinator while this decision is pending.

## Resume point

Resume plan 04-24 after the budget decision. Finish the transition/allowance judge, fresh-result matching in both directions, registered temporary-ledger refusal cases, generated compatibility documentation, and stale-document/schema/field controls. Complete code review before final verification, then run the plan's focused checks and the full CI/hygiene closeout once after final source changes. Keep the plan incomplete until all acceptance criteria pass.
