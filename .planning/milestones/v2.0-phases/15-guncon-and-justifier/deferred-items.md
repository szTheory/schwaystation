# Phase 15 Review Findings

## OpenBIOS shell hash regression — resolved

- **Status:** acknowledged
- **Files:** `src/frame.c`, `src/gpu.c`, `tests/gpu/beam.c`.
- **Cause:** Unconditional GPU synchronization at frame start changed the BIOS timing path even with no gun selected.
- **Fix:** Commit `d7ab38e` limits frame-start synchronization to direct-gun configurations while retaining valid sensor sampling at display mode zero.
- **Evidence:** `built.openbios.shell` now passes the exact 240-frame hash `df2704d7cf3f01ff70ab4534a443729bba77bf73e3488c836513f9f8b86a9372`; the focused OpenBIOS/GPU/state/SIO regression set passed 7/7.
