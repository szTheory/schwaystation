# Phase 15 Deferred Items

## OpenBIOS shell hash changes on the Phase 15 source head

- **Status:** Open; phase owner review/fix required before final verification.
- **File:** `tests/built/tests.cmake` (expected frame-240 hash); Phase 15 source changes affect the measured output.
- **Evidence:** `built.openbios.shell` fails on the Phase 15 head with `1b819a89fbb391d76011340ee7a762e7583b878e18a79d8f12d4300dbdd79552`. An isolated build from merge base `f08fc08d40a1ebcb42708e0a31be3111e3eed711` passes the same BIOS/region/frame command with the checked hash `df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372`. It was discovered as a CTest fixture setup dependency of `compat.ratchet.rows`; the selected Plan 15-10 tests themselves passed.
- **Why deferred:** The regression is in earlier Phase 15 implementation source, outside Plan 15-10's owned files. The orchestrator should trace and fix it during the required phase review before final phase verification.
