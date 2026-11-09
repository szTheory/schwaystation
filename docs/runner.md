# schwaystation-run

`schwaystation-run` is the headless runner. It runs the emulator for a fixed number of frames with no window,
sound device or input device. It can print a frame hash, write a PNG and report frame times. Scripts and CI use it
to check that a build produces the expected picture. It can also say what a PS-EXE file is, without running it.

This release loads no BIOS and no disc. Every run shows the built-in test card, described in
[embedding.md](embedding.md#the-test-card). `--probe` reads one file and runs nothing.

## Synopsis

```text
schwaystation-run [--region ntsc-u|ntsc-j|pal] --frames N [--hash] [--png N:FILE]
schwaystation-run [--region ntsc-u|ntsc-j|pal] --bench N [--hash] [--png N:FILE]
schwaystation-run --probe FILE
schwaystation-run --help | --version
```

## Options

| Option | Meaning |
|---|---|
| `--frames N` | Run N frames, N from 1 to 1000000. Give either `--frames` or `--bench`, once. |
| `--bench N` | Run N frames, time each one and print the [bench report](#bench-report). N is from 1 to 1000000. |
| `--hash` | After the last frame, print the SHA-256 of its picture. |
| `--png N:FILE` | After frame N, write its picture to FILE as a PNG. N is from 1 to the frame count. Give it once. |
| `--probe FILE` | Print what the PS-EXE file is, as `key=value` lines, and exit. See [Probe](#probe). Give it alone. |
| `--region NAME` | `ntsc-u` (the default), `ntsc-j` or `pal`: the console's region and its video timing. |
| `--help` | Print the usage text and the exit statuses. Give it alone. |
| `--version` | Print the version and the Git revision the build was configured from. Give it alone. |

Options may come in any order, except `--probe`, which takes no other option. Anything else on the command line is
a usage error and nothing runs; that includes a file name that does not follow `--probe` and any option this
release does not have.

## Examples

```sh
schwaystation-run --frames 60 --hash
schwaystation-run --bench 600
schwaystation-run --frames 60 --png 60:card.png
schwaystation-run --region pal --frames 50 --hash
schwaystation-run --probe game.exe
schwaystation-run --help
schwaystation-run --version
```

`--version` prints one line, the version then the revision, or `unknown` for a build configured outside a Git
checkout:

```text
schwaystation-run 0.0.0 0123456789abcdef0123456789abcdef01234567
```

## Frame hash

`--hash` prints one line after the last frame:

```text
frame 60 sha256 be3538480b252f5bcfe7cb5b876a71c72d3795621b4278e405af61c425c06d71
```

The hash is SHA-256 over the frame's video description, then its pixels. The description is 13 unsigned 32-bit
words, each stored low byte first: field, interlaced, vres, hres, dot_div, region, x1, y1, start_x, start_y,
enabled, rgb24 and bpp. The pixels follow as the displayed rows from top to bottom, each pixel a 16-bit word
stored low byte first, with no padding between rows. The hash is therefore the same on little-endian and
big-endian hosts. The card changes every frame, so the frame number matters.

## PNG output

`--png N:FILE` writes frame N unscaled as an 8-bit RGB PNG. Each pixel goes through the same 15-bit to XRGB8888
conversion the libretro core uses, with the X byte dropped. The runner builds the whole file in memory and writes
it in one call; a file that cannot be written in full is removed and the runner exits 1.

## Bench report

`--bench N` prints nine `key value` lines after the run, one whole number each:

```text
frames 300
p50_us 83
p90_us 86
p99_us 91
p99.9_us 114
p99.99_us 114
p99.999_us 114
max_us 114
max_rss_kib 2496
```

| Key | Value |
|---|---|
| `frames` | the number of frames timed |
| `p50_us` to `p99.999_us` | microseconds per frame at the 50th, 90th, 99th, 99.9th, 99.99th and 99.999th percentile |
| `max_us` | the slowest frame, in microseconds |
| `max_rss_kib` | the runner's peak resident memory, in KiB |

Only the time spent running each frame is measured, not hashing or writing files. The runner reads the host's
monotonic clock; the core never reads a clock.

Each percentile is a nearest-rank sample with no interpolation: with n frames, the p-th percentile is the frame
time at rank ⌈n × p⌉ in ascending order, so every value is a time that was measured. With one frame every line
shows that frame's time, and with fewer than 100,000 frames p99.999 is the maximum. Microseconds are truncated.

`max_rss_kib` is in KiB on every system, rounded up. WASI cannot report memory, so there the line is
`max_rss_kib 0`.

## Probe

`--probe FILE` reads the file, checks it with the core's PS-EXE validator and prints what it is. It makes no
instance, loads nothing and runs no frame. The report is these lines, in this order, each `key=value`:

| Key | Value |
|---|---|
| `format` | `exe` |
| `serial` | `n/a`: a bare executable has no disc serial |
| `licence_area` | the market named by the licence text at offset 4Ch: `japan`, `europe`, `north_america` or `none` |
| `region_guess` | `ntsc-j`, `pal` or `ntsc-u` for the three markets, `unknown` for `none` |
| `exe_name` | the file's name without its directory, any byte outside printable ASCII shown as `?` |
| `exe_size` | the file's size in bytes, in decimal |
| `sha256` | SHA-256 of the whole file, 64 lowercase hex digits |
| `rcheevos_md5` | the MD5 an achievement set is keyed on, 32 lowercase hex digits, see below |
| `pc0`, `gp0` | the entry point and the initial global pointer |
| `t_addr`, `t_size` | where the text loads and how many bytes it has |
| `d_addr`, `d_size`, `b_addr`, `b_size` | the data and BSS fields, printed as the header holds them |
| `s_addr`, `s_size` | the initial stack base and offset, zero when the header gives none |
| `lint` | notes on a valid file, as bits: `0x1` the text lies in the low 64 KiB, `0x2` bytes follow the text |

The ten header values, from `pc0` to `s_size`, are printed as `0x` and eight hex digits, and so is `lint`.

`rcheevos_md5` is the MD5 of the file's first `t_size` + 0800h bytes, which is the header and the text. This is the
rule RetroAchievements applies to a PS-EXE. A disc's hash also begins with the name of its boot file; a bare file
has none, so nothing is added. A file longer than that, as a `lint` of `0x2` reports, hashes as if it were cut there.

The file must be a PS-EXE the core would load: the `PS-X EXE` signature, a header the validator accepts, and a size
from 0800h to 0800h + 2 MiB. Otherwise nothing is printed on standard output and the runner prints one line to
standard error and exits 1:

```text
schwaystation-run: game.exe: PS-EXE invalid
```

The reason is `PS-EXE invalid` or `PS-EXE outside RAM` from the core, or `the file is empty`, `cannot open the
file` or `cannot read the file`. `--probe` with any other option, with no file or with a second file exits 2.

## Stopping

SIGINT (Ctrl+C) or SIGTERM ends the run after the frame in progress. That frame's PNG is still written,
`--hash` prints the last frame that ran, and `--bench` reports the frames that ran. The exit status is 0. A
signal that was already ignored when the runner started, as under `nohup`, stays ignored.

## Exit status

`schwaystation-run --help` prints this table:

```text
  0  done     every frame ran, or SIGINT or SIGTERM ended the run after a whole frame
  1  failure  a file could not be written or is not a valid PS-EXE, memory ran out or the core failed
  2  usage    the command line is invalid, so nothing ran
```

Errors and usage messages go to standard error. Standard output holds only the results described above.
