"""Verify the focused teaching-image set before Sphinx runs."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

EXPECTED = {
    "workbench_overview.png": (1200, 700),
    "workspace_documents.png": (700, 350),
    "documents_and_bindings.png": (1000, 550),
    "inspector_parameters.png": (280, 450),
    "inspector_dependencies.png": (280, 450),
    "host_features.png": (280, 450),
    "inspector_entry_points.png": (280, 450),
    # TimelineWidget is intentionally a compact transport strip. Width catches
    # collapsed layouts; 48 px retains readable controls without rejecting the
    # native size hint on Windows runners.
    "timeline_overview.png": (700, 48),
    "render_toy.png": (700, 300),
    "shader_toy.png": (700, 300),
    "source_generated_compare.png": (700, 350),
}
REQUIRED = tuple(EXPECTED)
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def png_size(path: Path) -> tuple[int, int]:
    data = path.read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError("bad PNG signature")
    offset = len(PNG_SIGNATURE)
    size = None
    compressed = bytearray()
    saw_end = False
    while offset + 12 <= len(data):
        length = struct.unpack(">I", data[offset : offset + 4])[0]
        kind = data[offset + 4 : offset + 8]
        payload_start = offset + 8
        payload_end = payload_start + length
        if payload_end + 4 > len(data):
            raise ValueError("truncated PNG chunk")
        payload = data[payload_start:payload_end]
        expected_crc = struct.unpack(">I", data[payload_end : payload_end + 4])[0]
        if zlib.crc32(kind + payload) & 0xFFFFFFFF != expected_crc:
            raise ValueError("invalid PNG chunk checksum")
        if kind == b"IHDR":
            if length != 13 or size is not None:
                raise ValueError("invalid PNG IHDR")
            width, height, depth, color, compression, filtering, interlace = struct.unpack(
                ">IIBBBBB", payload
            )
            if not width or not height or depth != 8 or color not in {2, 6}:
                raise ValueError("PNG must be non-empty 8-bit RGB or RGBA")
            channels = 3 if color == 2 else 4
            if compression or filtering or interlace:
                raise ValueError("unsupported PNG encoding")
            size = (width, height)
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            saw_end = True
            break
        offset = payload_end + 4
    if size is None or not compressed or not saw_end:
        raise ValueError("incomplete PNG")
    try:
        pixels = zlib.decompress(compressed)
    except zlib.error as error:
        raise ValueError("invalid PNG image data") from error
    expected_bytes = size[1] * (1 + size[0] * channels)
    if len(pixels) != expected_bytes:
        raise ValueError("unexpected PNG pixel data size")
    return size


def verify(directory: Path) -> list[str]:
    problems = []
    for name, minimum in EXPECTED.items():
        path = directory / name
        if not path.is_file():
            problems.append(f"missing documentation image: {name}")
            continue
        if path.stat().st_size == 0:
            problems.append(f"empty documentation image: {name}")
            continue
        try:
            size = png_size(path)
        except (OSError, ValueError) as error:
            problems.append(f"invalid documentation image {name}: {error}")
            continue
        if size[0] < minimum[0] or size[1] < minimum[1]:
            problems.append(
                f"undersized documentation image {name}: {size[0]}x{size[1]} "
                f"(minimum {minimum[0]}x{minimum[1]})"
            )
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", nargs="?", type=Path, default=Path("src/docs/images"))
    args = parser.parse_args()
    problems = verify(args.directory)
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
