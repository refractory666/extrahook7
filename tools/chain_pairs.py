import mmap, struct, collections
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
        secs.append({"no": s[0], "off": s[4], "size": s[5]})
    sh = secs[sx]
    names = mm[sh["off"] : sh["off"] + sh["size"]]
    for s in secs:
        s["name"] = names[s["no"] : names.find(b"\0", s["no"])].decode()
    by = {s["name"]: s for s in secs}
    text = mm[by[".text"]["off"] : by[".text"]["off"] + by[".text"]["size"]]

    pairs = collections.Counter()
    for i in range(len(text) - 4):
        if text[i + 1] != 0xF8:
            continue
        imm2 = text[i + 2]
        if imm2 not in (0x38, 0x3C, 0x64):
            continue
        win = text[max(0, i - 24) : i]
        for j in range(len(win) - 2):
            if win[j + 1] == 0xF8:
                imm1 = win[j + 2]
                if 0x40 <= imm1 <= 0x70:
                    pairs[(imm1, imm2)] += 1

    print("Chain pairs (any reg, 24 byte window):")
    for k, v in pairs.most_common(20):
        note = ""
        if k == (0x50, 0x64) or k == (0x50, 0x38):
            note = " old Transform path"
        if k[1] == 0x3C:
            note = " -> POSITION"
        print(f"  +0x{k[0]:02X} -> +0x{k[1]:02X}  {v}{note}")

    bone = collections.Counter()
    for i in range(len(text) - 4):
        if text[i + 1] == 0xF8 and text[i + 2] in (0x18, 0x1C, 0x20, 0x24):
            win = text[max(0, i - 12) : i]
            if b"\xF8\x08" in win:
                bone[text[i + 2]] += 1
    print("\nBone chain after +0x08:")
    for k, v in bone.most_common():
        print(f"  +0x{k:02X}: {v}")

    mm.close()
