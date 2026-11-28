# Phase 17 selected improvement evidence

## Selected outcome

Plan 06 selected compatibility breadth. The live compatibility producer now freshly measures `gpu_motion.exe` at frames 60 and 120 and chooses the measured value from each matching ledger predicate. It rejects a missing runner result, an unsupported frame, or a frame that does not match the row's serial. The existing `fixture/gpu_motion` frame-60 row and all other ledger rows remain unchanged. The new row uses the existing pinned frame-120 result:

```text
fixture/gpu_motion-frame120,65c83347edb2a0d5a74e53d929142bb4,-,gpu_motion,ntsc-u,exe,frame@120=f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d,pass,fixtures.gpu_motion.hash.120:exit=0;core=e844b7c,2026-11-28
```

This is a fresh frame hash for the named first-party executable under NTSC-U. It does not establish boot-to-play, audio, input-driven control, a complete session, retail compatibility, another region/profile, or broader library support.

## Source and measurement identity

- **Runner/core source revision:** `e844b7c948e1229aeddb36ec7428f4a441c70cb7`, dated 2026-11-27. Plan 17-07 changed compatibility tests and evidence only; no runner or emulator C source changed.
- **Plan 17-07 implementation commit:** `6082109` (`feat(17-07): add measured gpu_motion frame-120 compatibility evidence`).
- **Measurement date:** 2026-11-28 UTC.
- **Fixture:** existing `tests/fixtures/src/gpu_motion.asm`; the Plan 06 contract records its SHA-256 as `c9af0565caa7a5786c270fde6aaf661faea346ed51f597294fc0d9e7bec7eb1f`.
- **Pinned expectation:** `f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d`, already registered by `fixtures.gpu_motion.hash.120`.
- **Hosted run:** none required for this test/ledger/report-only candidate; no run URL.

## Before and after proof

Before editing, the existing runner, fixture and compatibility targets built successfully. The registered frame-120 fixture predicate passed independently, while `compat.ratchet.rows` passed with the generated fresh ledger containing only `fixture/gpu_motion` at frame 60. This reproduced the evidence-path gap without treating it as an emulator failure.

After appending the new ledger row but before adding its live producer, the same compatibility selector failed as expected with `compatibility ledger has no fresh-result producer for fixture/gpu_motion-frame120`. The final live producer then passed the unchanged frame-60 row and the new frame-120 row.

The direct runner observation was:

```text
Command: build/ci/schwaystation-run --exe build/ci/fixtures/gpu_motion.exe --frames 120 --hash
Exit: 0
Output: frame 120 sha256 f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d
```

A task-local negative control supplied `frame@90` for the frame-120 serial to `compat_live`:

```text
Command: cmake -DSOURCE_ROOT=. -DWORK_ROOT="$TASK_CASE" -DLEDGER="$TASK_CASE/ledger.csv" -DTEST_BINARY=/usr/bin/true -DRUN="$PWD/build/ci/schwaystation-run" -DFIXTURE_DIR="$PWD/build/ci/fixtures" -DEMULATOR=/usr/bin/env -DCASE=compat_live -P tests/compat/tests.cmake
Exit: 1 (expected)
Output: fixture/gpu_motion-frame120 must select frame 120, got frame 90
```

Here `$TASK_CASE` was the disposable `unsupported-predicate` directory under `/tmp`; `$PWD` was the repository root. `compat.ratchet.protected_controls` separately accepts the exact protected frame-120 row and rejects a wrong fresh hash and a missing frame-120 row.

## Verification record

All commands ran from the repository root on 2026-11-28 UTC. Local commands have no run URL.

| Command | Result |
| --- | --- |
| `cmake --build build/ci --target schwaystation-run schwaystation-fixtures schwaystation-test-compat --parallel 2` (before change and after change) | Exit 0 both times; final configure/build completed with no work to do. |
| `ctest --test-dir build/ci --output-on-failure -R '^fixtures[.]gpu_motion[.]hash[.]120$'` (before change) | Exit 0; 1/1 passed. |
| `ctest --test-dir build/ci --output-on-failure -R '^compat[.]ratchet[.]rows$'` (before change) | Exit 0; live fresh output still contained only the frame-60 row. |
| `ctest --test-dir build/ci --output-on-failure -R '^compat[.]ratchet[.]rows$'` (after row, before producer) | Exit 8 (expected test failure); CTest reported the missing frame-120 producer. |
| `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^fixtures[.]gpu_motion[.]hash[.](60|120|self_test)$'` | Exit 0; 3/3 passed, including both pinned frame hashes and the wrong-hash control. |
| `ctest --test-dir build/ci --no-tests=error --output-on-failure -R '^compat[.]ratchet[.](rows|transitions|protected_controls)$|^policy[.]compat_doc_fresh$'` | Exit 0; 9/9 passed, including fixture setup, live rows, wrong/missing protected controls, transitions, and generated-doc freshness. |
| `sh scripts/compat-doc.sh tests/compat/ledger.csv docs/compatibility.md` | Exit 0; the report shows frame 60 and frame 120 as separate measured rows. |
| `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake` | Exit 0. |
| `cmake -DROOT=. -P tests/cmake/budgets.cmake` | Exit 0; documentation byte budgets passed. |
| `cmake -DROOT=. '-DPATTERNS=scripts/*.sh;tests/cmake/*.cmake;.githooks/*' -DLIMIT=4694 -P tests/cmake/line_budget.cmake` | Exit 0; exactly 4,694 counted lines against the 4,694-line limit. |
| `git diff --check origin/main` | Exit 0. |

The frame-120 row is the only ledger addition. The generated documentation adds that row and updates the NTSC-U executable count from three to four; no unknown title, provider, profile, required-capability row, fixture, pin, or emulator behavior changed.
