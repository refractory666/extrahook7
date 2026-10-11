import struct, mmap
from pathlib import Path

P = Path(r"d:\webb\Standoff2 0.39.0 x32 libunity.so")
with open(P, "rb") as f:
    mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    e_shoff = struct.unpack_from("<I", mm, 32)[0]
    es = struct.unpack_from("<H", mm, 46)[0]
    n = struct.unpack_from("<H", mm, 48)[0]
    sx = struct.unpack_from("<H", mm, 50)[0]
    secs = []
    for i in range(n):
        o = e_shoff + i * es
        s = struct.unpack_from("<IIIIIIIIII", mm, o)
        secs.append({"no": s[0], "addr": s[3], "off": s[4], "size": s[5]})
    sh = secs[sx]
    names = mm[sh["off"] : sh["off"] + sh["size"]]
    for s in secs:
        s["name"] = names[s["no"] : names.find(b"\0", s["no"])].decode()
    by = {s["name"]: s for s in secs}
    ro = mm[by[".rodata"]["off"] : by[".rodata"]["off"] + by[".rodata"]["size"]]
    rv = by[".rodata"]["addr"]
    text = mm[by[".text"]["off"] : by[".text"]["off"] + by[".text"]["size"]]
    tv = by[".text"]["addr"]

    print("=== PlayerController field names ===")
    needles = [
        b"<Transform>k__BackingField",
        b"_transform",
        b"movementController",
        b"MovementController",
        b"_movementController",
        b"playerCharacterView",
        b"PlayerCharacterView",
        b"translationData",
        b"TranslationData",
        b"transform",
    ]
    for s in needles:
        i = 0
        hits = []
        while len(hits) < 3:
            p = ro.find(s, i)
            if p < 0:
                break
            hits.append(rv + p)
            i = p + 1
        if hits:
            print(f"  {s.decode():35} {[hex(x) for x in hits]}")

    # Find icall Transform_get_position by searching code pattern from Unity:
    # often reads transform internal at +0x38 or uses TransformAccess
    print("\n=== Consecutive pos floats LDR [base,#0x3C/0x40/0x44] ===")
    import collections
    tri = collections.Counter()
    for i in range(0, len(text) - 16, 4):
        imms = []
        for j in (0, 4, 8):
            w = struct.unpack_from("<I", text, i + j)[0]
            if (w & 0xFF700000) == 0xF8000000:
                imms.append(w & 0xFFF)
        if len(imms) >= 3 and imms[0] in (0x3C, 0x38) and imms[1] == imms[0] + 4 and imms[2] == imms[0] + 8:
            tri[imms[0]] += 1
    for k, v in tri.most_common(5):
        print(f"  Vector3 at +0x{k:X}: {v}")

    # Strong chain: [x,#0x50] then [y,#0x64 or #0x38] then [z,#0x3C]
    print("\n=== Full get-pos chain candidates ===")
    chains = collections.Counter()
    for i in range(0, len(text) - 24, 4):
        seq = []
        for j in range(0, 24, 4):
            w = struct.unpack_from("<I", text, i + j)[0]
            if (w & 0xFF700000) == 0xF8000000:
                seq.append(w & 0xFFF)
        if len(seq) < 3:
            continue
        if seq[0] in (0x48, 0x50, 0x54, 0x58) and seq[1] in (0x38, 0x64, 0x68) and seq[2] == 0x3C:
            chains[(seq[0], seq[1], seq[2])] += 1
    for k, v in chains.most_common(10):
        print(f"  +0x{k[0]:02X} -> +0x{k[1]:02X} -> +0x{k[2]:02X}  count={v}")

    mm.close()
