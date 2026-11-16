---
phase: 06-discs-and-fmv
plan: "30"
subsystem: cdrom
tags: [cdrom, subchannel-q, getlocp, save-state]
requires:
  - phase: 06-29
    provides: ReadN/HSTS and pinned DMA3 caller path
provides:
  - CRC/BCD/TOC-validated instance-owned Q records for GetlocP
  - Exact host controls for the three pinned GetlocL/GetlocP rows and state continuation
affects: [06-06, cdrom, save-state]
actuals:
  tokens: 5830
  tasks: 2
  commits: 2
  plan_head_before: 6aaac8089e4ce2c28050fb442a4243f6d918f1df
  plan_head_after: c93c94c6757c06cdb3d3e4728526dd66e696a265
tech-stack:
  added: []
  patterns:
    - Keep validated host Q bytes separate from the raw sector header and target LBA.
    - Validate serialized candidate Q bytes before applying state.
key-files:
  created: []
  modified:
    - src/cdrom.c
    - src/cdrom.h
    - src/cdrom_sector.c
    - src/cdrom_state.c
    - src/internal.h
    - tests/cdrom/model.c
    - tests/cdrom/reference.c
    - tests/core/test_state.c
    - docs/hardware/cdrom.md
    - docs/embedding.md
key-decisions:
  - "GetlocP returns the latest valid host Q record; synthesize a TOC record only before a valid record exists."
  - "Preserve the last valid Q record when later host subcode is absent or invalid."
  - "Keep the pinned seek rows as independent fixtures; apply no general target offset."
patterns-established:
  - "CD-ROM position state owns the exact 12-byte validated Q record and serializes it with the controller."
requirements-completed: []
coverage:
  - id: D1
    description: "Pinned GetlocL headers and Q-derived GetlocP payloads match all three reference seek rows, including lead-in and lead-out."
    requirement: DISC-01
    verification:
      - kind: integration
        ref: "ctest --test-dir build/disc-reference -R '^cdrom\\.reference\\.(getloc|controls)$' --no-tests=error --output-on-failure"
        status: pass
    human_judgment: false
  - id: D2
    description: "The Q record survives save/load; invalid Q is rejected or preserves the last good record, and absent Q has a TOC fallback."
    requirement: DISC-01
    verification:
      - kind: unit
        ref: "ctest --test-dir build/ci -R '^cdrom\\.(command|sectors)$|^core\\.state\\.' --no-tests=error --output-on-failure"
        status: pass
      - kind: integration
        ref: "The pinned reference controls in D1 verify exact payload bytes after state restore and malformed-Q cases."
        status: pass
    human_judgment: false
metrics:
  duration: ">=37m36s"
  completed: 2026-11-16
  commits: 2
status: complete
---

# Phase 6 Plan 30: Raw Q positions for GetlocP Summary

**GetlocP now returns validated host Q bytes independently of GetlocL's raw sector header, with state-format 10 persistence and no universal seek offset.**

## Performance

- **Duration:** At least 37m36s wall time from the 2026-11-16T02:17:11Z start marker through metadata closeout preparation at 02:54:47Z; the final commit follows this measured lower bound.
- **Started:** 2026-11-16T02:17:11Z
- **Completed:** 2026-11-16
- **Tasks:** 2/2
- **Files modified:** 10
- **Runtime/model:** Codex, OpenGSD 1.20.0; executor gpt-6-luna xhigh.

## Accomplishments

- Removed the 1x target-minus-five rule. GetlocL continues to use the target sector header; GetlocP uses the latest CRC-valid Q track, index, relative time and absolute time.
- Accepted the explicit lead-in `00/index 00` and lead-out `AA/index 01` exceptions, while rejecting malformed Q and normal track numbers outside the TOC. Missing or invalid Q preserves an existing good record; a TOC-generated record supplies the initial fallback.
- Serialized the 12-byte record and validity flag in state format 10, with validation against the serialized candidate bytes before state application. Bumped behavior revision to 10.
- Added controls for the pinned rows: seek `00:02:16` returns header `00:02:16` and Q `01/01`, absolute `00:02:11`, relative `00:00:11`; seek `00:00:30` returns header `00:00:29` and Q `01/00`, absolute `00:00:33`, relative `00:01:41`; seek `74:00:00` returns header `73:59:74` and Q `AA/01`, absolute `74:00:03`, relative `03:34:62`.

## Task Commits

1. **Task 1: Pin raw Q/GetlocP rows and state continuation** - `5e84d17` (test)
2. **Task 2: Retain validated Q records through GetlocP and save states** - `c93c94c` (feat)

**Plan metadata:** captured by the final closeout commit.

## Verification

Final reviewed gate, from the repository root, exited 0:

```sh
cmake --build build/ci --target schwaystation-test-cdrom schwaystation-test-state && ctest --test-dir build/ci -R '^cdrom\.(command|sectors)$|^core\.state\.' --no-tests=error --output-on-failure && cmake --build build/disc-reference --target schwaystation-test-cdrom-reference && ctest --test-dir build/disc-reference -R '^cdrom\.reference\.(getloc|controls)$' --no-tests=error --output-on-failure
```

Native CD-ROM and state selections passed 10/10; pinned guest and reference controls passed 2/2. The controls verify all three rows, last-good retention after invalid CRC, `00/index 00`, out-of-TOC track rejection, malformed-Q fallback and restored Q bytes. Review found no remaining blocking findings. `src/cdrom.c` remains below the 1,200-line cap at 1,178 lines.

## Decisions Made

- The three source-log rows are separate observations, not a general seek curve. The implementation consumes the Q record supplied for the requested sector without encoding any of those row offsets.
- Keep the host Q payload instance-owned and include it in state format 10; reject invalid candidate bytes before applying state.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 2 - Missing critical] Preserve and validate the last good Q record**
- **Found during:** Task 2 (Retain validated Q records through GetlocP and save states)
- **Issue:** Replacing a good Q record with absent/bad-CRC data, or validating the destination's old record instead of serialized candidate bytes, could expose wrong guest position data or accept malformed state.
- **Fix:** Preserve the prior record across invalid callbacks and seeks; synthesize only before a valid record exists; validate candidate CRC, BCD, track range and lead-in/lead-out exceptions before apply; add focused reference and state controls.
- **Files modified:** `src/cdrom_sector.c`, `src/cdrom_state.c`, `src/cdrom.h`, `tests/cdrom/reference.c`, `tests/core/test_state.c`, `docs/hardware/cdrom.md`.
- **Verification:** Final gate passed 10 native tests and 2 pinned tests; reviewer confirmed the candidate-reader fix.
- **Committed in:** `c93c94c`.

**Total deviations:** 1 auto-fixed correctness gap.
**Impact on plan:** Kept the changes within the Q/GetlocP/state scope. The measured wall lower bound exceeds the under-30-minute forecast because review required last-good Q preservation and state-candidate validation; no acceptance was removed.

## Issues Encountered

- The first final build caught an undeclared instance parameter in the state-validation call (exit 1). Passing the instance through the CD-ROM state helper fixed it; the complete final gate then exited 0.
- A model expectation initially used the wrong BCD byte order for the synthesized `00:02:00` record; the test was corrected to match the host Q layout.

## User Setup Required

None.

## Next Phase Readiness

Plan 06-30 is complete. DISC-01 remains partial until Plan 06-06 runs the full timing/Getloc gate; resume that plan next. No hosted URL applies to these local checks.

## Self-Check: PASSED

- Summary file exists at the required path.
- Task commits `5e84d17` and `c93c94c` are ancestors of HEAD.

---
*Phase: 06-discs-and-fmv*
*Completed: 2026-11-16*
