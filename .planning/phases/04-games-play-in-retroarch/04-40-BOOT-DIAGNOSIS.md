# 04-40 OpenBIOS boot diagnosis

**Status:** The POST and ATTR readback fixes are complete in plans 04-50 and 04-51. The checked BIOS now passes the ATTR poll but waits forever for DMA4 request block 1; its frame remains black. Plan 04-40 is paused pending focused DMA4 request/continuation work. No shell frame is accepted or pinned.

## Evidence

- `gh run view 43083684528 --repo szTheory/schwaystation --json headSha,conclusion,workflowName,createdAt,url` and `gh run download 43083684528 --repo szTheory/schwaystation --name openbios-handoff --dir /private/tmp/schwaystation-openbios-43083684528` exited 0. This prior successful run is at `11a79aeebacc9734c2bc5a5295e1815cfeb15b38`, not the current head. The downloaded image is 524288 bytes and SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8`.
- `build/ci/schwaystation-run --bios /private/tmp/schwaystation-openbios-43083684528/openbios.bin --frames 60 --hash --hash-state --png 60:/private/tmp/schwaystation-openbios-60.png` exited 0. Its PNG SHA-256 was `4767ea2e534326eb1cb0c8b97500c9b5f8100afbfd4f5d6511a521ed96a3a623`; visual inspection showed a uniformly black 256x240 image.
- The same runner command at `--frames 600` with `--tty` and `--png 600:/private/tmp/schwaystation-openbios-600.png` exited 0 and emitted 706707 blank `tty ` lines. At `--frames 1000`, it exited 0, emitted 1180520 blank `tty ` lines, and reported the same image hash as frame 60: `fe0084ed1daff80953abb18143fc3f573020edc1b2bc56654a8c7a5b6ca77ac0`. This CLI count is not the core video frame: public video metadata remained `frame=0`, `enabled=0`.
- A temporary C diagnostic using the public API and `SCHWAY_RUN_TRACE` exited 0 when linked against `build/ci/libschwaystation.a`. The first-fault record was `UNMAPPED_WRITE` (`kind=2`, device EXP2) at tick `1917508`, PC `0xBFC000A8`, address `0xBF802041` (physical `0x1F802041`). The trace shows the executing instruction `0xA2022041` (`sb v0,0x2041(s0)`) with `v0=0x0F`. With stop-on-fault disabled, 1000 bounded `schway_run` calls completed with exit 0 but video stayed disabled, VRAM remained zero-filled, and the trace ended in zero/NOP instructions at `0x800004A8`–`0x800004D4`.
- No BIOS bytes, screenshot, or raw TTY/trace log were added to the repository. The artifact and diagnostic outputs remain under `/private/tmp`.

### Boot after the POST fix

- `cmake --preset ci && cmake --build --preset ci --target schwaystation-run` exited 0 after plan 04-50. Running `build/ci/schwaystation-run --bios /private/tmp/schwaystation-openbios-43083684528/openbios.bin --frames 60 --tty --hash --hash-state --png 60:/private/tmp/schwaystation-openbios-postfixed-60.png` exited 0. The runner reported BIOS SHA-256 `92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8`, frame 60 hash `793f6a3459808ad68494aac85caf3d71afc6b8da7aeea21f612f3ec9140fc324`, and state hash `fcacfa83d34d63fccc088daa4c6968d98cdeb8e102c1d5a39325b0f931d052d`. The 640x239 PNG SHA-256 was `d54805b62ede706d1ef9b44538d714c4192c9b41ccd910b6550e24d77d768532`; independent visual inspection still showed a uniformly black frame. TTY output contained no blank-line flood. These values are diagnostics only; no shell hash is accepted.
- A temporary public-API trace harness was compiled with `cc -Iinclude -Ibuild/ci/generated /private/tmp/schway_openbios_first_spu_read.c build/ci/libschwaystation.a -o /private/tmp/schway_openbios_first_spu_read` and run against the checked BIOS. The compile and run exited 0. Each `schway_run` used `SCHWAY_RUN_TRACE` with a 10,000-cycle budget. At the first SPU control poll, the trace showed frame 41: cycle 23,388,289 `rd pc=0x800332a4 addr=0x1F801DAA value=0`; cycle 23,388,323 `wr pc=0x800332b4 addr=0x1F801DAA value=0x20`; cycle 23,388,348 `rd pc=0x800332c4 addr=0x1F801DAA value=0`. The following instruction at `0x800332cc` is opcode `0x30C60030` (`andi a2,a2,0x30`), followed by a branch back to the load. The loop therefore keeps observing transfer-mode bits `0x00` after writing mode `0x20`.
- After plan 04-51, `cmake --preset ci && cmake --build --preset ci --target schwaystation-run` exited 0. Running `build/ci/schwaystation-run --bios /private/tmp/schwaystation-openbios-43083684528/openbios.bin --frames 60 --tty --hash --hash-state --png 60:/private/tmp/schwaystation-openbios-attr-fixed-60.png` exited 0. It reported frame hash `793f6a3459808ad68494aac85caf3d71afc6b8da7aeea21f612f3ec9140fc324` and state hash `aadb0698b0cc136fde3352fbc353c16252c65fd8d3c419725f18c53e1396bfd8`. The 640x239 PNG SHA-256 was `d54805b62ede706d1ef9b44538d714c4192c9b41ccd910b6550e24d77d768532`; independent inspection showed it remained uniformly black, and TTY output contained no lines.
- `cc -Iinclude -Ibuild/ci/generated -Isrc /private/tmp/schway_openbios_diag.c build/ci/libschwaystation.a -o /private/tmp/schway_openbios_diag && /private/tmp/schway_openbios_diag /private/tmp/schwaystation-openbios-43083684528/openbios.bin` exited 0 after 1,000 traced `schway_run` calls. It reported `fault_present=0`, video frame 998, `enabled=0`, and zero nonzero RGB555 pixels. The trace repeatedly loaded `0x1F8010C8` at PC `0x80033308` with value `0x01000201`.
- The same diagnostic's per-instance DMA state was `DPCR=0x076F4321`, `CHCR[4]=0x01000201`, `BCR[4]=0x02F50010`, `MADR[4]=0x00037B3C`, `phase[4]=0`, `remain[4]=0`, `dreq=0`, `words[4]=0`, `cur=7`, and `busy_until=stall_until=0`; `spu_attr=0x8020`. DPCR has channel 4 enabled. BCR requests 757 blocks of 16 words, but no block has started. The channel remains marked active in CHCR while the engine correctly waits for DREQ4.

## Source cause

At the pinned MIT nugget source `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9`, `openbios/main/main.c` writes `POST = 0x0f` and then `POST = 0x0e` before `drawSplashScreen()` and `bootThunk()` (lines 90–108). `common/hardware/hwregs.h` defines `POST` as `HW_U8(0xbf802041)` (line 69). The original runner trace matches that first startup write. Before plan 04-50, `src/bus.c` accepted only EXP2 debug ports `0x1F802080` and `0x1F802082`; the POST write was faulted as an EXP2 gap before shell startup.

## Completed POST compatibility slice

Plan 04-50 added only the ignored byte-write compatibility case for the OpenBIOS POST status address `0x1F802041`. It preserved faults for reads, wider accesses, and every other EXP2 gap, with bus regression coverage and pinned source documentation. This was not a new hardware measurement and did not change API, state, or behavior revisions.

The POST case is complete. Plan 04-40 still needs its tracer, same-run portable lane, and refusal controls after the shell is independently inspected.

## Source-owned plan handoff

The planner rechecked the local bus dispatch and the two pinned upstream locations. Width is expressed in bytes, physical addresses are masked before EXP2 decode, and the current gap rejects POST before existing debug handling. These source-only commands exited 0:

- `gh api 'repos/pcsx-redux/nugget/contents/common/hardware/hwregs.h?ref=9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9' --jq .content | base64 -d | rg -n 'POST|HW_U8'`
- `gh api 'repos/pcsx-redux/nugget/contents/openbios/main/main.c?ref=9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9' --jq .content | base64 -d | sed -n '86,111p'`

Plan 04-50 owns only the ignored POST byte write, normal EXP2 charge, and source/docs/regression controls. No shell hash was accepted there; wider/read/neighbor gaps and existing debug behavior remain preserved. No firmware bytes or raw logs are added.

## Resolved boot blocker: SPU ATTR readback

At the pinned nugget commit `9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9`, `common/hardware/spu.h` maps `SPU_CTRL` to `HW_U16(0x1f801daa)` and `SPU_STATUS` to `HW_U16(0x1f801dae)`. `modplayer/modplayer.c` is the first caller reached by the trace: `SPUInit` writes control values 0 and `0x8000`; `SPUUnMute` writes `0xc000`; `SPUUploadInstruments` sets the transfer address, writes DMA-write mode `0x20` through `SPU_CTRL`, and then loops on `(SPU_CTRL & 0x0030) != 0x0020` (pinned lines 117–138, 157–169). The trace’s `lhu`, `andi` mask `0x30`, and branch match this caller. Plan 04-51 now provides the exact ATTR halfword readback. That fix passes this poll but does not provide DMA requests, status mirroring, or SPU transfer behavior.

The project’s pinned PSX-SPX source `6d7d1bc106a7`, section 1.5, and the [SPU register reference](https://psx-spx.consoledev.net/soundprocessingunitspu/) identify `0x1F801DAA` as ATTR, a read/write control register whose bits 5–4 select transfer mode. They identify `0x1F801DAE` as read-only STATX/SPUSTAT; mode bits 5–0 there reflect ATTR with a delay. Thus the observed loop reads ATTR, not SPUSTAT and not CD-ROM. The C-shell explicitly writes ATTR before this poll, so no hardware power-on ATTR value is needed for this caller. No SPU timing, audio output, or power-on-value claim follows from this trace.

## Completed ATTR readback slice

Plan 04-51 added an instance-owned halfword latch for exact ATTR accesses at physical `0x1F801DAA`, retaining unbuilt behavior for other SPU registers including STATX at `0x1F801DAE` and preserving revision 4. It added bus and state round-trip controls. The deterministic zero reset is an emulator model only, not a hardware power-on claim. It did not implement DMA requests, SPU RAM, timing, or audio.

## Next boot blocker: DMA4 request line

After the ATTR poll, the pinned `modplayer/modplayer.c` upload path sets `BCR[4]` to block size `0x10` words and the remaining block count, then writes `CHCR[4]=0x01000201` (lines 151–166). Its final loop waits for CHCR bit 24 to clear. The current trace matches that wait at `PC 0x80033308`, repeatedly reading `0x1F8010C8`. The state shows the channel enabled, active, and configured for 757 request-mode blocks of 16 words, while DREQ4 is low and the existing sink has received zero words.

The project's DMA documentation states that sync mode 1 transfers exactly one BCR block per request-line raise; the engine consumes that request when the block begins and leaves the CPU running while it waits for another (`docs/hardware/dma.md`, “Sync mode 0, sync mode 1 and the request line”). The pinned PSX-SPX SPU reference says STATX bit 8 is the DMA-write request, and its SPU RAM DMA-write sequence starts DMA4 with block size `10h` and control `01000201h`. The core has no SPU module to raise DREQ4; the generic channel-4 sink only consumes words after a block starts. This explains the wait without attributing a defect to the already-tested generic DMA block engine.

## Focused DMA4 re-slice proposal

Plan 04-52 should add the bounded ATTR-driven DMA4 request/continuation contract needed for this checked firmware upload, feeding the existing deterministic channel-4 sink and preserving the existing DMA cost semantics. Add instance/state and regression controls for request assertion, per-block continuation, completion, reset, and unaffected channels/modes; document the modeled FIFO/request behavior and its limits. This is not full SPU implementation: do not add audio, sample RAM, voices, or hardware-timing claims. The exact request-latency interpretation remains a modeled compatibility policy and must be labelled accordingly.

**Focused implementation forecast:** 15–25 minutes. This source-owned request/continuation work is separate from the remaining shell and portable-lane acceptance; plan 04-40 remains unaccepted and resumes after plan 04-52 for independent shell inspection and same-run portable evidence.
