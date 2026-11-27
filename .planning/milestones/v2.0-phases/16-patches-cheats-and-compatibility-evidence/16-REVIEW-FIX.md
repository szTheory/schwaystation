---
phase: 16-patches-cheats-and-compatibility-evidence
fixed_at: 2026-11-26T16:15:00Z
review_path: .planning/phases/16-patches-cheats-and-compatibility-evidence/16-REVIEW.md
iteration: 1
findings_in_scope: 3
fixed: 3
skipped: 0
status: all_fixed
---

# Phase 16 review fixes

The review covered committed source at `d7ebc888a8f5a65ff900ce1f687bd46618ebe188`. Fixes in `2e8df5a9908982bfae38a2e6924106507ed15c8d` were checked before the full local workflow. Runtime was Codex with OpenGSD 1.23.0 and the GPT-6 Luna xhigh reviewer. The coordinator model ID was unavailable.

- **WASI BPS staging.** The pinned toolchain reproduced the unsupported `tmpfile()` compile error. WASI now creates an exclusive scratch file and unlinks it before staging patch data. Native platforms use their existing staging path. A toolchain probe proved unlinked file reads and writes. The patched runner and the affected tests then built and ran under Wasmtime. Measured compiler frames showed more than 128 KiB in the patch and cheat test executables, so only those executables link with 256 KiB stacks. The patch CLI harness now uses the configured cross-emulator for both executable variants. All 15 focused WASI tests passed. [Commands and measurements](16-LOCAL-EVIDENCE.md)
- **Nightly launch targets.** A one-shot JSON assertion confirmed the fuzz preset includes the PPF and cheat campaign and replay pairs. All four binaries built with Clang 21 and sanitizers, and both replay selectors passed. This repair fulfills the existing workflow path and adds no requirement.
- **Capability status and configure attribution.** A mixed-status JUnit fixture reproduced unrelated failures being assigned to passing rows. Current reports distinguish pass, failure and skipped/unknown per testcase. The importer expects the configure record's SHA-256 in output from the coverage check. An actual native CTest report preserved its four patch passes, while a copy paired with a different configure record left all four unknown. The real package still stays experimental while provider and profile play evidence is missing.

Native patch, report and staged metadata checks passed. The repository-check inventory measured 4,692 of 4,694 allowed lines. The source changes were checked against the reported findings before launching the full local CI workflow.
