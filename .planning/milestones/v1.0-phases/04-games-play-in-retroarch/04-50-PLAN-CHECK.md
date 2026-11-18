# Phase 04 focused plan check: POST compatibility

## VERIFICATION PASSED

**Checked:** new plan 04-50 and its dependency/wave change to 04-40 at planning commit `a000942`, against parent `2239cce`.

The proposed code change matches the recorded first fault: OpenBIOS executes a byte store to physical `1F802041h` before splash startup, and the pinned source declares that POST address as an 8-bit register and writes `0Fh`/`0Eh`. The diagnosis explicitly treats this as source evidence, not a hardware measurement or proof of shell success.

Plan 04-50 limits acceptance to that physical byte write through existing EXP2 dispatch. It preserves the existing `charge_port` cost path, EXP2 gap fault behavior for reads, widths 2/4 and neighboring addresses, and profile-controlled debug-port behavior. Its regression covers KSEG aliases, no fault/unbuilt/TTY/exit/status mutation, forbidden access widths and neighbors, and the existing expansion controls. The new case is linked to the existing bus executable, registered with the bus/portable labels and timeout, and represented in the exact inventory. Source, test, and documentation changes are grouped together. The plan adds no API/state field, global state, revision, gate, requirement, or owner verification.

Plan 04-40 still requires actual boot reinspection, independently inspected shell evidence, and the same-run portable oracle. The POST fix grants none of that acceptance; 04-40 resumes after 04-50 and retains its existing coverage.

The native plan-structure check is valid for both plans. The DAG puts completed plans 04-23 and 04-39 before 04-50 in wave 25, then 04-40 in wave 26. The later incomplete waves shift by one; completed plans and their waves remain unchanged. The 50-plan roadmap lists 04-50 as the only open plan in wave 25 and 04-40 as its dependent successor.

**Sizing judgment:** 04-50 forecasts 8–12 minutes for implementation and focused checks, plus 4–6 minutes of ordinary closeout overhead. This is a concrete, bounded source/test/documentation slice under the 30-minute split threshold. It schedules one focused RED/GREEN cycle and one normal CI/hygiene closeout, with no repeated full-suite run for documentation alone and no requirement for a shell boot. No sizing or coverage revision is needed.

**Scope retention:** the original OpenBIOS boot acceptance remains ordered as `04-23 → 04-50 → 04-40`; the source fix only removes the diagnosed blocker, while shell and portable-oracle acceptance remain pending in 04-40. No actionable issue found.
