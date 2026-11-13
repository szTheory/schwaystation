# Testing

Every check CI runs, a contributor can run with one command. This page covers that command, the lanes, the test
inventories, how to add a test, the negative controls, the budgets and the performance gate.

## The entry point

```sh
cmake --workflow --preset ci
```

It needs CMake 4.3 or later (the presets' minimum; the library alone configures with 3.25), Ninja and a C and
C++ compiler. The workflow configures into `build/ci` (Release, warnings as errors, macOS deployment target 10.13),
builds, and runs every test except those labelled `release`, writing `build/ci/junit.xml`. The `release` tests
check the packaged archives and run after the package step, with `ctest --preset ci-release`.

`ctest --test-dir build/ci -R <regex> --output-on-failure` reruns part of the suite after a build;
`ctest --test-dir build/ci -L <label>` selects by label. `-L` takes a regular expression that matches anywhere in a
label, so `-L gte` would also select any longer label that contains it; write `-L '^gte$'`, `-L '^dma$'` and
`-L '^timers$'` to select exactly one.

**Reconfigure after each commit.** Some checks read git's list of tracked files, and the runner's revision is
embedded when CMake configures. A new file is seen only once git tracks it, and `build.revision_matches_head`
fails when the embedded revision is not `git rev-parse HEAD`. After committing, rerun the whole workflow preset,
which reconfigures, rather than `ctest` alone.

## Lanes

Each lane is one workflow preset, `cmake --workflow --preset <name>`, building into `build/<name>`.

| Preset | What it proves | Inventory | Tests run |
|---|---|---|---|
| `dev` | A Debug build passes, for day-to-day work. | `ci` | all but `release` |
| `ci` | The workflow above. Every Linux and macOS leg runs it. | `ci` | all but `release` |
| `ci-msvc` | The same with MSVC and the static C runtime. | `ci` | all but `release` |
| `hygiene` | The repository is clean: tree scan, BIOS and executable bytes, the integer-only text scan, titles, the publish script, the workflow's timeouts and action pins, and the gate-line and end-to-end budgets. | `hygiene` | `hygiene` |
| `asan` | No memory or undefined-behaviour fault, with clang-21's AddressSanitizer and UndefinedBehaviorSanitizer; any finding stops the test. | `ci` | all but `abi` and `release`, whose binary-shape checks the instrumentation changes |
| `fuzz` | The loader's libFuzzer target and its replay build with clang-21, AddressSanitizer and UndefinedBehaviorSanitizer. Configure and build only: the nightly job runs the binary. | `asan` | none |
| `nofp` | The core uses no floating point: GCC's `-mgeneral-regs-only` rejects any floating-point type in it. | `nofp` | `abi` |
| `be32` | The same hashes on a big-endian, 32-bit target with unsigned `char`: 32-bit PowerPC (`cmake/toolchains/ppc-linux-gnu.cmake`) under `qemu-ppc`. | `ci` | `portable` |
| `wasm32` | The same hashes in WebAssembly: clang-21 for `wasm32-wasi` (`cmake/toolchains/wasm32-wasi.cmake`) under the pinned wasmtime. WASI cannot load a libretro core, so this lane builds none. | `wasm32` | `portable` |
| `nightly` | All 55,000 public single-step vectors pass at their pin, and the pinned GPU clipping program matches its captured whole-VRAM reference. | `nightly` | `gpu.reference.clipping`, `vectors.r3000.full` |
| `perf` | A Release build for the instructions-per-frame job. Configure and build only. | `ci` | none |

The `gte`, `dma` and `timers` directories are in the `ci`, `asan` and `wasm32` inventories, and `be32` runs their
`portable` tests through `ci`. The `nofp`, `hygiene` and `nightly` inventories do not run them. Under `asan` the
register sweeps of `gte` and `dma` (`gte.regs.sweep`, `dma.regs.sweep`) are the bounds proof.

### OpenBIOS handoff

The required CI run builds the pinned OpenBIOS shell once, checks its size and SHA-256 before uploading it as the
`openbios-handoff` Actions artifact, then makes each applicable build lane download that artifact from the same
run. Each consumer verifies that handoff before configuring tests. `built.openbios.shell` boots those same bytes in
NTSC-U to frame 240 and checks the independently inspected shell-frame hash. `built.openbios.shell_controls` boots
the checked image for one frame under the NTSC-J and PAL selectors, then asks the existing comparison helper to
reject a missing lane, missing shell tuple and wrong native hash. The hash file adds that named profile/frame tuple
to the existing eight-lane comparison; CI requires the checked artifact, while a local
build-only run without it writes an explicit not-applicable row. `built.openbios.handoff_corruption` and
`built.openbios.handoff_missing` verify that changed and missing consumer copies are refused;
`built.openbios.digest_corrupt` also checks a same-size digest mismatch. Local or build-only configurations without
a handoff use the generated test fixture, while consumer jobs verify the downloaded file before configuring tests.
The artifact expires after one day and is never cached across runs. Packaging installs only the
library, runner and libretro components; release collection downloads only `archives-*`, so the firmware does not
enter a package or publication artifact. A transfer or digest failure is an infrastructure failure, not an
emulation result.

The shell test is a CTest fixture setup for `runner.hashes_file`. It clears the tuple before booting, and the hash
writer requires the tuple created by that current test run. Selecting only `runner.hashes_file` automatically runs
the shell setup first: `ctest --test-dir build/ci -R '^runner\.hashes_file$' --output-on-failure`.

`SCHWAYSTATION_TIMEOUT_SCALE` multiplies every test timeout: 3 for `asan`, 8 for `be32` and 5 for `wasm32`.

### Lanes that need tools only CI has

`dev`, `ci` and `hygiene` run on any Linux or macOS host with the tools above. The others need:

- `nofp`: GCC itself, since configure stops for any other compiler (on macOS, `gcc` is clang);
- `asan` and `fuzz`: `clang-21` with its sanitizer runtimes (and, for `fuzz`, its libFuzzer runtime);
- `be32`: a PowerPC cross compiler and `qemu-ppc`;
- `wasm32`: clang-21, lld-21, `wasi-libc`, the wasm32 compiler runtime, and wasmtime in `build/pins/wasmtime`;
- the `perf` job: Valgrind (Cachegrind);
- the RetroArch end-to-end job: a pinned RetroArch on a macOS runner.

On Ubuntu these come from the distribution's packages. Where a host lacks them, CI's run of that lane is the
evidence; the workflow is the same command.

### The full vectors

The `nightly` lane needs the 55 upstream vector files, which are never committed, cached or uploaded. `pins.txt`
names each by its git blob SHA-1 (`r3000-<NAME>` rows). Fetch a file with
`gh api repos/SingleStepTests/r3000/git/blobs/<sha> -H 'Accept: application/vnd.github.raw'` into
`build/sst/<NAME>.json.bin` and check it with `git hash-object`; configure fails naming any file
`SCHWAYSTATION_SST_DIR` lacks, so the lane never skips. The harness's `--upstream DIR --prefix-of SAMPLE` reads
them and fails unless the first records of each equal the committed sample. The `vectors-full` job in
`nightly.yml` does this on a schedule; it reports through the nightly issue and is not part of `CI required`. The
workflow's other jobs, the benches and the loader fuzz campaign below, report the same way and also sit outside
`CI required`.

### The GPU clipping capture

The `nightly` lane fetches the `ps1-tests` build-158 archive named in `pins.txt` and verifies its SHA-256 before
extracting only the clipping executable and its captured `vram.png` into `build/nightly/gpu-reference`. The
host's Python standard library decodes the bounded capture formats into a temporary little-endian RGB555 file;
alpha is ignored and no color management or rescaling is applied. The C judge loads the pinned executable with
the public core API and makes one bounded frame-advance call. It accepts the documented `FRAME` or no-fault
`BUDGET` stop after ticks advance; the executable waits in `VSync(0)`, so a bounded stop need not mean the guest
returned from that wait. It then compares all 524,288 VRAM words after clearing only bit 15. Every color word must
match. The archive, executable, PNG and decoded pixels are build inputs only: none is committed, uploaded or
packaged. A fetch or digest error is reported separately from a raster mismatch.

### The GTE, DMA and timer checks

Three directories hold the checks of the geometry engine, the DMA controller and the root counters. Each has one
test executable that runs the case named on its command line (`schwaystation-test-gte`, `schwaystation-test-dma`,
`schwaystation-test-timers`), reaches the device's register and step entry points directly, and carries the label of
its directory and `portable`.

```sh
ctest --test-dir build/ci -L '^gte$' --output-on-failure
ctest --test-dir build/ci -L '^dma$' --output-on-failure
ctest --test-dir build/ci -L '^timers$' --output-on-failure
```

**The 1,150 cases.** `gte.vectors.ps1tests` replays `tests/vectors/gte/ps1tests-1150.bin`, the hardware cases of the
ps1-tests GTE run transcoded by `tools/gte2bin.py` (see [Maintaining](maintaining.md)), in the upstream order. The
harness checks the file's histogram (50 register-only cases and 50 of each of 22 commands) and `--expect 1150`, which
is the zero-case guard. Its controls: `gte.vectors.empty.self_test` and `gte.vectors.truncated.self_test` feed it an
empty file and the first 1,000 bytes, and `gte.vectors.flipped.self_test` flips one expected bit of case 600 and must
fail naming that case. The quirk tests (`gte.quirk.*`, `gte.nclip_fake_bits`, `gte.unlisted_noop`, `gte.unr_table`)
pin what the hardware cases reach only in a few encodings.

**Two reference models.** A test that compares an implementation with a model written apart from it is only as good as
the model, so each has its own seed and case count, printed with every run so a failure replays.

| Test | Model | Seed | Cases |
|---|---|---|---|
| `gte.ref.property` | A straight C model of the GTE in `tests/gte/ref.c`, written from the public hardware description and not from the engine, replayed first over the 1,150 cases (`gte.ref.vectors`) | `0x6774652D70726F70` | 22 commands of 1,000 cases each, comparing all 64 registers (FLAG included) and the cycles |
| `timers.reference.random` | A per-cycle counter model in `tests/timers/ref.c`, against which the lazy one that the core runs is compared | `0x7154F4B1E5C0DE05` | 20,000 operations, comparing every read and every interrupt bit |

`gte.ref.property.self_test` flips one bit of one engine-side read and must fail with `engine differs from the model`.

**The `gte_vectors` guard.** `fixtures.gte_vectors.guard` recomputes every `gte_vectors` row of
`tests/diag/expected.csv` from the vector file in C and compares it with the CSV, so an expected value cannot drift
from the file it cites.

**Where a value comes from.** A row or a band is marked by its source. `hw` means the value is copied from a
hardware-captured artefact of an MIT-licensed upstream test at the pinned commit, with the file and the case or line in
the citation: it is hardware-captured per upstream, not verified here, and a value produced by this core or by another
emulator is never `hw`. `doc` means the published hardware description supplies the rule or constant; an exact value
derived from those documented constants remains `doc`, with the calculation included in its citation. `derived` means
a fitted model gives it; the DMA tests keep those as bands (a window, a block cost, the CPU slowdown of a looping list)
and never as exact pins. The timer records are `doc`: the expected line and dot counts follow the documented beam ratio,
line length and counter sources, rather than a measurement copied from this core or another emulator.

**The per-profile judge.** The legacy `timers_lines.exe` records keep their documented interrupt-latency bands and run
on NTSC-U, NTSC-J and PAL. The timing variants configure each dot divider and choose a phase-restoring window: the
window is the divider in NTSC; in PAL it is 5, 2, 7, 5 and 1 frames for dividers 10, 8, 7, 5 and 4. Timer 0 counts
GPU dots and Timer 1 counts HBlank edges. The fixture records the VBlank count, total dots, HBlanks per frame and total
HBlanks. Their exact totals are the documented line clock times 263 or 314 lines times the window, divided by the
selected dot divider; the windows make that division integral. The judge reads the common and matching profile rows
in record order, rejects missing, extra or duplicate record IDs, checks the mailbox status and count, and verifies the
record CRC before judging. A one-count-shift control must fail the dot and line records; a separate control rejects a
duplicate ID.

The fixture reads I_STAT twice before Timer 0 COUNT, then Timer 1 COUNT and I_STAT again. Tracing the PAL /4 case showed
the Timer 0 bus read 14 CPU cycles after both VBlank edges. At those edges the ratio carries are 75,185 and 203,030
modulo 451,584. An 8-cycle sample would advance 12 GPU clocks at one endpoint and 13 at the other; the fixed 14-cycle
sampling path advances 22 at both. This explains the earlier one-dot-short result as CPU instruction/IRQ endpoint
latency. It keeps the expected dot total derived from the documented beam model instead of fitting the oracle to the
observed output.

**The state lines.** With no picture yet, the ordinary frame hash is the test card whatever the CPU and devices did, so
`hashes.txt` carries frame 30 and state hashes for `hello`, `gte_vectors`, `dma_chain` and `timers_lines`. It also carries
`gpu_motion.exe` frame hashes at 60, 120 and 180 and state hashes at 150 and 180. This fixture draws moving pixels
through GP0 and DMA2, so its frame digest covers rendered output; its saved-state digests cover the VRAM and GPU state
needed to continue after frame 150. `hash-equality` compares every row across all eight lanes, including the portable
be32 lane.

### The vectors harness

`schwaystation-test-vectors` replays public single-step vectors through the CPU's one-instruction entry, replaying the
bus from each case's recorded cycles and comparing the final state field by field.

```text
schwaystation-test-vectors FILE --expect N [--only NAME,...] [--allow FILE] [--flip-expected FIELD[@NAME]]
schwaystation-test-vectors --upstream DIR --prefix-of SAMPLE --expect N [--allow FILE]
```

| Option | Meaning |
|---|---|
| `FILE` | The committed sample, `tests/vectors/r3000/sst-sample.bin`: the first 100 cases of each of the 55 upstream files. |
| `--expect N` | Fail unless at least N cases ran, so a run that read nothing or part of a file cannot pass. |
| `--only NAME,...` | Run only the named slices; a name that does not exist is an error. |
| `--allow FILE` | Accept the listed failing cases. A line is `YYYY-MM-DD NAME CASE-HEX ISSUE-URL`; a line that matches no failing case is an error, so an allowance goes when its failure is fixed. `tests/vectors/r3000/allow.txt` holds no entry. |
| `--flip-expected FIELD[@NAME]` | Invert one bit of one case's expected final field, the first case of slice `NAME` if one is named; the negative controls use it. |
| `--upstream DIR` | Replace each slice by the whole upstream file `DIR/NAME.json.bin`, which must begin with the sample's records byte for byte. Goes with `--prefix-of SAMPLE`, and then no positional `FILE`. |

The exit status is 0 when every case passed and `N` or more ran, 1 for a failure or a malformed file and 2 for a bad
command line. Two named rules, each counted in the output and each with its own control, adjust the comparison for
what the vectors record: a misaligned halfword or word access is compared by the exception it must raise, and the
unaligned loads and stores (LWL, LWR, SWL, SWR) by the set of bytes they cover. `vectors.r3000.sample` runs the sample on
every lane; `vectors.r3000.full` runs all 55,000 cases in the `nightly` lane.

### The loader fuzz target

`tests/fuzz/exe.c` is the target. Each input is a whole PS-X EXE file: it is probed and loaded into a power-reset
instance, and the probe and the load must agree, a refused file must change no RAM byte, an accepted one only the
text segment and the launch block, and two frames of the loaded program (`SCHWAY_RUN_STOP_ON_FAULT`) must each
return with a stop reason. A broken invariant prints which one and aborts. `tests/fuzz/seeds.c` builds the seeds in
memory (one either side of each header bound, spans that wrap or leave RAM, and code that hammers addresses no
device decodes), so no seed byte is committed.

`fuzz.regress.exe` replays every seed and every `tests/fuzz/regress/*.hex` through `schwaystation-fuzz-exe-replay`,
which has no fuzzing engine and so runs on every leg, `be32` and `wasm32` included. It gates like any other test.
The campaign is the report-only half: the `fuzz` job in `nightly.yml` builds the `fuzz` preset, writes the seeds with
`schwaystation-fuzz-exe-replay --write DIR`, and runs `schwaystation-fuzz-exe` for 60 seconds with
`-max_len=2099200`. A crash fails the job, which opens or updates the nightly issue and blocks nothing.

To keep a crasher, minimise it (`schwaystation-fuzz-exe -minimize_crash=1 -runs=100000 FILE`), commit it as
`tests/fuzz/regress/<name>.hex` (hexadecimal text; whitespace and `#` comments are skipped) with its fix, and add its
row to `tests/fixtures/manifest.txt`: path, `own generated bytes`, `-`, `MIT`, its SHA-256 and `-`.

## Libretro metadata claims

The generated schwaystation_libretro.info advertises capabilities that have an in-process host test. At configure
time, libretro/CMakeLists.txt runs tests/cmake/info_claims.cmake; libretro.info.claims repeats the same check
in CTest. The checker requires every true functional key to map to registered tests in
tests/libretro/inventory.txt. The host tests exercise the callback or loaded content; an inventory row alone is
not behavior evidence.

libretro.info.claims.self_test runs all 19 failure-direction controls through tests/cmake/expect_fail.cmake. It
checks missing and duplicate keys, malformed values, unsupported claims, removed inventory rows, and an
ungrounded camera=true claim. This confirms that a new true metadata key cannot pass configuration without a test.

| Claim | Behavior cases |
|---|---|
| supports_no_game | libretro.host, libretro.host.pal load without content and check the test-card path. |
| needs_fullpath | libretro.content.no_bios, libretro.content.psexe, libretro.content.lifecycle load by path. |
| core_options | libretro.host, libretro.host.pal check the v2 interface and the older variables fallback. |
| input_descriptors | libretro.input.descriptors checks both ports, all button and axis IDs and labels, plus table lifetime through load, run, unload and deinit. |
| memory_descriptors | libretro.memory.aliases, libretro.memory.boundaries resolve RAM and scratchpad addresses, holes, lifecycle and unavailable callbacks. |
| supported_extensions | libretro.content.no_bios runs .exe; libretro.content.psexe copies the real fixture to mixed-case .PsExE and runs it through the same fullpath loader. |

is_experimental stays true until v2.0. Disk control, cheats, save states, frontend save files and subsystems stay
false because those capabilities are not advertised by this phase.

### Libretro movie frame canaries

`libretro.input.movies` runs four input variants under both controller layouts for 300 frames each. The reference
captures each frame's dimensions independently. Before each module frame, the host poisons exactly the validated
packed `width * height` active-pixel extent; if either expected dimension is zero or exceeds the 640 by 576 host
buffer, it poisons the full buffer instead. Other host paths continue to use the full-buffer `run_one` fill.

The video callback packs each delivered row by its delivered width. The movie comparator first requires those
dimensions to match the reference, then checks every active pixel in the captured RLE runs and verifies the total
pixel count. Thus the canary still detects a missing write anywhere in the expected active image, while writes to
the unused trailing allocation are not repeated. Throwaway instrumentation measured 589,824,000 poisoned bytes
over the 2,400 module movie frames, compared with 3,538,944,000 bytes for full-buffer fills. This byte count
describes removed writes; it does not establish a runtime speedup or explain a hosted timeout.

## The inventories

The tests a lane registers are listed in two levels of plain text:

- `tests/inventory/<preset>.txt` selects directories, one per line (`ci.txt`, `hygiene.txt`, `nightly.txt`,
  `nofp.txt`, `wasm32.txt`); lines starting with `#` are comments.
- `tests/<dir>/inventory.txt` lists that directory's tests, one `name<TAB>labels` line each, labels sorted and
  comma-separated, lines sorted in byte order.

Configure fails when the registered tests, with their labels, differ in either direction from the union of the
selected directories' inventories (`cmake/SchwaystationTests.cmake`, reading them through
`tests/cmake/inventory.cmake`). The message lists each test that is listed but not registered and each that is
registered but not listed. A test cannot be dropped, renamed or registered on one platform only without its
inventory changing in the same commit, and two pull requests adding tests to different directories never touch
the same file.

The compatibility directory is included only when the selected inventory lists `compat`. The native CI and
BE32 presets select `ci`, so they retain every compatibility case; ASan and WASI use inventories that omit it.
The runner and host disc probes use private read-only openers that call `fopen_s` on MSVC and `fopen` elsewhere.
Selected test callers use `tests/test_open.h` for the same CRT-specific spelling while preserving their original
mode strings and NULL-on-open-failure behavior. The libretro content and BIOS readers likewise use a private
read-only opener. This avoids global warning suppression and keeps Windows `/WX` intact. Windows and GCC
cross-compiler acceptance is established by the hosted matrix, not by a native configure.

The CUE reader accepts a relative `FILE` component of up to 255 bytes. Its Windows boundary test confirms that a
255-byte component passes CUE validation and reaches the file-open step; the full identity/open positive uses the
same fixture on POSIX and macOS, where the build-tree prefix stays within the host path limit.

Compatibility scripts prefix each raw helper and runner call with the complete `CMAKE_CROSSCOMPILING_EMULATOR`
argument list. The shell library accepts one runner executable, so cross-emulated library checks use generated
build-tree launchers that quote the emulator arguments and pass runner arguments through unchanged.

| Label | Tests |
|---|---|
| `core` | The library's behaviour: the API contract, sizes, allocation, faults, state, the frame and the test card. |
| `portable` | Tests that hold on every platform, including the frame hashes; the emulated lanes run these. |
| `abi` | The public header compiled alone as C17 and C++17, the API test, and the library's symbols. |
| `runner` | `schwaystation-run` options, output, exit statuses and the PNG writer. |
| `cpu` | The R3000A core: instruction behaviour, exceptions, delay slots and the first-fault trace. |
| `bus` | The address map: physical decode, RAM mirrors, memory control, the I-cache, the scratchpad and the interrupt registers. |
| `vectors` | The committed sample of public single-step vectors, run case by case against the core. |
| `gte` | The geometry engine: the 1,150 hardware cases, the model comparison and the quirks, and the `gte_vectors` rows. |
| `dma` | The seven DMA channels: registers, costs, chopping, linked lists, the order-table channel and the CPU stall. |
| `timers` | The three root counters: modes, targets, the write hold, interrupts, and the `timers_lines` records per profile. |
| `fuzz` | The executable loader's committed fuzz seeds, replayed on every leg; the time-boxed campaign runs nightly and only reports. |
| `exe` | Loading a PS-X EXE under the stub kernel and `--probe`, including the rejected headers. |
| `fixtures` | The fixture toolchain: pinned downloads, the committed stub kernel's source, the manifest rows and the fixtures that run to a sealed mailbox. |
| `libretro` | The libretro core in an in-process host and its `.info` file. |
| `install` | The library installed and found by an outside project. |
| `perf` | Allocation-free frames and the instructions-per-frame gate's own logic. |
| `policy` | Rules about the repository, the documents and the tests themselves. |
| `release` | The packaged archives, run after the package step. |
| `hygiene` | The hygiene lane's checks. |

The directories `cpu`, `bus`, `vectors`, `fuzz`, `exe`, `fixtures`, `gte`, `dma` and `timers` each hold an
`inventory.txt` and carry the label of the same name. `tests/diag/` is a data directory, with no tests of its own: it holds the judged fixture rows
and their dated changes.

### The JUnit check

A zero exit from CTest does not prove that each test ran. After each test step CI runs, for example:

```sh
cmake -DJUNIT=build/ci/junit.xml -DINVENTORY=tests/inventory/ci.txt -DEXCLUDE=release \
  -P tests/cmake/check_junit.cmake
```

It fails unless every selected inventory test whose labels match `LABELS` and not `EXCLUDE` appears exactly once,
ran, and did not fail, error or skip. `policy.no_skip` fails when any test file sets a skip return code, a skip
regular expression or disables a test, so no test is ever conditional.

## The fixtures toolchain

The PS-EXE fixtures are programs written in MIPS assembly for the pinned armips, an MIT-licensed assembler. Their
sources are in `tests/fixtures/src` (`header.inc` and `mailbox.inc` are shared by all of them), and the stub kernel's
is `tests/fixtures/src/stub/stub.asm`.

- `pins.txt` has one `armips` row: a version, the URL of the archive of one full 40-character commit, and its
  SHA-256. `cmake/Armips.cmake` downloads the archive into `build/<preset>/pins`, fails configure unless the digest
  matches, and builds armips for the host. Nothing is installed on the machine. `fixtures.armips_pin.self_test` feeds
  the check a wrong digest.
- The `asan` and `fuzz` presets enable fixture production for their selected libretro consumers. Inventory selection
  still controls standalone test registration: CI keeps all 74 fixture cases, while ASan and fuzz register no
  standalone fixture cases and retain all 18 libretro cases.
- `tests/fixtures/tests.cmake` assembles each source into `build/<preset>/fixtures`; no fixture is committed. The
  one generated file that is, `src/stub_kernel.c`, is the stub kernel as a byte array, listed in
  `tests/fixtures/manifest.txt`; `fixtures.stub_drift` compares it with the freshly assembled image on every lane.
  [Maintaining](maintaining.md) has the commands to rebuild it.
- A test names the fixture it runs: `schwaystation-run --exe FIXTURE ...` for what the runner reports, and
  `schwaystation-test-boot CASE` for what only the core can show (the launch block, the stub's exception record, the
  debug-port bytes). The `nightly` preset sets `SCHWAYSTATION_FIXTURES=OFF` and never builds armips.
- Every hosted leg and every local build uses the same assembler, so a fixture's bytes, and with them the `exe` lines
  of `hashes.txt`, are the same everywhere.

## Judged fixtures

A fixture measures and never judges itself. It writes its results as records into the mailbox at the start of its
image and seals it; `tests/diag/expected.csv` holds the verdicts. A test such as `fixtures.exc_matrix` runs
`schwaystation-run --exe FIXTURE --stop-on mailbox --frames F`, so the mailbox is read at frame F, and the runner prints
one `mailbox ID=VALUE` line per record (see [the runner page](runner.md#mailbox)). The test then reads the
fixture's rows.

| Column | Meaning |
|---|---|
| `fixture` | the fixture's name, `exc_matrix` for `exc_matrix.exe` |
| `test` | the record's name |
| `profile` | the machine profile the row holds for, `*` for any; a test that names one (`-DPROFILE=pal`) judges only the rows for `*` and that profile, and numbers them in order |
| `expected` | eight hex digits, or `lo..hi` for a range |
| `source` | `doc` when a document gives the value, `hw` when a hardware capture does (see [where a value comes from](#the-gte-dma-and-timer-checks)) |
| `citation` | the document or the hardware capture behind the value, with its file and case or line, and no comma |

The nth row of a fixture judges the record with id n, so the order of the rows is the order of the records. A line
that starts with `#` is a comment. The test fails on a record outside its row, a row with no record, a record with no
row, a runner that does not exit 0, and a row whose source is not `doc` or `hw`. A row is `hw` only when a hardware
result exists to cite.

A value may change only through `tests/diag/allow.txt`: a dated line `YYYY-MM-DD fixture row value evidence` that names
the row, the new value as hex and the evidence for it. The test accepts that value for that row. A line that
applies to no record, because the row does not exist or the record has another value, is an error.

## Adding a test

1. Register it in its directory's `tests/<dir>/tests.cmake`:

   ```cmake
   schway_add_test(NAME runner.example
     COMMAND schwaystation-run --frames 1
     LABELS runner TIMEOUT 10)
   ```

   `NAME`, `COMMAND`, `LABELS` and `TIMEOUT` are required; `PASS_REGEX` makes the test pass only when its output
   matches and its command exits 0 (the helper runs it through `expect_fail.cmake` with `CODE` 0, because CTest's own
   pass pattern would override a non-zero exit), and `COST` orders long tests first. A comment above it names the fault the test prevents.
2. Add `runner.example<TAB>runner` to `tests/<dir>/inventory.txt` at its sorted place.
3. Run `cmake --workflow --preset ci`. If configure fails, its message names the difference between what was
   registered and what the inventories list.

A new directory also needs a line in each `tests/inventory/<preset>.txt` that should run it, and an `include()` in
`tests/tests.cmake`. Every lane registers the same tests for its inventory; a check that only one platform can
perform runs there as a CI step instead.

## Negative controls

A check that cannot fail proves nothing, so each repository check has a self test that feeds it a known fault.
`tests/cmake/expect_fail.cmake` runs a command and passes only when it exits with exactly the expected code and its
output matches the expected regular expression; a crash or a missing file does not count as the expected failure.
A test never uses CTest's bare `WILL_FAIL`, which passes on any failure at all.

```cmake
schway_add_test(NAME policy.no_skip.self_test
  COMMAND ${CMAKE_COMMAND} "-DCMD=${cmd}" -DCODE=1 "-DREGEX=CMakeLists.txt:1: ..."
    -P tests/cmake/expect_fail.cmake
  LABELS policy TIMEOUT 10)
```

`CMD` is a CMake list; replace its `;` with `$<SEMICOLON>` before passing it. Fixtures that would trip the
repository's own scans (a home directory path, an executable signature, a skip property) are built in pieces or
written into the build tree when CMake configures, so no tracked file holds them.

## Release firmware scan

The release-labeled release.firmware_scan check reads the retail BIOS MD5 and SHA-1 pairs from the same
src/bios_table.inc table used by the core. It scans every tracked file and every file member extracted from
each release archive, regardless of the member name or suffix. It rejects a listed BIOS digest, an exact
524,288-byte member, the PS-X EXE signature at offset 0, CD001 at the 2048-byte or raw-sector offset, or a
32-byte window of the Sony licence marker. Signatures and licence windows are compared as hex. Extraction
happens in a unique temporary directory outside the tracked tree and archive directories. A missing archive,
malformed table, failed extraction, or archive with no file members is an infrastructure error.

The registered release.firmware_scan.self_test creates synthetic bytes and a separate one-row digest table in
the build directory. It verifies that a matching tracked file and an archive member are rejected; the synthetic
row is never added to the production BIOS table. The archive-edge test also checks a nested matching member,
a one-byte-changed clean input, each prohibited content signature, a zero-file archive, and a malformed
archive. A separate BIOS test runs the synthetic all-zero key through the production identification binary
and confirms that it remains unknown. No retail firmware bytes are needed or used.

## Pinned OpenBIOS CI build

The Linux x64 CI job builds the public `pcsx-redux/nugget` source at
[`9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9`](https://github.com/pcsx-redux/nugget/commit/9db6d88d23c4da7a39b1a4b9729f1e5fff80dec9)
with the PSn00bSDK GCC 12.3.0 Linux archive pinned in `pins.txt` by SHA-256
`228f031a25cf2687d8845fd1421f625bafc211fa27da428e458e80d030a726f8`. Its OpenBIOS Makefile supports
`OPENBIOS_SHELL=shell`, the older C shell. CI selects that option to build the pinned ROM without the separate
`shello`/psyqo C++ target, which requires a `<utility>` header absent from the pinned compiler archive. Source
files remain unchanged. Plan 04-40 carries the later shell-specific checks.

The build command is `make -C openbios -j2 BUILD=Release BOARD=psx BOOT=rom OPENBIOS_SHELL=shell`. The output
stays in the hosted Linux runner's temporary directory. CI checks the size and expected SHA-256 with:

```sh
cmake "-DOPENBIOS_INPUT=$RUNNER_TEMP/nugget/openbios/openbios.bin" \
  -DEXPECTED_FILE=tests/built/expected.txt -P cmake/OpenBIOS.cmake
```

The image must be exactly 524,288 bytes and match SHA-256
`92c2f6d5e6ff056bf2ba177f37246e5468b6164337935297cf7b22266af20cc8`. The registered
`built.openbios.digest` and `built.openbios.digest_corrupt` tests exercise matching and one-byte corrupted
synthetic copies in the build tree; neither test stores firmware bytes in the repository.

The measured Linux step exited 0 in [workflow run 43060258449](https://github.com/szTheory/schwaystation/actions/runs/43060258449).
The ROM is neither committed, uploaded, nor reused across runs.

## Budgets

**Documents.** `docs.budgets` keeps `README.md` under 16,384 bytes, `include/schwaystation.h` under 40,960 bytes and
each `docs/*.md` and `docs/*/*.md` page under 61,440 bytes, each strictly below its limit, and names every file at or over it with
its size. Run it by hand with `cmake -DROOT=. -P tests/cmake/budgets.cmake`.

**Gate code.** Repository-check code is CMake `-P` scripts or POSIX sh. `policy.gate_budget` (hygiene lane) counts
every line of `scripts/*.sh`, `tests/cmake/*.cmake` and `.githooks/*` against a fixed limit set in
`tests/hygiene/tests.cmake`; the limit is an absolute line count. Phase 3 closed at 1513 lines; the approved Phase 4
ceiling is 2700 lines for the whole counted set. This phase ceiling is temporary: shipping a release measures
the total and ratchets the limit down to that total. A gate that does not fit replaces or folds an older one. Check the total with
`cat scripts/*.sh tests/cmake/*.cmake .githooks/* | wc -l`. Gate logic stays in those files: it is not moved
into `tests/<dir>/tests.cmake`, `cmake/` or workflow YAML to fit.

**End-to-end code.** Everything under `tests/retroarch` stays within 300 lines in total; `policy.e2e_budget`
counts them with the same script, `tests/cmake/line_budget.cmake`.

## Performance

**Instructions per frame.** CI's `perf` job counts the instructions `schwaystation-run` executes per frame under
Cachegrind. Test-card Ir per frame = (Ir at 360 frames − Ir at 60) / 300. The CPU/GPU `gpu_motion` fixture uses
(Ir at 60 frames − Ir at 10) / 50 to keep the Cachegrind workload within the measured job budget. Both compare
against the same fixture on the control commit. A pull request's head is measured once and its base twice, then
`tests/cmake/perf_compare.cmake` prints one line per fixture:

```text
perf testcard head=<n> base=<n> delta_bp=<n> verdict=PASS
perf gpu_motion head=<n> base=<n> delta_bp=<n> verdict=PASS
```

`delta_bp` is the change in basis points. `PASS` means at most 1.00 % slower (exactly +1.00 % passes); `FAIL` more
than that or more than the allow file permits; `UNSTABLE` the two base measurements differ; `NO_BASE` the base has
no `perf` preset. An empty or non-numeric measurement is an input error, never a pass.

**Gate mode.** CI sets `SCHWAYSTATION_PERF_MODE` to `gate`: `FAIL`, `UNSTABLE` and `NO_BASE` exit 1. The script
also accepts `report`, which prints the verdict and exits 0; bad input always fails. A manual run without inputs
measures both fixtures three times and reports their spread. A manual run with `control_ref` set to a full commit
SHA compares both fixtures on the current commit with that exact control. Before arming the GPU-motion row, run
42980564011 measured control commit `c87f40b` at 137,678,337 instructions per frame on all three GPU-motion
repeats (spread 0), and 1,620,845 on all three test-card repeats (spread 0). With the shorter GPU-motion window,
control run 42984345515 measured 137,678,369 on three repeats (spread 0). The final comparison run 42985306022
compared head `3bb84fa` against control `045b76a`:

```text
perf testcard head=1620845 base=1620845 delta_bp=0 verdict=PASS
perf gpu_motion head=137678369 base=137678369 delta_bp=0 verdict=PASS
```

The `perf` job took 1m39; `be32` was the slowest required job at 4m20, and the complete workflow took 4m48. The
comparison is at [Actions run 42985306022](https://github.com/szTheory/schwaystation/actions/runs/42985306022).

**Exceptions.** A pull request that must be slower adds a line to `tests/perf/allow.txt`:
`<fixture> <max % × 100> <expiry YYYY-MM-DD> <reason>`. It raises that fixture's limit through its expiry, at most
60 days away, and is removed after the pull request merges.

The `perf.gate.*` tests feed the comparison its edge cases on synthetic measurements, so they run on every lane
without Valgrind: identical runs, exactly +1.00 %, +1.01 %, an allowed and an expired exception, unstable bases, no
base, a missing measurement and an unset mode. A separate slow `gpu_motion` synthetic row must fail above +1.00 %.

The `nofp` job has a three-minute floor because its one-minute timeout cancelled the same release commit twice
while hosted checkouts/images were starting: runs [42840838785](https://github.com/szTheory/schwaystation/actions/runs/42840838785)
and [42841474140](https://github.com/szTheory/schwaystation/actions/runs/42841474140). The release commit then passed
on `main` in 9 seconds and an isolated rerun passed in 10 seconds, confirming that the cancellation was startup
contention rather than a core failure. Both no-fp legs passed again in the final hosted run above.

## CI jobs

`.github/workflows/ci.yml` runs on every pull request, every push to `main`, by hand, and when `release.yml` calls
it with a commit. Each job runs the same workflow preset a contributor runs, then the JUnit check above.

| Job | What it runs | Required |
|---|---|---|
| `changes` | Lists the pull request's files. `heavy=false` only when every path is under `.planning/`; then the build jobs skip. | yes |
| `hygiene` | The `hygiene` preset and its JUnit check. | yes |
| `title` | `scripts/check-title.sh` on the pull request's live title and body (pull requests only). | yes |
| `build` | The `ci` preset on `ubuntu-26.04`, `ubuntu-26.04-arm`, `macos-15` and `macos-15-intel`, `ci-msvc` on `windows-2025` and `windows-11-arm`. Pull requests also check out the exact base SHA's compatibility files before the head; the ratchet runs the copied base judge against same-run fixture results. Both JUnit checks (all but `release`, then `release`), the README install line and its flipped-byte control on macOS arm64, then uploads `archives-<os>-<arch>` (the three zips and `commit.txt`) and `hashes-<os>-<arch>`. | yes |
| `asan` | The `asan` preset; JUnit check without `abi` and `release`. | yes |
| `nofp` | The `nofp` preset on both Linux legs. | yes |
| `be32` | The `be32` preset (`portable` tests) and uploads `hashes-be32`. | yes |
| `hash-equality` | `tests/cmake/hashes.cmake` with `MODE=compare` and `COUNT=8`: the six build legs', `be32`'s and `wasm32`'s `hashes.txt` must be byte-identical. | yes |
| `wasm32` | The `wasm32` preset with the pinned wasmtime; uploads `hashes-wasm32` for the eight-lane comparison. | yes |
| `retroarch` | The pinned RetroArch 1.22.2 runs the macOS arm64 core with no content for 5 frames; its screenshot must equal `schwaystation-run --frames 5 --png 5:FILE` pixel for pixel at `320x240`. Pixel and wrong-dimension controls must fail. It also replays `pad_counter.exe` with Cross at guest frame 60; captures for guest frames 59 and 60 must differ and each must equal its runner PNG. | yes |
| `perf` | Cachegrind instructions per frame for testcard and `gpu_motion`, compared with the merge base in gate mode on pull requests. Manual runs measure both fixtures three times or compare `gpu_motion` with an explicit control commit. | yes |
| `rehearsal` | Rehearses publication on same-repository pull requests that change release inputs, signing a `rehearsal-*` release. See [maintaining.md](maintaining.md). | yes, when applicable |
| `cleanup` | Runs whatever the rehearsal did, once it started, and deletes the rehearsal release and its tag. See [maintaining.md](maintaining.md). | yes, when applicable |
| `CI required` | The one check the ruleset requires. It passes only when every job above succeeded or was skipped as allowed: heavy jobs when `heavy=false`, `title` and `perf` outside pull requests, `perf` in a call from `release.yml`, and rehearsal jobs when publication is not applicable. | the check |

`wasm32` became required after a successful run on `main`. Timeouts are twice the slowest successful duration
measured in this repository's own runs (5 minutes at least for network-bound and roll-up jobs); the shared build
timeout also covers an observed Windows ARM64 overrun. ASan has a 15-minute provisional job bound after run
43309540398 reached its old 5-minute cap at test 404 of 405, with `libretro.input.movies` active for 3m22s.
BE32 has a 25-minute provisional bound after the same run reached its old 15-minute cap at test 475 of 476, with
`libretro.input.movies` active for 7m54s. Earlier BE32 movie cases were also much slower than their x64 counterparts.
These bounds allow the remaining current workload to finish; neither lane has yet established a complete duration.
`libretro.input.movies` is serial with a 240-second base CTest timeout (scaled by each lane as usual); runs
43271212007 and 43292417084 reached its former 120-second limit. The longer bound and serialization preserve every
movie and assertion and are provisional; they do not claim a speedup or prove why earlier runs were slow. Each timeout names its evidence run and stays marked
`provisional` while fewer than 10 samples exist; `hygiene.timeout_comments` fails a `timeout-minutes` line that
names neither a run nor `provisional`, and `policy.action_pins` fails a `uses:` line that is not a full 40-hex
commit with a `# v` comment.

**Downloads.** `pins.txt` lists every download CI makes (wasmtime and the RetroArch DMG) with its SHA-256;
`scripts/fetch-pin.sh NAME DEST` refuses a file whose digest differs.

**A red retroarch job.** The job prints RetroArch's verbose logs and uploads `retroarch-frames` after success or
failure for one day, holding
`shot.png` and `runner.png` for the no-content comparison plus both content captures and runner references. The
step output names the first differing pixel or geometry. The one-day `retroarch-replay-probe` artifact contains
the raw same-run BSV, its guarded Cross variant, and the isolated record/playback logs; it is diagnostic input,
not a release archive.
Download the artifact with `gh run download <run id> -n retroarch-frames` and open both images. A size difference
means the core reported another geometry; a whole-image difference with the same layout usually means RetroArch's
frame and the runner's frame are not the same frame, which the test card's frame counter shows.

The hosted content check downloads `pad-counter-fixture` as a separate one-day artifact and keeps `system/` empty.
The guest speaks the five-byte digital SIO protocol, so this isolated session sets
`schwaystation_analog_mode = "digital"`; the core's default DualShock mode remains analog. RetroArch records a
fresh 60-limit replay on the same run. A bounded parser checks its v2 header, checkpoint lengths, all 59 records,
both complete joypad event sets, the zero-input Cross values and back-references. It copies the movie and changes
only the final port-0 Cross value from zero to one; the checkpoint and content metadata stay byte-identical.
Playback itself validates the untouched checkpoint through the pinned frontend. No movie bytes are committed or
cached across runs.

The pinned frontend counts presented video callbacks for `--max-frames`. Its default threaded task queue can
finish replay startup after different numbers of frontend iterations, so the isolated test config disables
`threaded_data_runloop_enable`. The synchronous queue processes replay startup after exactly one pre-replay
content frame; then the replay task restores the checkpoint at core frame 1 and preloads input group 1. Thus
frontend limits 59 and 60 consume 58 and 59 replay groups and capture guest outputs 59 and 60, respectively.
At limit 60, the frame-count exit check runs before a later paused cached-frame refresh, so this is the output
with group 59's Cross press rather than a duplicate. Each runner reference uses `--exe pad_counter.exe`, the same digital
`--inputs` script with Cross pressed at guest frame 60, and `--frames 59` or `--frames 60`. The screenshot check
occurs after the callback at the requested frontend count. The task-queue order and frame check are visible in
the pinned frontend sources: [threading default](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/config.def.h#L449-L458),
[queue initialization](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/retroarch.c#L8144-L8150),
[normal loop ordering](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/retroarch.c#L6063-L6065),
[replay task startup](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/tasks/task_movie.c#L71-L113),
[task dequeue before the core call](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/runloop.c#L7213-L7215), and
[frame-limit exit check](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/runloop.c#L5921-L5954). Also see the pinned
[RetroArch runloop frame-limit check](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/runloop.c#L5697),
[screenshot/exit ordering](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/runloop.c#L5921-L5945)
and [video callback counter](https://github.com/libretro/RetroArch/blob/69a4f0ea1e8aaf442ae4858f2e7f2b31a1776576/gfx/video_driver.c#L4324-L4347).
The guest output is 256x240: frame 59 is blank, and the frame-60 Cross edge draws one red pixel at (0,100).
The two decoded RetroArch captures must differ and each must be pixel-exact to its corresponding runner image.
The runner uses `--exe` without `--bios`, so it exercises the same BIOS-free stub-kernel path. This is the recurring
real-host proof; the in-process input, card and rumble matrices remain independent.

**Reproducing a red job.** Run the job's preset locally (`cmake --workflow --preset <name>`) and the same JUnit
check. Lanes this host cannot run are listed above under "Lanes that need tools only CI has".
