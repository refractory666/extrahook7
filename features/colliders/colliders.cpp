#include "colliders.h"
#include "map_service.h"
#include "console.h"
#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace colliders {

	static bool LoadOne(int index, const char* name) {
		(void)index;
		std::ifstream file(std::string("maps/") + name + ".glb", std::ios::binary | std::ios::ate);
		if (!file) {
			Console::Log("[-] colliders: cannot open maps/%s.glb", name);
			return false;
		}
		const std::streamsize size = file.tellg();
		if (size <= 0) return false;
		file.seekg(0, std::ios::beg);
		std::vector<unsigned char> data(static_cast<size_t>(size));
		if (!file.read(reinterpret_cast<char*>(data.data()), size)) return false;
		if (!map_service::LoadMapFromMemory(data.data(), data.size())) return false;
		Console::Log("[+] colliders: loaded maps/%s.glb", name);
		return true;
	}

	bool LoadEmbeddedByIndex(int index) {
		switch (index) {
		case 0: return LoadOne(0, "sandstone");
		case 1: return LoadOne(1, "province");
		case 2: return LoadOne(2, "dune");
		case 3: return LoadOne(3, "rust");
		case 4: return LoadOne(4, "zone7");
		case 5: return LoadOne(5, "breeze");
		case 6: return LoadOne(6, "hanami");
		case 7: return LoadOne(7, "calypso");
		case 8: return LoadOne(8, "hanari");
		case 9: return LoadOne(9, "favelas");
		case 10: return LoadOne(10, "lakeside");
		case 11: return LoadOne(11, "arena");
		default: return false;
		}
	}

}
