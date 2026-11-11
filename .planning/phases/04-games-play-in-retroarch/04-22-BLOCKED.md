# Phase 04 Plan 22: Partial execution checkpoint

**Status:** Implementation is in progress. Do not create a completion summary, mark GAME-05 complete, advance the plan counter, or start dependent plan 04-39 until the actual pinned Linux build and required hosted workflow pass on the committed head.

## Verified evidence

- Linux job from workflow run [43060258449](https://github.com/szTheory/schwaystation/actions/runs/43060258449/job/130674076227), attempt 3, built the pinned OpenBIOS image at `5c72a0e2f67c8b429f9816118a307958e2c2814f`. The log reports exactly 524288 bytes and SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8` using the pinned Linux GCC and the supported C `shell` target.
- The expected digest is recorded in `tests/built/expected.txt`; the synthetic expected-digest check and corrupted-copy refusal both passed in local CI.
- A fresh targeted WASI job [130674004487](https://github.com/szTheory/schwaystation/actions/runs/43060258449/job/130674004487) passed its WASI package setup, then exposed three independent harness issues: optimized test-main stack growth from two 131072-byte automatic buffers, missing Wasmtime preopen for the test working directory, and a nested runner invocation without the configured emulator.
- The fixes move those two test-only buffers to heap allocation, preopen `.` for Wasmtime test runs, and forward the existing `CMAKE_CROSSCOMPILING_EMULATOR` list into the nested runner invocation. The test inventories and assertions remain unchanged.
- Focused local configure/build and all 13 `card.*` and `runner.cards.*` tests passed. The final `cmake --workflow --preset ci` exited 0 with 466 regular tests and 5 release tests; `cmake --workflow --preset hygiene` exited 0 with all 23 checks.

## Hosted recovery history

Workflow run [43060258449](https://github.com/szTheory/schwaystation/actions/runs/43060258449) is not a successful aggregate acceptance run. Earlier apt-dependent Ubuntu jobs failed resolving `azure.archive.ubuntu.com`, `archive.ubuntu.com` and `security.ubuntu.com`; the workflow diff contains no DNS or mirror changes. A fresh-runner targeted WASI retry recovered apt setup and revealed the test-harness failures described above. No mirror, resolver or timeout workaround was added.

## Resume condition

After the implementation and partial-state commit, dispatch the normal CI workflow on that exact committed branch head. Capture `OPENBIOS_RUN_ID` from that actual run, confirm its head SHA matches `git rev-parse HEAD`, inspect the Linux OpenBIOS log for the expected byte count and SHA-256, and require the full `CI required` check to pass. If an attributable Phase 04 hosted failure remains, fix it and run the affected current-head checks again. No firmware bytes are committed, uploaded or published, and no dependent plan has started.
