---
phase: 06-discs-and-fmv
reviewed: 2026-11-17T06:05:36Z
depth: standard
files_reviewed: 17
files_reviewed_list:
  - tests/mdec/reference.c
  - tests/mdec/model.c
  - tests/mdec/tests.cmake
  - tests/mdec/inventory.txt
  - docs/hardware/mdec.md
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - CMakePresets.json
  - tests/cmake/hashes.cmake
  - tests/cmake/workflows.cmake
  - tests/disc/fixture.c
  - tests/disc/tests.cmake
  - tests/hygiene/inventory.txt
  - tests/hygiene/tests.cmake
  - tests/retroarch/run_retroarch.cmake
  - tests/runner/tests.cmake
  - docs/testing.md
findings:
  critical: 0
  warning: 1
  info: 0
  total: 1
  necessity: 0
status: issues_found
---

# Phase 06: Code Review Report

**Reviewed:** 2026-11-16T14:03:43Z

**Depth:** standard

**Files Reviewed:** 5

**Status:** issues_found

## Summary

The direct DMA1 caller contract is represented truthfully: the pinned helper writes its caller buffer pointer to DMA1 MADR without a CPU copy. The comparator reports the pinned hardware log address and this emulator run's DMA1 start separately, then checks the pinned bytes at the emulated start; their address difference is not treated as staging. The documentation distinguishes the matching 4-bit bytes from the unresolved 8-bit arithmetic mismatch. The focused `mdec.reference.4bit` test passed after this change (reported); no tests were run during this review. The prior status-oracle warning remains visible as a deferred tooling seed.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: Status guest oracle discards event identity and status-read count

**File:** `tests/mdec/reference.c:686-710`
**Severity:** WARNING
**Disposition:** Deferred tooling seed.
**Issue:** `collect_status_events` stores only event kinds and collapses consecutive events of the same kind. It accepts any status-read count of at least 100, and `status_order_matches` compares only the collapsed kind sequence. As a result, the pinned status selector can accept a run with missing or extra status reads, or different status/input/output values, whenever the remaining event-kind transitions match. This establishes event-class order, not an ordered match to the pinned records.
**Fix:** Preserve one parsed entry per event and compare exact event counts and order against the pinned log. Where TTY lines contain format placeholders, compare their expected status/data payloads against the guest's traced MDEC bus accesses rather than dropping payload identity.

---

_Reviewed: 2026-11-16T14:03:43Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 23 Review Addendum

**Reviewed:** 2026-11-17T05:56:00Z
**Depth:** standard
**Files Reviewed:** 11 changed files, plus `docs/testing.md` for cross-reference
**Status:** issues_found; all three Plan 23 findings were resolved in follow-up.

### Summary

Reviewed the Plan 23 CI, CTest, fixture, fuzz campaign and RetroArch changes against the plan, project rules and existing media/build documentation. The cross-lane fixture dependency and the five media campaign seed counts and input limits line up. No BLOCKER was found. The initial review found three WARNINGs; follow-up review confirms all three fixes. No tests or network calls were performed during these reviews, and no implementation files were changed by the reviewer.

The plan's hosted RetroArch and nightly campaign acceptance criteria remain pending until their hosted jobs run successfully. This review only confirms the wiring and bounded input setup; it does not establish those runtime outcomes.

### Warnings

#### WR-02: RetroArch job runs when its required OpenBIOS artifact producer fails

**File:** `.github/workflows/ci.yml:508-510`
**Severity:** WARNING
**Disposition:** Resolved in follow-up; the job condition now requires `needs.openbios.result == 'success'` while preserving runs after build-leg failures.
**Issue:** The job now needs `openbios`, which produces both artifacts the job downloads, but `!cancelled()` bypasses GitHub Actions' implicit dependency-success condition. If `openbios` fails before upload, the RetroArch job still starts and fails downloading artifacts it cannot receive. This also obscures the actual producer failure with a predictable secondary failure.
**Fix:** Add `needs.openbios.result == 'success'` to the condition. Keep the existing behavior that allows the host smoke to run when a build matrix leg fails.

#### WR-03: Disc log artifact upload runs before the disc smoke

**File:** `.github/workflows/ci.yml:635-646`
**Severity:** WARNING
**Disposition:** Resolved in follow-up; the upload now follows the disc run and failure-log step, and `docs/testing.md` matches the artifact contents.
**Issue:** At initial review, the `retroarch-replay-probe` upload ran before the disc session, so its `build/ra-disc/*.log` glob could not include the log; the updated guide consequently overstated artifact contents.
**Fix:** The upload now runs after the disc comparison and failure-log step, and retains `build/ra-disc/*.log` when present.

#### WR-04: Testing guide omits the new cross-lane and hosted disc checks

**File:** `docs/testing.md:592-594`
**Severity:** WARNING
**Disposition:** Resolved in follow-up; the guide now covers media/OpenBIOS/disc hashes, the frame-900 comparison, and disc diagnostics.
**Issue:** The `hash-equality` row still describes only `spu_tone.exe` rows, and the `retroarch` row lists the no-content, SPU-tone and pad checks without the new owned-disc boot and frame comparison. Maintainers consulting the guide will miss both Plan 23 acceptance paths.
**Fix:** Update the two table rows to describe the disc predicate/media digests and the frame-900 RetroArch versus runner comparison. Update the diagnostics paragraph below the table to include the third content capture and disc-run log.

---

_Reviewed: 2026-11-17T05:56:00Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 23 Re-review

**Reviewed:** 2026-11-17T06:05:36Z
**Scope:** The three Plan 23 findings above and the current diff for their fixes
**Result:** All three findings resolved; no new issue introduced by the fixes.

- **WR-02:** `retroarch` now checks `needs.openbios.result == 'success'`; it can still run after a build matrix failure when its required artifacts exist.
- **WR-03:** The failure handler prints the disc log, the artifact upload follows the disc run and includes `build/ra-disc/*.log`, and the guide accurately describes those logs.
- **WR-04:** `docs/testing.md` now documents the cross-lane disc/media hashes, frame-900 RetroArch comparison and the disc diagnostics.

No tests, network calls or implementation edits were made in this follow-up. The hosted RetroArch and nightly campaign acceptance criteria remain pending hosted execution.

---

_Reviewed: 2026-11-17T06:05:36Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 24 Review Addendum

**Reviewed:** 2026-11-17T06:14:59Z
**Scope:** Phase 6 branch changes since `d8b42d4`, plus the current Plan 23 CI/docs diff; DISC-01..04, locked decisions and cited hardware docs
**Depth:** deep
**Status:** issues_found

### Summary

Reviewed the CD-ROM, XA/CD-DA, disc boot/host, and MDEC/FMV paths and tests against the phase requirements and current docs. No tests were run. The nightly inventory registers `mdec.reference.8bit` and `mdec.reference.status`; those are distinct from local-only `mdec.guest_input`. The latest recorded MDEC evidence still has the 8-bit exact-output mismatch and a status guest that does not reach Done. DISC-03 therefore remains incomplete. The initial overclaim in the Plan 23 testing-guide row was corrected in follow-up to report these failures explicitly. No new blocker was confirmed in DISC-01, DISC-02, or DISC-04 during this review.

### Critical Issues

#### CR-01: Pinned 8-bit MDEC output still fails exact comparison

**Severity:** BLOCKER
**File:** `src/mdec.c:254-279`; `tests/mdec/model.c:132-173`
**Issue:** The production IDCT/output path still does not match the pinned 8-bit guest oracle: the last recorded `mdec.guest_input` run reports 12 exact-word mismatches, and the nightly `mdec.reference.8bit` selector reports byte 6 expected `04`, actual `05`. The local model selector is not itself in the nightly inventory, but the independent nightly reference selector is. DISC-03 requires bit-exact 8-bit reference output, so this remains a release blocker.
**Fix:** Correct the 8-bit transform/output arithmetic against the pinned `psx.log` bytes. Keep the expected bytes and both selectors intact; leave DISC-03 pending until the exact comparison passes.

### Warnings

#### WR-05: Nightly MDEC status guest does not complete within its bound

**Severity:** WARNING
**File:** `tests/mdec/reference.c:34-36, 293-355, 717-747`; `tests/nightly/inventory.txt:18-27`
**Issue:** The nightly status selector remains registered, but the latest recorded run exits 8 at the unchanged 4,915,200-cycle cap with 82 status events, 82 input writes, zero output reads, and no Done marker. This leaves a configured nightly reference case failing; it is not evidence that the status log matches.
**Fix:** Resolve the guest's stalled MDEC/DMA path while preserving the current run bound and selector; require it to reach Done before claiming this reference passes.

#### WR-06: Testing guide describes failing nightly MDEC references as passing

**Severity:** WARNING
**File:** `docs/testing.md:42`
**Disposition:** Resolved in follow-up; the row now explicitly reports that the latest recorded 8-bit and status reference runs fail and that DISC-03 remains unaccepted.
**Issue:** The initial Plan 23 edit said the nightly lane's “MDEC reference cases pass,” while the nightly inventory includes the failing 8-bit and status selectors. Although the table describes what a green preset would prove, the unqualified wording overstated current coverage.
**Fix:** State the nightly checks and their recorded failures explicitly, without claiming DISC-03 acceptance.

---

_Reviewed: 2026-11-17T06:14:59Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: deep_

## MDEC Scale-Arithmetic Candidate Review

**Reviewed:** 2026-11-17T07:12:31Z
**Scope:** `src/mdec.c`, `tests/mdec/model.c`, `tests/mdec/tests.cmake`, `tests/mdec/inventory.txt`, and `docs/hardware/mdec.md`
**Status:** issues_found

### Summary

The scale arithmetic matches the preparation note's top-13-bit table and `(sum + FFFh) / 2000h` formula; the note leaves exact signed rounding uncertain, and the hardware doc accurately describes the implementation's floor choice as uncertain. The new test is registered and its inventory entry is bytewise sorted. Follow-up review confirms the uniform expected output is correct. No tests were run. Existing CR-01 (8-bit reference mismatch) and WR-05 (status guest timeout) remain open and unchanged.

### Warnings

#### WR-07: Scale-table test expects a partial constant block

**Severity:** WARNING
**File:** `tests/mdec/model.c:222`
**Disposition:** Resolved in follow-up review at 2026-11-17T07:13:53Z; the uniform expectation is correct.
**Issue:** The initial expectation set only the first two words to `0x91919191` and the remaining 14 to neutral `0x80808080`. Rechecking the current uniform expectation against both IDCT index expressions confirms that it matches the input.
**Fix:** No change needed; retain `0x91919191` for all output words.

**Follow-up:** With the zero-scale DC producing coefficient 140 and every upper-13-bit scale equal to `floor(0x5A80 / 8) = 2896`, the first pass writes 49 to `work[0..7]` and zero to the other rows. In the second pass, each output row's `z=0` term reads `work[y]` for `y=0..7`, and all `z>0` terms read zero. Every output sample is therefore `floor((49 * 2896 + 0xFFF) / 0x2000) = 17`; unsigned packing maps each byte to `0x91`. All 16 expected words are uniform. No tests were run.

---

_Reviewed: 2026-11-17T07:12:31Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 45 Corrective-Changes Review Addendum

**Reviewed:** 2026-11-17T21:31:16Z
**Scope:** Current working-tree changes in the Phase 6 hosted-failure repair batch
**Files reviewed:** 7
**Status:** issues_found

### Summary

Reviewed the lazy psxavenc build graph, XA coding-byte normalization, shared MDEC prototype, and hygiene selector removal against the project rules and current call sites. No blocker was found. The pinned encoder source ORs coding flags into an uninitialized sector buffer; the normalization writes the requested value after retaining the duplicate-byte check. The PSX-SPX coding-info table confirms that stereo 37.8 kHz 4-bit audio is value `01h` ([CD-ROM XA subheader](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromformat/), lines 963–977). A small test gap remains for the new normalization, and one guide still names the removed selector.

### Warnings

#### WR-08: XA normalization has no deterministic regression test

**Severity:** WARNING
**File:** `tools/gen_disc.py:120-144`
**Disposition:** Tooling seed under AGENTS.md rule 7.
**Issue:** The generator normalizes the pinned encoder's uninitialized coding byte, but there is no deterministic test that feeds this helper a sector with a noncanonical yet duplicated coding byte and asserts that both copies become `01h`. The producer workflow exercises real encoding, but the original failure depends on stack contents; a future removal or breakage of normalization could pass on a run where those bytes happen to equal `01h`.
**Fix:** Add a small synthetic-sector check for a duplicated arbitrary coding byte becoming `01h`, while confirming mismatched duplicate bytes still fail. Keep the integration producer gate as-is.

### Info

#### IN-01: Asset policy names a removed hygiene selector

**Severity:** INFO
**File:** `ASSET_POLICY.md:154`
**Issue:** The registered `hygiene.no_bios_bytes` test was removed because it duplicated the full-tree scan, but the asset policy still describes that test as scanning every tracked file. The scan is now covered by `hygiene.tree`, with `hygiene.no_bios_bytes.self_test` as its focused negative control.
**Fix:** Update the guide to name `hygiene.tree` and the focused self-test.

---

_Reviewed: 2026-11-17T21:31:16Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 45 Follow-up Review Addendum

**Reviewed:** 2026-11-17T21:35:14Z
**Scope:** Synthetic XA normalization test and hygiene selector-reference updates
**Status:** issues_found

### Summary

The new `fixtures.disc.media_normalize` test directly exercises the helper with a duplicated noncanonical coding byte (`0xF5`), verifies only the two coding bytes change to `0x01`, and verifies mismatched copies fail without changing the file. Its `fixtures,portable` labels and bytewise inventory placement are correct. The prior stale selector references were removed from both documents. One remaining doc phrase points readers to the wrong self-test selector.

### Info

#### IN-02: Asset policy attributes the XA controls to the tree self-test

**Severity:** INFO
**File:** `ASSET_POLICY.md:154-155`
**Issue:** The sentence says `hygiene.tree`'s self-test covers BIOS-by-size and executable-by-content. Those controls are in `hygiene.no_bios_bytes.self_test`; `hygiene.tree.self_test` exercises path/address detection.
**Fix:** Name `hygiene.no_bios_bytes.self_test` explicitly for the BIOS/executable controls.

---

_Reviewed: 2026-11-17T21:35:14Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 45 Python Cache Review Addendum

**Reviewed:** 2026-11-17T21:39:45Z
**Scope:** `sys.dont_write_bytecode` before loading the generator in the synthetic fixture test
**Status:** clean

### Summary

The setting at `tests/fixtures/media_generator.py:10` runs before `exec_module`, so Python's source loader does not write a `.pyc` under `tools/__pycache__`. This prevents the test from dirtying the repository while preserving the in-process test of the actual generator module. No issue found; no tests run during this review.

---

_Reviewed: 2026-11-17T21:39:45Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

### Candidate verification update

The upper-13-bit arithmetic candidate and its isolated test were reverted after the focused 40-test local selection
showed that the candidate also breaks `mdec.vectors`; it did not resolve `mdec.guest_input` or
`mdec.reference.8bit`. The original arithmetic and documentation are restored. The added scale test is no longer
registered. This candidate is not part of the pending phase change.

The same selection passed all 19 disc tests, all five new fuzz replays, all four libretro host tests and the pinned
4-bit MDEC guest. A separate four-selector reference run passed `mdec.reference.4bit` and
`mdec.reference.controls`; `mdec.reference.8bit` and `mdec.reference.status` failed as recorded above. The
post-revert `mdec.vectors` result is recorded in the verification update below. No runtime/hardware result is
inferred from the candidate run.

---

_Reviewed: 2026-11-17T07:15:00Z_
_Reviewer: the agent_
_Depth: follow-up_

### Post-revert verification

The arithmetic candidate was removed and the original transform restored. The public [SCPH-5501 one-block MDEC
capture](https://psx.wiki/tests/raw/20260928T0000Z-5501a-card-h/mdecout_safe/12BLK.txt) prints the exact 8-bit
output words and CRC `728FE2ED`, matching the pinned `ps1-tests` fixture. This independently confirms the expected
bytes; the emulator still differs.

| Check | Result |
|---|---|
| `cmake --preset ci` | exit 0 |
| `cmake --build --preset ci --target schwaystation-test-mdec` | exit 0 |
| `ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure` | exit 8; 10/11 pass, `mdec.vectors` passes after the revert, `mdec.guest_input` retains its 12-word 8-bit mismatch |
| `cmake --build build/mdec-reference --target schwaystation-test-mdec-reference` | exit 0 |
| `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.(4bit|8bit|status|controls)$' --no-tests=error --output-on-failure` | exit 8; 4-bit and controls pass, 8-bit differs first at byte 6 (`04` expected, `05` actual), status reaches the unchanged 4,915,200-cycle cap after 82 input records and no output records |

The focused 40-test Plan 23 local selection built and passed all 19 disc tests, five new parser/device fuzz replays,
and four libretro host tests. It failed only `mdec.vectors` and `mdec.guest_input` while the candidate was present;
the post-revert MDEC run confirms `mdec.vectors` is green again. Hosted CI/nightly/RetroArch acceptance remains
unrun because the required 8-bit reference is still red. DISC-03 remains open; do not ship or claim phase completion.

---

_Reviewed: 2026-11-17T08:01:52Z_
_Reviewer: the agent_
_Depth: verification update_

### Public table-edge probe

The public [SCPH-5501 table-edge capture](https://psx.wiki/tests/raw/20260928T0000Z-5501a-card-h/mdecout_safe/03QTE.txt)
was fetched with `curl -fsSL` (exit 0; 4,268 bytes). A throwaway comparison across its 12 table/scale cases
covered 768 output bytes: the current transform matched 587, the top-13-bit/floor candidate matched 586, and
truncating signed division matched 516. This is exploratory evidence only, not a maintained test or acceptance
result. It did not support another arithmetic edit. The hardware description itself calls the published IDCT
rounding formula approximate and notes that further intermediate rounding may occur [psx-spx MDEC IDCT].

[psx-spx MDEC IDCT]: https://static.grumpycoder.net/pixel/document.pdf

### Status-guest bound diagnosis

The focused `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.status$' --no-tests=error --output-on-failure -V`
still exits 8 at the unchanged cycle cap. The final state has 64 MDEC output words queued and 174 command words
remaining; the last guest I/O was input word `002003dd`, followed by 9,761 emulated cycles before the cap. The
pinned [guest source](https://github.com/JaCzekanski/ps1-tests/blob/f727802fead11f1daa7549285548392ef87749cb/mdec/step-by-step-log/main.cpp)
calls `delay(10000)` after every input write, and the guest prints status text on each iteration. The evidence
indicates the guest had not reached its output-drain loop when the bound expired; it does not show a stuck MDEC
output engine or a passing full status trace. Preserve the cap and report this as an incomplete guest run.

### Status trace budget preflight

A cheap count of the pinned status log found 256 input writes and 512 output reads. The pinned guest calls
`delay(10000)` after each operation; its [delay implementation](https://github.com/JaCzekanski/ps1-tests/blob/f727802fead11f1daa7549285548392ef87749cb/common/delay.c)
uses the argument as a CPU-cycle delay. That requests 7,680,000 cycles before status printing and loop overhead,
which exceeds the fixed 4,915,200-cycle runner limit by 2,764,800. The preflight ran before retrying the status
selector and exited 0 after confirming the expected counts and over-budget result. The focused selector still exits
8 in 0.25 seconds at the preserved cap.

A temporary replay that assigned 1,000 cycles to every unstamped I/O failed at event 4: the capture expected status
`b60400fd`, while the replay had advanced to `b60500fd`. That replay was removed because it invented timing the
hardware log does not provide. Keep the guest gate and cap; this finding explains why that gate cannot complete
with its current delay policy, but does not prove production status equivalence.

### Public single-AC hardware probe

The public [MDEC Hand-Made Streams record](https://psx.wiki/tests/mdec2/) lists 63 signed 8-bit, q_scale-8,
single-AC output CRCs. A temporary driver fed those described streams through the production library and got zero
CRC matches; the driver command exited 0. Because the page exposes the test description and selected output words
but not the exact payload or executable, the synthesized stream is exploratory and does not establish the cause.
The q_scale-zero k=1 sample gives a narrower clue: public word 0 is `07141D23`, while the production stream built
from the published description gives `07141E23`; the DC-only control's reported word is reproduced. This supports
keeping intermediate rounding under investigation, not changing it from an incomplete oracle. The 63-case capture
has not been added as a gate.

### Plan 23 selector wiring preflight

The Plan 23 task 2 command initially selected hygiene policy tests from `build/ci`, where they are not registered;
`--no-tests=error` caught this immediately. The plan now configures `hygiene`, and the two budget selectors pass
2/2 from `build/hygiene`. This was a plan-command correction; no source or acceptance behavior changed.

### Plan 23 changed-source review

**Reviewed:** 2026-11-17T09:13:29Z
**Depth:** standard
**Scope:** Current changed source and CI files against `HEAD`; planning files excluded by the review workflow
**Files reviewed:** 12
**Result:** No new findings.

Reviewed the workflow artifact dependencies and job conditions, fuzz seed generation and bounded campaigns, CTest
fixture ordering, cross-lane hash output, RetroArch content/BIOS arguments, and the owned-disc mailbox/audio/frame
predicates. The current changes connect the generated disc predicates to `runner.hashes_file` in fixture-enabled
lanes and retain the existing eight-file equality check. No tests or network calls were performed. The known CR-01
8-bit MDEC mismatch and WR-05 status-guest bound failure remain open; this review does not clear either acceptance
blocker.

Files reviewed:

- `.github/workflows/ci.yml`
- `.github/workflows/nightly.yml`
- `CMakePresets.json`
- `docs/testing.md`
- `tests/cmake/hashes.cmake`
- `tests/cmake/workflows.cmake`
- `tests/disc/fixture.c`
- `tests/disc/tests.cmake`
- `tests/hygiene/inventory.txt`
- `tests/hygiene/tests.cmake`
- `tests/retroarch/run_retroarch.cmake`
- `tests/runner/tests.cmake`

---

_Reviewed: 2026-11-17T09:13:29Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

### Public single-AC probe correction

**Updated:** 2026-11-17

The initial exploratory driver filled the quant table with byte `0x02`; this was not consistent with the capture
record's parenthetical quant value of 200, so its 0/63 CRC result is superseded. Re-running the production path
with byte value 200 matched 27/63 published CRCs and 146/189 published words 0, 1 and 15. Other values from
`0x20` upward produce the same count because the dequantized coefficient clips, so this does not recover the
exact hardware input. At q_scale 8, k=1, the captured word 0 is `23647F7F` and the generated word is `23657F7F`,
while words 1 and 15 match. The public description lacks the executable and exact payload; this is diagnostic
evidence only. No production arithmetic change is supported. The pinned 8-bit mismatch and status guest bound
failure remain open.

The reproducing commands were `cc -Isrc -Iinclude -Ibuild/ci/generated -Itests -O2 -DNDEBUG -std=c17
build/mdec-reference/quant-probe.c build/ci/libschwaystation.a -o build/mdec-reference/quant-probe &&
build/mdec-reference/quant-probe 200 > build/mdec-reference/quant-probe-200.out` (exit 0) and
`python3 build/mdec-reference/compare-public-probe.py` (exit 0).

## CD-ROM TOC Review Addendum (2026-11-17)

**Reviewed:** 2026-11-17
**Depth:** standard
**Scope:** `include/schwaystation.h`, `src/cdrom.c`, `src/cdrom_sector.c`, `tests/cdrom/reference.c`, `tests/cdrom/model.c`, `tests/cdrom/sectors.c`, `tests/cdrom/audio.c`, and `tests/fuzz/cdrom.c`
**Status:** issues_found; one warning

### Summary

Reviewed the zero-based TOC migration through Play(track), Setloc/GetTD MSF conversion, sector track lookup, seek position updates, GetlocP host-Q and TOC fallback, state serialization, and the changed fixtures. The direct Play(track) index-1 LBA and absolute MSF lead-in conversions are consistent. A saved fallback position can be reconstructed from serialized `current_lba` and the host TOC; validated host Q remains serialized. The updated tests cover equal index-0/index-1 tracks, but do not cover a real pregap. No tests were run, as requested. The existing MDEC CR-01 blocker remains unresolved and its disposition is unchanged.

### Warnings

#### WR-08: TOC-synthesized Q reports pregap sectors as index 1

**File:** `src/cdrom_sector.c:145-159`
**Severity:** WARNING
**Issue:** When a track has `index0 < index1` and host subcode is absent or invalid, `sector_track` selects that track for LBAs beginning at `index0`, but `sector_make_q` always writes Q index `01` and sets relative time to zero until `index1`. Thus GetlocP's synthesized fallback misreports every pregap sector as the start of index 1. PSX Q position documentation identifies pregap as index `00` and says its relative time decreases during the pause ([PSX subchannel layout](https://psx-spx.consoledev.net/dev/ps1/cdr/cdromformat/)). The changed fixtures all set `index0 == index1`, so they do not exercise this case (`tests/cdrom/reference.c:262-263`).
**Fix:** For `index0 <= lba < index1`, synthesize index `00` and a relative time counting down to zero at `index1`; for `lba >= index1`, retain index `01` and count up from `index1`. Add a no-subcode GetlocP regression with distinct index-0 and index-1 LBAs.

---

_Reviewed: 2026-11-17_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 45 Repair-Batch Review and Local Verification (2026-11-17)

The hosted failures on the prior head were reviewed and addressed in one repair batch: media-tool setup is lazy,
the pinned encoder's uninitialized XA coding byte is normalized without weakening subheader validation, the MDEC
cross-file test helper has a shared prototype, and the duplicate hygiene full-tree scan is removed. The review
found no blocker in these changes. The synthetic XA regression test now covers arbitrary duplicated coding
bytes, byte preservation, and mismatched-copy rejection. The policy and requirement text name the active hygiene
selectors and their actual negative controls. A first hygiene run exposed a Python bytecode cache side effect;
the test now disables bytecode writes before dynamically loading the generator, and the tree is clean.

Fingerprint of the complete changed non-planning source set reviewed and locally verified: `afea1856461f5c3dedc1cac56196d333cb062259a76365d69a7ed25c33c3a6fe` (159 files). This includes the earlier
Plan 06-44 source snapshot and this repair batch, with `tests/fixtures/media_generator.py` newly included.

```sh
python3 -c 'import hashlib,pathlib,subprocess; tracked=subprocess.check_output(["git","diff","--name-only","origin/main"],text=True).splitlines(); untracked=subprocess.check_output(["git","ls-files","--others","--exclude-standard"],text=True).splitlines(); paths=sorted(p for p in set(tracked+untracked) if not p.startswith(".planning/")); h=hashlib.sha256(); [(h.update(p.encode()+b"\0"),h.update(pathlib.Path(p).read_bytes())) for p in paths]; print(f"{h.hexdigest()}  {len(paths)} changed non-planning files")'
```

Local evidence on this reviewed snapshot:

- `PKG_CONFIG_LIBDIR=/private/tmp/schwaystation-empty-pkgconfig cmake --preset ci` — exit 0, configure and generate completed without FFmpeg packages visible.
- `cmake --build build/ci --target schwaystation-disc-media --parallel 2` — exit 0; the lazy pinned encoder built and generated media with full coverage.
- Two independent `tools/gen_disc.py --output-dir ... --encoder build/ci/pins/psxavenc-bin/psxavenc` runs — both exit 0; byte comparisons of `cdda.wav`, `movie.str`, `xa.xa` and `media.sha256` matched.
- `python3 tests/fixtures/media_generator.py tools/gen_disc.py` — exit 0; the test's `sys.dont_write_bytecode` guard leaves no `tools/__pycache__`.
- `cmake --workflow --preset ci` — exit 0; 612/612 tests and the release lane 5/5 passed.
- `cmake --workflow --preset hygiene` — final exit 0; 23/23 passed. The first attempt failed only because the synthetic test generated `tools/__pycache__/gen_disc.cpython-314.pyc`; after the test-only fix, the rerun passed with the original full-tree scan retained once.
- `cmake -DROOT=. -DCASE=disc_registration -P tests/cmake/workflows.cmake`, bytewise inventory checks and `git diff --check origin/main -- . ':!tests/probe/rc_hash_disc.c'` — all exit 0.

This review and local result do not establish hosted acceptance. The failed prior hosted runs remain historical
evidence; exact-head CI/Nightly, hosted RetroArch, and the PR-only ship checks still need to pass on the pushed
repair head.

## Plan 06-44 Reviewed-Source Fingerprint

The Plan 06-40 through 06-43 implementation and contract changes were reviewed before final local acceptance. The broader current-worktree review found no open implementation finding after the API v7 correction; the Plan 06-43 follow-up warnings WR-09 and WR-10 were re-reviewed and resolved. Known limits on exact 8-bit and full frame/movie hardware output remain recorded as evidence limits under the approved v1.0 claim, not as passing hardware results. No production changes followed the review; subsequent edits were planning/state bookkeeping only.

Fingerprint of the changed non-planning files reviewed and verified locally: `e3b37688944e0d076f56914547cd2718743e4df6e6e2d3d7d533e6f0a4424752` (157 files). Recomputed from working-tree file contents, sorted by path, excluding `.planning/`; it includes production, tests, build/workflow configuration, examples and user documentation. The exact command was:

```sh
python3 -c 'import hashlib,pathlib,subprocess; tracked=subprocess.check_output(["git","diff","--name-only","origin/main"],text=True).splitlines(); untracked=subprocess.check_output(["git","ls-files","--others","--exclude-standard"],text=True).splitlines(); paths=sorted(p for p in set(tracked+untracked) if not p.startswith(".planning/")); h=hashlib.sha256(); [(h.update(p.encode()+b"\0"),h.update(pathlib.Path(p).read_bytes())) for p in paths]; print(f"{h.hexdigest()}  {len(paths)} changed non-planning files")'
```

This records the source snapshot associated with the final local workflow results below; it is not a hardware-conformance claim.

### WR-08 Follow-up (2026-11-17)

The revised branch emits index `00` with relative time `index1 - LBA` before index 1, then switches to index `01` with relative time `LBA - index1` at index 1. This matches the project's SCM.03 PSX subchannel reference, which specifies decreasing relative time during a pause, and the new controls cover the countdown near its end and the INDEX 01 boundary: LBA 149 yields `00:00:01`, then LBA 150 yields INDEX 01 at `00:00:00`. The hardware documentation describes the same behavior. No new issue found in the specified changes. No tests were run.

## Current Worktree Review Addendum (2026-11-17)

**Reviewed:** 2026-11-17T15:43:00Z
**Depth:** standard
**Scope:** Current Phase 6 worktree diff and adjacent APIs/tests, including profile compatibility, CD-XA scratch ownership, bounded boot-name hashing, embedded source registration, state-field test helpers, SPU MMIO coverage, fixture pins, CUE expectations and CI wiring
**Status:** no open findings in this reviewed diff after the compatibility correction below

### Summary

Reviewed the current implementation and test changes against the Phase 6 requirements, the API versioning decision, and the existing review history. No new security or correctness issue remains open in this diff. During review, the initial API v7 implementation rejected a valid API v6 profile; that BLOCKER was corrected in the shared worktree before review completion, and `tests/api/v7.c` now exercises both the legacy 12-byte behavior and the v7 field split. The bounded XA scratch remains instance-owned and temporary, the boot-name scan is bounded to the fixed 64-byte caller buffer, and the embed build source list contains the moved CD-ROM sector and XA modules. The changed CUE and fixture expectations match the supported parser modes and the corrected zero-based TOC inputs.

The known CR-01 MDEC mismatch and WR-05 incomplete status-guest run remain as recorded above; neither is introduced or resolved by this diff. No residual test/tooling finding from this review is promoted to a requirement; project rule 7 keeps such findings as seeds.

### Resolved Finding During Review

#### CR-02: API v7 profile split initially rejected valid API v6 callers

**Severity:** BLOCKER
**Disposition:** Resolved during review in the shared worktree; not an open finding.
**File:** `src/instance.c:355-371`; coverage in `tests/api/v7.c:37-52`
**Issue:** The initial API v7 implementation rejected `SCHWAY_PROFILE_CD_LICENSED` and `SCHWAY_PROFILE_CD_UNLOCK` in `.flags` whenever a profile used the released 12-byte API v6 size. Both bits were accepted in the API v6 flags mask, so an existing consumer would have received `SCHWAY_STATUS_ERR_PROFILE_REJECTED` despite the field being append-only.
**Fix:** Preserve the legacy CD bits in `.flags` when `profile.size < 16`; use the new split only for size-16 profiles. The current implementation and API v7 test cover both paths.

No tests were run during this review. This artifact records code inspection and the observed in-worktree correction; it does not claim runtime verification.

### Profile v7 Call-Site Follow-up

**Reviewed:** 2026-11-17T15:49:18Z
**Status:** no open findings

Rechecked the three missed production/example call sites after the API v7 split. `runner/main.c:1508-1514` now keeps the TTY setting in `flags` and licensed-disc override in `disc_flags`; `libretro/libretro.c:756-769,1444` names and forwards its parameter as `disc_flags`; `examples/sdl2/main.c:183-188` sets the disc override in `disc_flags`. A repository-wide scan of C headers and sources found no remaining production or example assignment of `SCHWAY_PROFILE_CD_LICENSED` or `SCHWAY_PROFILE_CD_UNLOCK` to `.flags`. Remaining `.flags` profile assignments use debug-port or GPU-v0 bits, and disc test callers use `disc_flags`. No similar missed assignment was found. No tests were run.

---

_Reviewed: 2026-11-17T15:49:18Z_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

## Plan 43 Selector and Evidence Review Addendum (2026-11-17)

**Reviewed:** 2026-11-17
**Depth:** standard
**Scope:** MDEC reference registration, nightly selector inventory, selector workflow consistency check, and current MDEC acceptance/testing documentation
**Status:** issues_found; two warnings

### Summary

The new 12-selector MDEC inventory is internally consistent across the normal nightly, MDEC-only, and CD-ROM-only CTest listings, and the direct 4-bit and 8-bit regression commands remain distinct. The source-coverage selectors are not being mistaken for full decode or image acceptance in the revised requirements and hardware notes. Two follow-ups remain: the testing guide still presents registration and checks as future work, and the unconditional MDEC setup makes the CD-ROM-only configuration fetch and prepare unrelated MDEC assets.

### Warnings

#### WR-09: Testing guide describes landed MDEC selectors as future, unrun checks

**File:** `docs/testing.md:66-80`
**Severity:** WARNING
**Issue:** The guide says Plan 06-43 still owns registration and that its selector list describes only a target inventory rather than tests that have run. The registrations are already present in `cmake/MDECReferences.cmake` and `tests/nightly/tests.cmake`; the focused MDEC reference label has also passed. This leaves maintainers with a stale account of the active test set and evidence.
**Fix:** Rewrite this paragraph in present tense, identify the checks that are registered, and record the focused result as current evidence. Keep the statement that overall DISC-03 remains pending until Plan 06-45 acceptance.

#### WR-10: CD-ROM-only configuration now prepares the entire MDEC reference corpus

**File:** `tests/nightly/tests.cmake:187-207`
**Severity:** WARNING
**Issue:** The file's focused-media comment at lines 48-52 says each media-only configuration prepares only the selected assets and target. The prior `cdrom_reference_only` branch registered MDEC inventory names without preparing their assets; the new unconditional block runs `schway_prepare_mdec_reference_assets` and `schway_prepare_mdec_color_reference_assets` for that configuration as well. The latter validates or fetches the shared MDEC/GPU archive and decodes four PNGs, so a CD-ROM-only configure can now fail or spend time on unrelated media assets when those pins/cache are unavailable.
**Fix:** Keep the exact MDEC selector names and labels in the CD-ROM-only CTest listing, but register them there using the existing missing-asset placeholder pattern instead of preparing MDEC assets/target. Run full MDEC asset preparation only in the normal nightly and MDEC-only configurations.

No implementation files were changed and no tests were run during this review.

---

_Reviewed: 2026-11-17_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_

### WR-09 / WR-10 Re-review (2026-11-17)

**Disposition:** Resolved by inspection. No tests run, as requested.

`docs/testing.md` now says Plan 06-43 registered the selectors and records the focused 12/12 model and nightly MDEC reference results, along with the matching 12-name/label rows in normal nightly, MDEC-only and CD-ROM-only configurations. It still says DISC-03 remains pending through Plans 06-44 and 06-45, so it does not overstate phase acceptance.

`tests/nightly/tests.cmake` now registers all 12 MDEC names with `mdec,nightly,portable` labels in the CD-ROM-only branch using placeholders; that branch does not load `MDECReferences.cmake` or prepare MDEC assets. Normal nightly and MDEC-only use the full asset preparation and real test registration path. The selector names and labels match by inspection; the orchestrator will validate the revised configuration.

Both findings are closed. This disposition is a source/documentation review only.

---

_Reviewed: 2026-11-17_
_Reviewer: the agent (gsd-code-reviewer)_
_Depth: standard_
