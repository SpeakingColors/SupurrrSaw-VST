#!/usr/bin/env python3
"""Adds the instruments of a pack folder (instruments.json + FLAC files, from cut_sampled_bars.py) to an existing assets/instruments.bin.
Instruments with a name that is already in the bin are skipped.
usage: append_instruments.py IN.bin PACK_DIR OUT.bin"""
import sys, os, json, struct

def main():
    src, pack, out = sys.argv[1:4]
    d = open(src, "rb").read()
    assert d[:4] == b"S87I"
    jl = struct.unpack_from("<I", d, 8)[0]
    J = json.loads(d[12:12 + jl])
    pos = 12 + jl
    blobs = []
    for inst in J["instruments"]:
        for _ in inst["zones"]:
            n = struct.unpack_from("<I", d, pos)[0]; pos += 4
            blobs.append(d[pos:pos + n]); pos += n
    have = {i["name"] for i in J["instruments"]}
    P = json.load(open(os.path.join(pack, "instruments.json")))
    added = 0
    for inst in P["instruments"]:
        if inst["name"] in have:
            print("skip (exists):", inst["name"]); continue
        J["instruments"].append(inst); added += 1
        for z in inst["zones"]:
            blobs.append(open(os.path.join(pack, z["file"]), "rb").read())
    js = json.dumps(J, separators=(",", ":")).encode()
    with open(out, "wb") as f:
        f.write(b"S87I" + struct.pack("<II", 1, len(js)) + js)
        for b in blobs:
            f.write(struct.pack("<I", len(b)) + b)
    print(added, "instruments added,", len(blobs), "files,", os.path.getsize(out) / 1e6, "MB")

if __name__ == "__main__":
    main()
