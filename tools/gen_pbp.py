#!/usr/bin/env python3
"""Build a small deterministic, first-party single-track PSISO PBP fixture."""

import argparse
import hashlib
import pathlib
import struct
import zlib

SECTOR_BYTES = 2352
SECTORS_PER_BLOCK = 16
BLOCK_BYTES = 0x9300
PBP_HEADER_BYTES = 0x28
PSAR_OFFSET = 0x10000
PSAR_DATA_BASE = 0x100000
DATA_BASE = PSAR_OFFSET + PSAR_DATA_BASE
TOC_OFFSET = PSAR_OFFSET + 0x800
INDEX_OFFSET = PSAR_OFFSET + 0x4000
MAX_PBP_BYTES = 1572864
MAX_SOURCE_BYTES = 524288
INDEX_ENTRIES = 80


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def bcd(value):
    if value < 0 or value >= 100:
        raise ValueError("MSF component is outside packed BCD range")
    return ((value // 10) << 4) | (value % 10)


def raw_deflate(data):
    compressor = zlib.compressobj(
        level=9,
        method=zlib.DEFLATED,
        wbits=-15,
        memLevel=9,
        strategy=zlib.Z_FIXED,
    )
    encoded = compressor.compress(data) + compressor.flush(zlib.Z_FINISH)
    decoder = zlib.decompressobj(wbits=-15)
    decoded = decoder.decompress(encoded) + decoder.flush()
    if (decoded != data or not decoder.eof or decoder.unused_data or
            decoder.unconsumed_tail):
        raise ValueError("fixture raw-DEFLATE self-check failed")
    return encoded


def make_toc(sector_count):
    leadout_frames = 150 + sector_count
    minutes, remainder = divmod(leadout_frames, 4500)
    seconds, frames = divmod(remainder, 75)
    leadout = bytes((bcd(minutes), bcd(seconds), bcd(frames)))
    entries = (
        bytes((0x41, 0, 0xA0, 0, 0, 0, 0, 0x01, 0x20, 0)),
        bytes((0x41, 0, 0xA1, 0, 0, 0, 0, 0x01, 0x00, 0)),
        bytes((0x41, 0, 0xA2, 0, 0, 0, 0)) + leadout,
        bytes((0x41, 0, 0x01, 0, 0x02, 0x01, 0, 0, 0x02, 0)),
    )
    return b"".join(entries) + bytes(0xA00 - len(entries) * 10)


def make_pbp(source):
    if not source or len(source) > MAX_SOURCE_BYTES or len(source) % SECTOR_BYTES:
        raise ValueError("source must contain complete raw 2352-byte sectors")
    sector_count = len(source) // SECTOR_BYTES
    if sector_count == 0 or sector_count % SECTORS_PER_BLOCK:
        raise ValueError("fixture source must contain complete 16-sector blocks")

    blocks = []
    for index in range(0, sector_count, SECTORS_PER_BLOCK):
        raw = source[index * SECTOR_BYTES:(index + SECTORS_PER_BLOCK) * SECTOR_BYTES]
        if len(raw) != BLOCK_BYTES:
            raise ValueError("fixture source block is not exactly 0x9300 bytes")
        compressed = raw_deflate(raw)
        blocks.append(compressed if len(compressed) < len(raw) else raw)
    if len(blocks) > INDEX_ENTRIES:
        raise ValueError("fixture has more blocks than the PSISO index can represent")

    total_bytes = DATA_BASE + sum(len(block) for block in blocks)
    if total_bytes > MAX_PBP_BYTES:
        raise ValueError(f"PBP fixture is {total_bytes} bytes; cap is {MAX_PBP_BYTES}")
    if total_bytes > 0xFFFFFFFF:
        raise ValueError("PBP fixture does not fit its 32-bit size fields")

    output = bytearray(DATA_BASE)
    output[:4] = b"\0PBP"
    struct.pack_into("<I", output, 4, 0x00010000)
    section_offsets = [PBP_HEADER_BYTES] + [0x3D8] * 6 + [PSAR_OFFSET]
    for index, offset in enumerate(section_offsets):
        struct.pack_into("<I", output, 8 + index * 4, offset)

    output[PSAR_OFFSET:PSAR_OFFSET + 12] = b"PSISOIMG0000"
    struct.pack_into("<I", output, PSAR_OFFSET + 12, total_bytes - PSAR_OFFSET)
    struct.pack_into("<I", output, PSAR_OFFSET + 16, total_bytes - 0x6420)
    output[PSAR_OFFSET + 0x400:PSAR_OFFSET + 0x40B] = b"_TEST_00000"
    output[TOC_OFFSET:TOC_OFFSET + 0xA00] = make_toc(sector_count)
    struct.pack_into("<I", output, PSAR_OFFSET + 0x1220, total_bytes - 0xD2CF)
    struct.pack_into("<I", output, PSAR_OFFSET + 0x1228, 0x7FF)
    game_name = b"SCHWAYSTATION"
    output[PSAR_OFFSET + 0x122C:PSAR_OFFSET + 0x122C + len(game_name)] = game_name

    for index, block in enumerate(blocks):
        entry_offset = INDEX_OFFSET + index * 32
        data_offset = len(output) - DATA_BASE
        struct.pack_into("<IHH", output, entry_offset, data_offset, len(block), 0)
        output.extend(block)

    return bytes(output), sector_count, len(blocks)


def write_cue_and_source_copy(source_path, cue_path):
    cue_path.parent.mkdir(parents=True, exist_ok=True)
    data_copy = cue_path.with_name("disc_boot_pbp.bin")
    data_copy.write_bytes(source_path.read_bytes())
    cue_path.write_text(
        'FILE "disc_boot_pbp.bin" BINARY\n'
        "  TRACK 01 MODE2/2352\n"
        "    INDEX 01 00:00:00\n",
        encoding="ascii",
        newline="\n",
    )
    return data_copy


def normalize_cue_newlines(path):
    content = path.read_bytes()
    normalized = content.replace(b"\r\n", b"\n")
    if b"\r" in normalized:
        raise ValueError("generated CUE contains a bare carriage return")
    if normalized != content:
        path.write_bytes(normalized)


def write_manifest(path, data_copy, output_path, generator_path,
                   output, sector_count, block_count):
    source_bytes = data_copy.read_bytes()
    generator_bytes = generator_path.read_bytes()
    rows = (
        "format=schwaystation-pbp-fixture-v1",
        f"source={data_copy.name}",
        f"source_bytes={len(source_bytes)}",
        f"source_sha256={sha256(source_bytes)}",
        f"generator={generator_path.name}",
        f"generator_sha256={sha256(generator_bytes)}",
        f"output={output_path.name}",
        f"output_bytes={len(output)}",
        f"output_sha256={sha256(output)}",
        f"sectors={sector_count}",
        f"blocks={block_count}",
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(rows) + "\n", encoding="ascii", newline="\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path)
    parser.add_argument("--output", type=pathlib.Path)
    parser.add_argument("--cue", type=pathlib.Path)
    parser.add_argument("--manifest", type=pathlib.Path)
    parser.add_argument("--normalize-cue", type=pathlib.Path)
    args = parser.parse_args()

    if args.normalize_cue is not None:
        normalize_cue_newlines(args.normalize_cue)
        return
    if any(value is None for value in (args.source, args.output, args.cue, args.manifest)):
        parser.error("--source, --output, --cue, and --manifest are required")

    source_bytes = args.source.read_bytes()
    first, sector_count, block_count = make_pbp(source_bytes)
    second, second_sector_count, second_block_count = make_pbp(source_bytes)
    if (first != second or sector_count != second_sector_count or
            block_count != second_block_count):
        raise ValueError("repeated PBP generation differs")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(first)
    data_copy = write_cue_and_source_copy(args.source, args.cue)
    write_manifest(args.manifest, data_copy, args.output,
                   pathlib.Path(__file__).resolve(), first, sector_count, block_count)
    print(f"PBP fixture: {len(first)} bytes, {sector_count} sectors, {block_count} blocks")


if __name__ == "__main__":
    main()
