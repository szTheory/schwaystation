---
phase: 06-discs-and-fmv
plan: "19"
subsystem: independent-disc-hash
tags: [disc, md5, provenance, vendored-source]
requirements-completed: []
coverage:
  - id: DISC-02-ORACLE
    description: "The owned disc boot-executable identity matches the pinned independent rcheevos PlayStation hash, and a changed executable changes both identities."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake --preset ci; cmake --build --preset ci --target schwaystation-test-disc-hash schwaystation-run; ctest --test-dir build/ci -R '^disc\\.hash$|^runner\\.disc\\.probe$|^libretro\\.vendored($|\\.self_test$)' --no-tests=error --output-on-failure — each exit 0; 4/4 selected tests passed. No hosted URL applies."
      status: pass
    human_judgment: false
  - id: VENDORED-DIGEST-CONTROL
    description: "Pinned vendored files match their expected digests, and the negative control rejects a mutated source copy."
    requirement: DISC-02
    verification:
      kind: integration
      ref: "cmake -DCASE=check -DROOT=. -P tests/cmake/vendored.cmake; cmake -DCASE=self_test -DROOT=. -DWORK=build/ci/tests/libretro/vendored-self-test -P tests/cmake/vendored.cmake — both exit 0; the self-test observes the expected digest mismatch. No hosted URL applies."
      status: pass
    human_judgment: false
metrics:
  duration: "Active time not reliably measured; execution resumed after the owner-reported 22-hour pause."
  tasks: 3
  files: 27
  runtime: "Codex/OpenGSD 1.20.0; primary GPT-6 session (exact model ID unavailable); standard gsd-code-reviewer using gpt-6-luna xhigh."
completed: 2026-11-17
status: complete
---

# Phase 6 Plan 19: Independent disc hash oracle

Vendored the exact pinned MIT rcheevos PlayStation disc-hash source into the test tree. The test adapter reads the owned fixture through the production image provider and compares the independent boot-executable identity with the production runner helper; a modified executable verifies both identities change. The runner and shipped library do not include oracle code.

## Review and verification

- Standard review initially found an inaccurate license-header statement and an unused shim. The notice now identifies the upstream root license as the basis, and the dead shim was removed. Both findings are resolved; the review disposition is recorded in [06-19-REVIEW.md](06-19-REVIEW.md).
- `cmake --preset ci` — exit 0.
- `cmake --build --preset ci --target schwaystation-test-disc-hash schwaystation-run` — exit 0.
- `ctest --test-dir build/ci -R '^disc\\.hash$|^runner\\.disc\\.probe$|^libretro\\.vendored($|\\.self_test$)' --no-tests=error --output-on-failure` — exit 0, 4/4.
- `cmake -DCASE=check -DROOT=. -P tests/cmake/vendored.cmake` — exit 0; the source digests match.
- `cmake -DCASE=self_test -DROOT=. -DWORK=build/ci/tests/libretro/vendored-self-test -P tests/cmake/vendored.cmake` — exit 0; the intentionally mutated copy is rejected.
- `LC_ALL=C sort -c tests/disc/inventory.txt`, `git diff --check`, and the 2,700-line repository-check budget — exit 0; the check code is 2,699 lines. The vendored SHA-256 is `03c25f0a834006ab8dbb4bebc83f6ccb7500d1e2d15ea36e8ff8a30641c80444`.
- One initial direct invocation of the self-test omitted its required `-DWORK` path and failed at the CMake `file(COPY)` call; rerunning with the registered test's scratch path passed. The harness and acceptance check required no change.

No physical PlayStation capture or hosted run was needed for this slice. The hardware capture remains available for a future hardware-only question; this automated hash check does not claim physical console equivalence.

## Remaining phase acceptance

DISC-02 and the other Phase 6 requirements remain pending aggregate verification. The remaining plans add the optional installed-library example, hostile-input replay, per-leg equality and hosted background RetroArch checks, followed by phase review and verification.

## Next

Continue Wave 20 with Plan 06-22: build the optional SDL example against the installed library.
