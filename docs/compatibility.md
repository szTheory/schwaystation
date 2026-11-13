# Compatibility results

Generated from `tests/compat/ledger.csv`.

## Counts

| Status | Region | Media | Rows |
| --- | --- | --- | ---: |
| pass | ntsc-u | bios | 1 |
| pass | ntsc-u | exe | 3 |

## Results

| Title | Serial | Status | Predicate | Confidence | Evidence | Date |
| --- | --- | --- | --- | --- | --- | --- |
| OpenBIOS shell | fixture/openbios-shell | pass | frame@240=df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372 | measured fixture | built.openbios.shell:exit=0;bios_sha256=92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8;core=5cc1dc2 | 2026-11-13 |
| gpu_motion | fixture/gpu_motion | pass | frame@60=20c3dcac8ba7b17ac4a6fd611164d1ca2c1aab03419f72c9ef58f04fe6d47042 | measured fixture | fixtures.gpu_motion.hash.60:exit=0;core=59ad2c5d589b | 2026-11-11 |
| memcard_roundtrip | fixture/memcard_roundtrip | pass | frame@120=cd1c037ef800397a9c95eb23ebc5524710f55db2eca8e03c975f841f0e9232db | measured fixture | fixtures.memcard_roundtrip.persistence:exit=0;core=59ad2c5d589b | 2026-11-11 |
| pad_counter | fixture/pad_counter | pass | frame@300=cfcae1a726602580ac1741eb597c951cd3977f8f96a8df6be30c9ea50d9543b4 | measured fixture | fixtures.pad_counter.press60:exit=0;core=59ad2c5d589b | 2026-11-11 |
