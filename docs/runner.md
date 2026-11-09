# schwaystation-run

`schwaystation-run` is the headless runner. It runs the emulator for a fixed number of frames with no window,
sound device or input device. It can print a frame hash, write a PNG and report frame times. Scripts and CI use it
to check that a build produces the expected picture. It can run a PS-EXE file and print the text the program writes,
and it can say what a PS-EXE file is without running it.

This release loads no BIOS and no disc. Without `--exe` every run shows the built-in test card, described in
[embedding.md](embedding.md#the-test-card). `--probe` reads one file and runs nothing.

## Synopsis

```text
schwaystation-run [--region ntsc-u|ntsc-j|pal] --frames N [--hash] [--png N:FILE]
schwaystation-run [--region ntsc-u|ntsc-j|pal] --bench N [--hash] [--png N:FILE]
schwaystation-run [--region ntsc-u|ntsc-j|pal] --exe FILE [--frames N] [--tty] [--stop-on mailbox] [--hash]
schwaystation-run --probe FILE
schwaystation-run --help | --version
```

## Options

| Option | Meaning |
|---|---|
| `--frames N` | Run N frames, N from 1 to 1000000. Give either `--frames` or `--bench`, once. |
| `--bench N` | Run N frames, time each one and print the [bench report](#bench-report). N is from 1 to 1000000. |
| `--exe FILE` | Load the PS-EXE file and run it. Without `--frames` or `--bench` the run is 600 frames; a count given with either option wins. Give it once. See [Running a PS-EXE](#running-a-ps-exe). |
| `--tty` | Print the program's text as [`tty` lines](#tty) and let it end the run through the [exit port](#exit-port). Needs `--exe`. |
| `--stop-on mailbox` | End the run when the program's [mailbox](#mailbox) reports done or fault; a run that reaches the frame cap with neither is a [hang](#mailbox). Needs `--exe`. Give it once. |
| `--hash` | After the last frame, print the SHA-256 of its picture. |
| `--png N:FILE` | After frame N, write its picture to FILE as a PNG. N is from 1 to the frame count. Give it once. |
| `--probe FILE` | Print what the PS-EXE file is, as `key=value` lines, and exit. See [Probe](#probe). Give it alone. |
| `--region NAME` | `ntsc-u` (the default), `ntsc-j` or `pal`: the console's region and its video timing. |
| `--help` | Print the usage text and the exit statuses. Give it alone. |
| `--version` | Print the version and the Git revision the build was configured from. Give it alone. |

Options may come in any order, except `--probe`, which takes no other option. Anything else on the command line is
a usage error and nothing runs; that includes a file name that does not follow `--exe` or `--probe`, `--exe` twice
or with `--probe`, `--tty` or `--stop-on` without `--exe`, `--stop-on` with anything but `mailbox`, and any option this release does not have.

## Examples

```sh
schwaystation-run --frames 60 --hash
schwaystation-run --bench 600
schwaystation-run --frames 60 --png 60:card.png
schwaystation-run --region pal --frames 50 --hash
schwaystation-run --exe game.exe --tty
schwaystation-run --exe game.exe --tty --stop-on mailbox
schwaystation-run --exe game.exe --frames 300 --hash
schwaystation-run --probe game.exe
schwaystation-run --help
schwaystation-run --version
```

`--version` prints one line, the version then the revision, or `unknown` for a build configured outside a Git
checkout:

```text
schwaystation-run 0.0.0 0123456789abcdef0123456789abcdef01234567
```

## Running a PS-EXE

`--exe FILE` reads the file, checks it with the core's PS-EXE validator and loads it, then runs frames as `--frames`
does. The stub kernel the core maps in place of a BIOS starts the program. Without `--frames` or `--bench` the run is
600 frames; a program that is still running at the end of the last frame is not an error, and the status is 0, unless
[`--stop-on mailbox`](#mailbox) was given.

A file that is missing, unreadable, empty or not a valid PS-EXE exits 1 with one line on standard error and runs
nothing, in the form `--probe` uses:

```text
schwaystation-run: game.exe: cannot open the file
```

`--hash`, `--png` and `--bench` work as they do without `--exe`. `--region` sets the region; it does not read one from
the file.

## TTY

`--tty` sets the profile's debug-port flag, which a retail run leaves clear, and after every frame reads the text the
program has written since the last frame. Programs write text two ways: bytes stored to the debug port at 1F802080h,
and the kernel's putchar calls, A(3Ch) and B(3Dh). Both reach the same buffer, in the order they were written.

Each completed line is printed as `tty ` followed by its bytes up to the line feed, which is not printed:

```text
tty hello port
tty hello putchar
```

The bytes are printed as the program wrote them, with no character-set conversion. A line still open when the run
ends is printed at the end, without its line feed. A line longer than 4096 bytes is printed in pieces of 4096. The
buffer holds 4 KiB per frame; a program that writes more in one frame keeps the first 4096 bytes and loses the rest.

## Exit port

With `--tty` a program can end the run by storing one byte to the exit port at 1F802082h. Byte 00h ends the run with
status 0. Any other byte prints one line and ends the run with status 6:

```text
exit code=0x2a
```

The text written before the exit is printed first. `--hash`, `--png` and `--bench` print nothing after an exit
with status 6. Without `--tty` the port is not decoded and the write has no effect.

## Mailbox

A test program reports its result in a 256-byte mailbox at RAM address 10000h, which is where an image loaded at
80010000h starts. The layout, in bytes:

| Offset | Content |
|---|---|
| 00h | the magic `SWMB` |
| 04h | CRC-32 (IEEE) of bytes 10h to FFh, little-endian |
| 08h | status: 80h running, 81h waiting for input, 00h done, FFh fault |
| 09h | protocol version, 1 |
| 0Ah | record count, at most 30 |
| 10h + 8n | record n: id (16 bits), flags (16 bits), value (32 bits), little-endian |

With `--stop-on mailbox` the runner reads the mailbox after every frame, and after a program's exit port report, through
the core's `schway_peek`; the core has no mailbox of its own. Until the magic is there and the status is 00h or
FFh the run goes on. A status of 00h or FFh ends it, and the runner prints one line, then one line per record:

```text
mailbox SWMB status=00 records=2
mailbox 0001=80018000
mailbox 0002=80200000
```

Each record line is the id, four lowercase hex digits, then the value, eight. Status 00h exits 0 when the CRC-32 matches
the bytes it covers. A CRC that does not match prints `mailbox crc mismatch` alone and exits 6. Status FFh, which a
program sets itself and the stub kernel sets when the program takes an exception, prints the same lines and exits 6;
its CRC is not checked, as the kernel does not rewrite it.

A run that reaches the frame cap, `--frames N` or the 600-frame default, with no such status is a hang. The runner
prints the frame count and the program counter, and exits 8:

```text
hang frames=600 pc=0x80010100
```

A program that ends the run through the [exit port](#exit-port) first, or a SIGINT or SIGTERM, is not a hang. As
after an exit port failure, `--hash` and `--bench` print nothing after status 6 or 8.

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
  6  fixture  the fixture reported failure: mailbox status FFh, a mailbox CRC mismatch or an exit port byte not 00h
  8  hang     --stop-on mailbox saw no done or fault status by the frame cap
```

Errors and usage messages go to standard error. Standard output holds only the results described above.
