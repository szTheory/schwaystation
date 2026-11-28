# Phase 17 lessons export

## Result

No new applicable lesson was identified. The Phase 17 CI evidence follows the exact-workload comparison discipline
already recorded in `SW-CI-001` and `SW-CI-002`; its missing PR execution SHA and exact hosted Wasm integer are
evidence limits, not newly reproduced mechanisms. The selected frame-120 result adds one existing NTSC-U fixture
predicate; it does not establish retail playability, another configuration, or a complete session.

## Evidence reviewed

- `sh /private/tmp/schwaystation-plan17-02/collect.sh` — exit 0; 40 exact-attempt runs and job payloads were
  collected for the Phase 17 cohort. See [the cohort report](17-CI-EFFICIENCY.md), including the source run URLs
  and its PR merge-revision limitation.
- `fixtures.gpu_motion.hash.120` and `compat.ratchet.rows` — exit 0 after the frame-120 producer/ledger change;
  the direct runner result was frame 120 hash
  `f92191242105428b4345519f237979aaf0b50b33431ce945f209f03b1d47257d`. See
  [selected-improvement evidence](17-IMPROVEMENT-EVIDENCE.md).
- `cmake -DQUALITY=quality.json -P tests/cmake/quality.cmake` — exit 0; retained registry bounds pass. The
  latest hosted WASI job [140343426889](https://github.com/szTheory/schwaystation/actions/runs/45949888885/job/140343426889)
  succeeded but does not expose an exact module size.

No lesson was appended to `.planning/LESSONS.md` because these findings add no new reproduced mechanism or reusable
fix beyond the existing entries.
