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

XA_INPUT_FRAMES = 7000
XA_SECTOR_BYTES = 2336
XA_OUTPUT_FRAMES_PER_SECTOR = 2352
CDDA_SECTOR_BYTES = 2352
CDDA_BYTES_PER_FRAME = 4
CDDA_OUTPUT_FRAMES_PER_SECTOR = CDDA_SECTOR_BYTES // CDDA_BYTES_PER_FRAME
CDDA_INPUT_FRAMES = 4410
STR_SECTOR_BYTES = 2048
STR_HEADER_BYTES = 32
STR_PAYLOAD_BYTES = STR_SECTOR_BYTES - STR_HEADER_BYTES
STR_MDEC_INPUT_BYTES = 32 * 4
STR_INPUT_FRAMES = 1


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
            pixels.extend((x * 4, y * 4, (x + y) * 2))
    path.write_bytes(f"P6\n{width} {height}\n255\n".encode("ascii") + pixels)


def cdda_pcm_frames(path):
    with wave.open(str(path), "rb") as cdda:
        if cdda.getnchannels() != 2 or cdda.getsampwidth() != 2 or cdda.getframerate() != 44100:
            raise ValueError("CD-DA source must be 44.1 kHz signed 16-bit stereo")
        source_frames = cdda.getnframes()
        if source_frames == 0:
            raise ValueError("CD-DA source has no PCM frames")
        if len(cdda.readframes(source_frames)) != source_frames * CDDA_BYTES_PER_FRAME:
            raise ValueError("CD-DA WAV declares frames that are missing from its PCM data")
    return source_frames


def validate_media_coverage(directory):
    directory = pathlib.Path(directory)
    xa = (directory / "xa.xa").read_bytes()
    if not xa or len(xa) % XA_SECTOR_BYTES:
        raise ValueError("XA input does not contain complete 2336-byte sectors")
    xa_sectors = len(xa) // XA_SECTOR_BYTES
    for index in range(xa_sectors):
        subheader = xa[index * XA_SECTOR_BYTES:index * XA_SECTOR_BYTES + 8]
        if (subheader[:4] != subheader[4:] or subheader[0] != 7 or subheader[1] != 3 or
                subheader[2] & 0x64 != 0x64 or subheader[3] != 1):
            raise ValueError(f"XA sector {index} has an unexpected 37.8 kHz stereo subheader")
    xa_frames = xa_sectors * XA_OUTPUT_FRAMES_PER_SECTOR
    if xa_frames < XA_INPUT_FRAMES:
        raise ValueError(f"XA media covers {xa_frames} native frames; input needs {XA_INPUT_FRAMES}")

    cdda_path = directory / "cdda.wav"
    cdda_source_frames = cdda_pcm_frames(cdda_path)
    cdda_sectors = (
        cdda_source_frames + CDDA_OUTPUT_FRAMES_PER_SECTOR - 1
    ) // CDDA_OUTPUT_FRAMES_PER_SECTOR
    cdda_frames = cdda_sectors * CDDA_OUTPUT_FRAMES_PER_SECTOR
    if cdda_frames < cdda_source_frames:
        raise ValueError(
            f"CD-DA media covers {cdda_frames} native frames; input needs {cdda_source_frames}"
        )

    movie = (directory / "movie.str").read_bytes()
    if not movie or len(movie) % STR_SECTOR_BYTES:
        raise ValueError("STR input does not contain complete 2048-byte sectors")
    frames = {}
    for offset in range(0, len(movie), STR_SECTOR_BYTES):
        header = movie[offset:offset + STR_HEADER_BYTES]
        if header[:4] != bytes((0x60, 0x01, 0x01, 0x80)):
            raise ValueError("STR sector has an unexpected video chunk header")
        chunk_index = int.from_bytes(header[4:6], "little")
        chunk_count = int.from_bytes(header[6:8], "little")
        frame_index = int.from_bytes(header[8:12], "little")
        frame_bytes = int.from_bytes(header[12:16], "little")
        width = int.from_bytes(header[16:18], "little")
        height = int.from_bytes(header[18:20], "little")
        if (chunk_count == 0 or chunk_index >= chunk_count or frame_bytes == 0 or
                frame_bytes > STR_MDEC_INPUT_BYTES or frame_bytes % 4 != 0 or
                frame_bytes > chunk_count * STR_PAYLOAD_BYTES or width != 16 or height != 16):
            raise ValueError("STR chunk count, one-block frame size or authored 16x16 dimensions are invalid")
        frame = frames.setdefault(frame_index, {"count": chunk_count, "bytes": frame_bytes, "chunks": set()})
        if frame["count"] != chunk_count or frame["bytes"] != frame_bytes or chunk_index in frame["chunks"]:
            raise ValueError("STR frame has inconsistent or duplicate chunk metadata")
        frame["chunks"].add(chunk_index)
    for frame in frames.values():
        if len(frame["chunks"]) != frame["count"]:
            raise ValueError("STR frame is missing one or more chunks")
    if len(frames) != STR_INPUT_FRAMES:
        raise ValueError(f"STR media has {len(frames)} complete frames; guest requests {STR_INPUT_FRAMES}")
    print(f"disc media coverage: XA={xa_frames}, CD-DA={cdda_frames}, STR={len(frames)} frames")


def encode(encoder, output):
    xa_input = output / "xa.wav"
    ppm_input = output / "frame.ppm"
    write_wave(xa_input, XA_INPUT_FRAMES, (330, 660))
    write_wave(output / "cdda.wav", CDDA_INPUT_FRAMES, (440, 440))
    write_ppm(ppm_input)
    subprocess.run([encoder, "-q", "-t", "xa", "-f", "37800", "-c", "2", "-b", "4",
                    "-F", "7", "-C", "3", str(xa_input), str(output / "xa.xa")], check=True)
    subprocess.run([encoder, "-q", "-t", "strv", "-v", "v2", "-s", "16x16",
                    str(ppm_input), str(output / "movie.str")], check=True)
    xa_input.unlink()
    ppm_input.unlink()
    validate_media_coverage(output)
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


def split_disc(image, cue, cdda, xml_path, max_image_bytes=None):
    raw = image.read_bytes()
    if len(raw) % 2352:
        raise ValueError("mkpsxiso image is not a whole number of raw CD sectors")
    cue_text = cue.read_text(encoding="ascii")
    matches = re.findall(r"TRACK\s+02\s+AUDIO\s+INDEX\s+00\s+(\d{2}:\d{2}:\d{2})", cue_text)
    if len(matches) != 1:
        raise ValueError("mkpsxiso cue has no unique audio INDEX 00")
    minute, second, frame = (int(part) for part in matches[0].split(":"))
    data_sectors = (minute * 60 + second) * 75 + frame
    cdda_frames = cdda_pcm_frames(cdda)
    cdda_sectors = (cdda_frames + CDDA_OUTPUT_FRAMES_PER_SECTOR - 1) // CDDA_OUTPUT_FRAMES_PER_SECTOR
    total_sectors = len(raw) // 2352
    generated_pregap_sectors = total_sectors - data_sectors - cdda_sectors
    expected_pregap_sectors = xml_pregap_sectors(xml_path)
    if (data_sectors < 18 or generated_pregap_sectors <= 0 or
            generated_pregap_sectors > expected_pregap_sectors or cdda_sectors == 0):
        raise ValueError("disc track lengths disagree with the authored audio pregap")
    final_bytes = (data_sectors + expected_pregap_sectors + cdda_sectors) * CDDA_SECTOR_BYTES
    if max_image_bytes is not None and final_bytes > max_image_bytes:
        raise ValueError(
            f"disc image including both tracks is {final_bytes} bytes; limit is {max_image_bytes}"
        )

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
    parser.add_argument("--max-image-bytes", type=int)
    parser.add_argument("--check-media", type=pathlib.Path)
    args = parser.parse_args()
    if args.check_media is not None:
        if (args.split_image is not None or args.output_dir is not None or args.encoder is not None or
                args.cue is not None or args.cd_da is not None or args.xml is not None or
                args.max_image_bytes is not None):
            parser.error("--check-media cannot be combined with encode or split options")
        validate_media_coverage(args.check_media)
    elif args.split_image is not None:
        if (args.cue is None or args.cd_da is None or args.output_dir is not None or
                args.encoder is not None or args.xml is None or
                (args.max_image_bytes is not None and args.max_image_bytes <= 0)):
            parser.error("--split-image requires --cue, --cd-da and --xml")
        split_disc(args.split_image, args.cue, args.cd_da, args.xml, args.max_image_bytes)
    else:
        if (args.output_dir is None or args.encoder is None or args.cue is not None or
                args.cd_da is not None or args.xml is not None or args.max_image_bytes is not None):
            parser.error("--output-dir and --encoder are required for media generation")
        args.output_dir.mkdir(parents=True, exist_ok=True)
        encode(args.encoder, args.output_dir)


if __name__ == "__main__":
    main()
