import struct, mmap
from pathlib import Path

P = Path(r"d:\webb\Standoff2 0.39.0 x32 libunity.so")
with open(P, "rb") as f:
    mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    for name, t in [
        ("PlayerManager", 0xDB1B3F),
        ("PhotonNetwork", 0x1040C0D),
        ("BoltInventory", 0xD9A4D3),
    ]:
        b = struct.pack("<I", t)
        pos = 0
        refs = []
        while len(refs) < 10:
            i = mm.find(b, pos)
            if i < 0:
                break
            refs.append(i)
            pos = i + 1
        print(f"{name} @ 0x{t:X}: {len(refs)} ptr refs in file, first={[hex(x) for x in refs[:5]]}")
    mm.close()
