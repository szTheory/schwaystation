# Phase 4 plan recheck

## VERIFICATION PASSED

**Phase:** Games play in RetroArch  
**Plans rechecked:** 32  
**Status:** The five grouped findings in `04-PLAN-CHECK.md` are resolved. No remaining blocker, warning, or advisory was found in this focused recheck.

### Recheck evidence

- **First ledger baseline:** `04-24` selects first-introduction mode only when both base ledger and judge are absent, treating prior results as empty/all-unknown while checking candidate rows against measured fixture results and obtaining independent review. Exactly one missing base artifact fails. Later comparisons copy and run both base artifacts, and deleting either head artifact cannot select introduction mode.
- **Tracer execution:** The affected tracer tasks add their test registration, matching inventory row, and executable linkage before verification. Their selectors name the new case; `04-01` reuses the existing core pixel and metadata tests instead of introducing a second GPU executable. `04-03` creates the first GPU test executable with its fill tracer registered before selection. The fixture tasks register their cases and inventory before their workflow/label checks.
- **First-plan scope and budget ownership:** `04-01` now declares 14 files. Its task actions reuse existing pixel/metadata coverage and assign `tests/hygiene/tests.cmake` plus `docs/testing.md` budget changes to `04-12`, which applies the 1,850-line ceiling before adding counted hash checks. The 14-file first-plan scope is the CMake skeleton, five required per-directory inventories, and applicable selectors required by D-23; the former stale ownership entries are absent.
- **Necessary subsystem scopes:** `04-14`, `04-16`, and `04-20` retain their 14 required files and each explains why the files form one executable integration tracer. The scope is retained with its test, state/API/build wiring, and documentation rather than dropping required coverage.
- **Temporary compatibility fixture:** `04-25`/`04-26` define one test-only SWAY-00001 cue/bin fixture with MD5 `12481bed29c6c0d54a6f776ef5cec981`, an independently measured track SHA-1, and matching temporary ten-column ledger and DAT inputs. The test requires exact identity fields and `metadata.identity=pass`, rejects an unjoined or all-unavailable result, and keeps boot/play unknown. The temporary-fixture test is registered in `ci`; the local `library` preset remains outside `ci*` and reads owner paths only for an explicit local invocation.
- **Nightly clipping reference:** `04-30` adds the real reference executable and nightly build target, registers `gpu.reference.clipping` with its nightly inventory, builds the nightly preset, and runs that exact clipping comparison. The CI quantization checks supplement this nightly run; they do not replace it.
- **Coverage and coordination:** All seven GAME requirements remain claimed in plan frontmatter and mapped in `04-COVERAGE.md`; the supplied decision-coverage result is 25/25. The supplied structure result reports all 32 plans valid, and the path and failing-direction results have no findings or read errors. The refreshed plan index shows 14 declared files for `04-01` and no same-wave file overlaps.
- **Command integrity:** The repaired task command blocks have balanced markup/quotes and one-backslash shell regexes. The supplied one-shot check reports `/bin/sh -n` success for every automated shell command; no plan command was executed during this review.

No plan or source files were changed for this review. The next step is phase-4 execution; the standing boundary remains before phase-4 ship.
