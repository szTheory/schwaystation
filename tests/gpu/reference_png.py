#!/usr/bin/env python3
"""Decode the pinned PS1 GPU PNG captures into canonical RGB555 words.

This is test-input adaptation only. It intentionally accepts the bounded
formats used by the selected build-158 captures and does no color management,
alpha compositing, or rescaling.
"""

import argparse
import hashlib
import pathlib
import struct
import sys
import zlib


WIDTH = 1024
HEIGHT = 512
WORD_COUNT = WIDTH * HEIGHT
MAX_PNG_BYTES = 1024 * 1024
SIGNATURE = b"\x89PNG\r\n\x1a\n"
ALLOWED_ANCILLARY = {b"gAMA", b"cHRM", b"bKGD", b"pHYs", b"tIME", b"tEXt"}


def _invalid(message):
    raise ValueError(message)


def _paeth(left, up, upper_left):
    estimate = left + up - upper_left
    left_distance = abs(estimate - left)
    up_distance = abs(estimate - up)
    upper_left_distance = abs(estimate - upper_left)
    if left_distance <= up_distance and left_distance <= upper_left_distance:
        return left
    if up_distance <= upper_left_distance:
        return up
    return upper_left


def _parse_chunks(data):
    if len(data) < 57 or len(data) > MAX_PNG_BYTES:
        _invalid("PNG byte length is outside the supported bound")
    if data[:8] != SIGNATURE:
        _invalid("PNG signature is invalid")

    offset = 8
    chunk_index = 0
    seen_ihdr = False
    seen_plte = False
    seen_idat = False
    ended_idat = False
    seen_iend = False
    ihdr = None
    palette = None
    idat_parts = []

    while offset < len(data):
        if len(data) - offset < 12:
            _invalid("truncated PNG chunk header")
        length = int.from_bytes(data[offset : offset + 4], "big")
        chunk_type = data[offset + 4 : offset + 8]
        if any(not (65 <= byte <= 90 or 97 <= byte <= 122) for byte in chunk_type):
            _invalid("PNG chunk type contains a non-letter")
        if chunk_type[2] & 0x20:
            _invalid("PNG chunk type uses the reserved lowercase bit")
        if length > len(data) - offset - 12:
            _invalid("PNG chunk exceeds the input bounds")

        chunk_data_start = offset + 8
        chunk_data_end = chunk_data_start + length
        chunk_data = data[chunk_data_start:chunk_data_end]
        expected_crc = int.from_bytes(data[chunk_data_end : chunk_data_end + 4], "big")
        if zlib.crc32(chunk_type + chunk_data) & 0xFFFFFFFF != expected_crc:
            _invalid("PNG chunk CRC mismatch")
        offset = chunk_data_end + 4

        if chunk_index == 0 and chunk_type != b"IHDR":
            _invalid("IHDR is not the first PNG chunk")
        chunk_index += 1

        if chunk_type == b"IHDR":
            if seen_ihdr or length != 13:
                _invalid("duplicate or malformed IHDR")
            seen_ihdr = True
            ihdr = struct.unpack(">IIBBBBB", chunk_data)
        elif not seen_ihdr:
            _invalid("PNG chunk appears before IHDR")
        elif chunk_type == b"PLTE":
            if seen_plte or seen_idat or length == 0 or length % 3 != 0:
                _invalid("duplicate, late, or malformed PLTE")
            seen_plte = True
            palette = tuple(
                tuple(chunk_data[index : index + 3])
                for index in range(0, length, 3)
            )
        elif chunk_type == b"IDAT":
            if ended_idat:
                _invalid("PNG IDAT chunks are not consecutive")
            seen_idat = True
            idat_parts.append(chunk_data)
        elif chunk_type == b"IEND":
            if length != 0 or not seen_idat or offset != len(data):
                _invalid("malformed IEND or trailing PNG bytes")
            seen_iend = True
            break
        else:
            if seen_idat:
                ended_idat = True
            if chunk_type == b"tRNS":
                _invalid("PNG transparency chunks are unsupported")
            if chunk_type == b"PLTE":
                _invalid("malformed PLTE")
            if chunk_type not in ALLOWED_ANCILLARY:
                if chunk_type[0] & 0x20:
                    _invalid("PNG ancillary chunk is outside the selected corpus")
                _invalid("unknown critical PNG chunk")
            if chunk_type == b"gAMA" and length != 4:
                _invalid("malformed gAMA chunk")
            if chunk_type == b"cHRM" and length != 32:
                _invalid("malformed cHRM chunk")
            if chunk_type == b"pHYs" and length != 9:
                _invalid("malformed pHYs chunk")
            if chunk_type == b"tIME" and length != 7:
                _invalid("malformed tIME chunk")
            if chunk_type == b"tEXt" and (length == 0 or b"\0" not in chunk_data):
                _invalid("malformed tEXt chunk")
        if seen_idat and chunk_type != b"IDAT" and chunk_type != b"IEND":
            ended_idat = True

    if not seen_ihdr or not seen_idat or not seen_iend or ihdr is None:
        _invalid("PNG is missing a required chunk")
    width, height, depth, color_type, compression, filter_method, interlace = ihdr
    if width == 0 or height == 0:
        _invalid("PNG dimensions must be nonzero")
    if (depth, color_type) not in {(8, 2), (8, 6), (4, 3), (8, 3)}:
        _invalid("PNG bit depth and color type are outside the selected corpus")
    if compression != 0 or filter_method != 0 or interlace != 0:
        _invalid("PNG compression, filter, or interlace method is unsupported")
    if color_type == 3:
        if not seen_plte or palette is None or len(palette) > (1 << depth):
            _invalid("indexed PNG has no valid palette")
    elif seen_plte:
        _invalid("palette on a non-indexed PNG is outside the selected corpus")
    if not idat_parts:
        _invalid("PNG has no IDAT data")
    return width, height, depth, color_type, palette, b"".join(idat_parts)


def decode_png(data, expected_dimensions=(WIDTH, HEIGHT)):
    width, height, depth, color_type, palette, compressed = _parse_chunks(data)
    if expected_dimensions is not None and (width, height) != expected_dimensions:
        _invalid("PNG dimensions do not match the expected VRAM capture")
    if width > WIDTH or height > HEIGHT:
        _invalid("PNG dimensions exceed the VRAM bound")

    channel_count = {2: 3, 3: 1, 6: 4}[color_type]
    bits_per_pixel = channel_count * depth
    row_bytes = (width * bits_per_pixel + 7) // 8
    expected_raw_size = height * (row_bytes + 1)
    decompressor = zlib.decompressobj()
    try:
        scanlines = decompressor.decompress(compressed, expected_raw_size + 1)
        if len(scanlines) > expected_raw_size:
            _invalid("PNG decompressed data exceeds the exact output bound")
        if decompressor.unconsumed_tail:
            _invalid("PNG decompression exceeds the exact output bound")
        scanlines += decompressor.flush()
    except zlib.error as error:
        _invalid("PNG zlib stream is invalid: " + str(error))
    if (
        len(scanlines) != expected_raw_size
        or not decompressor.eof
        or decompressor.unused_data
        or decompressor.unconsumed_tail
    ):
        _invalid("PNG zlib stream is incomplete, oversized, or has trailing data")

    bytes_per_pixel = max(1, (bits_per_pixel + 7) // 8)
    rows = []
    previous = bytearray(row_bytes)
    offset = 0
    alpha_values = set()
    for _ in range(height):
        filter_type = scanlines[offset]
        filtered = scanlines[offset + 1 : offset + 1 + row_bytes]
        offset += row_bytes + 1
        if filter_type > 4:
            _invalid("PNG row uses an unsupported filter")
        row = bytearray(row_bytes)
        for index, value in enumerate(filtered):
            left = row[index - bytes_per_pixel] if index >= bytes_per_pixel else 0
            up = previous[index]
            upper_left = (
                previous[index - bytes_per_pixel]
                if index >= bytes_per_pixel
                else 0
            )
            if filter_type == 0:
                predictor = 0
            elif filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = up
            elif filter_type == 3:
                predictor = (left + up) // 2
            else:
                predictor = _paeth(left, up, upper_left)
            row[index] = (value + predictor) & 0xFF
        rows.append(row)
        previous = row

    canonical = bytearray()
    for row in rows:
        for x in range(width):
            if color_type == 2:
                pixel = x * 3
                red, green, blue = row[pixel : pixel + 3]
            elif color_type == 6:
                pixel = x * 4
                red, green, blue, alpha = row[pixel : pixel + 4]
                alpha_values.add(alpha)
            elif depth == 8:
                palette_index = row[x]
                if palette_index >= len(palette):
                    _invalid("indexed PNG pixel exceeds its palette")
                red, green, blue = palette[palette_index]
            else:
                packed = row[x // 2]
                palette_index = packed >> 4 if x % 2 == 0 else packed & 0x0F
                if palette_index >= len(palette):
                    _invalid("indexed PNG pixel exceeds its palette")
                red, green, blue = palette[palette_index]

            word = (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)
            canonical.extend((word & 0xFF, word >> 8))

    if len(canonical) != width * height * 2:
        _invalid("canonical VRAM output has an unexpected size")
    alpha_summary = None
    if color_type == 6:
        alpha_summary = (
            min(alpha_values),
            max(alpha_values),
            tuple(sorted(alpha_values)),
        )
    return bytes(canonical), alpha_summary


def _chunk(chunk_type, data):
    return (
        struct.pack(">I", len(data))
        + chunk_type
        + data
        + struct.pack(">I", zlib.crc32(chunk_type + data) & 0xFFFFFFFF)
    )


def _paeth_filter(left, up, upper_left):
    return _paeth(left, up, upper_left)


def _encode_filter(row, previous, bytes_per_pixel, filter_type):
    result = bytearray(len(row))
    for index, value in enumerate(row):
        left = row[index - bytes_per_pixel] if index >= bytes_per_pixel else 0
        up = previous[index]
        upper_left = (
            previous[index - bytes_per_pixel]
            if index >= bytes_per_pixel
            else 0
        )
        if filter_type == 0:
            predictor = 0
        elif filter_type == 1:
            predictor = left
        elif filter_type == 2:
            predictor = up
        elif filter_type == 3:
            predictor = (left + up) // 2
        else:
            predictor = _paeth_filter(left, up, upper_left)
        result[index] = (value - predictor) & 0xFF
    return bytes((filter_type,)) + result


def _build_png(
    width,
    height,
    depth,
    color_type,
    rows,
    filters,
    palette=None,
    before_idat=(),
    compressed_override=None,
    invalid_filter_row=None,
):
    channels = {2: 3, 3: 1, 6: 4}[color_type]
    bytes_per_pixel = max(1, (channels * depth + 7) // 8)
    raw = bytearray()
    previous = bytes((width * channels * depth + 7) // 8)
    for row_index, row in enumerate(rows):
        filter_type = filters[row_index % len(filters)]
        if row_index == invalid_filter_row:
            raw.extend((5,))
            raw.extend(row)
        else:
            raw.extend(_encode_filter(row, previous, bytes_per_pixel, filter_type))
        previous = row
    compressed = (
        zlib.compress(bytes(raw))
        if compressed_override is None
        else compressed_override
    )
    header = struct.pack(">IIBBBBB", width, height, depth, color_type, 0, 0, 0)
    parts = [SIGNATURE, _chunk(b"IHDR", header)]
    for chunk_type, chunk_data in before_idat:
        parts.append(_chunk(chunk_type, chunk_data))
    if palette is not None:
        parts.append(_chunk(b"PLTE", bytes(value for color in palette for value in color)))
    midpoint = max(1, len(compressed) // 2)
    parts.append(_chunk(b"IDAT", compressed[:midpoint]))
    if midpoint < len(compressed):
        parts.append(_chunk(b"IDAT", compressed[midpoint:]))
    parts.append(_chunk(b"IEND", b""))
    return b"".join(parts)


def _rgb_rows(width, height, first_red=255):
    rows = []
    for y in range(height):
        row = bytearray()
        for x in range(width):
            row.extend(
                (
                    first_red if x == 0 and y == 0 else (x * 53 + y * 17) & 0xFF,
                    (x * 29 + y * 71) & 0xFF,
                    (x * 83 + y * 11) & 0xFF,
                )
            )
        rows.append(bytes(row))
    return rows


def _pack_indices(indices, depth):
    if depth == 8:
        return bytes(indices)
    packed = bytearray()
    for index in range(0, len(indices), 2):
        high = indices[index]
        low = indices[index + 1] if index + 1 < len(indices) else 0
        packed.append((high << 4) | low)
    return bytes(packed)


def _expected_from_rgb_rows(rows, width):
    result = bytearray()
    for row in rows:
        for x in range(width):
            red, green, blue = row[x * 3 : x * 3 + 3]
            word = (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)
            result.extend((word & 0xFF, word >> 8))
    return bytes(result)


def _expect_invalid(data, expected_dimensions=None):
    try:
        decode_png(data, expected_dimensions=expected_dimensions)
    except ValueError:
        return
    raise AssertionError("invalid PNG input was accepted")


def run_self_tests():
    rgb_rows = _rgb_rows(5, 5)
    rgb_png = _build_png(5, 5, 8, 2, rgb_rows, (0, 1, 2, 3, 4))
    decoded, alpha = decode_png(rgb_png, (5, 5))
    assert decoded == _expected_from_rgb_rows(rgb_rows, 5)
    assert alpha is None

    changed_rows = _rgb_rows(5, 5, first_red=247)
    changed, _ = decode_png(
        _build_png(5, 5, 8, 2, changed_rows, (0, 1, 2, 3, 4)), (5, 5)
    )
    changed_words = sum(
        decoded[index : index + 2] != changed[index : index + 2]
        for index in range(0, len(decoded), 2)
    )
    assert changed_words == 1

    rgba_rows = []
    rgba_alpha = (0, 64, 128, 192, 255)
    for y in range(2):
        row = bytearray()
        for x in range(3):
            row.extend((x * 40, y * 50, 255 - x * 30, rgba_alpha[x + 2 * y]))
        rgba_rows.append(bytes(row))
    rgba_png = _build_png(3, 2, 8, 6, rgba_rows, (0, 4))
    rgba_decoded, alpha = decode_png(rgba_png, (3, 2))
    assert alpha == (0, 255, (0, 64, 128, 192, 255))
    assert len(rgba_decoded) == 3 * 2 * 2

    palette8 = ((0, 0, 0), (248, 128, 16), (255, 255, 255))
    indices8 = (0, 1, 2, 2, 1, 0)
    rows8 = (bytes(indices8[:3]), bytes(indices8[3:]))
    png8 = _build_png(3, 2, 8, 3, rows8, (0, 2), palette=palette8)
    decoded8, _ = decode_png(png8, (3, 2))
    expected8 = bytearray()
    for index in indices8:
        red, green, blue = palette8[index]
        word = (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)
        expected8.extend((word & 0xFF, word >> 8))
    assert decoded8 == bytes(expected8)

    palette4 = ((0, 0, 0), (248, 0, 0), (0, 248, 0), (0, 0, 248))
    indices4 = (0, 1, 2, 3, 1, 3)
    rows4 = (
        _pack_indices(indices4[:3], 4),
        _pack_indices(indices4[3:], 4),
    )
    png4 = _build_png(3, 2, 4, 3, rows4, (0, 1), palette=palette4)
    decoded4, _ = decode_png(png4, (3, 2))
    expected4 = bytearray()
    for index in indices4:
        red, green, blue = palette4[index]
        word = (red >> 3) | ((green >> 3) << 5) | ((blue >> 3) << 10)
        expected4.extend((word & 0xFF, word >> 8))
    assert decoded4 == bytes(expected4)

    bad_crc = bytearray(rgb_png)
    bad_crc[-5] ^= 1
    _expect_invalid(bytes(bad_crc), (5, 5))
    _expect_invalid(rgb_png[:-1], (5, 5))

    bad_filter_png = _build_png(
        5, 5, 8, 2, rgb_rows, (0,), invalid_filter_row=0
    )
    _expect_invalid(bad_filter_png, (5, 5))

    short_stream = zlib.compress(b"\0")
    _expect_invalid(
        _build_png(
            5,
            5,
            8,
            2,
            rgb_rows,
            (0,),
            compressed_override=short_stream,
        ),
        (5, 5),
    )
    oversized_stream = zlib.compress(bytes((5 * 3 + 1) * 5 + 1))
    _expect_invalid(
        _build_png(
            5,
            5,
            8,
            2,
            rgb_rows,
            (0,),
            compressed_override=oversized_stream,
        ),
        (5, 5),
    )
    _expect_invalid(
        _build_png(
            5,
            5,
            8,
            2,
            rgb_rows,
            (0,),
            compressed_override=zlib.compress(bytes((5 * 3 + 1) * 5))
            + b"trailing",
        ),
        (5, 5),
    )

    bad_palette = _build_png(
        3,
        2,
        4,
        3,
        (_pack_indices((0, 1, 2), 4), _pack_indices((0, 1, 3), 4)),
        (0,),
        palette=palette4[:3],
    )
    _expect_invalid(bad_palette, (3, 2))
    transparent = _build_png(
        5,
        5,
        8,
        2,
        rgb_rows,
        (0,),
        before_idat=((b"tRNS", b"\0\0"),),
    )
    _expect_invalid(transparent, (5, 5))
    unknown_critical = _build_png(
        5,
        5,
        8,
        2,
        rgb_rows,
        (0,),
        before_idat=((b"ABCD", b""),),
    )
    _expect_invalid(unknown_critical, (5, 5))
    _expect_invalid(rgb_png, (4, 5))


def decode_file(source_path, output_path):
    source = pathlib.Path(source_path)
    output = pathlib.Path(output_path)
    if source.resolve() == output.resolve():
        _invalid("input and output paths must be different")
    if source.stat().st_size > MAX_PNG_BYTES:
        _invalid("PNG file exceeds the input bound")
    data = source.read_bytes()
    if len(data) > MAX_PNG_BYTES:
        _invalid("PNG file exceeds the input bound")
    canonical, alpha = decode_png(data)
    if len(canonical) != WORD_COUNT * 2:
        _invalid("capture did not produce the complete VRAM word domain")

    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    try:
        temporary.write_bytes(canonical)
        temporary.replace(output)
    except OSError:
        try:
            temporary.unlink()
        except OSError:
            pass
        raise

    alpha_text = "none"
    if alpha is not None:
        alpha_text = "min={} max={} unique={}".format(
            alpha[0], alpha[1], ",".join(str(value) for value in alpha[2])
        )
    print(
        "decoded {}x{} words={} source_sha256={} rgb555le_sha256={} alpha={}".format(
            WIDTH,
            HEIGHT,
            WORD_COUNT,
            hashlib.sha256(data).hexdigest(),
            hashlib.sha256(canonical).hexdigest(),
            alpha_text,
        )
    )


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    decode_parser = subparsers.add_parser("decode")
    decode_parser.add_argument("png")
    decode_parser.add_argument("rgb555le")
    subparsers.add_parser("self-test")
    arguments = parser.parse_args(argv)

    try:
        if arguments.command == "self-test":
            run_self_tests()
            print("PNG adapter controls passed")
        else:
            decode_file(arguments.png, arguments.rgb555le)
    except (OSError, ValueError, AssertionError) as error:
        print("gpu.reference.png: {}".format(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
