---
phase: 06-discs-and-fmv
status: local_pass_hosted_pending
verified: 2026-11-17T21:43:00Z
source: reviewed repair batch committed on phase/06-discs-and-fmv; non-planning source fingerprint afea1856461f5c3dedc1cac56196d333cb062259a76365d69a7ed25c33c3a6fe
---

# Phase 6 verification

The revised Phase 6 contract has passed local acceptance on the reviewed repair snapshot. Phase 6 is **not fully verified** until exact-head hosted acceptance and the ship checks in Plan 06-45 pass. The retained evidence does not claim exact 8-bit hardware output or full frame/movie hardware conformance: the inputs are complete, while the frame/movie guest output remains unresolved. These limitations are explicit in the approved v1.0 claim.

## Current local acceptance (2026-11-17)

The source fingerprint in this file's front matter matches the repaired snapshot recorded in `06-REVIEW.md`. Source and test files did not change after that repair-batch review; only planning, state and evidence bookkeeping followed. Cheap checks ran before the full workflows.

| Gate | Command and result | Status |
|---|---|---|
| Cheap consistency checks | `PKG_CONFIG_LIBDIR=/private/tmp/schwaystation-empty-pkgconfig cmake --preset ci` (exit 0, no FFmpeg package visible); `python3 tests/fixtures/media_generator.py tools/gen_disc.py` (exit 0, no source-tree bytecode cache); sorted fixture/hygiene inventories; `cmake -DROOT=. -DCASE=disc_registration -P tests/cmake/workflows.cmake`; `git diff --check origin/main -- . ':!tests/probe/rc_hash_disc.c'`; all exited 0. | Pass |
| Generated-media determinism | Built `schwaystation-disc-media` with the lazy pinned encoder; exit 0. Two independent media-generation commands exited 0, reported XA/CD-DA/STR coverage, and byte comparisons of `cdda.wav`, `movie.str`, `xa.xa` and `media.sha256` matched. | Pass |
| Complete CI and release lane | `cmake --workflow --preset ci`; exit 0, 612/612 tests passed and `ci-release` 5/5 passed. This includes the deterministic XA-normalization regression, MDEC prototype build, owned disc boot/FMVs and platform test inventory. | Pass |
| Complete hygiene lane | `cmake --workflow --preset hygiene`; final exit 0, 23/23. The first attempt failed because the Python synthetic test created `tools/__pycache__/gen_disc.cpython-314.pyc`; the test now disables bytecode output, the generated cache was removed, and the full workflow passed with one repository tree scan. | Pass |
| Disc and owned-media path | `ctest --test-dir build/ci -L disc --no-tests=error --output-on-failure`; exit 0, 25/25. The complete CI run also passed owned disc boot/FMVs, runner hashes, libretro disk control/host checks, frozen API callers and the built SDL example. | Pass |
| Main-CI MDEC | The full workflow passed 12/12 MDEC selectors, including exact pinned 4-bit, separate 8-bit regression, direct controls and source coverage. | Pass |
| Media fuzz replays | The full workflow passed all five media fuzz replay selectors. | Pass |
| Nightly MDEC references | `ctest --test-dir build/mdec-reference -L mdec --no-tests=error --output-on-failure`; exit 0, 12/12. The producer-written JUnit inventory check passed and confirmed each selected test ran once. | Pass |
| CD-ROM references | `cmake --build build/cdrom-reference --target schwaystation-test-cdrom-reference`; exit 0. `ctest --test-dir build/cdrom-reference -R '^cdrom[.]reference[.](timing|getloc|controls)$' --no-tests=error --output-on-failure`; exit 0, 3/3. The producer-written JUnit inventory check passed. | Pass |
| Reference registration parity | Nightly, MDEC-only and CD-ROM-only CTest JSON comparisons each matched all 12 MDEC selector names and labels. CD-ROM-only configuration also confirmed it did not prepare the unselected MDEC corpus. Main-CI inventory and existing workflow matcher checks passed. | Pass |

These results close the local portion of the previously pending 06-23/24 acceptance. Their hosted RetroArch, exact-head nightly and release checks remain pending Plan 06-45; the phase is not complete and must not be shipped until those checks pass.

## Initial hosted failure and repair (2026-11-17)

The first workflow-dispatch batch was run on `15061b2c8ab8766f2e235f8270d10989675c30f9` and did not pass acceptance:

| Workflow | Run | Observed failures |
|---|---|---|
| CI | [44177128399](https://github.com/szTheory/schwaystation/actions/runs/44177128399) | OpenBIOS media generation rejected XA sector 0's nondeterministic coding byte; `perf` could not configure without FFmpeg despite not building media; two identical full-tree hygiene scans timed out at 60 seconds. |
| Nightly | [44177128259](https://github.com/szTheory/schwaystation/actions/runs/44177128259) | `fuzz` and `bench-wasm32` hit the same eager FFmpeg configure dependency; GCC's full vector build found the MDEC cross-file helper missing a shared prototype. |

These run results are retained as failed attempts, not passes. The repair batch committed at `a621c081703e77bc5a9791d6e2dd7e642b283334` addresses those causes. Exact-head hosted retries and the hosted RetroArch disc comparison are still pending; there is no owner action or hardware capture required.

## Earlier diagnostic evidence (historical)

The results below document the investigation before the approved successor contract and are not the final local acceptance result. The unproved 8-bit/full-image selectors were subsequently replaced only as authorized; their evidence and expected bytes remain preserved.

## Local evidence

| Requirement/path | Command and result | Status |
|---|---|---|
| Disc controller, fixtures, XA/CD-DA and owned boot | Prior focused run: `ctest --test-dir build/ci -R '^disc\.|^mdec\.' --no-tests=error --output-on-failure`; 30 selected, 29 passed, only `mdec.guest_input` failed. The 19 disc selectors passed. | Partial pass |
| Zero-based TOC, Getloc and pregap Q | Built the CD controller, audio, XA, runner-disc and CD-ROM replay targets with `cmake --build --preset ci --target schwaystation-test-cdrom schwaystation-test-cdrom-audio schwaystation-test-cdxa schwaystation-test-runner-disc schwaystation-fuzz-cdrom-replay`; exit 0. `ctest --test-dir build/ci -R '^(cdrom\.|disc\.(xa|audio\.|fixture\.)|runner\.disc\.|fuzz\.regress\.cdrom$)' --no-tests=error --output-on-failure`; exit 0, 19/19 in 12.28 seconds. Nightly reference build exit 0; `ctest --test-dir build/nightly -R '^cdrom\.reference\.(timing|getloc|controls)$' --no-tests=error --output-on-failure`; exit 0, 3/3 in 2.29 seconds. The old Getloc result used a stale binary; fresh Debug, Release and `-O0` builds had the same failure. The test TOCs were corrected to the public zero-based-LBA contract, and production track selection and synthesized pregap Q timing now use that contract. | Pass locally |
| Libretro host and runner hash seam | Built `schwaystation-test-libretro-host` and `schwaystation-run`; exit 0. `ctest --test-dir build/ci -R '^(libretro\.host|runner\.hashes_file)$' --no-tests=error --output-on-failure`; exit 0. CTest's final log records all four selected tests/dependencies passing: `disc.fixture.fmv` (3.57s), `built.openbios.shell` (0.64s), `runner.hashes_file` (11.70s), and `libretro.host` (39.12s). | Pass locally |
| Disc controller, parser/device fuzz replays and libretro host | `cmake --build --preset ci --target schwaystation-test-disc-fixture schwaystation-test-libretro-host schwaystation-run && ctest --test-dir build/ci -R '^disc\.|^libretro\.host|^fuzz\.regress\.(cue|cnf|m3u|cdrom|mdec)$' --no-tests=error --output-on-failure`; exit 0, 28/28 passed in 53.31 seconds: 19 disc, five replay, and four libretro host tests. The red `mdec.guest_input` and nightly reference selectors remain registered and were not included in this focused slice. | Pass locally |
| MDEC completed-command status | `cmake --build --preset ci --target schwaystation-test-mdec`; exit 0. `ctest --test-dir build/ci -R '^mdec\.status$' --no-tests=error --output-on-failure`; exit 0, 1/1. Checks MDEC(2)/MDEC(3) final status `8004FFFFh` and `FFFFh` after decode input is consumed while a block is pending. | Pass locally |
| MDEC local model | `ctest --test-dir build/ci -L mdec --output-on-failure`; exit 8, 10/11 passed. Status, 4-bit, vectors, color, FIFO and DMA cases pass; `mdec.guest_input` still reports 12 exact 8-bit output-word mismatches. | Fail |
| Pinned MDEC references | `cmake --build build/mdec-reference --target schwaystation-test-mdec-reference`; exit 0. `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.(4bit|8bit|status|controls)$' --no-tests=error --output-on-failure`; exit 8. The 4-bit and control selectors pass. The 8-bit selector first differs at byte 6 (`04` expected, `05` actual). The status guest reaches the unchanged 4,915,200-cycle cap after 82 input records with no CPU output reads; a focused state trace shows 64 words already queued in the MDEC output FIFO. | Fail |
| First-divergence diagnosis | `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.(4bit|8bit|controls)$' --no-tests=error --output-on-failure --parallel 3`; exit 8, with 4-bit and controls passing and 8-bit failing at byte 6. The captured post-IDCT sample is `-123`, which maps to actual `0x05`; expected `0x04` implies `-124`. The same signed-sample XOR `0x80` conversion explains all 16 differing bytes. Captured DMA1 bytes are present at the exact DMA1 RAM target, and expected output is absent elsewhere in RAM. This rules out packing and target-address errors but does not distinguish parser/dequantization from IDCT arithmetic. No supported formula correction was found; no source or expected bytes changed. | Unresolved hardware arithmetic |
| Cheap consistency checks | `git diff --check`; exit 0. `LC_ALL=C sort -c tests/mdec/inventory.txt`; exit 0. `LC_ALL=C sort -c tests/hygiene/inventory.txt`; exit 0. `cmake --preset ci`; exit 0. | Pass |
| Plan 23 policy-selector preflight | The initial Plan 23 command configured `ci` then selected `policy.e2e_budget` and `policy.gate_budget` from `build/ci`; CTest selected none and exited 8. `cmake --preset hygiene`; exit 0. `ctest --test-dir build/hygiene -R '^policy\.e2e_budget$|^policy\.gate_budget$' --no-tests=error --output-on-failure`; exit 0, 2/2. The plan command now uses the preset that owns those selectors. | Pass |
| Plan 23 workflow preflight | `cmake -DROOT=. -DCASE=disc_registration -P tests/cmake/workflows.cmake && cmake --list-presets >/dev/null && LC_ALL=C sort -c tests/mdec/inventory.txt && LC_ALL=C sort -c tests/hygiene/inventory.txt && git diff --check`; exit 0. Confirms CI/nightly wiring, preset parsing, sorted inventories and patch formatting without rerunning the known-red MDEC suite. | Pass |
| Disc predicates in per-leg hashes | `ctest --test-dir build/ci -R '^runner\.hashes_file$' --no-tests=error --output-on-failure`; exit 0, 3/3 in 16.07 seconds. CTest ran `disc.fixture.fmv` and `built.openbios.shell` as registered producers, then `runner.hashes_file` consumed their outputs. `python3 -c 'from pathlib import Path; import re; rows=[line for line in Path("build/ci/hashes.txt").read_text().splitlines() if re.match(r"^(boots|mailbox|audio|frame)@(?:600|900) disc_boot ", line)]; assert len(rows)==8; assert rows[0]=="boots@600 disc_boot none pass" and rows[4]=="boots@600 disc_boot press@300 pass"; assert all("sha256=" in line for line in rows if line.startswith(("mailbox@", "audio@", "frame@")))'`; exit 0, all eight predicates present in fixed order. | Pass locally |
| Status trace budget preflight | `python3 -c 'from pathlib import Path; import re; s=Path("build/mdec-reference/source/mdec-step-psx.log").read_bytes(); i=len(re.findall(rb"^MDEC_DATA <- 0x[0-9a-fA-F]{8}",s,re.M)); o=len(re.findall(rb"^MDEC_DATA -> 0x[0-9a-fA-F]{8}",s,re.M)); print(f"input={i} output={o} requested_delay={(i+o)*10000} budget={19200*256}"); assert i==256 and o==512 and (i+o)*10000>19200*256'`; exit 0. It prints 256 input writes, 512 output reads, 7,680,000 requested delay cycles and a 4,915,200-cycle guest limit. | Confirms the full guest cannot fit this bound |
| Public single-AC exploratory probe | From the repository root: `cc -Isrc -Iinclude -Ibuild/ci/generated -Itests -O2 -DNDEBUG -std=c17 build/mdec-reference/quant-probe.c build/ci/libschwaystation.a -o build/mdec-reference/quant-probe && build/mdec-reference/quant-probe 200 > build/mdec-reference/quant-probe-200.out`; exit 0. `python3 build/mdec-reference/compare-public-probe.py`; exit 0. Against the public capture, the synthesized q_scale-8 streams matched 27/63 CRCs and 146/189 published words 0, 1 and 15. The input payload/executable is not published, so this is exploratory, not a gate. | Inconclusive |
| Final reviewed-worktree CI | `cmake --workflow --preset ci`; exit 8. Configuration/build passed, then 609/610 CTest tests passed in 65.77 seconds. The only failure was `mdec.guest_input`; all 25 disc-labeled tests passed in the same run. | Fail: one known MDEC gate |
| Final focused disc label | `ctest --test-dir build/ci -L disc --no-tests=error --output-on-failure`; exit 0, 25/25 in 11.48 seconds. | Pass |
| Final focused MDEC label | `ctest --test-dir build/ci -L mdec --no-tests=error --output-on-failure`; exit 8, 10/11 in 0.08 seconds. `mdec.guest_input` reports 12 exact word mismatches, first actual `0x0005FFFF`, expected `0x0004FFFF`. | Fail |
| Final hygiene workflow | `cmake --workflow --preset hygiene`; exit 0, 24/24; CTest wall time 39.61 seconds. | Pass |
| CI after status-counter correction | `cmake --workflow --preset ci`; exit 8, 609/610 tests passed in 66.57 seconds. The only failure remains `mdec.guest_input`; no cross-module regression appeared. | Fail: known 8-bit MDEC oracle |
| Hygiene after final documentation update | `cmake --workflow --preset hygiene`; exit 0, 24/24 tests passed in 39.77 seconds. | Pass |

An experimental upper-13-bit scale arithmetic change was reviewed and tested, then reverted: although its isolated test
passed, it also broke `mdec.vectors` and did not fix the pinned 8-bit output. It is absent from the current worktree.

## Independent hardware evidence

The public [SCPH-5501 one-block MDEC capture](https://psx.wiki/tests/raw/20260928T0000Z-5501a-card-h/mdecout_safe/12BLK.txt)
prints the same 8-bit output words and CRC `728FE2ED` as the pinned `ps1-tests` log. The expected bytes are
independently corroborated; the emulator output remains wrong.

The [SCPH-5501 MDEC status/decode capture](https://psx.wiki/tests/mdec-status-decode-timing/) reports
`8004FFFFh` after a complete table load and `2204FFFFh` after all decode input is consumed while output remains
unread. The model now reproduces the terminal parameter counter for MDEC(1), MDEC(2) and MDEC(3); this local
correction does not alter the pinned status guest or its 4,915,200-cycle cap.

The [public status/decode timing record](https://psx.wiki/tests/mdec-status-decode-timing/) measures hardware output
and timing for separate streams. It does not supply a completion allowance for the pinned step-by-step guest. The
pinned [guest source](https://github.com/JaCzekanski/ps1-tests/blob/f727802fead11f1daa7549285548392ef87749cb/mdec/step-by-step-log/main.cpp)
calls `delay(10000)` after each of its input writes and output reads; the pinned
[delay implementation](https://github.com/JaCzekanski/ps1-tests/blob/f727802fead11f1daa7549285548392ef87749cb/common/delay.c)
loops for the requested cycle count. The complete recorded sequence therefore requests 7,680,000 delay cycles,
2,764,800 more than the fixed 4,915,200-cycle guest limit, before its repeated status printing and control flow.
Keep the limit unchanged.

The focused `ctest --test-dir build/mdec-reference -R '^mdec\.reference\.status$' --no-tests=error --output-on-failure`
previously exited 8 in 0.25 seconds at the unchanged bound. Its final state has 64 output words queued, 174 command words
remaining, and no CPU output reads. The last input was `002003dd`; the guest had not reached its output-drain loop.
This is a bounded guest incompletion, not evidence of a stuck output engine. A temporary host-side replay with an
invented 1,000-cycle spacing changed status at event 4 (`b60400fd` expected, `b60500fd` actual); it was discarded.
The pinned log has no cycle stamps, so that replay cannot establish the captured timing or replace the guest gate.

The public [table-edge MDEC capture](https://psx.wiki/tests/raw/20260928T0000Z-5501a-card-h/mdecout_safe/03QTE.txt)
was fetched with `curl -fsSL` (exit 0; 4,268 bytes). A throwaway comparison of 12 table/scale scenarios across
768 output bytes matched 587 bytes with the current transform, 586 with the top-13-bit/floor candidate, and 516
with truncating signed division. [psx-spx's MDEC note](https://psx-spx.consoledev.net/macroblockdecodermdec/)
reports that hardware uses the scale table's upper 13 bits, but still labels the intermediate rounding as
approximate and precision as unknown. The project's
pinned [ares decoder](https://github.com/ares-emulator/ares/blob/9408cb43d4948fc3ea6e152a307a34348df3fe04/ares/ps1/mdec/decoder.cpp)
uses the same two-pass `(sum + 0x8000) >> 16` transform as the current implementation. Together the public capture,
the pinned baseline and the documentation do not identify an exact arithmetic change that fixes the 8-bit mismatch.
This comparison remains exploratory and was not added as a maintained gate.

Independent reviews confirmed that both pinned mono guests use the same 128-byte `heart.mdec` blob and the same
quantization and scale tables [SCM.12]. The current dequantization and two-pass transform match the permitted ares
implementation [SCM.10]; applying the documented upper-13-bit scale representation with the approximate hardware
rounding did not remove this vector's mismatch. The available captures corroborate output bytes but do not expose
intermediate values or a precise hardware rounding rule. No production arithmetic change is supported.

The public [MDEC Hand-Made Streams capture](https://psx.wiki/tests/mdec2/) publishes 63 signed 8-bit, q_scale-8,
single-AC cases with per-block CRCs and selected output words; the [raw 05MAC record](https://psx.wiki/tests/raw/20260928T0000Z-5501a-card-h/mdec2/05MAC.txt)
was fetched with `curl -fsSL` (exit 0; 4,974 bytes). The first temporary driver incorrectly filled each quant-table
byte with `0x02`, giving 0/63 CRCs; that result is superseded. The record's parenthetical says the quant value is
200, so a second production-library probe used byte value 200 and generated 27/63 matching CRCs and 146/189
matching published words 0, 1 and 15. Other byte fills at or above `0x20` also gave 27/63 because the dequantized
coefficient clips before IDCT, so this does not independently establish the raw table bytes. The page does not
include the test executable or exact input payload; the reconstruction remains exploratory. At q_scale 8, k=1,
word 0 is `23647F7F` in the capture and `23657F7F` in the generated output; words 1 and 15 match. At q_scale 0,
k=1, the three published words each differ from the generated value by one sample. The DC-only control reproduces
the public `19191919` sample. This narrows the lead to small output differences on these reconstructed streams,
but does not identify an exact hardware rule or justify an arithmetic change. No production arithmetic or expected
data was changed.

The subsequent public-source search found no second full-output oracle for this mismatch. The psx.wiki [hardware
checks](https://psx.wiki/tests/hardware-checks/) include an additional 8-bit 06FMD case with 48 output words,
CRC `A9DEB035`, and first/last words `8F9EA9AF` / `118AFF72`, plus a 32-word case with CRC `F60B4B13` and
first/last words `FFFB7B20` / `118AFF72`; the exact payload and intervening bytes are absent. The pinned
[ps1-tests repository](https://github.com/JaCzekanski/ps1-tests/tree/f727802fead11f1daa7549285548392ef87749cb)
has a sanitized single-block `mdec/8bit` input but no matching captured output log. These sources add aggregate
controls, not a replacement bit-exact vector.

A bounded alternative-transform probe used the BSD-3 MAME MDEC source at
[commit afd3f3d](https://github.com/mamedev/mame/blob/afd3f3da405d535b244d175b2eef395c27635a2b/src/devices/cpu/psx/mdec.cpp).
MAME combines both cosine factors before summing and shifts once, unlike the current two-pass rounded IDCT. The
scratch driver passed a baseline consistency check: its direct production IDCT matched the existing parser probe's
CRC and selected-word summaries on all 63 inputs. Against the capture, the current transform matched 27/63 CRCs
and 146/189 published words; the MAME candidate matched only 3/63 CRCs and 65/189 words. For k=1 it matched word
0 but missed words 1 and 15 by one byte; the current transform matches those latter words. Reject the MAME
candidate; it is not a hardware result and was not applied. The corrected comparison exited 0:

```sh
cc -std=c11 -Iinclude -Isrc -Ibuild/mdec-reference/generated build/mdec-reference/probe-current-idct.c build/mdec-reference/libschwaystation.a -o build/mdec-reference/probe-current-idct && build/mdec-reference/probe-current-idct > build/mdec-reference/current-idct.out && cc -std=c11 -Iinclude -Isrc -Ibuild/mdec-reference/generated build/mdec-reference/probe-mame-idct.c build/mdec-reference/libschwaystation.a -o build/mdec-reference/probe-mame-idct && build/mdec-reference/probe-mame-idct > build/mdec-reference/mame-idct.out && python3 build/mdec-reference/compare-idct-probes.py
```

## Cheap preflight sequence

Before the focused status run, a cheap preflight counted the pinned trace's 256 input writes and 512 output reads,
then compared their requested delays with the fixed run limit. The mismatch was established before another guest
run: 7.68M requested delay cycles cannot fit in 4.9152M. The count is recorded as one-off evidence, not a new gate;
the CTest itself already fails quickly. A separate selector preflight caught that the Plan 23 policy tests belong
to the hygiene preset; the plan command was corrected and the two selectors passed there. Diff and sorted-inventory
checks also passed. For the CD-ROM correction, CTest inventory preflights confirmed 19 CI and three nightly
selectors before the targeted build/test commands; `git diff --check` passed. Broader CI and host checks stay
deferred until the pinned 8-bit selector passes.

## Hosted and hardware checks

Hosted CI, the complete nightly MDEC/CD-ROM matrix, fuzz campaigns and the background RetroArch disc comparison were
not run. The focused nightly CD-ROM timing/Getloc/control selectors passed locally; the full nightly lane remains
deferred because the required 8-bit MDEC selector is already red locally. No hosted run URL or owner-hardware
capture is claimed. A lightweight local libFuzzer/ASan/UBSan probe using installed Homebrew LLVM 21 did not return
within 30 seconds and was interrupted, so the five new campaigns were not launched locally. The Xcode clang
fallback also lacked its libFuzzer runtime. Public captures independently confirm the pinned expected output, but
the lack of intermediate coefficients prevents a source-backed correction. No owner capture is available or
planned.

## Evidence correction and next action (2026-11-17)

The former 296/300 frame and 162/240 movie counts were decoder output progress, incorrectly described as source
coverage. Plan 06-42's bounded source scanner established complete pinned spans: frame 56,960 bytes/1,800 blocks/
300 macroblocks and movie first frame 6,784 bytes/1,440 blocks/240 macroblocks. Structural source coverage does
not establish successful output or fidelity.

After their in-process source guards, the four pinned frame/movie guests consumed their complete input but failed
within 0.7 seconds total. The two frame guests ended with empty MDEC output and no DMA1; the two movie guests
left DMA1 active with output empty. A temporary, reverted diagnostic allowed the frame's existing visible-pixel
comparison to continue and it failed at (2,0); the original assertions and full-VRAM bounds were restored. This
diagnostic did not weaken a committed gate. The pinned CPU reader waits once more after its final requested word,
and the adapted public SCPH-5501 PIO capture shows a related final-strip stall, but the emulator output does not
match the existing image reference. The movie DMA1 stall is separate and has no matching capture. See the
[pinned caller](https://github.com/JaCzekanski/ps1-tests/blob/f727802fead11f1daa7549285548392ef87749cb/common/mdec.cpp)
and [public hardware record](https://psx.wiki/tests/mdec-status-decode-timing/).

The owner-approved limited v1.0 MDEC claim remains in force. Preserve complete-source checks, the 4-bit hardware
guest, deterministic 8-bit regression, direct model checks and owned `disc_boot` STR path. Replace the full
external frame/movie conformance selectors with gating source-coverage selectors; preserve the original pins and
the execution/output limitations as evidence. Do not change arithmetic, hardware expected bytes or the fixed
4,915,200-cycle cap. Plans 06-43 and 06-44 completed selector synchronization and local acceptance. Plan 06-45
owns exact-head hosted acceptance, ship and release checks; no owner capture or further approval is needed.
