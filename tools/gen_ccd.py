#!/usr/bin/env python3
"""Build a bounded CloneCD image, descriptor, and independent SUB fixture."""

import argparse
import hashlib
import pathlib
import re

SECTOR_BYTES = 2352
SUB_BYTES = 96


def digest(data):
    return hashlib.sha256(data).hexdigest()


def write_text_lf(path, text):
    with path.open("w", encoding="ascii", newline="\n") as output:
        output.write(text)


def msf(lba):
    frames = lba + 150
    return frames // 4500, (frames // 75) % 60, frames % 75


def parse_cue(path):
    text = path.read_text(encoding="ascii")
    if "\r" in text or "\0" in text:
        raise ValueError("source CUE must use plain LF text")
    files = []
    tracks = []
    current = None
    for line in text.splitlines():
        file_match = re.fullmatch(r'FILE "([^"]+)" BINARY', line)
        track_match = re.fullmatch(r"  TRACK ([0-9]{2}) (MODE2/2352|AUDIO)", line)
        index_match = re.fullmatch(r"    INDEX ([0-9]{2}) ([0-9]{2}):([0-9]{2}):([0-9]{2})", line)
        if file_match:
            name = file_match.group(1)
            if pathlib.PurePosixPath(name).name != name or name in (".", ".."):
                raise ValueError("source CUE has an unsafe companion path")
            files.append(name)
        elif track_match:
            current = {"number": int(track_match.group(1)), "mode": track_match.group(2), "indexes": {}}
            tracks.append(current)
        elif index_match and current is not None:
            index = int(index_match.group(1))
            minute, second, frame = (int(index_match.group(i)) for i in range(2, 5))
            if second >= 60 or frame >= 75:
                raise ValueError("source CUE has invalid MSF geometry")
            current["indexes"][index] = (minute * 60 + second) * 75 + frame
        elif line:
            raise ValueError("source CUE has an unsupported line")
    if len(files) != 2 or len(tracks) != 2:
        raise ValueError("mixed source CUE must have exactly two tracks and companions")
    if tracks[0]["number"] != 1 or tracks[0]["mode"] != "MODE2/2352" or tracks[0]["indexes"] != {1: 0}:
        raise ValueError("first source track geometry is not the owned Mode 2 track")
    if tracks[1]["number"] != 2 or tracks[1]["mode"] != "AUDIO" or tracks[1]["indexes"] != {0: 0, 1: 150}:
        raise ValueError("second source track geometry is not the owned audio pregap")
    return files, tracks


def entry(number, point, control, track, pmin, psec, pframe, plba):
    return (
        f"[Entry {number}]\nSession=1\nPoint=0x{point:02x}\nADR=0x01\nControl=0x{control:02x}\n"
        f"TrackNo={track}\nAMin=0\nASec=0\nAFrame=0\nALBA=-150\nZero=0\n"
        f"PMin={pmin}\nPSec={psec}\nPFrame={pframe}\nPLBA={plba}\n"
    )


def descriptor(track1_sectors, track2_sectors):
    index0 = track1_sectors
    index1 = index0 + 150
    leadout = index0 + track2_sectors
    lead_min, lead_sec, lead_frame = msf(leadout)
    t2_min, t2_sec, t2_frame = msf(index1)
    return (
        "[CloneCD]\nVersion=3\n"
        "[Disc]\nTocEntries=5\nSessions=1\nDataTracksScrambled=0\nCDTextLength=0\n"
        "[Session 1]\nPreGapMode=0\nPreGapSubC=0\n"
        + entry(0, 0xa0, 4, 0, 1, 32, 0, 6750)
        + entry(1, 0xa1, 4, 0, 2, 0, 0, 8850)
        + entry(2, 0xa2, 4, 0, lead_min, lead_sec, lead_frame, leadout)
        + entry(3, 1, 4, 1, 0, 2, 0, 0)
        + entry(4, 2, 0, 2, t2_min, t2_sec, t2_frame, index1)
        + f"[TRACK 1]\nMODE=2\nINDEX 1=0\n"
        + f"[TRACK 2]\nMODE=0\nINDEX 0={index0}\nINDEX 1={index1}\n"
    )


def image_cue(track1_sectors, track2_sectors):
    index0 = track1_sectors
    index1 = index0 + 150
    leadout = index0 + track2_sectors
    def stamp(lba):
        frames = lba
        return f"{frames // 4500:02}:{(frames // 75) % 60:02}:{frames % 75:02}"
    return (
        'FILE "disc_boot_legacy.img" BINARY\n'
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n"
        "  TRACK 02 AUDIO\n"
        f"    INDEX 00 {stamp(index0)}\n"
        f"    INDEX 01 {stamp(index1)}\n"
    )


def make_sub(sectors):
    output = bytearray()
    for lba in range(sectors):
        for channel in range(8):
            for offset in range(12):
                output.append((0x35 + lba * 13 + channel * 29 + offset * 7) & 0xff)
    return bytes(output)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-cue", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", type=pathlib.Path, required=True)
    args = parser.parse_args()
    files, tracks = parse_cue(args.source_cue)
    pieces = [(args.source_cue.parent / name).read_bytes() for name in files]
    if any(not data or len(data) % SECTOR_BYTES for data in pieces):
        raise ValueError("each source track must contain complete raw sectors")
    counts = [len(data) // SECTOR_BYTES for data in pieces]
    if counts[0] != 64 or counts[1] <= 150 or tracks[1]["indexes"][1] != 150:
        raise ValueError("source tracks do not match the bounded owned fixture geometry")
    image = b"".join(pieces)
    sub = make_sub(len(image) // SECTOR_BYTES)
    ccd = descriptor(*counts).encode("ascii")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    image_path = args.output_dir / "disc_boot_legacy.img"
    ccd_path = args.output_dir / "disc_boot_legacy.ccd"
    sub_path = args.output_dir / "disc_boot_legacy.sub"
    image_cue_path = args.output_dir / "disc_boot_legacy_image.cue"
    image_path.write_bytes(image)
    ccd_path.write_bytes(ccd)
    sub_path.write_bytes(sub)
    write_text_lf(image_cue_path, image_cue(*counts))
    generator = pathlib.Path(__file__).read_bytes()
    source_cue = args.source_cue.read_bytes()
    fields = [
        "format=schwaystation-legacy-ccd-v1",
        f"source_cue_sha256={digest(source_cue)}",
        f"source_track1={files[0]}", f"source_track1_bytes={len(pieces[0])}",
        f"source_track1_sectors={counts[0]}", f"source_track1_sha256={digest(pieces[0])}",
        f"source_track2={files[1]}", f"source_track2_bytes={len(pieces[1])}",
        f"source_track2_sectors={counts[1]}", f"source_track2_sha256={digest(pieces[1])}",
        f"source_sectors={len(image) // SECTOR_BYTES}", f"source_bytes={len(image)}",
        f"generator_sha256={digest(generator)}",
        f"img_bytes={len(image)}", f"img_sha256={digest(image)}",
        f"ccd_bytes={len(ccd)}", f"ccd_sha256={digest(ccd)}",
        f"sub_bytes={len(sub)}", f"sub_sha256={digest(sub)}",
        f"image_cue_bytes={image_cue_path.stat().st_size}",
        f"image_cue_sha256={digest(image_cue_path.read_bytes())}",
        f"guest_reads={len(image) // SECTOR_BYTES}",
        f"guest_outputs={len(image) // SECTOR_BYTES}",
        "",
    ]
    write_text_lf(args.output_dir / "disc_boot_legacy_ccd.manifest", "\n".join(fields))


if __name__ == "__main__":
    main()
