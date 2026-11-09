# Embedding the library

The library is one static archive and one header, `schwaystation.h`, written in C17. It allocates memory only
through the allocator you pass, keeps all state in the instance, starts no threads, opens no files, reads no clock
and uses no floating point. It needs nothing from the C library beyond `memcpy`, `memmove`, `memset` and `memcmp`.
Two instances in one process share nothing, so a host may run several side by side.

This release shows the built-in test card: it loads no BIOS or disc yet. A PS-EXE can be loaded into RAM (see
"Loading a PS-EXE"), but nothing runs it yet.

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

**Input structs** (`schway_config`, `schway_profile`, `schway_budget`, `schway_audio`):

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
- `schway_run_frame` runs to the end of the current frame and returns. The optional `schway_run_result` gets the stop reason
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

`schway_run` advances until the end of the current frame or until `max_cycles` CPU cycles have run, whichever comes
first, and `stop` in the result says which:

| Stop | Value | Meaning |
|------|-------|---------|
| `SCHWAY_STOP_FRAME` | 1 | the frame ended; a budget that ends exactly on the boundary reports this |
| `SCHWAY_STOP_BUDGET` | 2 | `max_cycles` ran out inside the frame |
| `SCHWAY_STOP_HOOK` | 4 | the guest wrote the exit port (profile flag `SCHWAY_PROFILE_DEBUG_PORT` set); `exit_code` holds the byte |
| `SCHWAY_STOP_FAULT` | 5 | `SCHWAY_RUN_STOP_ON_FAULT` is set and a fault is latched, or the guest double-faulted at an exception vector |

Values 3 and 6 are not used. A call never runs past the end of the frame, so to run N cycles call again until the
result's `ticks` has moved N. With a program loaded a run stops at the first instruction boundary at or after the
budget or the frame's end, so `ticks` can pass either by the rest of one instruction (see
[the CPU page](hardware/cpu.md)).

- `max_cycles` 0 returns `SCHWAY_STOP_BUDGET` and changes nothing, the profile lock included.
- `flags` takes `SCHWAY_RUN_STOP_ON_FAULT` or 0; any other bit returns `SCHWAY_STATUS_ERR_ARGUMENT`. With it set, a
  latched fault stops the run with `SCHWAY_STOP_FAULT` before any cycle runs.
- `schway_run_frame` is the same advance with no limit. After a run that stopped inside a frame it finishes that
  frame.
- A stopped run resumes where it stopped. With no program loaded, running N cycles and then M leaves exactly what
  N + M cycles leave: the same state bytes, frame, ticks and audio pairs. With one loaded the same holds where N is
  an instruction boundary the one-shot run passes through, for instance where a run of the same budget stopped.
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

| Device | Value | Device | Value | Device | Value |
|--------|-------|--------|-------|--------|-------|
| `SCHWAY_DEVICE_NONE` | 0 | `SCHWAY_DEVICE_IRQ` | 5 | `SCHWAY_DEVICE_SIO` | 11 |
| `SCHWAY_DEVICE_RAM` | 1 | `SCHWAY_DEVICE_DMA` | 6 | `SCHWAY_DEVICE_SPU` | 12 |
| `SCHWAY_DEVICE_SCRATCH` | 2 | `SCHWAY_DEVICE_TIMER` | 7 | `SCHWAY_DEVICE_EXP1` | 13 |
| `SCHWAY_DEVICE_BIOS` | 3 | `SCHWAY_DEVICE_CDROM` | 8 | `SCHWAY_DEVICE_EXP2` | 14 |
| `SCHWAY_DEVICE_MEMCTL` | 4 | `SCHWAY_DEVICE_GPU` | 9 | `SCHWAY_DEVICE_EXP3` | 15 |
| | | `SCHWAY_DEVICE_MDEC` | 10 | `SCHWAY_DEVICE_GTE` | 16 |
| | | | | `SCHWAY_DEVICE_UNMAPPED` | 17 |

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
before the first frame: a call after a frame has run returns `SCHWAY_STATUS_ERR_ARGUMENT`, and a power reset erases
what was loaded, so load again after one. A refused file changes nothing. A `NULL` pointer or `n` of 0 returns
`SCHWAY_STATUS_ERR_ARGUMENT`.

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

## Profile flags

`schway_profile.flags` is the second profile field, after `region`. A caller whose struct ends before it gets no
flags. A bit this version does not define returns `SCHWAY_STATUS_ERR_PROFILE_REJECTED`. The flags survive a power
reset and lock with the region.

- `SCHWAY_PROFILE_DEBUG_PORT`: the guest's byte writes to the two debug ports (a TTY byte and an exit code) take effect.
  Retail hosts leave it clear, and then those writes do nothing.

## Video

`schway_get_video` describes the current frame. `pixels` points into the instance and stays valid until the next
`schway_run_frame`, `schway_load_state`, `schway_reset` or `schway_destroy`; copy what you need before then. The
frame is `width` by `height` 16-bit words in host byte order, rows `pitch_bytes` apart. Copy `width * 2` bytes per
row; the pitch is wider than a row, so never copy the frame as one block.

Each word is `0BBBBBGGGGGRRRRR` with bit 15 the mask bit, exactly as the console holds it. To show it, drop bit 15
and widen each 5-bit channel `c` to 8 bits as `(c << 3) | (c >> 2)`. The other fields (`hres`, `vres`, `dot_div`,
`interlaced`, `field`, `x1`, `y1`, `start_x`, `start_y`, `enabled`, `rgb24`) report the display mode the frame was
made in.

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
