#!/usr/bin/env python3
# Adapted from GabeSud0/pokeplatinum-3ds (GPL-3.0): builder/extract_rom.py, cut down to the NitroFS reader
# (no extraction, no ROM-hash check). Read-only: nothing is written or uploaded.
"""List the files of a Nintendo DS ROM's NitroFS (path, offset, size)."""
from __future__ import annotations

import struct

def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def parse_nitrofs(data: bytes) -> list[dict]:
    """Return FAT file IDs and safe relative paths from a NitroFS image."""
    if len(data) < 0x200:
        raise ValueError("Truncated NDS header")
    fnt_off, fnt_size = u32(data, 0x40), u32(data, 0x44)
    fat_off, fat_size = u32(data, 0x48), u32(data, 0x4c)
    if not fnt_size or fnt_off < 0x200 or fnt_off + fnt_size > len(data):
        raise ValueError("Invalid FNT bounds")
    if not fat_size or fat_size % 8 or fat_off < 0x200 or fat_off + fat_size > len(data):
        raise ValueError("Invalid FAT bounds")
    fnt = data[fnt_off:fnt_off + fnt_size]
    count = fat_size // 8
    if len(fnt) < 8:
        raise ValueError("Truncated FNT root")
    dir_count = u16(fnt, 6)
    if not dir_count or dir_count * 8 > len(fnt):
        raise ValueError("Invalid FNT directory table")
    files: dict[int, str] = {}
    visited: set[int] = set()

    def walk(directory: int, prefix: tuple[str, ...]) -> None:
        idx = directory - 0xf000
        if idx < 0 or idx >= dir_count or directory in visited:
            raise ValueError("Invalid or cyclic FNT directory")
        visited.add(directory)
        off, first_id, _ = struct.unpack_from("<IHH", fnt, idx * 8)
        if off >= len(fnt):
            raise ValueError("Invalid FNT subtable offset")
        file_id = first_id
        while True:
            if off >= len(fnt):
                raise ValueError("Unterminated FNT subtable")
            kind = fnt[off]
            off += 1
            if kind == 0:
                break
            length = kind & 0x7f
            if not length or off + length > len(fnt):
                raise ValueError("Invalid FNT name")
            raw = fnt[off:off + length]
            off += length
            name = raw.decode("ascii")
            if name in (".", "..") or "/" in name or "\\" in name or ":" in name:
                raise ValueError("Unsafe FNT path")
            if kind & 0x80:
                if off + 2 > len(fnt):
                    raise ValueError("Truncated FNT directory reference")
                child = u16(fnt, off)
                off += 2
                walk(child, prefix + (name,))
            else:
                if file_id >= count or file_id in files:
                    raise ValueError("Invalid or duplicate file ID")
                files[file_id] = "/".join(prefix + (name,))
                file_id += 1

    walk(0xf000, ())
    result = []
    for file_id in sorted(files):
        start, end = struct.unpack_from("<II", data, fat_off + file_id * 8)
        if end < start or end > len(data):
            raise ValueError("Invalid FAT file extent")
        result.append({"id": file_id, "path": files[file_id], "size": end - start,
                       "offset": start})
    return result
