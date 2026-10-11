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
    names = mm[sh["off"] : sh["off"] + sh["size"]]
    for s in secs:
        s["name"] = names[s["no"] : names.find(b"\0", s["no"])].decode()
    by = {s["name"]: s for s in secs}
    ro = mm[by[".rodata"]["off"] : by[".rodata"]["off"] + by[".rodata"]["size"]]
    rv = by[".rodata"]["addr"]
    text = mm[by[".text"]["off"] : by[".text"]["off"] + by[".text"]["size"]]
    tv = by[".text"]["addr"]

    getters = [
        b"get_transform",
        b"get_Transform",
        b"get_movementController",
        b"get_MovementController",
        b"get_translationData",
        b"get_TranslationData",
        b"get_position",
        b"get_Position",
    ]
    print("=== Property getters in rodata ===")
    for g in getters:
        p = ro.find(g)
        print(f"  {g.decode():30} {hex(rv+p) if p>=0 else 'NO'}")

    # Thumb1 LDR Rt,[Rn,#imm*4] for small offsets - encodes player fields
    # Count halfword LDR with Rn=0 (this) imm5*4
    print("\n=== Thumb1 LDR [R0,#off] (simple getters) ===")
    t1 = collections.Counter()
    for i in range(0, len(text) - 2, 2):
        h = struct.unpack_from("<H", text, i)[0]
        # 01101 imm5 Rn Rt  -> STR/LDR immediate
        if (h & 0xF800) == 0x6800:  # LDR Rt,[Rn,#imm5*4] actually 01001xx
            pass
        # LDR Rt, [Rn, #imm5*4]: format 01101xx - 0x6800 base for Rt=0,Rn=0,imm=0
        # Actually: bits 15-11=01101 for LDR
        if (h & 0xF800) == 0x6800:
            imm = ((h >> 6) & 0x1F) * 4
            rt = h & 0x7
            rn = (h >> 3) & 0x7
            if rn == 0 and rt == 0 and 0x40 <= imm <= 0x80:
                t1[imm] += 1
    for imm, c in t1.most_common(15):
        print(f"  +0x{imm:02X}: {c}")

    # Search 16-bit LDR R0,[R0,#imm] pattern: 0x6??0 range
    t1b = collections.Counter()
    for i in range(0, len(text) - 2, 2):
        h = struct.unpack_from("<H", text, i)[0]
        if (h & 0xFF00) == 0x6800:  # LDR R0,[R0,#imm*4] when rt=0,rn=0? 6800 + imm<<6
            imm = ((h >> 6) & 0x1F) * 4
            if 0x40 <= imm <= 0x90:
                t1b[imm] += 1
        elif (h & 0xF800) == 0x5800:  # LDR R0,[R0,#imm*4] alternate encoding
            imm = ((h >> 6) & 0x1F) * 4
            if 0x40 <= imm <= 0x90:
                t1b[imm] += 1
    print("\n=== LDR R0,[R0,#imm] thumb16 approx ===")
    for imm, c in t1b.most_common(15):
        tag = " <<" if imm == 0x50 else ""
        print(f"  +0x{imm:02X}: {c}{tag}")

    mm.close()
