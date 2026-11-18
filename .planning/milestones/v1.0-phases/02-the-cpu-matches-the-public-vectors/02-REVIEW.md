---
phase: 02-the-cpu-matches-the-public-vectors
reviewed: 2026-11-09T00:00:00Z
depth: standard
files_reviewed: 120
files_reviewed_list:
  - .gitattributes
  - .github/workflows/ci.yml
  - .github/workflows/nightly.yml
  - .gitignore
  - CMakeLists.txt
  - CMakePresets.json
  - PROVENANCE.md
  - README.md
  - THIRD-PARTY-NOTICES.md
  - cmake/Armips.cmake
  - cmake/SchwaystationTests.cmake
  - docs/embedding.md
  - docs/hardware/bus.md
  - docs/hardware/cop0.md
  - docs/hardware/cpu.md
  - docs/maintaining.md
  - docs/runner.md
  - docs/testing.md
  - include/schwaystation.h
  - pins.txt
  - runner/CMakeLists.txt
  - runner/exit_status.h
  - runner/mailbox.c
  - runner/mailbox.h
  - runner/main.c
  - runner/md5.c
  - runner/md5.h
  - runner/probe.c
  - runner/probe.h
  - runner/trace.c
  - runner/trace.h
  - src/bus.c
  - src/bus.h
  - src/cop0.c
  - src/cpu.c
  - src/cpu.h
  - src/cpu_exec.inc
  - src/exe.c
  - src/frame.c
  - src/instance.c
  - src/internal.h
  - src/irq.c
  - src/state.c
  - src/state.h
  - src/stub_kernel.c
  - tests/abi/inventory.txt
  - tests/abi/tests.cmake
  - tests/api/v1.c
  - tests/api/v2.c
  - tests/bus/bus_tests.c
  - tests/bus/cache_tests.c
  - tests/bus/inventory.txt
  - tests/bus/tests.cmake
  - tests/cmake/api_names.cmake
  - tests/cmake/budgets.cmake
  - tests/cmake/exit_table.cmake
  - tests/cmake/hashes.cmake
  - tests/core/bus_unbuilt.c
  - tests/core/inventory.txt
  - tests/core/test_core.c
  - tests/core/test_run.c
  - tests/core/test_state.c
  - tests/core/test_trace.c
  - tests/core/tests.cmake
  - tests/cpu/cop0_tests.c
  - tests/cpu/cop2_prephase.c
  - tests/cpu/delay_tests.c
  - tests/cpu/exception_tests.c
  - tests/cpu/inventory.txt
  - tests/cpu/tests.cmake
  - tests/cpu/timing_tests.c
  - tests/diag/allow.txt
  - tests/diag/expected.csv
  - tests/embed/CMakeLists.txt
  - tests/exe/exe_tests.c
  - tests/exe/inventory.txt
  - tests/exe/tests.cmake
  - tests/fixtures/boot_tests.c
  - tests/fixtures/inventory.txt
  - tests/fixtures/manifest.txt
  - tests/fixtures/src/exc_default.asm
  - tests/fixtures/src/exc_matrix.asm
  - tests/fixtures/src/exit_port.asm
  - tests/fixtures/src/fault.asm
  - tests/fixtures/src/hang.asm
  - tests/fixtures/src/header.inc
  - tests/fixtures/src/hello.asm
  - tests/fixtures/src/mailbox.inc
  - tests/fixtures/src/mailbox_fail.asm
  - tests/fixtures/src/stub/stub.asm
  - tests/fixtures/src/unbuilt.asm
  - tests/fixtures/tests.cmake
  - tests/fuzz/exe.c
  - tests/fuzz/inventory.txt
  - tests/fuzz/replay_main.h
  - tests/fuzz/seeds.c
  - tests/fuzz/seeds.h
  - tests/fuzz/tests.cmake
  - tests/header/header.c
  - tests/header/header.cpp
  - tests/hygiene/tests.cmake
  - tests/install/tests.cmake
  - tests/inventory/asan.txt
  - tests/inventory/ci.txt
  - tests/inventory/nightly.txt
  - tests/inventory/wasm32.txt
  - tests/nightly/inventory.txt
  - tests/nightly/tests.cmake
  - tests/policy/inventory.txt
  - tests/policy/tests.cmake
  - tests/runner/inventory.txt
  - tests/runner/test_trace.c
  - tests/runner/tests.cmake
  - tests/tests.cmake
  - tests/vectors/inventory.txt
  - tests/vectors/r3000/allow.txt
  - tests/vectors/r3000/sst-sample.bin
  - tests/vectors/tests.cmake
  - tests/vectors/vectors.c
  - tools/sst_slice.py
findings:
  critical: 0
  warning: 6
  info: 7
  total: 13
  necessity: 0
status: issues_found
---

# Phase 2: Code Review Report

**Reviewed:** 2026-11-09
**Depth:** standard
**Files Reviewed:** 120
**Status:** issues_found

## Summary

Production code was read in full: `src/cpu_exec.inc`, `cpu.c`, `cpu.h`, `cop0.c`, `bus.c`, `bus.h`, `exe.c`, `irq.c`, `frame.c`, `state.c`, `state.h`, `instance.c`, `internal.h`, `include/schwaystation.h` and the runner's `main.c`, `probe.c`, `mailbox.c`, `md5.c`, `trace.c`. The vector harness and the fuzz replay main were read in full. The remaining tests, cmake scripts, workflows, fixtures and docs were skimmed or spot-checked rather than read line by line, so findings there are limited to what was visible.

Overall the core is carefully built. I traced the multiply and divide classification, the LWL/LWR/SWL/SWR merges, the interrupt retirement stamp and its interaction with exception priority, the delay-slot EPC/TAR logic, the memory-control cost formula and its `uint8_t` range, the I/O slot decode, `ram_locate` bank arithmetic, the state walk's VALIDATE/APPLY symmetry, and the per-instruction cycle bound (`SCHWAY__INSN_CYCLES_MAX`; worst case I could build is about 400 cycles, so the 1,024 margin holds for any loaded memory-control value). I found no memory-safety defect and no unbounded loop in the core. There is no floating point, no wall clock and no file or environment read under `src/`.

The two named vector rules were checked for masking. Rule (a) ignores only the CE bits of CAUSE and still asserts vector, ExcCode, BadVaddr, bus activity and every other final field; it hides nothing the core could get wrong beyond CE, which the core correctly leaves 0. Rule (b) compares the covered byte set exactly but, by design, not the chunk decomposition (see IN-07). Neither rule masks a core defect I could find.

No finding reaches BLOCKER. The six warnings are one missing architectural check (user-mode instruction fetch), one hole in state-load validation, one icache fill defect, one heuristic that can misreport a progressing guest as a fault, one runner output-hardening gap and one stale doc sentence. I did not load the necessity reference and raised no necessity findings.

## Warnings

### WR-01: User-mode (SR.KUc = 1) instruction fetch from KSEG0, KSEG1 or the BIOS window is not an address error

**File:** `src/cpu_exec.inc:65-82` and `src/cpu_exec.inc:629-672`
**Issue:** The data path rejects user-mode access outside KUSEG in `cpu_data_check` (line 421), which `docs/hardware/cpu.md` documents and PSX-SYSTEM section 2 requires ("user mode (KUc = 1) may touch only KUSEG"; "AdEL/AdES ... on user-mode access outside KUSEG"). The fetch path has no equivalent. `cpu_step` checks only `(pc & 3u) != 0u` before calling `cpu_fetch`, and `cpu_fetch` decodes the segment for caching but never looks at `SR.KUc`. Scenario: a guest clears `SR.KUc`'s complement, i.e. enters user mode (RFE restoring KUp = 1, or an `MTC0` to SR with bit 1 set) and a branch or jump lands at `80010000h` or `BFC00180h`. The core executes it as if in kernel mode instead of raising AdEL with BadVaddr = pc. The vector harness cannot catch this because it forces SR to 0, and neither `cpu.md` nor `cop0.md` mention the fetch case, so the gap is silent. The asymmetry also means a program that runs user-mode code can read KSEG0 data only through an instruction fetch, which hardware forbids.
**Fix:**
```c
/* in cpu_step, replacing the misaligned-only test */
uint32_t user_fetch = (sr & SCHWAY__SR_KUC) != 0u && (pc & 0x80000000u) != 0u &&
                      c->log == NULL ? 1u : 0u;
uint32_t misaligned = (pc & 3u) != 0u ? 1u : 0u;
uint32_t op = (misaligned | user_fetch) != 0u ? 0u : cpu_fetch(inst, pc, &cycles, &fetch_err);
...
} else if (user_fetch != 0u) {
    /* A handler may take it, so no first fault, as for the data case. */
    trapped = cpu_exception(inst, pc, SCHWAY__EXC_ADEL, 0u, 1u, pc);
}
```
Note `sr` is declared after `op` today and must be read first. Add a `cpu.exception.*` test that enters user mode and fetches from KSEG0, and state the rule in `docs/hardware/cpu.md` under Instruction fetch.

### WR-02: State load accepts a non-zero `r0` and arbitrary values in COP0 bits the CPU never produces

**File:** `src/cpu.c:63` and `src/cpu.c:67`
**Issue:** `schway__state_u32_array(io, name_r, c->r, 32u, 0u, UINT32_MAX)` gives all 32 general registers the same full range, including `r[0]`. `cpu_step` reads `c->r[rs_n]` and `c->r[rt_n]` without special-casing register 0 (lines 642-643, 197, 545), and the only protection of `r[0]` is that writes are skipped. A state with a valid CRC and `r[0] != 0` therefore makes every `move`, `lui`-free `addu rd, $zero, rt`, `beq $zero, ...` and address computation off `$zero` use the wrong value for the rest of the run. The project's rule for a hostile state is a bound on every walked field, and the vector harness already treats a non-zero `r0` as malformed (`vectors.c:165`); the loader does not. The same array treatment applies to `cop0.r` (line 67): SR bits 6-7, 20-21 and 23-27 (which `SR_WRITABLE` says read 0) and the read-only EPC, TAR, BadVaddr and PRID are all accepted, so a loaded SR can hold bits no `MTC0` could set, and CAUSE bit 10 need not match `irq.stat & irq.mask`. None of this is memory-unsafe, but it lets a state reach CPU configurations the model otherwise rules out, and a later `MFC0` of SR exposes them to the guest.
**Fix:** Validate the invariants in the walk. For `r0`, walk it as its own field with `min = max = 0` and the remaining 31 as the array, or add a VALIDATE-only check on the decoded bytes:
```c
(void)schway__state_u32(io, name_r0, &c->r[0], 0u, 0u);
schway__state_u32_array(io, name_r, &c->r[1], 31u, 0u, UINT32_MAX);
```
For SR, bound the field to the writable mask (it is a range check today, so add a `schway__state_check` on `(sr & ~SR_WRITABLE) == 0` using the decoded value) and, for CAUSE, check bit 10 against the line after the interrupt registers are walked. Bump `SCHWAY__STATE_FORMAT` only if the field layout changes.

### WR-03: A bus error while refilling later words of an I-cache line overwrites a still-valid cached word with 0

**File:** `src/cpu.c:124-132` (with `src/bus.c:539-541`)
**Issue:** The fill loop passes `&c->icache_word[line][k]` straight to `schway__bus_fetch`. `bus_access` zeroes `*value` for every non-write access before it decodes (bus.c:539-541), including on the error return. The cache keeps "the words already valid" on a same-tag miss (bus.md, "A miss"), so word `k` can be valid from an earlier fill. If its address now bus-errors (for example the guest rewrote `RAM_SIZE` so the address falls past the banks, or the line lies in a window that has since become a gap), the loop writes 0 into a word whose valid bit is still set and then breaks. The next fetch of that word returns 0, which executes as `SLL r0, r0, 0`, a silent NOP, instead of the instruction the cache held. `bus.md` says a bus error "leaves those words invalid", which is not what happens for a previously valid word.
**Fix:** Fetch into a local and assign only on success, clearing the valid bit on failure:
```c
uint32_t more = 0u;
if (schway__bus_fetch(inst, base + 4u * k, &more, &sink) != 0) {
    inst->fault = held;
    c->icache_valid[line] = (uint8_t)(c->icache_valid[line] & ~(1u << k));
    break;
}
c->icache_word[line][k] = more;
```
Add a `bus.icache_*` case that fills a line, makes a later word unmapped, refills from word 0 and checks the later word's valid bit and content.

### WR-04: The exception-loop heuristic reports a guest that is making progress as a first fault

**File:** `src/cop0.c:60-75` (limit in `src/cpu.h:59-61`)
**Issue:** The "loop" test counts consecutive exceptions with one EPC and one ExcCode and latches `exception_loop` at 1,000. It cannot tell a handler that never advances from a program that legitimately raises the same exception at the same address 1,000 times in a row. Concrete case: a guest polling loop that executes one `SYSCALL` (or `BREAK`, or a trapping `ADD`) at a fixed address, whose handler returns to EPC + 4 and which then branches back to that address; with no other exception or EPC in between, the 1,001st iteration latches the fault. The runner's default is stop-on-fault, so such a program ends with status 7 mid-run and the exit code says the guest hung. Interrupts are excluded by design, but nothing else breaks the run. It is a first fault only, so the cost is a wrong report and an early stop, not a wrong emulation result, but the doc comment says "only a loop is a fault".
**Fix:** Either count only when progress is also absent, for example require that the saved EPC register value is unchanged and that no other bus-visible store or register state changed (cheap proxy: the instruction retirement count since the previous entry is small, say at most a handler's length), or make the count reset when the exception is a SYSCALL/BREAK whose handler returned past EPC (EPC + 4 executed) before the next entry. At minimum document the false-positive shape in `docs/hardware/cop0.md` next to the loop-counter paragraph so a user reading status 7 knows a tight syscall loop can trip it.

### WR-05: The runner prints guest TTY bytes unfiltered, so a hostile executable can drive the user's terminal

**File:** `runner/main.c:446-478`
**Issue:** `tty_emit` writes the guest's bytes with `fwrite` exactly as received ("no character-set conversion"). A PS-EXE is untrusted input (CLAUDE.md rule 4 names hostile executables), and the same file sanitises hostile text elsewhere: `probe.c:19-25` replaces every byte outside printable ASCII so "a hostile name cannot add a line or a control sequence". With `--tty`, an executable can write ESC sequences (cursor movement, title set, some terminals' clipboard or answerback queries) or a carriage return that overwrites the `tty ` prefix on screen, and the runner relays them. Line structure is safe (the prefix is re-added per line), so machine parsers are unaffected; the exposure is to a person reading the run in a terminal.
**Fix:** Pass bytes through the same filter as `put_text` but keep tab, for example:
```c
for (uint32_t i = 0; i < t->len; i++) {
    uint8_t c = t->line[i];
    fputc((c >= 0x20u && c < 0x7Fu) || c == 0x09u ? (int)c : '?', stdout);
}
```
Update `docs/runner.md` and the tty test expectations (they presumably use ASCII) to state that non-printable bytes print as `?`.

### WR-06: `docs/hardware/cpu.md` says the CPU does not take interrupts

**File:** `docs/hardware/cpu.md:320-321`
**Issue:** "The CPU does not take interrupts yet; the vertical sync only sets the bit, and CAUSE bit 10 follows `I_STAT` and `I_MASK`" contradicts `cop0.md` (Interrupts section), the retirement stamp in `cpu_step`, and the `cpu.exception.*` tests, all of which take the vertical-blank interrupt. CLAUDE.md rule 5 requires behaviour and docs to land together; a reader of cpu.md's run-loop section gets the wrong model of frame end.
**Fix:** Replace the sentence with: "The vertical sync raises `I_STAT` bit 0; when `I_MASK` and SR allow it, the CPU takes the interrupt two retirements later (see [cop0.md](cop0.md), Interrupts)."

## Info

### IN-01: The interrupt entry re-fetches the word at EPC from the bus, with side effects, and ignores the I-cache

**File:** `src/cpu_exec.inc:343-348`
**Issue:** The comment says the word is read "without charge and without a first fault", but `schway__bus_fetch` also runs `schway__bus_unbuilt` for any unbuilt device page, which bumps `unbuilt_n` and overwrites `last_unbuilt` (state fields the runner prints), and it reads memory, not the I-cache line the CPU actually executed. If code at EPC was cached and the RAM behind it changed, the hook sees a different word than the one that ran. Today the hook only classifies a COP2 command, so the effect is cosmetic, but the GTE plan will execute that word. Capture the executed word in `cpu_step` (it is already in `op` for the instruction that was interrupted, when the entry follows that instruction's own fetch) or read through the same cache path.
**Fix:** Pass the fetched word into `cpu_irq_enter` rather than re-reading, and drop the bus call.

### IN-02: `print_hash` does not bound the video height, unlike `write_png`

**File:** `runner/main.c:352-377`
**Issue:** `write_png` rejects `v.height > 512` (line 399); `print_hash` checks `bpp` and `width` only and then walks `v.height` rows of `pitch_bytes` from `v.pixels`. `schway_get_video` computes `height = y2 - y1` as an unsigned difference (`frame.c:195`), which wraps if the display range is ever reprogrammed with y2 < y1. Unreachable now (the display is a constant), but the GPU phase makes it guest-controlled, and the hash path would then read past VRAM.
**Fix:** Add `|| v.height > 512u` to the check in `print_hash`, or share one validator with `write_png`.

### IN-03: Vector harness reads an uninitialised cycle record in two malformed-file paths and ORs overlapping store chunks

**File:** `tests/vectors/vectors.c:683-698` and `tests/vectors/vectors.c:583-611`
**Issue:** `struct vcase c` is uninitialised on each iteration. In the rule (b) block, when a case for LWL/LWR/SWL/SWR has `ncycles < 2`, `coalesce` returns 0 and the error report reads `c.cycle[1].size` which `parse_case` never wrote; the `!store` branch also writes `cycle[1]` past a `ncycles` of 1 (harmless to the replay, still a wild read for MSan). Separately `coalesce` ORs each chunk's bytes into `data`, so two chunks that wrote the same byte with different values, or the same byte twice, compare equal to a single correct write. The harness is meant to catch core decomposition errors, and this makes a double write invisible. Low severity: tooling, and the vectors themselves have exactly the right chunks.
**Fix:** `memset(&c, 0, sizeof c)` before `parse_case`; in `coalesce` set a `seen` mask and return 0 if a byte is covered twice.

### IN-04: The `nofp` job's 1-minute timeout leaves no margin for runner start-up

**File:** `.github/workflows/ci.yml:207`
**Issue:** `timeout-minutes: 1` against a measured 28 s. Every other job carries a 5-minute floor with a stated reason ("apt", "API", "roll-up"); this one is the single required job that can fail from a slow checkout or image pull and block a merge with no code cause. The comment says twice the slowest of five runs, but five samples of a hosted runner do not bound its tail.
**Fix:** Use the same 5-minute floor, or at least 3.

### IN-05: `tools/sst_slice.py` writes a container whose name field is not 16 bytes if a file stem is longer

**File:** `tools/sst_slice.py:37`
**Issue:** `name.encode('ascii').ljust(16, b'\0')` pads but never truncates or rejects, so a stem of 17 or more characters shifts every following field and the harness then fails with "short file" far from the cause; `assert KEEP <= n` runs only after the loop that would already have raised `struct.error` on a short file. Maintainer tool, never run by CI.
**Fix:** `assert len(name) < 16, name` before packing, and move the `n >= KEEP` check ahead of the loop.

### IN-06: `schway_load_exe` refuses only `frame != 0`, so it can replace RAM under a CPU that has already run

**File:** `src/exe.c:139`
**Issue:** The header comment says "before the first frame". An instance that ran less than a frame (`schway_run` with a small budget, so `frame == 0` and `ticks > 0`, with an earlier exe loaded) passes the guard; a second load overwrites text and the launch block while the CPU is mid-run and leaves the first image's other bytes in RAM. Deterministic, but the result is neither the new image's boot nor the old one's run. It is also unspecified in the public header.
**Fix:** Guard on `inst->ticks != 0u` (or `inst->exe_loaded != 0u`) as well, returning `SCHWAY_STATUS_ERR_ARGUMENT`, and say so in `docs/embedding.md`.

### IN-07: SWL and SWR reach the bus as separate 4/2/1-byte writes, each queued and charged on its own, and the vectors do not check that

**File:** `src/cpu_exec.inc:564-572`
**Issue:** Rule (b) deliberately makes chunking uncompared, so the only evidence for the decomposition is the upstream recording. The core issues each chunk through `cpu_bus_write`, so one SWL/SWR gets up to two write-queue entries and, to a KSEG1 target, a drain between chunks (the second write sees the first still queued). A device that cares about access width (the I/O page's unbuilt registers already log width) sees two narrow writes rather than one. If real hardware issues a single word write with byte lanes, timing and device effects will differ once devices exist. I could not establish which is right from the repository, so this is a note for the phase that builds the first width-sensitive device, not a defect today.
**Fix:** Before DMA or GPU registers land, decide the bus contract for partial stores, and add a test that pins it (one write with a lane mask, or the chunk list) instead of relying on the harness's coalescing.

---

_Reviewed: 2026-11-09_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: standard_
