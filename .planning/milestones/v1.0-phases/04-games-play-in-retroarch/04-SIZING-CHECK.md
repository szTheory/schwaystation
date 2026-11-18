# Phase 04 execution-evidence sizing check

Compared commit `2d9d9622d524e3a34bf06fb39c420ad4eed5be06` with parent `e5c3ccddb9533222a97371c148922b1fe7398ef4`. This is a static review; no application or test workflow was run.

## ISSUES FOUND

**Plans checked:** 04-16 through 04-49, plus unchanged 04-13 for its same-wave interaction  
**Issues:** 0 blockers, 2 warnings

### Warnings

**1. [scope_sanity] Per-slice check load stays within the 30-minute forecast.**

- Plans: 04-16 through 04-49
- Evidence: Each of the 34 plans runs `cmake --workflow --preset ci` in both task verifications and again in plan closeout. Plans 04-46, 04-47, and 04-48 also run the full nightly workflow in both task verifications. `04-SIZING.md` budgets 4–6 minutes for local checks, summary, and commit; the repeated full workflows make that forecast unreliable and add work after test-only expansion tasks.
- Example fix: Keep task-level verification focused on the registered case, then run one complete CI/hygiene closeout per plan. For 04-46–04-48, run the required nightly oracle at the evidence-producing boundary without repeating it for documentation or synthetic controls.

**2. [verify_command_format_sanity] The GPU log selector names only the intended cases.**

- Plan: 04-48, Tasks 1–2
- Evidence: `ctest -R '^gpu.reference.(gp0-e1|mask-bit|gpustat)$'` leaves the dots unescaped, so it also accepts different names with any character in those positions.
- Example fix: Escape the literal dots in the anchored test-name expression.

### Scope retention and sizing judgment

The original plan IDs 04-16–04-32 remain the first callers; added IDs 04-33–04-49 form dependent behavior slices, and the coverage chain maps all original items into those slices. The meaningful boundaries are retained: card read/write/import/state; runner input and persistence; BIOS identity, pinned Linux build, same-run transfer and inspected shell; protected ledger baseline; bounded ISO/CUE metadata and its matching synthetic ledger/DAT result; libretro geometry, persistence and functional claims; then canonical GPU judging, ten real image captures and three real log cases. The work does not reduce the recorded acceptance scope or claim fabricated reference captures.

The contracts reviewed remain explicit: API/state/behavior v4 with frozen v1–v3 consumers; SHA-256 reporting distinct from DAT MD5/SHA-1 recognition; absent-both versus one-missing protected base handling; firmware digest checks before same-run transfer and boot; private-library metadata identity measured from the matching temporary fixture while boot/play stay unknown; todo ownership retained; the 1,850-line gate ceiling and 300-line RetroArch harness bound; no owner UAT; and deferred SPU, CD-ROM, MDEC, and unresolved physical timing work stays out of scope. Original plans with substantial boot or image-fix work keep that work visible and require new defect work to be separately sized.

The native plan-structure query accepts all 34 plans 04-16–04-49, with two complete tasks each. Frontmatter parsing succeeds; all 68 task commands have a corresponding nonempty `<fails_when>` and pass `/bin/sh -n`. Dependencies are acyclic and wave-consistent, and no same-wave plans overlap declared files, including 04-13 and 04-33. Time forecasts in `04-SIZING.md` stay at or below 30 minutes and cite completed phase durations; native token-estimate confidence remains low. The repeated suite invocations above are the remaining sizing risk.

```yaml
issues:
  - plan: "04-16 through 04-49"
    dimension: "scope_sanity"
    severity: "warning"
    required_property: "Per-slice verification overhead is consistent with the stated 30-minute forecast."
    description: "Every checked plan runs the full CI workflow for both tasks and again at closeout; 04-46 through 04-48 also repeat the full nightly workflow per task. The sizing artifact budgets 4–6 minutes for local checks, summary, and commit, so these repeated full runs can invalidate otherwise in-bound forecasts."
    fix_hint: "Use focused registered-case checks between tasks and retain one full CI/hygiene closeout per plan; run nightly oracle workflows only where they produce new required evidence."
  - plan: "04-48"
    task: 1
    dimension: "verify_command_format_sanity"
    severity: "warning"
    required_property: "The CTest selector matches only the three required GPU log test names."
    description: "Both task verification commands use `^gpu.reference.(gp0-e1|mask-bit|gpustat)$`; the unescaped dots can match different test names and allow a required log case to be absent while another case satisfies the filter."
    fix_hint: "Escape the literal dots in the selector."
```

### Recommendation

No blocking coverage or dependency defect was found. The two warnings should be addressed before treating the resized plan set as ready for execution.

## Focused warning recheck — commit `8868485ed366b402ecf82c0957c85aa28138e399`

**Disposition: one warning resolved; one remains.** The task commands now configure with the existing `ci` or `nightly` preset, build their linked consumer/test targets, and select the new or changed test cases. Full CI and hygiene remain once at plan closeout. CMakePresets defines the referenced configure/build presets; the plans identify the targets and test-registration files. Static structure and `/bin/sh -n` checks pass for all 34 changed plans, and the commands retain the actual clipping, nine image, three log, OpenBIOS build/transfer/shell, and hosted run-ID evidence. The 04-48 CTest selector now escapes its literal dots.

**Remaining warning — 04-47, Task 2:** Its action still says “Run all nine actual fetched programs,” while the new verification runs only `gpu.reference.allowance_controls` and the same action later says not to repeat unchanged fetched programs. This leaves contradictory execution instructions that can reintroduce the full nightly rerun the change was meant to remove. Keep the nine actual comparisons in Task 1 and make Task 2’s action consistent with its focused synthetic-control verification.

No build or test workflow was executed during this recheck.

## Coordinator disposition

The residual 04-47 action now reuses Task 1 evidence for all nine actual programs and requires affected real-program retests only when judge, renderer or input changes invalidate it. The contradictory unconditional rerun sentence is removed. Both review warnings are addressed; no behavior or captured expectation changed.
