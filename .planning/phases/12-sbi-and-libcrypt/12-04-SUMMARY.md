---
phase: 12-sbi-and-libcrypt
plan: "04"
subsystem: cdrom
tags: [sbi, libcrypt, q-subchannel, getlocp, disc-swap]
requires:
  - phase: 12-01
    provides: bounded SBI parser and CD-ROM state seam
provides:
  - Bad-Q reads and seeks retain the last accepted Q position while sector payloads remain available
  - Applying a new disc provider clears prior Q-position state
affects: [CRYPT-01, CD-ROM GetlocP, disc swap]
tech-stack:
  added: []
  patterns: [provider bad-Q decision before Q-state mutation, provider-swap cache invalidation]
key-files:
  created: []
  modified: [src/cdrom.c, src/cdrom_sector.c, src/disc.c, tests/cdrom/sectors.c, tests/cdrom/sbi.c]
key-decisions:
  - "Bad-Q metadata changes Q-position acceptance only; valid sector bytes and target advancement remain intact."
  - "A new provider resets both saved Q and fallback current LBA before new-disc queries."
requirements-completed: [CRYPT-01]
coverage:
  - id: D1
    description: "A bad-Q data read with valid-looking Q preserves prior-good GetlocP, advances the target, and retains readable sector bytes."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "cdrom.delivery"
        status: pass
    human_judgment: false
  - id: D2
    description: "A SeekL to a bad-Q sector retains the previous accepted Q position while storing the selected sector."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "cdrom.delivery"
        status: pass
    human_judgment: false
  - id: D3
    description: "Swapping disc providers clears the prior disc's saved Q and fallback position before GetlocP."
    requirement: CRYPT-01
    verification:
      - kind: unit
        ref: "cdrom.sbi"
        status: pass
    human_judgment: false
duration: 22min
completed: 2026-11-23
status: complete
---

# Phase 12 Plan 04: Preserve accepted Q state Summary

**Bad-Q sectors remain readable without replacing the prior accepted Q position, and a new disc starts with clean position state.**

## Performance

- **Duration:** 22 minutes for the observed resume and closeout interval; earlier implementation time was not separately captured.
- **Tasks:** 3
- **Files modified:** 5
- **Runtime/model:** Codex/OpenGSD 1.20.0; primary model ID not exposed.

## Accomplishments

- Gated Q-record and current-position updates on the provider's bad-Q result for both scheduled reads and seek completion.
- Preserved sector payload storage and target-LBA advancement for bad-Q data sectors.
- Cleared cached Q bytes, validity and fallback LBA when a new provider is applied.
- Added guest-visible GetlocP regressions for read, seek and two-disc swap behavior.

## Task Commits

1. **Preserve accepted Q on bad reads and seeks** - `edd7eda` (`fix(12-04): preserve accepted Q on bad reads and seeks`)
2. **Clear cached Q state on provider swap** - `ddedc6b` (`fix(12-04): clear cached Q state on provider swap`)

## Test Evidence

- Cheap preflights passed: sorted CD-ROM inventory, `git diff --check`, the fixed 3,901-line repository-check cap, the 300-line RetroArch cap, and discovery of all three expected CTest selectors.
- The focused CI-preset CD-ROM build targets had passed after the final test vectors were added.
- `ctest --preset ci --no-tests=error -R '^cdrom[.](delivery|sbi|sectors)$' --output-on-failure` - exit 0; 3/3 passed in 0.01 seconds.
- `check evaluation-scope --plan 12-04 --commits-only --raw` - exit 0; both plan-scoped commits resolve.
- No hosted CI or Docker run was needed for this plan slice; the changed-head phase gates remain required later.

## Issues Encountered

The total elapsed time before the saved checkpoint was not observable; the 22-minute metric records only the measured resume/closeout interval. No implementation issue remains open from this plan.

## Next Phase Readiness

Plan 12-05 is dependency-ready and will exercise the selected SBI provider through bounded CD-ROM commands and keep the compatibility ledger limited to the behavior those tests prove.

## Self-Check: PASSED

- All three plan outcomes have passing focused coverage.
- `git diff --check`, inventory ordering, budget limits, selector discovery, and plan commit-scope checks pass.
- The summary reports no retail-game or protected-title compatibility claim.

---
*Phase: 12-sbi-and-libcrypt*
*Completed: 2026-11-23*
