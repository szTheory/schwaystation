# Phase 11 Deferred Items

## ASan legacy preflight bounds issue

- **Status:** Open; out of Plan 11-10 scope.
- **File:** `tests/disc/legacy.c:replace_manifest_field`
- **Evidence:** A focused CTest attempt in the ASan fuzz build failed in the inherited `disc.image.legacy_preflight` setup with a heap-buffer-overflow. The affected parity/replay tests did not run in that attempt. The CI-preset ECM/CCD selector passed 4/4, and the ECM/CCD campaign binaries built and completed bounded one-run seeded smoke checks under ASan/UBSan.
- **Next owner:** Plan 11-12 should determine whether its Nightly/ASan wiring exercises this fixture preflight; if so, reproduce and fix the bounds issue before relying on that lane.
