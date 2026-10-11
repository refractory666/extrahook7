#!/usr/bin/env python3
"""Fast Il2Cpp offset scan — seconds, not minutes."""
import struct, mmap, collections
from pathlib import Path

P = Path(r"d:\webb\Standoff2 0.39.0 x32 libunity.so")

def main():
    with open(P, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        data = mm

        e_shoff = struct.unpack_from("<I", data, 32)[0]
        e_shentsize = struct.unpack_from("<H", data, 46)[0]
        e_shnum = struct.unpack_from("<H", data, 48)[0]
        e_shstrndx = struct.unpack_from("<H", data, 50)[0]

        secs = []
        for i in range(e_shnum):
            o = e_shoff + i * e_shentsize
            s = struct.unpack_from("<IIIIIIIIII", data, o)
            secs.append({"no": s[0], "type": s[1], "addr": s[3], "off": s[4], "size": s[5]})
        shstr = secs[e_shstrndx]
        names = data[shstr["off"]:shstr["off"] + shstr["size"]]
        for s in secs:
            n = s["no"]
            s["name"] = names[n:names.find(b"\0", n)].decode()

        by = {s["name"]: s for s in secs}
        text = by[".text"]
        ro = by[".rodata"]
        dat = by[".data"]
        tb = data[text["off"]:text["off"] + text["size"]]
        rb = data[ro["off"]:ro["off"] + ro["size"]]
        db = data[dat["off"]:dat["off"] + dat["size"]]

        print(f"libunity.so | .text=0x{text['size']:X} .rodata=0x{ro['size']:X} .data=0x{dat['size']:X}")

        # 1) strings
        want = [b"PlayerManager", b"PhotonNetwork", b"BoltInventory", b"PlayerControls", b"6000."]
        print("\n--- strings ---")
        sv = {}
        for w in want:
            p = rb.find(w)
            if p >= 0:
                va = ro["addr"] + p
                sv[w.decode()] = va
                print(f"  {w.decode():16} @ 0x{va:X}")

        # 2) static_fields / parent — one aligned pass
        sf = collections.Counter()
        par = 0
        i, n = 0, len(tb) - 4
        while i < n:
            w = struct.unpack_from("<I", tb, i)[0]
            if (w & 0xFF70F000) == 0xF8008000:  # LDR.W Rt,[Rn,#imm]
                rt, rn, imm = (w >> 12) & 0xF, (w >> 16) & 0xF, w & 0xFFF
                if rt == rn and 0x28 <= imm <= 0x98 and (imm & 3) == 0:
                    sf[imm] += 1
                if imm == 0x2C:
                    par += 1
            i += 2
        print("\n--- Il2CppClass (same-reg LDR.W) ---")
        print(f"  parent +0x2C hits: {par}")
        print("  static_fields candidates:")
        for imm, c in sf.most_common(8):
            tag = " <<<<" if imm in (0x84, 0x88, 0x8C) else ""
            print(f"    +0x{imm:02X}  count={c}{tag}")

        # 3) TypeInfo: one pass .text -> set of pointers into .data, then scan .data only
        print("\n--- TypeInfo hunt ---")
        dva = dat["addr"]
        dhi = dva + dat["size"]
        refs = set()
        for j in range(0, len(tb) - 3, 4):
            v = struct.unpack_from("<I", tb, j)[0]
            if dva <= v < dhi:
                refs.add(v)

        def resolve_typeinfo(label):
            if label not in sv:
                print(f"\n  [{label}] string NOT FOUND")
                return
            str_va = sv[label]
            print(f"\n  [{label}] str=0x{str_va:X}")

            best = None
            for off in range(0, len(db) - 4, 4):
                slot = dva + off
                if slot not in refs:
                    continue
                klass = struct.unpack_from("<I", db, off)[0]
                if klass < 0x10000:
                    continue
                lb = struct.pack("<I", slot)
                hp = tb.find(lb)
                if hp < 0:
                    continue
                func = text["addr"] + hp
                lo = max(0, hp - 64)
                hi = min(len(tb), hp + 64)
                imms = []
                for j in range(lo, hi - 3, 2):
                    w = struct.unpack_from("<I", tb, j)[0]
                    if (w & 0xFF70F000) == 0xF8008000:
                        imms.append(w & 0xFFF)
                sf_off = next((x for x in (0x8C, 0x88, 0x84, 0x5C, 0x80) if x in imms), None)
                if not sf_off:
                    continue
                has_parent = 0x2C in imms
                inst = next((x for x in imms if x <= 0x40 and x not in (0x2C, sf_off)), 0)
                score = (3 if has_parent else 0) + (1 if inst == 0 else 0)
                cand = (score, off, slot, klass, func, sf_off, inst, has_parent)
                if best is None or cand[0] > best[0]:
                    best = cand
            if best:
                _, off, slot, klass, func, sf_off, inst, has_parent = best
                print(f"    TypeInfo RVA     0x{off:08X}")
                print(f"    TypeInfo abs     0x{slot:X}")
                print(f"    klass            0x{klass:X}")
                print(f"    get_Instance     0x{func:X}")
                print(f"    static_fields    klass+0x{sf_off:X}")
                print(f"    parent           klass+0x2C ({has_parent})")
                print(f"    instance field   static_fields+0x{inst:X}")
            else:
                print("    NOT FOUND (try metadata / runtime dump)")

        for cls in ("PlayerManager", "PhotonNetwork", "BoltInventory"):
            resolve_typeinfo(cls)

        mm.close()

if __name__ == "__main__":
    main()
