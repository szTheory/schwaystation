# 04-40 OpenBIOS boot diagnosis

**Status:** Plan acceptance is paused pending a focused, source-owned fix. No shell frame is accepted or pinned.

## Evidence

- `gh run view 43083684528 --repo szTheory/schwaystation --json headSha,conclusion,workflowName,createdAt,url` and `gh run download 43083684528 --repo szTheory/schwaystation --name openbios-handoff --dir /private/tmp/schwaystation-openbios-43083684528` exited 0. This prior successful run is at `11a79aeebacc9734c2bc5a5295e1815cfeb15b38`, not the current head. The downloaded image is 524288 bytes and SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8`.
- `build/ci/schwaystation-run --bios /private/tmp/schwaystation-openbios-43083684528/openbios.bin --frames 60 --hash --hash-state --png 60:/private/tmp/schwaystation-openbios-60.png` exited 0. Its PNG SHA-256 was `4767ea2e534326eb1cb0c8b97500c9b5f8100afbfd4f5d6511a521ed96a3a623`; visual inspection showed a uniformly black 256x240 image.
- The same runner command at `--frames 600` with `--tty` and `--png 600:/private/tmp/schwaystation-openbios-600.png` exited 0 and emitted 706707 blank `tty ` lines. At `--frames 1000`, it exited 0, emitted 1180520 blank `tty ` lines, and reported the same image hash as frame 60: `fe0084ed1daff80953abb18143fc3f573020edc1b2bc56654a8c7a5b6ca77ac0`. This CLI count is not the core video frame: public video metadata remained `frame=0`, `enabled=0`.
- A temporary C diagnostic using the public API and `SCHWAY_RUN_TRACE` exited 0 when linked against `build/ci/libschwaystation.a`. The first-fault record was `UNMAPPED_WRITE` (`kind=2`, device EXP2) at tick `1917508`, PC `0xBFC000A8`, address `0xBF802041` (physical `0x1F802041`). The trace shows the executing instruction `0xA2022041` (`sb v0,0x2041(s0)`) with `v0=0x0F`. With stop-on-fault disabled, 1000 bounded `schway_run` calls completed with exit 0 but video stayed disabled, VRAM remained zero-filled, and the trace ended in zero/NOP instructions at `0x800004A8`–`0x800004D4`.
- No BIOS bytes, screenshot, or raw TTY/trace log were added to the repository. The artifact and diagnostic outputs remain under `/private/tmp`.

## Source cause

At the pinned MIT nugget source `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9`, `openbios/main/main.c` writes `POST = 0x0f` and then `POST = 0x0e` before `drawSplashScreen()` and `bootThunk()` (lines 90–108). `common/hardware/hwregs.h` defines `POST` as `HW_U8(0xbf802041)` (line 69). The runner trace matches that first startup write. In this repository, `src/bus.c:562–581` accepts only EXP2 debug ports `0x1F802080` and `0x1F802082`; the POST write is therefore faulted as an EXP2 gap before shell startup. The core later runs from zero-filled RAM rather than reaching the shell.

## Focused re-slice proposal

Add only an ignored byte-write compatibility case for the OpenBIOS POST status address `0x1F802041`. Preserve faults for reads, wider accesses, and every other EXP2 gap. Add regression assertions in `tests/bus/bus_tests.c`, register the focused bus test in `tests/bus/tests.cmake`, byte-sort `tests/bus/inventory.txt`, and explain the OpenBIOS POST compatibility behavior in `docs/hardware/bus.md` with the pinned upstream source as provenance. This is not a new hardware measurement and does not change API, state, or behavior revisions.

**Focused implementation forecast:** 8–12 minutes. Re-run the actual boot inspection before accepting a shell hash. Plan 04-40 still needs its tracer, same-run portable lane and refusal controls after the fix.

## Source-owned plan handoff

The planner rechecked the local bus dispatch and the two pinned upstream locations. Width is expressed in bytes, physical addresses are masked before EXP2 decode, and the current gap rejects POST before existing debug handling. These source-only commands exited 0:

- `gh api 'repos/pcsx-redux/nugget/contents/common/hardware/hwregs.h?ref=9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9' --jq .content | base64 -d | rg -n 'POST|HW_U8'`
- `gh api 'repos/pcsx-redux/nugget/contents/openbios/main/main.c?ref=9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9' --jq .content | base64 -d | sed -n '86,111p'`

Plan 04-50 owns only the ignored POST byte write, normal EXP2 charge and source/docs/regression controls. Its implementation/test forecast is 8–12 minutes plus normal check/metadata overhead. Plan 04-40 waits for it, then resumes actual shell observation and diagnoses further concrete failures if present. No shell hash is accepted here; wider/read/neighbor gaps and existing debug behavior remain preserved. No firmware bytes or raw logs are added.
