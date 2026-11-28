# Phase 17 selected improvement contract

## Selection

The [quality review](17-QUALITY-REVIEW.md) scores compatibility breadth and upgrade path equally at 2, then selects compatibility breadth by observed impact. This contract selects candidate G from [Plan 17-06](17-06-PLAN.md): make the already-pinned `gpu_motion` frame-120 predicate a freshly measured row in the compatibility ledger. This is one evidence-path improvement; it adds no emulator behavior, new fixture, title, session, profile, provider, or retail-playability claim.

The source premise is confirmed at current HEAD `00f6bbe`; the supporting audit artifacts are dated 2026-11-27. `tests/compat/tests.cmake` runs `gpu_motion.exe` only with `--frames 60`, assigns `frame@60` to the `fixture/gpu_motion` ledger row, and does not select the requested frame from its predicate. The existing fixture source `tests/fixtures/src/gpu_motion.asm` has SHA-256 `c9af0565caa7a5786c270fde6aaf661faea346ed51f597294fc0d9e7bec7eb1f`; `tests/fixtures/tests.cmake` already registers `fixtures.gpu_motion.hash.120` with pinned frame hash `f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d`. The fixture also retains its frame-60 and frame-180 checks. The current ledger has only the frame-60 `fixture/gpu_motion` row. These files demonstrate the omitted live predicate without treating the fixture test as an integrated playability result.

## Exact outcome and ownership

| Item | Contract |
| --- | --- |
| Before | The live compatibility producer measures frame 60 and rewrites the existing `fixture/gpu_motion` row as `frame@60`; there is no live frame-120 result row, even though the existing fixture predicate and expected hash are registered. |
| After | The live producer freshly measures both supported `gpu_motion` predicates, frames 60 and 120, and selects the matching result for each distinct ledger row. It rejects a missing result or any unsupported `gpu_motion` frame. The existing frame-60 pass stays byte-for-byte present, and the additional row uses serial `fixture/gpu_motion-frame120`, the same fixture title/identity/configuration, and predicate `frame@120=f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d`. |
| Implementation/test files | `tests/compat/tests.cmake`, `tests/compat/ledger.csv` (append exactly one new measured row), and generated `docs/compatibility.md`. Do not change `tests/fixtures/tests.cmake`, fixture source or expected hash, required-capability rows, unknown title/provider/profile rows, emulator code, CI routing, or dependency pins. |
| Evidence report | Plan 17-07 records the exact fixture identity and source revision, command, exit status, UTC date, measured result and claim limit in `17-IMPROVEMENT-EVIDENCE.md`. The implementation must use the existing runner and generator. |
| Claim limit | The report gains one additional deterministic frame hash for the named first-party EXE under NTSC-U at frame 120. This does not prove boot-to-play, audio, input-driven control, a full session, retail compatibility, any other profile/region, or broader library support. |

The ledger uses `(serial, title)` as its key. Keep the existing `fixture/gpu_motion` identity unchanged and use the distinct frame-specific serial above so both measured predicates can coexist without weakening duplicate-key rejection.

## Verification contract for Plan 17-07

First reproduce the current before-state on the pre-change source: build the existing runner/fixture targets, run the registered frame-120 predicate, and run the current live compatibility row. Confirm that the pinned frame-120 test is independently available while the current generated compatibility result remains frame 60. Record the exact pre-change source revision and command results; this is an evidence-path gap, not a failing emulator test.

After the change, build the existing runner, fixture and compatibility targets. Run the existing selectors from the repository root:

```sh
ctest --test-dir build/ci --output-on-failure -R '^fixtures[.]gpu_motion[.]hash[.](60|120|self_test)$'
ctest --test-dir build/ci --output-on-failure -R '^compat[.]ratchet[.](rows|transitions|protected_controls)$|^policy[.]compat_doc_fresh$'
cmake -DROOT=. -P tests/cmake/budgets.cmake
git diff --check origin/main
```

The first selector must keep both pinned frame hashes and reject the existing wrong-hash control. The second must freshly accept the unchanged frame-60 row and new frame-120 row, regenerate a fresh matching report, preserve prior ledger results, and reject a wrong or missing frame-120 result through the protected controls. The documentation freshness test must pass after generation; the repository-check count must remain at or below the existing 4,694-line cap. No test/inventory row or required capability is removed or relaxed.

Record each command's actual exit status and the exact source revision/date. Do not claim a broader compatibility or speed improvement. No hosted proof is required for this test/ledger/report-only candidate; the phase's existing hosted gates remain required at final phase verification and are not pre-claimed here.

## Deferred upgrade path

Upgrade path is tied for the minimum score but did not win the impact tie-break. D-11 therefore does not select TODO-010 in this plan: leave `.planning/todos/pending/010-dependabot-config.md` pending and unchanged, do not add `.github/dependabot.yml`, and keep Dependabot auto-merge off. The current audit records enabled vulnerability alerts, zero open alerts, mostly current active action pins and a reviewed Wasmtime patch candidate with exact digests; that does not turn the unselected tie into authorization.
