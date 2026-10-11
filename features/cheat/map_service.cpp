#include "map_service.h"
#include "colliders.h"
#include "console.h"
#include "imgui.h"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <unordered_set>
#include <windows.h>
#include <cctype>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#include "tiny_gltf.h"

namespace ms = map_service_internal;

static const char* s_collider_map_names[] = {
	"sandstone_colliders.glb", "province_colliders.glb", "dune_colliders.glb",
	"rust_colliders.glb", "zone7_colliders.glb", "breeze_colliders.glb", "hanami_colliders.glb", "calypso_colliders.glb",
	"hanari_colliders.glb", "favelas_colliders.glb", "lakeside_colliders.glb", "arena_colliders.glb",
	"cobleway_colliders.glb", "pipeline_colliders.glb", "bridge.glb", "yard.glb"
};
static const char* s_collider_display_names[] = {
	"sandstone", "province", "dune", "rust", "zone7", "breeze", "hanami", "calypso",
	"hanari", "favelas", "lakeside", "arena", "cobleway", "pipeline", "bridge", "yard"
};
const char* map_service::g_collider_map_names[] = {
	"sandstone_colliders.glb", "province_colliders.glb", "dune_colliders.glb",
	"rust_colliders.glb", "zone7_colliders.glb", "breeze_colliders.glb", "hanami_colliders.glb", "calypso_colliders.glb",
	"hanari_colliders.glb", "favelas_colliders.glb", "lakeside_colliders.glb", "arena_colliders.glb",
	"cobleway_colliders.glb", "pipeline_colliders.glb", "bridge.glb", "yard.glb"
};
const char* map_service::g_collider_map_display_names[] = {
	"sandstone", "province", "dune", "rust", "zone7", "breeze", "hanami", "calypso",
	"hanari", "favelas", "lakeside", "arena", "cobleway", "pipeline", "bridge", "yard"
};

int map_service::g_collider_map_count = (int)
(sizeof(s_collider_map_names) / sizeof(s_collider_map_names[0]));
int map_service::g_collider_map_index = 0;

void map_service::LoadColliderMapByIndex(int index) {
	if (index < 0 || index >= g_collider_map_count) return;
	g_loaded = false;

	if (index >= 0 && index <= 11 && colliders::LoadEmbeddedByIndex(index))
		return;

	const char* name = g_collider_map_names[index];
	std::string pathC = std::string("C:\\") + name;
	if (LoadMap(pathC)) {
		Console::Log("[+] colliders: loaded from %s", pathC.c_str());
		return;
	}
	char exePath[MAX_PATH] = {};
	if (GetModuleFileNameA(NULL, exePath, MAX_PATH)) {
		std::string dir(exePath);
		size_t last = dir.find_last_of("\\/");
		if (last != std::string::npos) dir.resize(last + 1);
		std::string path = dir + name;
		if (LoadMap(path)) {
			Console::Log("[+] colliders: loaded from %s", path.c_str());
			return;
		}
	}
	Console::Log("[-] colliders: failed to load %s", name);
}

bool map_service::g_enabled = false;
unsigned int map_service::g_color = 0xFFFFFFFFu; // ImU32 white
bool map_service::g_loaded = false;
MapConfig map_service::g_config;

static std::vector<MapTriangle> s_rawTriangles;
static std::vector<BakedTriangle> s_bakedTriangles;
static std::unordered_map<long long, std::vector<int>> s_grid;
static std::mutex s_mutex;

static MapVector3 s_lastScale, s_lastRot, s_lastPos;
static bool s_lastFlipX = false, s_lastFlipY = false, s_lastFlipZ = false;

static const float CellSize = 5.0f;

static float DegToRad(float deg) {
	return deg * (3.14159265358979323846f / 180.0f);
}

static MapQuaternion CreateFromYawPitchRoll(float yaw, float pitch, float roll) {
	float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
	float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
	float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
	MapQuaternion q;
	q.w = cr * cp * cy + sr * sp * sy;
	q.x = cr * sp * cy + sr * cp * sy;
	q.y = cr * cp * sy - sr * sp * cy;
	q.z = sr * cp * cy - cr * sp * sy;
	return q;
}

static MapVector3 TransformPoint(MapVector3 p, MapQuaternion q) {
	if (map_service::g_config.flipX) p.x = -p.x;
	if (map_service::g_config.flipY) p.y = -p.y;
	if (map_service::g_config.flipZ) p.z = -p.z;
	p.x *= map_service::g_config.mapScale.x;
	p.y *= map_service::g_config.mapScale.y;
	p.z *= map_service::g_config.mapScale.z;
	p = q * p;
	p.x += map_service::g_config.mapOffset.x;
	p.y += map_service::g_config.mapOffset.y;
	p.z += map_service::g_config.mapOffset.z;
	return p;
}

static long long GetGridHash(int x, int y, int z) {
	return ((long long)x * 73856093) ^ ((long long)y * 19349663) ^ ((long long)z * 83492791);
}

static void BakeMap() {
	s_bakedTriangles.clear();
	s_grid.clear();
	s_lastScale = map_service::g_config.mapScale;
	s_lastRot = map_service::g_config.mapRotation;
	s_lastPos = map_service::g_config.mapOffset;
	s_lastFlipX = map_service::g_config.flipX;
	s_lastFlipY = map_service::g_config.flipY;
	s_lastFlipZ = map_service::g_config.flipZ;

	MapQuaternion rotation = CreateFromYawPitchRoll(
		DegToRad(map_service::g_config.mapRotation.y),
		DegToRad(map_service::g_config.mapRotation.x),
		DegToRad(map_service::g_config.mapRotation.z)
	);

	for (size_t i = 0; i < s_rawTriangles.size(); i++) {
		const MapTriangle& raw = s_rawTriangles[i];
		MapVector3 v1 = TransformPoint(raw.v1, rotation);
		MapVector3 v2 = TransformPoint(raw.v2, rotation);
		MapVector3 v3 = TransformPoint(raw.v3, rotation);
		MapVector3 minV(
			(std::min)({ v1.x, v2.x, v3.x }),
			(std::min)({ v1.y, v2.y, v3.y }),
			(std::min)({ v1.z, v2.z, v3.z })
		);
		MapVector3 maxV(
			(std::max)({ v1.x, v2.x, v3.x }),
			(std::max)({ v1.y, v2.y, v3.y }),
			(std::max)({ v1.z, v2.z, v3.z })
		);
		BakedTriangle bt;
		bt.v1 = v1; bt.v2 = v2; bt.v3 = v3;
		bt.min = minV; bt.max = maxV;
		bt.materialName = raw.materialName;
		s_bakedTriangles.push_back(bt);

		int minX = (int)floorf(minV.x / CellSize), maxX = (int)floorf(maxV.x / CellSize);
		int minY = (int)floorf(minV.y / CellSize), maxY = (int)floorf(maxV.y / CellSize);
		int minZ = (int)floorf(minV.z / CellSize), maxZ = (int)floorf(maxV.z / CellSize);
		for (int x = minX; x <= maxX; x++)
			for (int y = minY; y <= maxY; y++)
				for (int z = minZ; z <= maxZ; z++)
					s_grid[GetGridHash(x, y, z)].push_back((int)i);
	}
}

bool map_service::LoadMap(const std::string& filename){
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;



	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
	if (!ret) return false;

	std::lock_guard<std::mutex> lock(s_mutex);
	s_rawTriangles.clear();

	for (const auto& mesh : model.meshes) {
		for (const auto& primitive : mesh.primitives) {
			if (primitive.mode != TINYGLTF_MODE_TRIANGLES) continue;
			auto itPos = primitive.attributes.find("POSITION");
			if (itPos == primitive.attributes.end()) continue;

			std::string matName;
			if (primitive.material >= 0 && primitive.material < (int)model.materials.size())
				matName = model.materials[primitive.material].name;
			if (!mesh.name.empty())
				matName = mesh.name + (matName.empty() ? "" : " " + matName);

			const tinygltf::Accessor& posAccessor = model.accessors[itPos->second];
			const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
			const float* positions = reinterpret_cast<const float*>(&posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);

			if (primitive.indices >= 0) {
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& indexBuffer = model.buffers[indexView.buffer];
				const size_t idxOff = indexView.byteOffset + indexAccessor.byteOffset;

				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* indices = reinterpret_cast<const uint16_t*>(&indexBuffer.data[idxOff]);
					for (size_t i = 0; i < indexAccessor.count; i += 3) {
						MapTriangle tri;
						tri.v1 = MapVector3(positions[indices[i] * 3], positions[indices[i] * 3 + 1], positions[indices[i] * 3 + 2]);
						tri.v2 = MapVector3(positions[indices[i + 1] * 3], positions[indices[i + 1] * 3 + 1], positions[indices[i + 1] * 3 + 2]);
						tri.v3 = MapVector3(positions[indices[i + 2] * 3], positions[indices[i + 2] * 3 + 1], positions[indices[i + 2] * 3 + 2]);
						tri.materialName = matName;
						s_rawTriangles.push_back(tri);
					}
				} else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					const uint32_t* indices = reinterpret_cast<const uint32_t*>(&indexBuffer.data[idxOff]);
					for (size_t i = 0; i < indexAccessor.count; i += 3) {
						MapTriangle tri;
						tri.v1 = MapVector3(positions[indices[i] * 3], positions[indices[i] * 3 + 1], positions[indices[i] * 3 + 2]);
						tri.v2 = MapVector3(positions[indices[i + 1] * 3], positions[indices[i + 1] * 3 + 1], positions[indices[i + 1] * 3 + 2]);
						tri.v3 = MapVector3(positions[indices[i + 2] * 3], positions[indices[i + 2] * 3 + 1], positions[indices[i + 2] * 3 + 2]);
						tri.materialName = matName;
						s_rawTriangles.push_back(tri);
					}
				}
			} else {
				for (size_t i = 0; i < posAccessor.count; i += 3) {
					MapTriangle tri;
					tri.v1 = MapVector3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
					tri.v2 = MapVector3(positions[(i + 1) * 3], positions[(i + 1) * 3 + 1], positions[(i + 1) * 3 + 2]);
					tri.v3 = MapVector3(positions[(i + 2) * 3], positions[(i + 2) * 3 + 1], positions[(i + 2) * 3 + 2]);
					tri.materialName = matName;
					s_rawTriangles.push_back(tri);
				}
			}
		}
	}

	BakeMap();
	g_loaded = true;
	return true;
}

bool map_service::LoadMapFromMemory(const unsigned char* data, size_t size) {
	if (!data || size == 0 || size > 0x7FFFFFFFu) return false;
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;
	bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, data, (unsigned int)size, "");
	if (!ret) return false;

	std::lock_guard<std::mutex> lock(s_mutex);
	s_rawTriangles.clear();

	for (const auto& mesh : model.meshes) {
		for (const auto& primitive : mesh.primitives) {
			if (primitive.mode != TINYGLTF_MODE_TRIANGLES) continue;
			auto itPos = primitive.attributes.find("POSITION");
			if (itPos == primitive.attributes.end()) continue;

			std::string matName;
			if (primitive.material >= 0 && primitive.material < (int)model.materials.size())
				matName = model.materials[primitive.material].name;
			if (!mesh.name.empty())
				matName = mesh.name + (matName.empty() ? "" : " " + matName);

			const tinygltf::Accessor& posAccessor = model.accessors[itPos->second];
			const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer& posBuffer = model.buffers[posView.buffer];
			const float* positions = reinterpret_cast<const float*>(&posBuffer.data[posView.byteOffset + posAccessor.byteOffset]);

			if (primitive.indices >= 0) {
				const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& indexView = model.bufferViews[indexAccessor.bufferView];
				const tinygltf::Buffer& indexBuffer = model.buffers[indexView.buffer];
				const size_t idxOff = indexView.byteOffset + indexAccessor.byteOffset;

				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
					const uint16_t* indices = reinterpret_cast<const uint16_t*>(&indexBuffer.data[idxOff]);
					for (size_t i = 0; i < indexAccessor.count; i += 3) {
						MapTriangle tri;
						tri.v1 = MapVector3(positions[indices[i] * 3], positions[indices[i] * 3 + 1], positions[indices[i] * 3 + 2]);
						tri.v2 = MapVector3(positions[indices[i + 1] * 3], positions[indices[i + 1] * 3 + 1], positions[indices[i + 1] * 3 + 2]);
						tri.v3 = MapVector3(positions[indices[i + 2] * 3], positions[indices[i + 2] * 3 + 1], positions[indices[i + 2] * 3 + 2]);
						tri.materialName = matName;
						s_rawTriangles.push_back(tri);
					}
				} else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
					const uint32_t* indices = reinterpret_cast<const uint32_t*>(&indexBuffer.data[idxOff]);
					for (size_t i = 0; i < indexAccessor.count; i += 3) {
						MapTriangle tri;
						tri.v1 = MapVector3(positions[indices[i] * 3], positions[indices[i] * 3 + 1], positions[indices[i] * 3 + 2]);
						tri.v2 = MapVector3(positions[indices[i + 1] * 3], positions[indices[i + 1] * 3 + 1], positions[indices[i + 1] * 3 + 2]);
						tri.v3 = MapVector3(positions[indices[i + 2] * 3], positions[indices[i + 2] * 3 + 1], positions[indices[i + 2] * 3 + 2]);
						tri.materialName = matName;
						s_rawTriangles.push_back(tri);
					}
				}
			} else {
				for (size_t i = 0; i < posAccessor.count; i += 3) {
					MapTriangle tri;
					tri.v1 = MapVector3(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
					tri.v2 = MapVector3(positions[(i + 1) * 3], positions[(i + 1) * 3 + 1], positions[(i + 1) * 3 + 2]);
					tri.v3 = MapVector3(positions[(i + 2) * 3], positions[(i + 2) * 3 + 1], positions[(i + 2) * 3 + 2]);
					tri.materialName = matName;
					s_rawTriangles.push_back(tri);
				}
			}
		}
	}

	BakeMap();
	g_loaded = true;
	return true;
}

void map_service::Update() {
	if (!g_loaded) return;
	bool changed = s_lastScale.x != g_config.mapScale.x || s_lastScale.y != g_config.mapScale.y || s_lastScale.z != g_config.mapScale.z
		|| s_lastRot.x != g_config.mapRotation.x || s_lastRot.y != g_config.mapRotation.y || s_lastRot.z != g_config.mapRotation.z
		|| s_lastPos.x != g_config.mapOffset.x || s_lastPos.y != g_config.mapOffset.y || s_lastPos.z != g_config.mapOffset.z
		|| s_lastFlipX != g_config.flipX || s_lastFlipY != g_config.flipY || s_lastFlipZ != g_config.flipZ;
	if (changed) {
		std::lock_guard<std::mutex> lock(s_mutex);
		BakeMap();
	}
}

static bool world_to_screen_map(const float* matrix, float width, float height, const MapVector3& pos, ImVec2& out) {
	float x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
	float y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
	float w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];
	if (w < 0.01f) return false;
	float inv_w = 1.0f / w;
	out.x = (width * 0.5f) + (x * inv_w * width * 0.5f);
	out.y = (height * 0.5f) - (y * inv_w * height * 0.5f);
	return true;
}

static bool RayHitAABB(const MapVector3& origin, const MapVector3& dir, const MapVector3& boxMin, const MapVector3& boxMax, float maxT) {
	float tmin = 0.0f, tmax = maxT;
	const float* o = &origin.x;
	const float* d = &dir.x;
	const float* mn = &boxMin.x;
	const float* mx = &boxMax.x;
	for (int i = 0; i < 3; i++) {
		if (std::fabs(d[i]) < 1e-7f) {
			if (o[i] < mn[i] || o[i] > mx[i]) return false;
			continue;
		}
		float t1 = (mn[i] - o[i]) / d[i], t2 = (mx[i] - o[i]) / d[i];
		if (t1 > t2) { float t = t1; t1 = t2; t2 = t; }
		if (t1 > tmin) tmin = t1;
		if (t2 < tmax) tmax = t2;
		if (tmin > tmax || tmax < 0.0f) return false;
	}
	return tmin <= tmax && tmax >= 0.0f && tmin <= maxT;
}

static bool IntersectRayTriangle(const MapVector3& origin, const MapVector3& dir, const BakedTriangle& tri, float& t) {
	using namespace ms;
	const float EPSILON = 0.0000001f;
	MapVector3 edge1 = tri.v2 - tri.v1;
	MapVector3 edge2 = tri.v3 - tri.v1;
	MapVector3 h = Cross(dir, edge2);
	float a = Dot(edge1, h);
	if (a > -EPSILON && a < EPSILON) return false;
	float f = 1.0f / a;
	MapVector3 s = origin - tri.v1;
	float u = f * Dot(s, h);
	if (u < 0.0f || u > 1.0f) return false;
	MapVector3 q = Cross(s, edge1);
	float v = f * Dot(dir, q);
	if (v < 0.0f || u + v > 1.0f) return false;
	t = f * Dot(edge2, q);
	return t > EPSILON;
}

float map_service::RaycastWallWithMaterial(MapVector3 origin, MapVector3 dir, float maxDist, std::string* outMaterial) {
	if (outMaterial) outMaterial->clear();
	if (!g_loaded) return FLT_MAX;
	std::lock_guard<std::mutex> lock(s_mutex);
	if (s_bakedTriangles.empty()) return FLT_MAX;
	float minT = FLT_MAX;
	size_t bestIndex = (size_t)-1;
	const float minWallDist = 0.4f;
	for (size_t i = 0; i < s_bakedTriangles.size(); i++) {
		const BakedTriangle& tri = s_bakedTriangles[i];
		if (!RayHitAABB(origin, dir, tri.min, tri.max, maxDist)) continue;
		float t;
		if (!IntersectRayTriangle(origin, dir, tri, t)) continue;
		if (t > minWallDist && t < maxDist && t < minT) {
			minT = t;
			bestIndex = i;
		}
	}
	if (outMaterial && bestIndex != (size_t)-1)
		*outMaterial = s_bakedTriangles[bestIndex].materialName;
	return minT;
}

float map_service::RaycastClosestWithName(MapVector3 origin, MapVector3 dir, float maxDist, std::string* outMaterial) {
	if (outMaterial) outMaterial->clear();
	if (!g_loaded) return FLT_MAX;
	std::lock_guard<std::mutex> lock(s_mutex);
	if (s_bakedTriangles.empty()) return FLT_MAX;
	float minT = FLT_MAX;
	size_t bestIndex = (size_t)-1;
	const float minDist = 0.02f;
	for (size_t i = 0; i < s_bakedTriangles.size(); i++) {
		const BakedTriangle& tri = s_bakedTriangles[i];
		if (!RayHitAABB(origin, dir, tri.min, tri.max, maxDist)) continue;
		float t;
		if (!IntersectRayTriangle(origin, dir, tri, t)) continue;
		if (t <= minDist || t >= maxDist || t >= minT) continue;
		minT = t;
		bestIndex = i;
	}
	if (outMaterial && bestIndex != (size_t)-1)
		*outMaterial = s_bakedTriangles[bestIndex].materialName;
	return minT;
}

float map_service::RaycastWallExcluding(MapVector3 origin, MapVector3 dir, float maxDist, const std::vector<std::string>& excludeSubstrings) {
	if (!g_loaded) return FLT_MAX;
	std::lock_guard<std::mutex> lock(s_mutex);
	if (s_bakedTriangles.empty()) return FLT_MAX;
	float minT = FLT_MAX;
	const float minWallDist = 0.4f;
	for (size_t i = 0; i < s_bakedTriangles.size(); i++) {
		const BakedTriangle& tri = s_bakedTriangles[i];
		if (!RayHitAABB(origin, dir, tri.min, tri.max, maxDist)) continue;
		float t;
		if (!IntersectRayTriangle(origin, dir, tri, t)) continue;
		if (t <= minWallDist || t >= maxDist || t >= minT) continue;
		std::string nameLower = tri.materialName;
		for (char& c : nameLower) c = (char)std::tolower((unsigned char)c);
		bool skip = false;
		for (const auto& sub : excludeSubstrings) {
			std::string subLower = sub;
			for (char& c : subLower) c = (char)std::tolower((unsigned char)c);
			if (nameLower.find(subLower) != std::string::npos) {
				skip = true;
				break;
			}
		}
		if (!skip)
			minT = t;
	}
	return minT;
}

float map_service::RaycastWallClosest(MapVector3 origin, MapVector3 dir, float maxDist) {
	if (!g_loaded) return FLT_MAX;
	std::lock_guard<std::mutex> lock(s_mutex);
	if (s_bakedTriangles.empty()) return FLT_MAX;
	float minT = FLT_MAX;
	const float minWallDist = 0.02f; // только отсечь самопересечение, не 0.4 — чтобы детектить стену в упор
	for (size_t i = 0; i < s_bakedTriangles.size(); i++) {
		const BakedTriangle& tri = s_bakedTriangles[i];
		if (!RayHitAABB(origin, dir, tri.min, tri.max, maxDist)) continue;
		float t;
		if (!IntersectRayTriangle(origin, dir, tri, t)) continue;
		if (t <= minWallDist || t >= maxDist || t >= minT) continue;
		minT = t;
	}
	return minT;
}

bool map_service::GetColliderAABB(const char* nameSubstring, MapVector3* outMin, MapVector3* outMax) {
	if (!nameSubstring || !outMin || !outMax || !g_loaded) return false;
	std::string sub(nameSubstring);
	for (char& c : sub) c = (char)std::tolower((unsigned char)c);
	std::lock_guard<std::mutex> lock(s_mutex);
	bool found = false;
	float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
	float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
	for (const BakedTriangle& tri : s_bakedTriangles) {
		std::string nameLower = tri.materialName;
		for (char& c : nameLower) c = (char)std::tolower((unsigned char)c);
		if (nameLower.find(sub) == std::string::npos) continue;
		found = true;
		minX = (std::min)(minX, tri.min.x); minY = (std::min)(minY, tri.min.y); minZ = (std::min)(minZ, tri.min.z);
		maxX = (std::max)(maxX, tri.max.x); maxY = (std::max)(maxY, tri.max.y); maxZ = (std::max)(maxZ, tri.max.z);
	}
	if (!found) return false;
	outMin->x = minX; outMin->y = minY; outMin->z = minZ;
	outMax->x = maxX; outMax->y = maxY; outMax->z = maxZ;
	return true;
}

float map_service::RaycastColliderByName(MapVector3 origin, MapVector3 dir, float maxDist, const char* nameSubstring) {
	if (!nameSubstring || !g_loaded) return FLT_MAX;
	std::string sub(nameSubstring);
	for (char& c : sub) c = (char)std::tolower((unsigned char)c);
	std::lock_guard<std::mutex> lock(s_mutex);
	float minT = FLT_MAX;
	const float minDist = 0.02f;
	for (const BakedTriangle& tri : s_bakedTriangles) {
		std::string nameLower = tri.materialName;
		for (char& c : nameLower) c = (char)std::tolower((unsigned char)c);
		if (nameLower.find(sub) == std::string::npos) continue;
		if (!RayHitAABB(origin, dir, tri.min, tri.max, maxDist)) continue;
		float t;
		if (!IntersectRayTriangle(origin, dir, tri, t)) continue;
		if (t <= minDist || t >= maxDist || t >= minT) continue;
		minT = t;
	}
	return minT;
}

void map_service::RenderColliderHighlight(ImDrawList* draw_list, const float* view_matrix, float screen_w, float screen_h, const char* nameSubstring, unsigned int color) {
	if (!draw_list || !view_matrix || !nameSubstring || !g_loaded) return;
	std::string sub(nameSubstring);
	for (char& c : sub) c = (char)std::tolower((unsigned char)c);
	std::lock_guard<std::mutex> lock(s_mutex);
	for (const BakedTriangle& tri : s_bakedTriangles) {
		std::string nameLower = tri.materialName;
		for (char& c : nameLower) c = (char)std::tolower((unsigned char)c);
		if (nameLower.find(sub) == std::string::npos) continue;
		float cx = (tri.v1.x + tri.v2.x + tri.v3.x) * (1.f / 3.f);
		float cy = (tri.v1.y + tri.v2.y + tri.v3.y) * (1.f / 3.f);
		float cz = (tri.v1.z + tri.v2.z + tri.v3.z) * (1.f / 3.f);
		float w = cx * view_matrix[3] + cy * view_matrix[7] + cz * view_matrix[11] + view_matrix[15];
		if (w < 0.05f) continue;
		ImVec2 s1, s2, s3;
		if (world_to_screen_map(view_matrix, screen_w, screen_h, tri.v1, s1) &&
		    world_to_screen_map(view_matrix, screen_w, screen_h, tri.v2, s2) &&
		    world_to_screen_map(view_matrix, screen_w, screen_h, tri.v3, s3)) {
			draw_list->AddLine(s1, s2, color);
			draw_list->AddLine(s2, s3, color);
			draw_list->AddLine(s3, s1, color);
		}
	}
}

void map_service::Render(ImDrawList* draw_list, const float* view_matrix, float screen_w, float screen_h, const std::string* filterMaterialName) {
	if (!g_enabled || !g_loaded || !draw_list) return;
	std::lock_guard<std::mutex> lock(s_mutex);
	ImU32 col = (ImU32)g_color;
	const bool filter = (filterMaterialName != nullptr && !filterMaterialName->empty());
	for (size_t idx = 0; idx < s_bakedTriangles.size(); idx++) {
		const BakedTriangle& tri = s_bakedTriangles[idx];
		if (filter && tri.materialName != *filterMaterialName)
			continue;
		float cx = (tri.v1.x + tri.v2.x + tri.v3.x) * (1.f / 3.f);
		float cy = (tri.v1.y + tri.v2.y + tri.v3.y) * (1.f / 3.f);
		float cz = (tri.v1.z + tri.v2.z + tri.v3.z) * (1.f / 3.f);
		float w = cx * view_matrix[3] + cy * view_matrix[7] + cz * view_matrix[11] + view_matrix[15];
		if (w < 0.05f) continue;
		ImVec2 s1, s2, s3;
		if (world_to_screen_map(view_matrix, screen_w, screen_h, tri.v1, s1) &&
			world_to_screen_map(view_matrix, screen_w, screen_h, tri.v2, s2) &&
			world_to_screen_map(view_matrix, screen_w, screen_h, tri.v3, s3)) {
			draw_list->AddLine(s1, s2, col);
			draw_list->AddLine(s2, s3, col);
			draw_list->AddLine(s3, s1, col);
		}
	}
}
