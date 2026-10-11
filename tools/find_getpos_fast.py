#!/usr/bin/env python3
import struct, mmap, collections
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
    names = mm[sh["off"]:sh["off"] + sh["size"]]
    for s in secs:
        s["name"] = names[s["no"]:names.find(b"\0", s["no"])].decode()
    by = {s["name"]: s for s in secs}
    text = mm[by[".text"]["off"]:by[".text"]["off"] + by[".text"]["size"]]
    ro = mm[by[".rodata"]["off"]:by[".rodata"]["off"] + by[".rodata"]["size"]]
    tv, rv = by[".text"]["addr"], by[".rodata"]["addr"]

    def cnt_imm(imm):
        c = 0
        for b0 in range(0xD0, 0xE0):
            for tail in (0x00, 0x10):
                c += text.count(bytes([b0, 0xF8, imm, tail]))
        return c

    print("=== LDR.W [R*,#imm] all regs ===")
    for imm in [0x08, 0x18, 0x1C, 0x20, 0x24, 0x38, 0x3C, 0x48, 0x50, 0x54, 0x60, 0x64, 0x78, 0x7C, 0x84]:
        c = cnt_imm(imm)
        if c:
            tag = ""
            if imm == 0x3C: tag = " pos"
            elif imm in (0x64, 0x38): tag = " transform internal"
            elif imm == 0x50: tag = " player field?"
            elif imm in (0x18, 0x1C, 0x20, 0x24): tag = " bone"
            print(f"  +0x{imm:02X}: {c}{tag}")

    # get_position string xref
    gp = rv + ro.find(b"get_position")
    sb = struct.pack("<I", gp)
    refs = []
    pos = 0
    while len(refs) < 8:
        p = text.find(sb, pos)
        if p < 0:
            break
        refs.append(tv + p)
        pos = p + 4
    print(f"\nget_position str 0x{gp:X} xrefs: {[hex(x) for x in refs]}")

    # near get_position icall: scan +/- 256 for LDR imms
    def imms_near(off, radius=256):
        lo = max(0, off - radius)
        hi = min(len(text), off + radius)
        imms = collections.Counter()
        for j in range(lo, hi - 3, 2):
            w = struct.unpack_from("<I", text, j)[0]
            if (w & 0xFF700000) == 0xF8000000:
                imms[w & 0xFFF] += 1
        return imms

    if refs:
        off = refs[0] - tv
        im = imms_near(off)
        print(f"  LDR imms near first xref (0x{refs[0]:X}):")
        for k, v in im.most_common(15):
            print(f"    +0x{k:03X}: {v}")

    # PlayerController string xref
    pc = rv + ro.find(b"PlayerController")
    sb2 = struct.pack("<I", pc)
    pos = 0
    refs2 = []
    while len(refs2) < 5:
        p = text.find(sb2, pos)
        if p < 0:
            break
        refs2.append(tv + p)
        pos = p + 4
    print(f"\nPlayerController str xrefs: {len(refs2)}")

    # chain pairs sampled every 4 bytes
    pairs = collections.Counter()
    for i in range(0, len(text) - 16, 4):
        w1 = struct.unpack_from("<I", text, i)[0]
        if (w1 & 0xFF700000) != 0xF8000000:
            continue
        imm1 = w1 & 0xFFF
        if imm1 not in range(0x40, 0x71):
            continue
        for j in (4, 8, 12, 16):
            if i + j + 4 > len(text):
                break
            w2 = struct.unpack_from("<I", text, i + j)[0]
            if (w2 & 0xFF700000) != 0xF8000000:
                continue
            imm2 = w2 & 0xFFF
            if imm2 in (0x38, 0x3C, 0x64):
                pairs[(imm1, imm2)] += 1
    print("\nplayer_field -> internal/pos chains:")
    for (a, b), c in pairs.most_common(15):
        print(f"  +0x{a:02X} -> +0x{b:02X}  {c}")

    mm.close()
