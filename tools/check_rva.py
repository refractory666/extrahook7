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
        secs.append({"no": s[0], "type": s[1], "addr": s[3], "off": s[4], "size": s[5]})
    sh = secs[sx]
    names = mm[sh["off"] : sh["off"] + sh["size"]]
    for s in secs:
        s["name"] = names[s["no"] : names.find(b"\0", s["no"])].decode()

    print("Program sections:")
    for s in secs:
        if s["type"] == 1:
            print(f"  {s['name']:16} vaddr=0x{s['addr']:08X} size=0x{s['size']:X}")

    old = 0x90E4BC4
    for s in secs:
        if s["type"] == 1 and s["addr"] <= old < s["addr"] + s["size"]:
            fo = s["off"] + (old - s["addr"])
            val = struct.unpack_from("<I", mm, fo)[0]
            print(f"Old PM RVA 0x{old:X} -> ptr 0x{val:X} in {s['name']}")

    # PhotonNetwork old 0x80F63D4
    for name, rva in [("PlayerManager", 0x90E4BC4), ("PhotonNetwork", 0x80F63D4), ("BoltInventory", 0x80F2860)]:
        for s in secs:
            if s["type"] == 1 and s["addr"] <= rva < s["addr"] + s["size"]:
                fo = s["off"] + (rva - s["addr"])
                val = struct.unpack_from("<I", mm, fo)[0]
                print(f"Old {name} 0x{rva:X} -> klass 0x{val:X}")
                break
        else:
            print(f"Old {name} 0x{rva:X} -> OUT OF RANGE for this build")

    mm.close()
