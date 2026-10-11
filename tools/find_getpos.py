#!/usr/bin/env python3
"""Find Transform / get-position offsets in libunity.so (ARM32)."""
import struct
import mmap
import collections
from pathlib import Path

P = Path(r"d:\webb\Standoff2 0.39.0 x32 libunity.so")


def parse_elf(mm):
    e_shoff = struct.unpack_from("<I", mm, 32)[0]
    es = struct.unpack_from("<H", mm, 46)[0]
    n = struct.unpack_from("<H", mm, 48)[0]
    sx = struct.unpack_from("<H", mm, 50)[0]
    secs = []
    for i in range(n):
        o = e_shoff + i * es
        s = struct.unpack_from("<IIIIIIIIII", mm, o)
        secs.append({"no": s[0], "type": s[1], "addr": s[3], "off": s[4], "size": s[5]})
    sh = secs[sx]
    names = mm[sh["off"] : sh["off"] + sh["size"]]
    for s in secs:
        s["name"] = names[s["no"] : names.find(b"\0", s["no"])].decode()
    return secs


def ldr_w_at(b, i):
    if i + 3 >= len(b):
        return None
    w = struct.unpack_from("<I", b, i)[0]
    if (w & 0xFF700000) == 0xF8000000:
        return (w >> 12) & 0xF, (w >> 16) & 0xF, w & 0xFFF
    return None


def main():
    with open(P, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        secs = parse_elf(mm)
        by = {s["name"]: s for s in secs}
        text = bytes(mm[by[".text"]["off"] : by[".text"]["off"] + by[".text"]["size"]])
        ro = bytes(mm[by[".rodata"]["off"] : by[".rodata"]["off"] + by[".rodata"]["size"]])
        tv = by[".text"]["addr"]
        rv = by[".rodata"]["addr"]

        print("=== Unity / Transform strings ===")
        for needle in [
            b"get_position",
            b"get_position_Injected",
            b"Transform::get_position",
            b"TransformAccess",
            b"TransformHierarchy",
            b"localPosition",
            b"PlayerController",
            b"MovementController",
            b"TranslationData",
            b"PlayerTranslationData",
        ]:
            hits = []
            i = 0
            while len(hits) < 3:
                p = ro.find(needle, i)
                if p < 0:
                    p = mm.find(needle, i)
                    base = 0 if p < 0 or p >= by[".rodata"]["off"] else rv
                if p < 0:
                    break
                if p >= by[".rodata"]["off"] and p < by[".rodata"]["off"] + by[".rodata"]["size"]:
                    hits.append(rv + (p - by[".rodata"]["off"]))
                else:
                    hits.append(p)
                i = (p - by[".rodata"]["off"] + 1) if p >= by[".rodata"]["off"] else p + 1
            if hits:
                print(f"  {needle.decode():30} {[hex(x) for x in hits]}")

        # Pattern: read ptr at [R0,#A], then read 3 floats at [R1,#B] — world pos chain
        # LDR R1,[R0,#imm1]  then  LDR.W R2,[R1,#imm2] x3 or VLD
        print("\n=== Transform internal chain candidates ===")
        # Common: LDR Rt,[Rn,#imm] where next is LDR [Rt,#0x3C] or VLDR from +0x3C
        chain_ctr = collections.Counter()
        chain_examples = {}
        for i in range(0, len(text) - 12, 2):
            d1 = ldr_w_at(text, i)
            if not d1:
                continue
            rt1, rn1, imm1 = d1
            for j in range(i + 4, i + 16, 2):
                d2 = ldr_w_at(text, j)
                if not d2:
                    continue
                rt2, rn2, imm2 = d2
                if rn2 == rt1 and imm2 in (0x30, 0x34, 0x38, 0x3C, 0x40, 0x44, 0x48):
                    key = (imm1, imm2)
                    chain_ctr[key] += 1
                    if key not in chain_examples:
                        chain_examples[key] = tv + i
        print("  ptr_field -> pos_field (count, example func):")
        for (imm1, imm2), c in chain_ctr.most_common(20):
            ex = chain_examples[(imm1, imm2)]
            mark = " <<" if imm2 == 0x3C else ""
            print(f"    player/transform+0x{imm1:02X} -> internal+0x{imm2:02X}  count={c}  ex=0x{ex:X}{mark}")

        # TransformBone chain: trans+0x8 -> obj, obj+0x20 matrix, obj+0x24 index, matrix+0x18 list
        print("\n=== TransformBone hierarchy (get_transform_info) ===")
        bone_chains = collections.Counter()
        for i in range(0, len(text) - 24, 2):
            ops = []
            for j in range(i, i + 24, 2):
                d = ldr_w_at(text, j)
                if d:
                    ops.append(d[2])
            if len(ops) < 3:
                continue
            # look for 0x8, 0x20/0x24, 0x18/0x1C sequence
            if 0x8 in ops and any(x in ops for x in (0x20, 0x24)) and any(x in ops for x in (0x18, 0x1C)):
                bone_chains[tuple(ops[:5])] += 1
        # simpler: count fixed patterns
        pats = {
            "TransObj+0x8": bytes([0xD0, 0xF8, 0x08, 0x00]),
            "Matrix+0x20": bytes([0xD0, 0xF8, 0x20, 0x00]),
            "Index+0x24": bytes([0xD0, 0xF8, 0x24, 0x00]),
            "List+0x18": bytes([0xD0, 0xF8, 0x18, 0x00]),
            "Indices+0x1C": bytes([0xD0, 0xF8, 0x1C, 0x00]),
            "Internal1+0x64": bytes([0xD0, 0xF8, 0x64, 0x00]),
            "Internal2+0x38": bytes([0xD0, 0xF8, 0x38, 0x00]),
            "Position+0x3C": bytes([0xD0, 0xF8, 0x3C, 0x00]),
        }
        for name, pat in pats.items():
            print(f"  LDR [R0,{name.split('+')[1]}]: {text.count(pat)}")

        # PlayerController Transform field: code reads [player,#X] then transform chain
        # Search functions with LDR [R?,#imm_player] where imm in 0x40-0x60 followed by chain to 0x3C
        print("\n=== PlayerController Transform field candidates ===")
        player_imm = collections.Counter()
        for i in range(0, len(text) - 32, 2):
            d0 = ldr_w_at(text, i)
            if not d0 or not (0x40 <= d0[2] <= 0x70):
                continue
            # within 20 bytes: another ldr using result reg, then ldr #0x3C
            rt0 = d0[0]
            found_3c = False
            for j in range(i + 4, i + 28, 2):
                d = ldr_w_at(text, j)
                if d and d[2] == 0x3C:
                    found_3c = True
                    break
            if found_3c:
                player_imm[d0[2]] += 1
        for imm, c in player_imm.most_common(12):
            mark = " <<" if imm == 0x50 else ""
            print(f"  PlayerController+0x{imm:02X} -> ... -> +0x3C  count={c}{mark}")

        # MovementController path: player+MC -> trans_data -> vector
        print("\n=== MovementController / TranslationData path ===")
        mc_pats = [(0x48, 0x64, 0x60), (0x50, 0x64, 0x60), (0x50, 0x60, 0x60)]
        for p_off, mc_off, td_off in mc_pats:
            # count proximity: LDR #p_off then within 40 bytes LDR #mc_off and LDR #td_off
            c = 0
            for i in range(0, len(text) - 48, 2):
                d = ldr_w_at(text, i)
                if not d or d[2] != p_off:
                    continue
                win = text[i : i + 48]
                has_mc = any(win.find(bytes([0xD0, 0xF8, mc_off, 0x00])) >= 0 for _ in [0])
                has_td = bytes([0xD0, 0xF8, td_off, 0x00]) in win
                if has_mc and has_td:
                    c += 1
            if c:
                print(f"  player+0x{p_off:X} -> MC+0x{mc_off:X} -> data+0x{td_off:X} nearby: {c}")

        mm.close()


if __name__ == "__main__":
    main()
