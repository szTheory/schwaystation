# Phase 11 Deferred Items

## ASan legacy preflight bounds issue

- **Status:** acknowledged
- **File:** `tests/disc/legacy.c:read_manifest`
- **Evidence:** Plan 11-10 recorded an ASan heap-buffer-overflow while the legacy fixture setup passed file bytes to string routines without a NUL terminator. Plan 11-12 confirmed Nightly now runs this preflight and reproduced the failure in a fresh ASan/UBSan configuration. `cmake --build build/plan11-12-asan --target schwaystation-test-disc-legacy schwaystation-legacy-disc-fixtures --parallel 2` exited 0; `ctest --test-dir build/plan11-12-asan --no-tests=error -R '^disc[.]image[.]legacy_(preflight|parity)$' --output-on-failure` exited 0 with 2/2 tests in 0.25 seconds after adding the missing terminator.
- **Why in Plan 11-12:** The new Nightly preflight makes this existing fixture harness a direct prerequisite for the first caller's reliability.
