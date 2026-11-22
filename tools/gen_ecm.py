#!/usr/bin/env python3
"""Build deterministic literal and Mode 2 ECM fixtures from an owned raw image."""

import argparse
import hashlib
import pathlib
import re
import struct

SECTOR_BYTES = 2352
IMAGE_LIMIT = 64 * 1024 * 1024
SYNC = b"\0" + b"\xff" * 10 + b"\0"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def write_text_lf(path, text):
    with path.open("w", encoding="ascii", newline="\n") as output:
        output.write(text)


def edc(data):
    value = 0
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = (value >> 1) ^ (0xD8018001 if value & 1 else 0)
    return value


def write_count(output, kind, count):
    if kind not in range(4) or count <= 0 or count > 0x100000000:
        raise ValueError("invalid ECM record")
    value = count - 1
    first = ((value & 0x1f) << 2) | kind
    value >>= 5
    output.append(first | (0x80 if value else 0))
    while value:
        byte = value & 0x7f
        value >>= 7
        output.append(byte | (0x80 if value else 0))


def ecm_header(output):
    output.extend(b"ECM\0")


def ecm_end(output, decoded):
    output.extend((0xfc, 0xff, 0xff, 0xff, 0x3f))
    output.extend(struct.pack("<I", edc(decoded)))


def literal_ecm(source):
    output = bytearray()
    ecm_header(output)
    for offset in range(0, len(source), 65536):
        chunk = source[offset:offset + 65536]
        write_count(output, 0, len(chunk))
        output.extend(chunk)
    ecm_end(output, source)
    return bytes(output)


def modeled_ecm(source):
    output = bytearray()
    ecm_header(output)
    for offset in range(0, len(source), SECTOR_BYTES):
        sector = source[offset:offset + SECTOR_BYTES]
        if (len(sector) != SECTOR_BYTES or sector[:12] != SYNC or sector[15] != 2 or
                sector[16:20] != sector[20:24]):
            raise ValueError(f"source sector {offset // SECTOR_BYTES} is not raw Mode 2")
        write_count(output, 0, 16)
        output.extend(sector[:16])
        kind = 3 if sector[18] & 0x20 else 2
        write_count(output, kind, 1)
        output.extend(sector[16:20])
        output.extend(sector[24:2348 if kind == 3 else 2072])
    ecm_end(output, source)
    return bytes(output)


def source_cue_text(path, source_name):
    text = path.read_text(encoding="ascii")
    if "\r" in text or "\0" in text:
        raise ValueError("source CUE must use plain LF text")
    pattern = r'FILE "([^"]+)" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n'
    match = re.fullmatch(pattern, text)
    if match is None or pathlib.PurePosixPath(match.group(1)).name != source_name:
        raise ValueError("source CUE does not describe the owned one-track image")
    return text


def write_manifest(path, source_path, cue_path, literal, modeled, generator):
    source = source_path.read_bytes()
    cue = cue_path.read_bytes()
    fields = [
        "format=schwaystation-legacy-ecm-v1",
        f"source={source_path.name}",
        f"source_bytes={len(source)}",
        f"source_sectors={len(source) // SECTOR_BYTES}",
        f"source_sha256={digest(source)}",
        f"source_cue_sha256={digest(cue)}",
        f"generator=gen_ecm.py",
        f"generator_sha256={digest(generator)}",
        f"literal_bytes={len(literal)}",
        f"literal_sha256={digest(literal)}",
        f"modeled_bytes={len(modeled)}",
        f"modeled_sha256={digest(modeled)}",
        f"guest_reads={len(source) // SECTOR_BYTES}",
        f"guest_outputs={len(source) // SECTOR_BYTES}",
        "",
    ]
    write_text_lf(path, "\n".join(fields))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path, required=True)
    parser.add_argument("--source-cue", type=pathlib.Path, required=True)
    parser.add_argument("--output-dir", type=pathlib.Path, required=True)
    args = parser.parse_args()
    source = args.source.read_bytes()
    if not source or len(source) > IMAGE_LIMIT or len(source) % SECTOR_BYTES:
        raise ValueError("source must contain bounded complete raw sectors")
    cue_text = source_cue_text(args.source_cue, args.source.name)
    literal = literal_ecm(source)
    modeled = modeled_ecm(source)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    source_cue = args.output_dir / "disc_boot_legacy_source.cue"
    write_text_lf(source_cue, cue_text)
    (args.output_dir / "disc_boot_legacy_literal.ecm").write_bytes(literal)
    (args.output_dir / "disc_boot_legacy_modeled.ecm").write_bytes(modeled)
    write_text_lf(args.output_dir / "disc_boot_legacy_literal.cue",
        'FILE "disc_boot_legacy_literal.ecm" BINARY\n'
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n")
    write_text_lf(args.output_dir / "disc_boot_legacy_modeled.cue",
        'FILE "disc_boot_legacy_modeled.ecm" BINARY\n'
        "  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n")
    write_manifest(args.output_dir / "disc_boot_legacy_ecm.manifest", args.source,
                   source_cue, literal, modeled, pathlib.Path(__file__).read_bytes())


if __name__ == "__main__":
    main()
