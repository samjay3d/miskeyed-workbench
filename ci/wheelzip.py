"""CI helper: inspect or normalize a wheel's ZIP container for PyPI compliance.

PyPI's upload validator rejects ZIPs that use "unnecessary/uncommon" archive
features (data descriptors, trailing/prepended data, duplicate/mis-ordered central
directory, ZIP64 when not required). Some build/CI paths emit those; this tool

  inspect  <whl...>   -> print structure + flag any non-compliant features
  normalize <whl...>  -> rewrite each wheel as a canonical, seekable ZIP in place

Normalize copies every member's bytes verbatim (so RECORD hashes stay valid) into a
fresh archive with clean ZipInfo — no data descriptors, no extra fields, one central
directory, no trailing data.
"""
from __future__ import annotations

import os
import struct
import sys
import zipfile

SIG_LOCAL = 0x04034B50
SIG_CENTRAL = 0x02014B50
SIG_EOCD = 0x06054B50
SIG_DATADESC = 0x08074B50
SIG_ZIP64_EOCD = 0x06064B50
SIG_ZIP64_LOC = 0x07064B50


def inspect(path: str) -> bool:
    """Return True if the archive looks PyPI-compliant, else False (and print why)."""
    data = open(path, "rb").read()
    ok = True
    print(f"[inspect] {os.path.basename(path)}  size={len(data)}")

    with zipfile.ZipFile(path) as zf:
        infos = zf.infolist()
        dd = [i.filename for i in infos if i.flag_bits & 0x08]
        z64 = [i.filename for i in infos if i.compress_size >= 0xFFFFFFFF or i.file_size >= 0xFFFFFFFF]
        names = [i.filename for i in infos]
        dupes = sorted({n for n in names if names.count(n) > 1})
        print(f"          entries={len(infos)} data_descriptor={len(dd)} zip64={len(z64)} duplicates={len(dupes)}")
        if dd:
            ok = False
            print(f"          !! data-descriptor entries: {dd[:5]}")
        if dupes:
            ok = False
            print(f"          !! duplicate names: {dupes[:5]}")

    # Sequential raw scan: PyPI walks records this way and rejects on the first
    # unrecognized signature or leftover bytes before the central directory.
    off = n = 0
    while off + 4 <= len(data):
        sig = struct.unpack_from("<I", data, off)[0]
        if sig == SIG_LOCAL:
            n += 1
            (_, _, flags, _, _, _, _, csize, _, nlen, elen) = struct.unpack_from("<IHHHHHIIIHH", data, off)
            off += 30 + nlen + elen
            if flags & 0x08:
                p = data.find(struct.pack("<I", SIG_DATADESC), off)
                if p == -1:
                    print(f"          !! data-descriptor flag but no descriptor near {off}")
                    return False
                off = p + 16
            else:
                off += csize
        elif sig in (SIG_CENTRAL, SIG_ZIP64_EOCD, SIG_ZIP64_LOC, SIG_EOCD):
            break
        else:
            print(f"          !! UNKNOWN RECORD SIGNATURE 0x{sig:08X} at offset {off} (after {n} locals)")
            print(f"             context: {data[off:off+16].hex()}")
            return False
    print(f"          raw scan: {'OK' if ok else 'ISSUES ABOVE'} ({n} local records)")
    return ok


def normalize(path: str) -> None:
    tmp = path + ".norm"
    with zipfile.ZipFile(path) as zin, zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as zout:
        seen = set()
        for item in zin.infolist():
            if item.filename in seen:
                continue  # drop accidental duplicates; PyPI rejects them
            seen.add(item.filename)
            info = zipfile.ZipInfo(item.filename, date_time=item.date_time)
            info.external_attr = item.external_attr
            info.internal_attr = item.internal_attr
            info.create_system = item.create_system
            if item.is_dir():
                info.compress_type = zipfile.ZIP_STORED
                zout.writestr(info, b"")
            else:
                info.compress_type = item.compress_type
                zout.writestr(info, zin.read(item.filename))
    os.replace(tmp, path)
    print(f"[normalize] rewrote {os.path.basename(path)}")


def main(argv: list[str]) -> int:
    if len(argv) < 3 or argv[1] not in ("inspect", "normalize"):
        print(__doc__)
        return 64
    cmd, paths = argv[1], argv[2:]
    if cmd == "inspect":
        return 0 if all(inspect(p) for p in paths) else 1
    for p in paths:
        normalize(p)
        inspect(p)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
