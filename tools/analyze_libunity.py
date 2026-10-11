#!/usr/bin/env python3
"""Analyze ARM32 libunity.so for Il2Cpp offsets (Standoff2)."""
import struct
import collections
from pathlib import Path

PATH = Path(r"d:\webb\Standoff2 0.39.0 x32 libunity.so")


def parse_elf(data: bytes):
    assert data[:4] == b"\x7fELF" and data[4] == 1
    e_shoff = struct.unpack_from("<I", data, 32)[0]
    e_shentsize = struct.unpack_from("<H", data, 46)[0]
    e_shnum = struct.unpack_from("<H", data, 48)[0]
    e_shstrndx = struct.unpack_from("<H", data, 50)[0]

    sections = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        sh = struct.unpack_from("<IIIIIIIIII", data, off)
        sections.append(
            {
                "i": i,
                "name_off": sh[0],
                "type": sh[1],
                "addr": sh[3],
                "off": sh[4],
                "size": sh[5],
            }
        )

    shstr = sections[e_shstrndx]
    shstr_data = data[shstr["off"] : shstr["off"] + shstr["size"]]

    def sec_name(s):
        n = s["name_off"]
        end = shstr_data.find(b"\0", n)
        return shstr_data[n:end].decode("ascii", "ignore")

    for s in sections:
        s["name"] = sec_name(s)
    return sections


def vaddr_to_file(sections, data, va):
    for s in sections:
        if s["type"] != 1:
            continue
        if s["addr"] <= va < s["addr"] + s["size"]:
            return s["off"] + (va - s["addr"])
    return None


def decode_ldr_imm32(h1: int, h2: int):
    w = (h1 << 16) | h2
    # LDR.W Rt, [Rn, #imm12]  F8D0 Rt00 / F890
    if (w & 0xFF700000) == 0xF8D00000:
        rt = (w >> 12) & 0xF
        rn = (w >> 16) & 0xF
        imm = w & 0xFFF
        return rt, rn, imm
    return None


def find_string_hits(blob: bytes, base_va: int, needles: list[bytes]):
    hits = {}
    for t in needles:
        idx = 0
        locs = []
        while True:
            p = blob.find(t, idx)
            if p < 0:
                break
            locs.append(base_va + p)
            idx = p + 1
        if locs:
            hits[t.decode()] = locs
    return hits


def find_literal_refs(text: bytes, text_va: int, target_va: int):
    tb = struct.pack("<I", target_va)
    refs = []
    pos = 0
    while True:
        p = text.find(tb, pos)
        if p < 0:
            break
        refs.append(text_va + p)
        pos = p + 1
    return refs


def analyze_get_instance_near(text: bytes, text_va: int, ref_va: int, window: int = 128):
    """Disassemble thumb window before literal ref for klass/static_fields chain."""
    off = ref_va - text_va
    start = max(0, off - window)
    end = min(len(text), off + 16)
    chain = []
    i = start
    while i < end - 3:
        h1 = struct.unpack_from("<H", text, i)[0]
        h2 = struct.unpack_from("<H", text, i + 2)[0]
        d = decode_ldr_imm32(h1, h2)
        if d:
            rt, rn, imm = d
            chain.append((text_va + i, rt, rn, imm))
            i += 4
            continue
        i += 2
    return chain


def find_typeinfo_candidates(data_sec: bytes, data_va: int, text: bytes, text_va: int):
    """Heuristic: .data qwords that are referenced from .text and point to in-range ptrs."""
    candidates = []
    for off in range(0, len(data_sec) - 8, 4):
        klass = struct.unpack_from("<I", data_sec, off)[0]
        if klass < 0x10000:
            continue
        # klass should point into module range
        if not (0x01000000 <= klass <= 0xA0000000):
            continue
        rva = data_va + off
        refs = find_literal_refs(text, text_va, rva)
        if not refs:
            continue
        candidates.append((rva, klass, len(refs)))
    candidates.sort(key=lambda x: -x[2])
    return candidates[:200]


def main():
    data = PATH.read_bytes()
    sections = parse_elf(data)
    by_name = {s["name"]: s for s in sections}

    print(f"File: {PATH.name} ({len(data)} bytes)")
    for n in [".text", ".rodata", ".data", ".bss"]:
        if n in by_name:
            s = by_name[n]
            print(f"  {n}: vaddr=0x{s['addr']:X} file=0x{s['off']:X} size=0x{s['size']:X}")

    text_s = by_name[".text"]
    ro_s = by_name[".rodata"]
    data_s = by_name[".data"]
    text = data[text_s["off"] : text_s["off"] + text_s["size"]]
    rodata = data[ro_s["off"] : ro_s["off"] + ro_s["size"]]
    data_sec = data[data_s["off"] : data_s["off"] + data_s["size"]]

    needles = [
        b"PlayerManager",
        b"PhotonNetwork",
        b"BoltInventory",
        b"PlayerControls",
        b"NetworkingPeer",
        b"get_Instance",
        b"Assembly-CSharp",
        b"Axlebolt.Standoff",
        b"Standoff",
    ]
    hits = find_string_hits(rodata, ro_s["addr"], needles)
    print("\n.rodata strings:")
    for k, v in hits.items():
        print(f"  {k}: {len(v)} @ {[hex(x) for x in v[:6]]}")

    # Pattern scan static_fields offsets
    patterns = {
        0x2C: bytes.fromhex("2C 00"),  # partial
        0x5C: bytes.fromhex("5C 00"),
        0x84: bytes.fromhex("84 00"),
        0x88: bytes.fromhex("88 00"),
        0x8C: bytes.fromhex("8C 00"),
    }
    print("\nLDR [R0,#imm] thumb tail counts (F8 ?? imm low byte):")
    for imm, tail in patterns.items():
        pat = bytes([0x00, tail[0]])  # xx 8C in LE halfwords -> search 8C 00 as halfword pair in stream
        # search F8D0 008C style: bytes D0 F8 8C 00 and variants with other regs
        count = 0
        for reg in range(16):
            b = bytes([0xD0 | (reg & 1), 0xF8, imm & 0xFF, (0x00 if reg < 8 else 0x10) | ((reg >> 1) & 0xF)])
            # simpler: count imm byte after F8 D? pattern
        count = text.count(bytes([imm, 0x00]))
        print(f"  imm 0x{imm:02X} byte tail: {count}")

    # Better: count LDR.W same register patterns for static_fields candidates
    off_counts = collections.Counter()
    parent_counts = collections.Counter()
    i = 0
    while i < len(text) - 4:
        h1 = struct.unpack_from("<H", text, i)[0]
        h2 = struct.unpack_from("<H", text, i + 2)[0]
        d = decode_ldr_imm32(h1, h2)
        if d:
            rt, rn, imm = d
            if rt == rn and 0x20 <= imm <= 0xB0:
                off_counts[imm] += 1
            if imm == 0x2C:
                parent_counts[rn] += 1
        i += 2

    print("\nTop LDR.W [Rn,#imm] same-reg (static_fields candidates):")
    for off, cnt in off_counts.most_common(12):
        print(f"  +0x{off:03X}: {cnt}")

    # Analyze get_Instance functions referencing known strings
    for cls in ["PlayerManager", "PhotonNetwork", "BoltInventory"]:
        if cls not in hits:
            continue
        for sva in hits[cls][:3]:
            refs = find_literal_refs(text, text_s["addr"], sva)
            print(f"\n=== {cls} string 0x{sva:X}, literal refs in code: {len(refs)} ===")
            for ref in refs[:5]:
                chain = analyze_get_instance_near(text, text_s["addr"], ref)
                ldr_ops = [(hex(a), f"R{rt}", f"R{rn}", hex(imm)) for a, rt, rn, imm in chain[-8:]]
                print(f"  near ref 0x{ref:X}:")
                for row in ldr_ops:
                    print(f"    {row}")

    # Find TypeInfo globals referenced from code (high ref count .data slots)
    print("\n=== TypeInfo candidates in .data (referenced from .text) ===")
    cands = find_typeinfo_candidates(data_sec, data_s["addr"], text, text_s["addr"])
    print(f"Found {len(cands)} candidates (top 30 by xref count):")
    for rva, klass, nrefs in cands[:30]:
        print(f"  TypeInfo RVA 0x{rva - data_s['addr']:08X} abs=0x{rva:X} klass=0x{klass:X} xrefs={nrefs}")

    # Search metadata magic
    mp = data.find(b"\xAF\x1B\xB1\xFA")
    print(f"\nglobal-metadata magic: {'0x'+hex(mp)[2:].upper() if mp >= 0 else 'NOT FOUND'}")

    # il2cpp exports strings
    print("\nil2cpp symbol strings:")
    for sym in [
        "il2cpp_domain_get",
        "il2cpp_class_from_name",
        "il2cpp_runtime_class_init",
        "il2cpp_codegen_register",
        "s_GlobalMetadata",
    ]:
        b = sym.encode()
        p = data.find(b)
        print(f"  {sym}: {'0x'+hex(p)[2:].upper() if p >= 0 else 'NOT FOUND'}")


if __name__ == "__main__":
    main()
