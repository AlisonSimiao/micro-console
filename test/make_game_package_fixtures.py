#!/usr/bin/env python3

import json
import struct
import sys
from pathlib import Path


HEADER_SIZE = 12


def build_package(entries, version=1, count_override=None):
    index_size = sum(1 + len(path.encode("ascii")) + 8 for path, *_ in entries)
    next_offset = HEADER_SIZE + index_size
    records = []
    payloads = []
    offsets = {}

    for path, content, offset_override, size_override in entries:
        encoded_path = path.encode("ascii")
        offset = next_offset if offset_override is None else offset_override
        size = len(content) if size_override is None else size_override
        records.append(
            bytes((len(encoded_path),))
            + encoded_path
            + struct.pack("<II", offset, size)
        )
        payloads.append(content)
        offsets[path] = offset
        next_offset += len(content)

    count = len(entries) if count_override is None else count_override
    header = struct.pack("<4sHHI", b"ALIP", version, count, index_size)
    return header + b"".join(records) + b"".join(payloads), offsets


def write_fixture(directory, name, entries, version=1, count_override=None):
    data, offsets = build_package(entries, version, count_override)
    (directory / name).write_bytes(data)
    return data, offsets


def manifest(entrypoint="Codes/main.js", cover="Capa.bin", package_version=1):
    return json.dumps(
        {
            "id": "fixture-game",
            "name": "Fixture Game",
            "version": "1.0.0",
            "packageVersion": package_version,
            "author": "Test",
            "entrypoint": entrypoint,
            "cover": cover,
        },
        separators=(",", ":"),
    ).encode("utf-8")


def main():
    if len(sys.argv) != 2:
        raise SystemExit(
            "usage: python3 test/make_game_package_fixtures.py OUTPUT_DIRECTORY"
        )

    directory = Path(sys.argv[1])
    directory.mkdir(parents=True, exist_ok=True)
    code = b"// fixture entrypoint\n" + b"J" * 2048
    cover = b"\x89PNG\r\n\x1a\nfixture"
    large = bytes(range(256)) * 64

    valid_entries = [
        ("manifest.json", manifest(), None, None),
        ("Codes/main.js", code, None, None),
        ("Capa.bin", cover, None, None),
        ("Assets/large.bin", large, None, None),
    ]
    valid, offsets = write_fixture(directory, "valid.ali", valid_entries)

    write_fixture(
        directory,
        "missing-manifest.ali",
        [
            ("Codes/main.js", code, None, None),
            ("Capa.bin", cover, None, None),
        ],
    )
    write_fixture(
        directory,
        "invalid-manifest.ali",
        [
            ("manifest.json", b"{", None, None),
            ("Codes/main.js", code, None, None),
            ("Capa.bin", cover, None, None),
        ],
    )
    write_fixture(directory, "unsupported-version.ali", valid_entries, version=2)
    write_fixture(
        directory,
        "missing-entrypoint.ali",
        [
            ("manifest.json", manifest(entrypoint="Codes/missing.js"), None, None),
            ("Codes/main.js", code, None, None),
            ("Capa.bin", cover, None, None),
        ],
    )
    write_fixture(
        directory,
        "missing-cover.ali",
        [
            ("manifest.json", manifest(cover="missing.bin"), None, None),
            ("Codes/main.js", code, None, None),
            ("Capa.bin", cover, None, None),
        ],
    )
    write_fixture(
        directory,
        "invalid-path.ali",
        [
            ("manifest.json", manifest(), None, None),
            ("../escape.js", code, None, None),
            ("Capa.bin", cover, None, None),
        ],
    )

    inconsistent, _ = build_package(
        [
            ("manifest.json", manifest(), None, None),
            ("Codes/main.js", code, None, len(code) + 32),
            ("Capa.bin", cover, None, None),
        ]
    )
    (directory / "inconsistent-size.ali").write_bytes(inconsistent)

    overlap, _ = build_package(
        [
            ("manifest.json", manifest(), None, None),
            ("Codes/main.js", code, None, None),
            ("Capa.bin", cover, offsets["Codes/main.js"], None),
        ]
    )
    (directory / "overlapping-entries.ali").write_bytes(overlap)

    (directory / "truncated-index.ali").write_bytes(valid[: HEADER_SIZE + 5])
    (directory / "truncated-payload.ali").write_bytes(valid[:-1])

    oversized_manifest = b'{"value":"' + b"x" * 4100 + b'"}'
    write_fixture(
        directory,
        "oversized-manifest.ali",
        [
            ("manifest.json", oversized_manifest, None, None),
            ("Codes/main.js", code, None, None),
            ("Capa.bin", cover, None, None),
        ],
    )

    too_many, _ = build_package([], count_override=65)
    (directory / "too-many-entries.ali").write_bytes(too_many)
    (directory / "bad-magic.ali").write_bytes(b"NOPE" + valid[4:])

    print(f"Wrote .ali fixtures to {directory}")


if __name__ == "__main__":
    main()
