# Embedding the library

The library is one static archive and one header, `schwaystation.h`, written in C17. It allocates memory only
through the allocator you pass, keeps all state in the instance, starts no threads, opens no files, reads no clock
and uses no floating point. It needs nothing from the C library beyond `memcpy`, `memmove`, `memset` and `memcmp`.
Two instances in one process share nothing, so a host may run several side by side.

This release runs the built-in test card only: it loads no BIOS or disc yet.

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

**Input structs** (`schway_config`, `schway_profile`, `schway_audio`):

- A size below the struct's first released size returns `SCHWAY_STATUS_ERR_STRUCT_SIZE`.
- A larger size from a newer header is accepted only if every byte past the library's struct is zero; a field
  added later keeps the old behaviour when it is zero.
- A size above 4096 bytes is refused whatever the bytes hold, so a garbage tag never makes the library read far
  past your object.

**Output structs** (`schway_versions`, `schway_info`, `schway_run_result`, `schway_video`):

- A size below the struct's first released size returns `SCHWAY_STATUS_ERR_STRUCT_SIZE` and leaves the struct as
  it was.
- Any larger size is accepted: the library writes the smaller of your size and its own, keeps your `size` value
  and leaves every byte past its own struct as you set it.

A call that returns an error changes nothing: not the instance, not your struct.

## The allocator

```c
static void *my_alloc(void *user, size_t size) { (void)user; return malloc(size); }
static void my_free(void *user, void *ptr, size_t size) { (void)user; (void)size; free(ptr); }

cfg.allocator.alloc = my_alloc;
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
- `schway_set_profile` picks the region (`SCHWAY_REGION_NTSC_J`, `NTSC_U` or `PAL`); any other value returns
  `SCHWAY_STATUS_ERR_PROFILE_REJECTED`. It works only before the first `schway_run_frame`; after that it returns
  `SCHWAY_STATUS_ERR_PROFILE_LOCKED`, even after a reset. To change region, destroy and create again.
- `schway_run_frame` runs one frame and returns. The optional `schway_run_result` gets the stop reason
  (`SCHWAY_STOP_FRAME`), the frame number since power-on and the CPU cycles run since power-on.
  If the next frame would exceed the save format's inclusive limits (2^62 ticks or
  floor(2^62 / 566,203) frames), it returns `SCHWAY_STATUS_ERR_STATE_FIELD` without changing the instance
  or result. Save the current state or use a power reset to restart the counters.
- `schway_reset(inst, SCHWAY_RESET_POWER)` returns to frame 0 with the same region; other kinds return
  `SCHWAY_STATUS_ERR_ARGUMENT`.
- `schway_get_info` gives the display size, the frame rate as a fraction (53,693,175 / 897,619 for NTSC,
  53,203,425 / 1,069,484 for PAL), the sample rate (44,100 / 1), the most stereo pairs one frame can make (738 or
  887) and the region.

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

`schway_state_visit` calls your visitor once per field with its name, its values in host order, the number of
values and the width of each (1, 2, 4 or 8 bytes). The names are static strings you may keep, such as
`instance.frame` and `gpu.vram`. Writing each visited value little-endian, in visit order, gives exactly the bytes
a save writes after its header.

## Statuses

Every function that can fail returns a `schway_status`; `SCHWAY_STATUS_OK` is 0. A `NULL` instance or required
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
