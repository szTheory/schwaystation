---
quick_id: 261029-ldd
status: complete
requirements-completed:
  - FRAME-02
  - FRAME-09
coverage:
  - id: eight-lane-hashes
    description: All eight lane hashes must match; a missing or different eighth file fails.
    requirement: FRAME-02
    verification:
      kind: integration
      ref: runner.hashes_compare.eight and both negative controls; PR 6 hosted hash-equality
      status: passed
    human_judgment: false
  - id: performance-gate
    description: Two zero-spread controls precede enforcement of the existing +1.00 percent limit.
    requirement: FRAME-09
    verification:
      kind: integration
      ref: perf.gate.* and control runs 40939706087 and 40939709809; PR 6 perf job
      status: passed
    human_judgment: false
  - id: measured-build-timeout
    description: Shared build timeout includes the observed ARM64 overrun and successful samples.
    requirement: Existing tooling seed 024; no new acceptance requirement
    verification:
      kind: other
      ref: Run 40929433476 attempts 1 and 2 and run 39459524424; hygiene.timeout_comments
      status: passed
    human_judgment: false
---

# Phase-1 CI tail

Implementation commit: `37a029b`. Tail PR: https://github.com/szTheory/schwaystation/pull/6.

The eight-file comparison and required dependency replace wasm32 reporting.
The measured timeout seed is completed; no recurring gate was added and the
1,392-line policy budget remains unchanged. Local verification passed 79 tests
and two archive checks, 21 hygiene checks and each exact-inventory check.
The one-off roll-up check exercised ten cases: all-pass, wasm32 failure,
cancellation and wrong skips (PR, push and reusable call), hash failure,
planning-only skips, main push and release call.

Runtime: Codex, OpenGSD 1.19.0, inline quick fallback.
Model: GPT-6 session; exact model ID not exposed.

Hosted run 40940434499 passes on implementation commit 37a029b: all six
native builds, wasm32, be32, sanitizers, eight-file equality, performance,
RetroArch, rehearsal, cleanup and CI required. The performance job reports
zero change in Ir/frame. The final metadata commit changes only planning files;
its head CI and subsequent merge/main evidence are tracked in PR #6.
Next command after it merges green: `$gsd-discuss-phase 2`.

Code review completed inline before verification: inspected workflow dependency
ordering, exact eight-artifact comparison, required-check skip handling,
performance mode and its existing failure tests, measured timeout evidence,
CMake argument escaping, test inventory and documentation. No findings remain.
The phase-2 roll-up gate remains deferred under todo 001.

## Command-generated evidence

- Command: `actionlint -shellcheck= -pyflakes= -ignore 'label "ubuntu-26\.04(-arm)?" is unknown' -ignore 'unexpected key "queue" for "concurrency" section' .github/workflows/ci.yml .github/workflows/release.yml .github/workflows/nightly.yml` — exit 0.

- Command: `cmake --workflow --preset ci` — exit 0.

- Command: `cmake --workflow --preset hygiene` — exit 0.

- Command: `python3 /tmp/schway-ci-tail-rollup.py` — exit 0.

- Command: `cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release -P tests/cmake/check_junit.cmake` — exit 0.

- Command: `cmake -DJUNIT=build/ci/junit-release.xml -DINVENTORY=tests/inventory/ci.txt -DLABELS=release -P tests/cmake/check_junit.cmake` — exit 0.

- Command: `cmake -DJUNIT=build/hygiene/junit.xml -DINVENTORY=tests/inventory/hygiene.txt -P tests/cmake/check_junit.cmake` — exit 0.

- Command: `gh api --allow-escape-sequences repos/szTheory/schwaystation/actions/jobs/123679608811/logs` — exit 0; three measurements of 1,620,424 Ir/frame, zero spread. Run: https://github.com/szTheory/schwaystation/actions/runs/40939706087.

- Command: `gh api --allow-escape-sequences repos/szTheory/schwaystation/actions/jobs/123680601466/logs` — exit 0; three measurements of 1,620,424 Ir/frame, zero spread. Run: https://github.com/szTheory/schwaystation/actions/runs/40939709809.

- Command: `sh scripts/apply-repo-settings.sh --check` — exit 0.

- Command: `gh pr view 5 --json state,url,mergeCommit --jq 'select(.state == "MERGED")'` — exit 0.

- Command: `gh run view 40935782980 --json conclusion,url --jq 'select(.conclusion == "success")'` — exit 0.

- Command: `gh run view 40939706087 --json conclusion,url --jq 'select(.conclusion == "success")'` — exit 0.

- Command: `gh run view 40939709809 --json conclusion,url --jq 'select(.conclusion == "success")'` — exit 0.

- Command: `gh api repos/szTheory/schwaystation/actions/runs/40929433476/attempts/1/jobs --jq '.jobs[] | select(.name == "build (windows-arm64)") | {id,conclusion,started_at,completed_at}'` — exit 0.

- Command: `gh api repos/szTheory/schwaystation/actions/runs/40929433476/attempts/2/jobs --jq '.jobs[] | select(.name == "build (windows-arm64)") | {id,conclusion,started_at,completed_at}'` — exit 0.

- Command: `gh api repos/szTheory/schwaystation/actions/runs/39459524424/jobs --jq '.jobs[] | select(.name == "build (windows-arm64)") | {id,conclusion,started_at,completed_at}'` — exit 0.

- Command: `bash -o pipefail -c 'gh api --allow-escape-sequences repos/szTheory/schwaystation/actions/jobs/123682177268/logs | rg "perf testcard head=[0-9]+ base=[0-9]+ delta_bp=0 verdict=PASS"'` — exit 0.

- Command: `bash -o pipefail -c 'gh api --allow-escape-sequences repos/szTheory/schwaystation/actions/jobs/123683069524/logs | rg "hashes: 8 identical files"'` — exit 0.

- Command: `bash -o pipefail -c 'gh run view 40940434499 --json status,conclusion,headSha,url | jq -e '"'"'select(.status == "completed" and .conclusion == "success" and .headSha == "37a029b99bcfbafe5b88529fcdea9ff27c325622")'"'"''` — exit 0.

- Command: `gh run view 40940434499 --json jobs --jq '.jobs[] | select(.name == "CI required" or .name == "rehearsal" or .name == "cleanup") | {name,conclusion}'` — exit 0.

## Outcome

Both planned tasks are implemented and verified. One Conventional Commit
`ci:` PR carries the entire tail; its final metadata and squash auto-merge are
tracked in https://github.com/szTheory/schwaystation/pull/6. The released v0.1.0
is unchanged, release PR #5 is green, and no release PR is pending.

No owner verification or subsequent GSD step was run.

- Command: `git diff --check` — exit 0.

- Command: `sh scripts/hygiene.sh --tree` — exit 0.
