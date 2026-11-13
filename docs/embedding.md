# Embedding the library

The library is one static archive and one header, `schwaystation.h`, written in C17. It allocates memory only
through the allocator you pass, keeps all state in the instance, starts no threads, opens no files, reads no clock
and uses no floating point. It needs nothing from the C library beyond `memcpy`, `memmove`, `memset` and `memcmp`.
Two instances in one process share nothing, so a host may run several side by side.

With no host-supplied BIOS or PS-EXE, an instance shows the built-in test card. A host may load a BIOS or PS-EXE
before execution. The core opens no files; the host supplies BIOS bytes through the API.

## Getting it

**Source tree.** `add_subdirectory` or `FetchContent` gives the `schwaystation::schwaystation` target. Only the C
compiler is needed: an embedding build gets the library alone, without the runner, the tests or this project's
warning flags (`SCHWAYSTATION_BUILD_RUNNER` and `SCHWAYSTATION_BUILD_TESTS` default to off when schwaystation is
not the top-level project).

```cmake
add_subdirectory(third_party/schwaystation)
target_link_libraries(app PRIVATE schwaystation::schwaystation)
```

`schwaystation.h` includes `schwaystation_version.h`, which the build generates from `version.txt`; the target's
include path covers both, and `cmake --install` puts them side by side.

**Release archive.** `schwaystation-library-<os>-<arch>.zip` holds `include/`, the static library in `lib/`, a
CMake package in `lib/cmake/schwaystation/` and `lib/pkgconfig/schwaystation.pc`. Unzip it anywhere; both files
find the rest from their own location. The CMake package accepts a request from the same major and minor version
only, because before 1.0 a minor release may change the API.

```cmake
# configure with -DCMAKE_PREFIX_PATH=<unzipped directory>
find_package(schwaystation CONFIG REQUIRED)
target_link_libraries(app PRIVATE schwaystation::schwaystation)
```

```sh
PKG_CONFIG_PATH=<unzipped directory>/lib/pkgconfig cc app.c $(pkg-config --cflags --libs schwaystation)
```

The header compiles as C and as C++.

## Structs and sizes

Every struct you pass starts with `uint32_t size`. Zero the whole struct with `memset`, then set `size`:

```c
schway_config cfg;
memset(&cfg, 0, sizeof cfg);
cfg.size = (uint32_t)sizeof cfg;
```

The size tag lets a program built against one header run with a library built from another.

**Input structs** (`schway_config`, `schway_profile`, `schway_budget`, `schway_audio`, `schway_tty`):

- A size below the struct's first released size returns `SCHWAY_STATUS_ERR_STRUCT_SIZE`.
- A larger size from a newer header is accepted only if every byte past the library's struct is zero; a field
  added later keeps the old behaviour when it is zero.
- A size above 4096 bytes is refused whatever the bytes hold, so a garbage tag never makes the library read far
  past your object.

**Output structs** (`schway_versions`, `schway_info`, `schway_run_result`, `schway_video`, `schway_exe_info`,
`schway_fault`):

- A size below the struct's first released size returns `SCHWAY_STATUS_ERR_STRUCT_SIZE` and leaves the struct as
  it was.
- Any larger size is accepted: the library writes the smaller of your size and its own, keeps your `size` value
  and leaves every byte past its own struct as you set it.

A call that returns an error changes nothing: not the instance, not your struct.

## Pad input and frame reports

`schway_input` is a host snapshot for both controller ports. `device` uses `SCHWAY_INPUT_DEVICE_NONE` to
disconnect a port, `SCHWAY_INPUT_DEVICE_DIGITAL` for a two-button-byte pad and `SCHWAY_INPUT_DEVICE_DUALSHOCK`
for a DualShock. Set bits in `buttons` mean pressed; SIO returns those buttons active-low. The four raw axis bytes
are right X, right Y, left X and left Y, with 128 at centre. `analog_button` is a held 0/1 input; a rising edge
toggles analog mode when the pad has not locked that button.

Zero the struct, set its `size`, then call `schway_set_input`. The snapshot takes effect at the next frame start,
so input set during a frame cannot change a poll already in progress. Host input itself is outside save states;
the latched pad and in-progress serial protocol are saved.

The appended `schway_run_result.lag` is 1 when no complete controller poll occurred in that frame; peeks do not
count. `rumble[0]` and `rumble[1]` report each port as `large << 8 | small`, where large is 0–255 and small is
0 or 1. A smaller result struct from an older caller keeps its existing fields and is not written past its `size`.

### DualShock commands and rumble

An analog DualShock answers a normal `42h` poll with ID `73h`; digital mode uses `41h`. Config mode answers
`F3h`, and its `42h` poll always includes buttons and all four axes. Command `43h` enters config with data byte
`01h` and leaves it with `00h`. Outside config its six reply bytes are the normal poll; inside config they are
zeroes.

In config mode, command `44h` sets the analog LED (data byte 0: `00h` off, `01h` on) and analog-button lock
(data byte 1: `03h` locks; `00h`–`02h` unlock). Its response is `00 00 Err 00 00 00`. Command `45h` reports
`01 02 analogMode 02 01 00`. Command `46h` selects actuator information: selector `00h` reports
`00 00 01 02 00 0A`; selector `01h` reports `00 00 01 01 01 14`. Command `47h` returns
`00 00 02 00 01 00`. Command `4Ch` returns `00 00 00 04 00 00` for selector `00h` or
`00 00 00 07 00 00` for `01h`.

Command `4Dh` returns the previous six motor-map bytes while installing the new mapping when the transfer
completes. Each entry is `00h` for the small motor's bit 0, `01h` for the large motor's byte, or `FFh` to ignore
that poll byte. The default is six `FFh` entries; `00 01 FF FF FF FF` enables both motors.

Before a config command is used, the older two-byte motor request turns on the single motor when its first data
byte is `40h`–`7Fh` and bit 0 of the second byte is set; `00 00` turns it off. Config use disables this older
method until reset. If no command arrives for exactly 33,868,800 emulated CPU ticks, the pad exits config, returns
to digital mode and locks rumble until reset. The host receives the motor levels in `schway_run_result.rumble`.

## The allocator

```c
static void *my_alloc(void *user, size_t size) { (void)user; return malloc(size); }
static void my_free(void *user, void *ptr, size_t size) { (void)user; (void)size; free(ptr); }

cfg.allocator.alloc = my_alloc; /* cfg.allocator is a schway_allocator */
cfg.allocator.free = my_free;
cfg.allocator.user = NULL; /* passed back to both */
```

Both hooks are required; a missing one returns `SCHWAY_STATUS_ERR_ARGUMENT`. `alloc` must return memory aligned
for any type (`max_align_t`). `schway_create` calls `alloc` exactly once, and `schway_destroy` calls `free` once
with the same pointer and size. Nothing in between allocates or frees, so a host may run a frame from an audio
callback or on a fixed-memory target. If `alloc` returns `NULL`, `schway_create` returns
`SCHWAY_STATUS_ERR_NO_MEMORY` and leaves `*out` as it was.

## Running

```c
schway *inst = NULL;
if (schway_create(&cfg, &inst) != SCHWAY_STATUS_OK) { /* ... */ }

schway_profile profile;
memset(&profile, 0, sizeof profile);
profile.size = (uint32_t)sizeof profile;
profile.region = SCHWAY_REGION_PAL;
schway_set_profile(inst, &profile);

for (;;) {
    schway_run_frame(inst, NULL); /* or a schway_run_result */
    /* schway_get_video, then schway_get_audio */
}
schway_destroy(inst); /* NULL is fine */
```

- `schway_create` powers on an NTSC-U machine at frame 0.
- `schway_set_profile` picks the region (`SCHWAY_REGION_NTSC_J`, `SCHWAY_REGION_NTSC_U` or `SCHWAY_REGION_PAL`) and
  the flags (below); any other value returns `SCHWAY_STATUS_ERR_PROFILE_REJECTED`. It works only before the first
  cycle runs; after that it returns `SCHWAY_STATUS_ERR_PROFILE_LOCKED`, even after a reset. To change region, destroy
  and create again.
- `schway_run_frame` runs to the current field wrap for loaded content and returns. With no loaded program it keeps the
  deterministic test-card frame boundary. The optional `schway_run_result` gets the stop reason
  (`SCHWAY_STOP_FRAME`), the frame number since power-on and the CPU cycles run since power-on.
  If the next frame would exceed the save format's inclusive limits (2^62 ticks or
  floor(2^62 / 566,203) frames), it returns `SCHWAY_STATUS_ERR_STATE_FIELD` without changing the instance
  or result. Save the current state or use a power reset to restart the counters.
- `schway_reset(inst, SCHWAY_RESET_POWER)` returns to frame 0 with the same region; other kinds return
  `SCHWAY_STATUS_ERR_ARGUMENT`.
- `schway_get_info` gives the display size, the frame rate as a fraction (53,693,175 / 897,619 for NTSC,
  53,203,425 / 1,069,484 for PAL), the sample rate (44,100 / 1), the most stereo pairs one frame can make (738 or
  887) and the region.

## Bounded runs

```c
schway_budget budget;
memset(&budget, 0, sizeof budget);
budget.size = (uint32_t)sizeof budget;
budget.max_cycles = 100000;
schway_run_result result;
memset(&result, 0, sizeof result);
result.size = (uint32_t)sizeof result;
schway_run(inst, &budget, &result);
```

`schway_run` advances until the field/frame boundary or until `max_cycles` CPU cycles have run, whichever comes first.
For loaded content, each call also has an independent maximum-field interval; if repeated GPU resets prevent a field
wrap, the call returns `SCHWAY_STOP_BUDGET` at that limit. The result says which stop occurred:

| Stop | Value | Meaning |
|------|-------|---------|
| `SCHWAY_STOP_FRAME` | 1 | the frame ended; a budget that ends exactly on the boundary reports this |
| `SCHWAY_STOP_BUDGET` | 2 | `max_cycles` or the independent loaded-content call bound ran out inside the frame |
| `SCHWAY_STOP_HOOK` | 4 | the guest wrote the exit port (profile flag `SCHWAY_PROFILE_DEBUG_PORT` set); `exit_code` holds the byte |
| `SCHWAY_STOP_FAULT` | 5 | `SCHWAY_RUN_STOP_ON_FAULT` is set and a fault is latched, or the guest double-faulted at an exception vector |

Values 3 and 6 are not used. A loaded-content call is bounded by one maximum regional field interval plus the existing
instruction overrun. With a BIOS or program loaded, a run stops at the first instruction boundary at or after the
budget or field wrap, so `ticks` can pass either by the rest of one instruction (see
[the CPU page](hardware/cpu.md)).

- `max_cycles` 0 returns `SCHWAY_STOP_BUDGET` and changes nothing, the profile lock included.
- `flags` takes `SCHWAY_RUN_STOP_ON_FAULT`, `SCHWAY_RUN_TRACE` or both; any other bit returns
  `SCHWAY_STATUS_ERR_ARGUMENT`. With the first set, a latched fault stops the run with `SCHWAY_STOP_FAULT` before any
  cycle runs. The second is [the trace](#the-trace).
- `schway_run_frame` is the same advance with no caller limit. After a budgeted run it finishes the current field, unless
  repeated GPU resets reach the independent call bound first.
- A stopped run resumes where it stopped. With no BIOS or program loaded, running N cycles and then M leaves exactly
  what N + M cycles leave: the same state bytes, frame, ticks and audio pairs. With a BIOS or program loaded, the same
  holds where N is an instruction boundary the one-shot run passes through, for instance where a run of the same
  budget stopped.
- In the result, `exit_code` is the code a guest asks to stop with (0 until one does) and `fault_present` is 1 once a
  fault is latched; `schway_run_frame` fills both too.
- A run that would pass the save format's limits (see Running) returns `SCHWAY_STATUS_ERR_STATE_FIELD` and changes
  nothing.

## Peeking at memory

```c
uint32_t word;
schway_peek(inst, SCHWAY_SPACE_RAM, 0x10000, 4, &word);
```

`schway_peek` reads RAM (`SCHWAY_SPACE_RAM`, offsets 0 to 1FFFFFh) or the scratchpad (`SCHWAY_SPACE_SCRATCHPAD`,
offsets 0 to 3FFh) as a little-endian value of 1, 2 or 4 bytes at an offset that is a multiple of the width. The
offset is inside the space, not a CPU address, and RAM has no mirrors here. Any other space, width, offset or a
`NULL` value returns `SCHWAY_STATUS_ERR_ARGUMENT` and leaves `value` as it was. A peek records no fault, counts
nothing and changes no byte of the instance, so peeking everything leaves the state bytes as they were.

## The first fault

The first event since power-on that the model calls impossible or does not build yet is latched once and kept until a
power reset. `schway_get_fault` copies it into a `schway_fault`; with none latched, `kind` is `SCHWAY_FAULT_NONE` and
every field is 0. `fault_present` in a run result tells you without the call.

`device` says what the access reached, `width` is its size in bytes, `frame` and `ticks` are the time it was latched,
`pc` and `opcode` the instruction, `addr` the address the access named, `line` and `dot` the beam position (0 until
the GPU keeps one) and `note` extra detail (the repeat count for `SCHWAY_FAULT_EXCEPTION_LOOP`, 0 otherwise).

| Kind | Value | Meaning |
|------|-------|---------|
| `SCHWAY_FAULT_NONE` | 0 | nothing latched |
| `SCHWAY_FAULT_UNMAPPED_READ` | 1 | a read from an address no device owns |
| `SCHWAY_FAULT_UNMAPPED_WRITE` | 2 | a write to an address no device owns |
| `SCHWAY_FAULT_UNMAPPED_FETCH` | 3 | an instruction fetch from such an address |
| `SCHWAY_FAULT_MISALIGNED_READ` | 4 | a load not aligned to its width |
| `SCHWAY_FAULT_MISALIGNED_WRITE` | 5 | a store not aligned to its width |
| `SCHWAY_FAULT_MISALIGNED_FETCH` | 6 | a fetch from an address that is not a multiple of 4 |
| `SCHWAY_FAULT_RESERVED_INSTRUCTION` | 7 | an encoding the CPU does not define |
| `SCHWAY_FAULT_COP_UNUSABLE` | 8 | an instruction for coprocessor 1 or 3, which the console lacks |
| `SCHWAY_FAULT_EXCEPTION_LOOP` | 9 | the same exception return address taken 1,000 times |
| `SCHWAY_FAULT_DMA` | 10 | a DMA setup the model does not run: a channel other than OTC written with the reserved sync mode 3 and the start bit; `device` is `SCHWAY_DEVICE_DMA`, `addr` the channel's `CHCR`, `opcode` the value written and `pc` the writing instruction |

| Device | Value | Device | Value | Device | Value |
|--------|-------|--------|-------|--------|-------|
| `SCHWAY_DEVICE_NONE` | 0 | `SCHWAY_DEVICE_IRQ` | 5 | `SCHWAY_DEVICE_SIO` | 11 |
| `SCHWAY_DEVICE_RAM` | 1 | `SCHWAY_DEVICE_DMA` | 6 | `SCHWAY_DEVICE_SPU` | 12 |
| `SCHWAY_DEVICE_SCRATCH` | 2 | `SCHWAY_DEVICE_TIMER` | 7 | `SCHWAY_DEVICE_EXP1` | 13 |
| `SCHWAY_DEVICE_BIOS` | 3 | `SCHWAY_DEVICE_CDROM` | 8 | `SCHWAY_DEVICE_EXP2` | 14 |
| `SCHWAY_DEVICE_MEMCTL` | 4 | `SCHWAY_DEVICE_GPU` | 9 | `SCHWAY_DEVICE_EXP3` | 15 |
| | | `SCHWAY_DEVICE_MDEC` | 10 | `SCHWAY_DEVICE_GTE` | 16 |
| | | | | `SCHWAY_DEVICE_UNMAPPED` | 17 |

## The trace

```c
schway_budget budget;
memset(&budget, 0, sizeof budget);
budget.size = (uint32_t)sizeof budget;
budget.flags = SCHWAY_RUN_STOP_ON_FAULT | SCHWAY_RUN_TRACE;
budget.max_cycles = 100000;
schway_run(inst, &budget, &result);

schway_trace trace;
memset(&trace, 0, sizeof trace);
trace.size = (uint32_t)sizeof trace;
for (uint32_t i = 0; schway_get_trace(inst, i, &trace) == SCHWAY_STATUS_OK; i++) {
    /* record i, the oldest first; trace.count records are held */
}
```

A run whose budget has `SCHWAY_RUN_TRACE` set keeps what the CPU did in a ring of 4,096 records inside the instance
(about 128 KiB, allocated with it). A run that was not asked to trace adds nothing and pays nothing: the traced loop is
a second compile of the executor, chosen once for the call, and the untraced one holds no trace code. A longer run keeps
the last 4,096 records and a shorter one all of them, across calls, until a power reset empties the ring.
`schway_run_frame` never traces.

`schway_get_trace` copies record `index` into a `schway_trace`, counting from the oldest; `count` in it is the number of
records held. An `index` at or past `count`, an instance whose ring is empty, a `NULL` pointer or a size tag below the
first released size returns an error and leaves `trace` as it was. The ring is not in the saved state, a hash or the
visitor, and tracing changes none of them: the same run traced and untraced gives the same state bytes, video, audio and
fault record.

| Event | Value | Record |
|-------|-------|--------|
| `SCHWAY_TRACE_EXEC` | 1 | an instruction: `pc` and `opcode` |
| `SCHWAY_TRACE_RD` | 2 | a data read: `pc` of the instruction, `addr`, `width` in bytes, `device` and the `value` read (0 for a bus error) |
| `SCHWAY_TRACE_WR` | 3 | a data write: the same, with the `value` written, cut to the width |
| `SCHWAY_TRACE_IRQ` | 4 | an interrupt taken: `pc` the address returned to, `opcode` the word there and `value` CAUSE as it was |
| `SCHWAY_TRACE_FAULT` | 5 | the first fault, from its record, in the block that latched it: `value` is the kind, with `pc`, `opcode`, `addr`, `width`, `device` and `note` as `schway_fault` has them |

Every record has `frame`, `cycle` (CPU cycles since power-on: the instruction's start, plus what it spent before an
access), `line` and `dot`. `device` is a `SCHWAY_DEVICE_*` value, `SCHWAY_DEVICE_NONE` for the events that name no
device and for a data access with the cache isolated. `note` is 0 except in a fault record, which carries the repeat
count of `SCHWAY_FAULT_EXCEPTION_LOOP` (up to 65,535). A fault ends the tail: under `SCHWAY_RUN_STOP_ON_FAULT` its
record is the last one.

Reads and writes are records too, so the ring spans about 1,500 to 2,500 instructions of history, not 4,096. How far
back it reaches depends on how much of the code touches memory, and is documented, not fixed. `line` and `dot` are
derived, not measured: see [the CPU page](hardware/cpu.md).

## Loading a PS-EXE

```c
schway_exe_info info;
memset(&info, 0, sizeof info);
info.size = (uint32_t)sizeof info;
if (schway_probe_exe(bytes, n, &info) == SCHWAY_STATUS_OK) {
    schway_load_exe(inst, bytes, n);
}
```

`schway_probe_exe` checks a file and describes it without needing an instance; `schway_load_exe` runs the same checks
and then copies the file's text segment into RAM at its address. Load after `schway_create` or a power reset and
before guest execution advances: a call after any guest ticks, after a frame has run, or after an executable or BIOS
is already loaded returns `SCHWAY_STATUS_ERR_ARGUMENT`. A power reset erases an executable, so load it again after
one. A refused file changes nothing. A `NULL` pointer or `n` of 0 returns `SCHWAY_STATUS_ERR_ARGUMENT`.

The libretro adapter accepts only fullpath `.exe` content. It reads at most the PS-EXE header plus 2 MiB, validates
the complete file before replacing the active instance, then passes its bytes to `schway_load_exe`. PS-EXE content
always starts with the core's real-MIPS stub; the BIOS option applies only to a no-content load.
`retro_load_game(NULL)` defaults to the built-in test card. A frontend may explicitly select one of the BIOS choices
in core options; the selected file is read relative to the frontend's system directory and passed to
`schway_load_bios`. A missing or wrong-sized selected file refuses that load instead of falling back to the test
card. Disc and playlist formats are not accepted.

### Libretro controllers and rumble

Each port offers DualShock, PlayStation Controller and None; DualShock is the default device. The
`schwaystation_analog_mode` option selects the DualShock wire mode (`analog` or `digital`). With
`schwaystation_analog_toggle` enabled, holding Select, Start, L1, L2, R1 and R2 together for 60 frames toggles that
mode. Buttons and stick axes are sampled into one input snapshot per `retro_run`; signed stick endpoints map to
0–255 with zero at 128. Selecting None disconnects that port, and an unsupported device request leaves its prior
selection in place. The frontend device and wire mode are separate: a DualShock selected in digital mode sends
the two-byte button response, while analog mode sends the six-byte packet with both sticks.

When the frontend provides `GET_RUMBLE_INTERFACE`, every successful frame reports strong and weak motor levels
for both ports, including unchanged levels. Strong values scale the core's 0–255 level by 257; weak is 65535 for
on and 0 for off. Reset and unload send zero to both effects on both ports. A frontend without a rumble interface
continues to run normally.

The Region option takes effect at content load. After changing it, reload content to apply the selection;
the current game, memory spans, card data and timing remain intact until unload. Input options continue to
apply on the next frame.

The libretro adapter converts native 15-bit and packed 24-bit frames to XRGB8888 in its fixed maximum-size host
buffer. It announces `SET_GEOMETRY` from `retro_run` only when frame dimensions change. The frontend reads the
selected region's rate through `retro_get_system_av_info` after loading content. A malformed or out-of-range native frame is dropped
before it changes geometry state or reaches the video callback.

### Libretro memory and cards

While a game is loaded, `RETRO_MEMORY_SAVE_RAM` exposes card 1 as its stable 131,072-byte instance span.
`RETRO_MEMORY_SYSTEM_RAM` exposes the 2 MiB main RAM span. Their pointers remain valid until unload or deinit.
The core does not write card 1's `.srm` file; the frontend owns that file.

For fullpath PS-EXE content, the adapter asks `GET_SAVE_DIRECTORY` for the save location and uses the filename
without `.exe` as the card basename. Card 2 loads from `<basename>.1.mcr`; a missing or rejected image leaves the
instance's blank card in place. After a successful guest card write changes its generation, unload and deinit
persist card 2 through the host's atomic card-file helper. An unchanged generation causes no write. If the save
directory callback or a usable directory is absent, the game still runs and card 2 stays in memory for that session.
If an atomic save fails, the previous complete file remains in place. Card bytes and generations remain outside
ordinary save states.

The registered `PSX` memory map describes the 2 MiB RAM span at physical `00000000h`, cached `80000000h` and
uncached `A0000000h` addresses. Bits 21–22 are disconnected mirror bits; the region selector ignores those bits
and the in-region byte offset. The 1 KiB scratchpad is mapped at `1F800000h` and its cached alias `9F800000h`;
there is no scratchpad descriptor at `BF800000h`. Descriptor pointers refer to existing instance spans and remain
valid for the loaded game, including across a power reset. The adapter clears the map at unload and deinit, then
registers fresh descriptors for the next game. A frontend that declines the optional map can still use
`RETRO_MEMORY_SYSTEM_RAM`. Unmapped holes remain outside a descriptor.

The loader writes the text segment at its physical address and a 16-byte launch block at `0200h` of RAM: the entry
point, the global pointer, the stack address and the stack size as four little-endian words. It writes nothing else.

| Status | The file |
|--------|----------|
| `SCHWAY_STATUS_ERR_EXE_INVALID` | is shorter than the 0800h-byte header, lacks the `PS-X EXE` signature, has a text size that is not a multiple of 0800h or passes the end of the file, has a text address or entry point that is not a multiple of 4, or has an entry point of 0 |
| `SCHWAY_STATUS_ERR_EXE_RANGE` | is longer than the header plus 2 MiB, puts its text in KSEG2 or past the end of RAM, or gives a non-zero stack address whose stack leaves RAM |

An empty text is accepted. A stack address of 0 is accepted with any size.

`schway_exe_info` holds the header's words (`pc0`, `gp0`, `t_addr`, `t_size`, `d_addr`, `d_size`, `b_addr`, `b_size`,
`s_addr` and `s_size`), `hash_len` (`t_size` plus 0800h, the bytes a disc database hashes), `licence_area` and
`lint`:

- `licence_area` compares the text at 4Ch with the three region strings byte for byte: `SCHWAY_LICENCE_JAPAN` 1,
  `SCHWAY_LICENCE_EUROPE` 2 or `SCHWAY_LICENCE_NORTH_AMERICA` 3, and `SCHWAY_LICENCE_NONE` 0 when none matches.
- `lint` is a set of notes, never a refusal: `SCHWAY_EXE_LINT_LOW_64K` (the text starts in the kernel's low 64 KiB)
  and `SCHWAY_EXE_LINT_TRAILING` (bytes follow the text).

## Loading a BIOS

`schway_load_bios` accepts exactly `SCHWAY_BIOS_SIZE` bytes (524,288). It copies them into the instance before guest
execution starts, so the caller may change or release its input buffer after the call. Any exact-size image is
loadable, including an unknown image or OpenBIOS; a file name does not establish identity. A null pointer, wrong
size, or load after a PS-EXE or guest execution returns `SCHWAY_STATUS_ERR_ARGUMENT` without changing the current
BIOS. A power reset preserves the loaded image and starts the CPU again at `BFC00000h`.

`schway_get_bios_info` reports the copied image's SHA-256 in `sha256`. When a host image is loaded, `flags` contains
`SCHWAY_BIOS_INFO_LOADED`; it also contains `SCHWAY_BIOS_INFO_RECOGNIZED` when the image matches a pinned retail row.
`status` is `SCHWAY_BIOS_STATUS_UNKNOWN` or `SCHWAY_BIOS_STATUS_RECOGNIZED`, and `region` is the matched
`SCHWAY_REGION_*`, `SCHWAY_BIOS_REGION_ANY` for a region-free row, or `SCHWAY_BIOS_REGION_UNKNOWN`. With no host image,
all flags, status, region and digest bytes are zero; the built-in PS-EXE stub is not reported as a loaded BIOS.

The reported SHA-256 is computed from the exact loaded image. Retail recognition separately matches the actual MD5
facts in the pinned libretro `System.dat` rows in [ASSET_POLICY.md](../ASSET_POLICY.md); the associated SHA-1 values
are retained as source metadata. The core does not infer SHA-256 from either DAT digest. Unknown images and OpenBIOS
remain loadable and report unknown recognition.

The host selects and reads the image, then supplies its bytes to `schway_load_bios`; the core opens no BIOS path.
OpenBIOS is used as a checked CI handoff and is not included in the library, runner, libretro core or release
archives. Loading it therefore leaves the retail-recognition flag clear and reports unknown status, regardless of
the source file's name.

The original 40-byte `schway_bios_info` prefix remains valid for callers built before status and region were added.
When such a caller sets `size` to 40, the core writes through `sha256` and leaves the appended fields untouched.

BIOS bytes are not included in ordinary save states. A state stores the loaded-image flag and SHA-256 identity, and
`schway_load_state` refuses a state made with a different BIOS before changing the instance.

## Profile flags

`schway_profile.flags` is the second profile field, after `region`. A caller whose struct ends before it gets no
flags. A bit this version does not define returns `SCHWAY_STATUS_ERR_PROFILE_REJECTED`. The flags survive a power
reset and lock with the region.

- `SCHWAY_PROFILE_DEBUG_PORT`: the guest's byte writes to the two debug ports (a TTY byte and an exit code) take effect.
  Retail hosts leave it clear, and then those writes do nothing.
- `SCHWAY_PROFILE_GPU_V0`: select the early GPU v0 model, which lacks rectangle texture flips. The default is GPU v2.

## Video

`schway_get_video` describes the current frame. `pixels` points into the instance and stays valid until the next
`schway_run_frame`, `schway_load_state`, `schway_reset` or `schway_destroy`; copy what you need before then. The
native field height is `Y2 - Y1`, including interlace. Rows are tightly packed and `pitch_bytes` is their byte
stride. In 15-bit mode, each pixel is a host-order `uint16_t` word `0BBBBBGGGGGRRRRR`, with bit 15 the mask bit,
exactly as the console holds it. In packed 24-bit mode, each pixel is three bytes in RGB order. A disabled display
returns zero pixels and `enabled == 0`. A malformed or out-of-range GP1 display window returns an empty frame
(`width == 0`, `height == 0`, `pitch_bytes == 0`) so a host can reject it before reading pixels. Valid display
windows wrap around VRAM edges. To show a 15-bit pixel, drop bit 15 and widen each 5-bit channel `c` to 8 bits as
`(c << 3) | (c >> 2)`. The other fields (`hres`, `vres`, `dot_div`, `interlaced`, `field`, `x1`, `y1`, `start_x`,
`start_y`, `enabled`, `rgb24`) report the display mode the frame was made in.

## VRAM

`schway_get_vram` returns the instance's complete 1024 x 512 VRAM as a read-only `schway_span`. Its `data` points
to 16-bit words in host byte order and `size` is the span's byte length. The pointer stays stable for the lifetime
of the instance; its contents can change as the emulated GPU runs, when a state is loaded or when the instance is
reset. Copy data you need before destroying the instance.

## Memory spans and cards

`schway_get_memory` exposes read-only spans selected by `SCHWAY_MEMORY_RAM`, `SCHWAY_MEMORY_SCRATCHPAD`,
`SCHWAY_MEMORY_VRAM`, `SCHWAY_MEMORY_BIOS`, `SCHWAY_MEMORY_CARD1` or `SCHWAY_MEMORY_CARD2`. It returns the
instance's 2 MiB RAM, 1 KiB scratchpad, 1 MiB VRAM, current BIOS image or one of two 131,072-byte card images.
RAM, scratchpad, VRAM and card pointers remain stable for the instance lifetime. The BIOS span initially names the
built-in PS-EXE stub; after a host BIOS is loaded it names the instance's copied image and stays stable through
resets. RAM, scratchpad and VRAM contents change as the core runs; card pointers and bytes remain stable across
reads, state loads and power resets. `schway_get_vram` remains a convenience accessor for the VRAM span.

Cards start as formatted blank images. Before execution, a host can import a raw image up to 131,072 bytes.
Short images are padded with `FFh` through the end of the card, as required for an unallocated raw card area:

```c
uint64_t generation = 0;
schway_load_memcard(inst, SCHWAY_CARD_SLOT_1, raw_card, raw_card_size);
schway_memcard_generation(inst, SCHWAY_CARD_SLOT_1, &generation);
```

The one-based slots are `SCHWAY_CARD_SLOT_1` and `SCHWAY_CARD_SLOT_2`; each image has `SCHWAY_MEMCARD_SIZE`
bytes. The core accepts raw bytes only; the host owns file access and any wrapper decoding. A null pointer, invalid
slot, oversized image or import after execution returns `SCHWAY_STATUS_ERR_ARGUMENT` without changing the current
card or its generation. Successful imports keep the span address stable and reset that slot's generation to zero.

Ordinary save states include the pending SIO/card protocol but exclude card bytes and generations. Before applying
a state, the core validates card phase and address progression, checks checksum consistency where the saved fields
allow it, and verifies the staged write payload. A refused state leaves the instance untouched. Loading a state
therefore never rewinds persistent card data, including after a completed write. A completed successful `47h`
write updates one sector and increments that slot's generation once; reads, rejected writes and interrupted writes
do not increment it.

## Audio

```c
int16_t buf[2 * 4096];
schway_audio audio;
memset(&audio, 0, sizeof audio);
audio.size = (uint32_t)sizeof audio;
audio.samples = buf;
audio.capacity = 4096; /* stereo pairs */
schway_get_audio(inst, &audio); /* audio.written pairs, left then right */
```

Each frame queues one stereo pair for every 768 CPU cycles it ran: 737 or 738 pairs for NTSC, 886 or 887 for PAL,
at 44,100 pairs per second. The queue holds 3,548 pairs, four of the longest frames; pairs that do not fit are
dropped, and `dropped` reports how many since the last drain. Drain every frame. A drain with `capacity` 0 writes
nothing and leaves the pairs queued; `samples` may be `NULL` only then. This release's audio is silent.

## TTY

`schway_get_tty` drains the text the guest wrote. Set `size`, point `bytes` at room for `capacity` bytes and call it:

```c
uint8_t text[256];
schway_tty tty;
memset(&tty, 0, sizeof tty);
tty.size = (uint32_t)sizeof tty;
tty.bytes = text;
tty.capacity = sizeof text;
schway_get_tty(inst, &tty); /* tty.written bytes, oldest first */
```

A guest has two ways to write a byte, and both need `SCHWAY_PROFILE_DEBUG_PORT`: a byte store to the debug port at
`1F802080h`, and the putchar kernel call, `A(3Ch)` or `B(3Dh)`, which the stub kernel turns into the same store. A
profile without the flag decodes neither, and the ring stays empty.

The bytes queue in a ring of 4,096 in the order written. A drain returns up to `capacity` of them, oldest first, and
leaves the rest queued; `written` is how many came back. A byte written to a full ring is dropped, and `dropped`
reports how many were lost since the last drain and then starts again from zero. Drain every frame, as for audio. A
drain with `capacity` 0 writes nothing and reports and clears the count; `bytes` may be `NULL` only then. The bytes
are not text-decoded or line-split, and the core never prints them.

The ring is not part of the machine state: it is outside `schway_save_state`, `schway_state_visit` and every hash, a
load leaves it as it is, and a power reset empties it.

## Save states

```c
size_t n = schway_state_size(inst);
void *buf = malloc(n);
schway_save_state(inst, buf, n);
/* ... */
schway_load_state(inst, buf, n);
```

A state is the same bytes on every platform: a 20-byte header (magic, state format, total size, CRC-32 of the
rest) and then every field, little-endian. `schway_save_state` returns `SCHWAY_STATUS_ERR_BUFFER_TOO_SMALL` if
`size` is below `schway_state_size`. `schway_load_state` checks the whole state before it changes anything:

| Status | Meaning |
|--------|---------|
| `SCHWAY_STATUS_ERR_STATE_MAGIC` | not a state |
| `SCHWAY_STATUS_ERR_STATE_VERSION` | another state format |
| `SCHWAY_STATUS_ERR_STATE_CRC` | the bytes were changed |
| `SCHWAY_STATUS_ERR_STATE_FIELD` | wrong size, a field out of range, or another region's state |

A load empties the audio queue. Saving, loading and saving again gives the same bytes.

The bus state includes the exact 16-bit SPU ATTR value written at `1F801DAAh`; loading a state restores that value.
The emulator's power reset clears this latch to its deterministic zero model. This narrow control readback does not
model SPU audio, transfer side effects or STATX timing; see [the bus model](hardware/bus.md#spu-control-readback).

`schway_state_visit` calls your `schway_state_visitor` once per field with its name, its values in host order, the number of
values and the width of each (1, 2, 4 or 8 bytes). The names are static strings you may keep, such as
`instance.frame` and `gpu.vram`. Writing each visited value little-endian, in visit order, gives exactly the bytes
a save writes after its header.

## Statuses

Every function that can fail returns a `schway_status`; `SCHWAY_STATUS_OK` is 0. `SCHWAY_STATUS_ERR_EXE_INVALID` (11)
and `SCHWAY_STATUS_ERR_EXE_RANGE` (12) come only from the PS-EXE calls. A `NULL` instance or required
pointer returns `SCHWAY_STATUS_ERR_ARGUMENT`. `schway_status_string` gives a short English text for any value,
an unknown one included, and never `NULL`.

## Versions

`schway_get_versions` reports the library version, `api` (`SCHWAY_API_VERSION` of the header the library was
built with), `abi`, `behaviour_revision` (raised when the same inputs give different frames, audio or states) and
`state_format`.

`SCHWAY_API_VERSION` is 4. A version only adds names, so a program built against an earlier header runs unchanged.
Version 3 added `SCHWAY_FAULT_DMA`; version 4 adds read-only `schway_span` memory views, card loading and card
generation queries, plus copied BIOS loading and SHA-256 reporting. A program that switches on the fault kind should
treat a value it does not know as a fault it cannot name.

## The test card

Until a BIOS boots, every frame shows a built-in 320 x 240 card that changes every frame. Each band catches one
kind of host mistake:

- **Corner block**, white, 8 x 8 at the top left only: a mirror, flip or transpose moves it.
- **Border**, one white dot on every edge: a crop, an offset or a wrong size cuts or shifts it.
- **Row-ID strip**, x 1 to 8: each row carries its own row number as the colour word, so a wrong pitch or a
  skipped or repeated row shows.
- **Ramps and grey**, ten dots per level, all 32 levels: red, green, blue, then grey. Swapped channels or a lost
  bit of depth shows. Red across the top and green across the bottom tell an R/B swap from a vertical flip.
- **Mask twin**, under the grey: the same grey with bit 15 set. Shown, it looks the same as the grey, since a
  display drops bit 15; in the native words and the frame hash it differs, so a host that keeps the words
  verbatim keeps it.
- **Checker**, one-dot black and white: scaling or filtering blurs it to grey.
- **Counter strip**, rows 193 to 208: the frame number since power-on, low 32 bits.

To read the counter, start at x 32: there are 32 cells of 8 dots, the most significant bit on the left, white for
1 and black for 0; the strip is mid-grey outside x 32 to 287. Frame 5 shows white in the last cell and the third
from last. The first frame after power-on shows 1.
