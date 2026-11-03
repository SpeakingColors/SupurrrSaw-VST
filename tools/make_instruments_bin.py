#!/usr/bin/env python3
"""Builds a stand-alone pack file (same S87I format as assets/instruments.bin) from a pack folder (instruments.json + FLAC files).
Used to split a large pack into several files so each stays under GitHub's 100 MB file limit.
usage: make_instruments_bin.py PACK_DIR OUT.bin FIRST COUNT   (instruments FIRST .. FIRST+COUNT-1 of the pack, in json order)"""
import sys, os, json, struct

def main():
    pack, out, first, count = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
    P = json.load(open(os.path.join(pack, "instruments.json")))
    insts = P["instruments"][first:first + count]
    blobs = []
    for inst in insts:
        for z in inst["zones"]:
            blobs.append(open(os.path.join(pack, z["file"]), "rb").read())
            z.pop("file", None)
    J = {"version": 1, "credit": P.get("credit", ""), "instruments": insts}
    js = json.dumps(J, separators=(",", ":")).encode()
    with open(out, "wb") as f:
        f.write(b"S87I" + struct.pack("<II", 1, len(js)) + js)
        for b in blobs:
            f.write(struct.pack("<I", len(b)) + b)
    print(len(insts), "instruments,", len(blobs), "files,", round(os.path.getsize(out) / 1e6, 1), "MB")

if __name__ == "__main__":
    main()
