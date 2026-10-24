#!/usr/bin/env python3
"""Packs an instrument folder (instruments.json + FLAC files, made by make_instruments.py) into one file,
assets/instruments.bin, that is compiled into the plugin.
Format (little endian): "S87I", u32 version=1, u32 jsonLen, json bytes, then for every zone in file order: u32 len, bytes.
The json keeps the zone "file" names; the loader finds each file by its position in the same order as the json lists them.
usage: pack_instruments.py PACK_DIR OUT.bin"""
import sys, os, json, struct

def main():
    src, out = sys.argv[1:3]
    J = json.load(open(os.path.join(src, "instruments.json")))
    blobs = []
    for inst in J["instruments"]:
        for z in inst["zones"]:
            blobs.append(open(os.path.join(src, z["file"]), "rb").read())
    js = json.dumps(J, separators=(",", ":")).encode()
    with open(out, "wb") as f:
        f.write(b"S87I" + struct.pack("<II", 1, len(js)) + js)
        for b in blobs:
            f.write(struct.pack("<I", len(b)) + b)
    print(len(blobs), "files,", os.path.getsize(out) / 1e6, "MB")

if __name__ == "__main__":
    main()
