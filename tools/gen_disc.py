#!/usr/bin/env python3
"""Create small, deterministic first-party PS1 disc media inputs."""

import argparse
import hashlib
import pathlib
import re
import struct
import subprocess
import wave
import xml.etree.ElementTree as ET


def write_wave(path, samples, frequencies):
    rate = 44100
    with wave.open(str(path), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(rate)
        frames = bytearray()
        for index in range(samples):
            for frequency in frequencies:
                phase = index * frequency % rate
                triangle = (phase * 4 // rate - 1) if phase < rate // 2 else (3 - phase * 4 // rate)
                value = triangle * 5000
                frames.extend(struct.pack("<h", value))
        output.writeframes(frames)


def write_ppm(path):
    width = height = 16
    pixels = bytearray()
    for y in range(height):
        for x in range(width):
            pixels.extend(((x * 17) & 255, (y * 17) & 255, ((x + y) * 9) & 255))
    path.write_bytes(f"P6\n{width} {height}\n255\n".encode("ascii") + pixels)


def encode(encoder, output):
    xa_input = output / "xa.wav"
    ppm_input = output / "frame.ppm"
    write_wave(xa_input, 8820, (330, 660))
    write_wave(output / "cdda.wav", 4410, (440, 440))
    write_ppm(ppm_input)
    subprocess.run([encoder, "-q", "-t", "xa", "-f", "37800", "-c", "2", "-b", "4",
                    "-F", "7", "-C", "3", str(xa_input), str(output / "xa.xa")], check=True)
    subprocess.run([encoder, "-q", "-t", "strv", "-v", "v2", "-s", "16x16",
                    str(ppm_input), str(output / "movie.str")], check=True)
    xa_input.unlink()
    ppm_input.unlink()
    files = ("cdda.wav", "movie.str", "xa.xa")
    rows = [f"{hashlib.sha256((output / name).read_bytes()).hexdigest()}  {name}" for name in files]
    (output / "media.sha256").write_text("\n".join(rows) + "\n", encoding="ascii", newline="\n")


def xml_pregap_sectors(xml_path):
    root = ET.parse(xml_path).getroot()
    audio_tracks = [track for track in root.findall("track") if track.get("type") == "audio"]
    if len(audio_tracks) != 1:
        raise ValueError("disc XML must contain exactly one audio track")
    pregaps = audio_tracks[0].findall("track_pregap")
    if len(pregaps) != 1:
        raise ValueError("disc XML must contain exactly one audio pregap")
    duration = pregaps[0].get("duration", "")
    match = re.fullmatch(r"(\d{2}):(\d{2}):(\d{2})", duration)
    if match is None:
        raise ValueError("disc XML audio pregap must use MM:SS:FF")
    minute, second, frame = (int(part) for part in match.groups())
    if second >= 60 or frame >= 75:
        raise ValueError("disc XML audio pregap is outside the MSF range")
    sectors = (minute * 60 + second) * 75 + frame
    if sectors <= 0:
        raise ValueError("disc XML audio pregap must be nonzero")
    return sectors


def split_disc(image, cue, cdda, xml_path):
    raw = image.read_bytes()
    if len(raw) % 2352:
        raise ValueError("mkpsxiso image is not a whole number of raw CD sectors")
    cue_text = cue.read_text(encoding="ascii")
    matches = re.findall(r"TRACK\s+02\s+AUDIO\s+INDEX\s+00\s+(\d{2}:\d{2}:\d{2})", cue_text)
    if len(matches) != 1:
        raise ValueError("mkpsxiso cue has no unique audio INDEX 00")
    minute, second, frame = (int(part) for part in matches[0].split(":"))
    data_sectors = (minute * 60 + second) * 75 + frame
    cdda_sectors = (cdda.stat().st_size + 2351) // 2352
    total_sectors = len(raw) // 2352
    generated_pregap_sectors = total_sectors - data_sectors - cdda_sectors
    expected_pregap_sectors = xml_pregap_sectors(xml_path)
    if (data_sectors < 18 or generated_pregap_sectors <= 0 or
            generated_pregap_sectors > expected_pregap_sectors or cdda_sectors == 0):
        raise ValueError("disc track lengths disagree with the authored audio pregap")

    data_track = bytearray(raw[:data_sectors * 2352])
    pvd_offset = 16 * 2352 + 24
    if data_track[pvd_offset] != 1 or data_track[pvd_offset + 1:pvd_offset + 6] != b"CD001":
        raise ValueError("mkpsxiso output has no ISO9660 primary volume descriptor")
    volume_size = data_sectors.to_bytes(4, "little") + data_sectors.to_bytes(4, "big")
    data_track[pvd_offset + 80:pvd_offset + 88] = volume_size
    image.write_bytes(data_track)
    track02 = image.parent / f"{image.stem}_track02.bin"
    source_track02 = raw[data_sectors * 2352:]
    source_pregap_bytes = generated_pregap_sectors * 2352
    added_pregap = b"\0" * ((expected_pregap_sectors - generated_pregap_sectors) * 2352)
    track02.write_bytes(source_track02[:source_pregap_bytes] + added_pregap +
                        source_track02[source_pregap_bytes:])
    cue.write_text(
        f'FILE "{image.name}" BINARY\n'
        "  TRACK 01 MODE2/2352\n"
        "    INDEX 01 00:00:00\n"
        f'FILE "{track02.name}" BINARY\n'
        "  TRACK 02 AUDIO\n"
        "    INDEX 00 00:00:00\n"
        f"    INDEX 01 {expected_pregap_sectors // 4500:02d}:"
        f"{expected_pregap_sectors % 4500 // 75:02d}:{expected_pregap_sectors % 75:02d}\n",
        encoding="ascii",
        newline="\n",
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=pathlib.Path)
    parser.add_argument("--encoder")
    parser.add_argument("--split-image", type=pathlib.Path)
    parser.add_argument("--cue", type=pathlib.Path)
    parser.add_argument("--cd-da", type=pathlib.Path)
    parser.add_argument("--xml", type=pathlib.Path)
    args = parser.parse_args()
    if args.split_image is not None:
        if (args.cue is None or args.cd_da is None or args.output_dir is not None or
                args.encoder is not None or args.xml is None):
            parser.error("--split-image requires --cue, --cd-da and --xml")
        split_disc(args.split_image, args.cue, args.cd_da, args.xml)
    else:
        if (args.output_dir is None or args.encoder is None or args.cue is not None or
                args.cd_da is not None or args.xml is not None):
            parser.error("--output-dir and --encoder are required for media generation")
        args.output_dir.mkdir(parents=True, exist_ok=True)
        encode(args.encoder, args.output_dir)


if __name__ == "__main__":
    main()
