#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "esp.h"
#include "Offset.h"
#include "memory.h"
#include "console.h"
#include "map_service.h"
#include "triggerbot.h"
#include "misc.h"
#include "imgui.h"
#include "clipper2/clipper.h"
#include "clipper2/clipping.h"
#include "glow_shader.h"
#include "overlay.h"
#include "menu.h"
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <unordered_set>
#include <cctype>
#include <cstring>

using namespace Clipper2Lib;

namespace esp
{
	static std::map<uint32_t, void*> s_avatar_cache;
	static std::vector<glow_vertex> s_glow_triangles;
	static std::unordered_set<uint32_t> s_avatar_failed;
	static std::map<uint32_t, int> s_prev_health;
	static std::map<uint32_t, float> s_display_hp;
	static std::map<uint32_t, float> s_display_armor;
	static const float s_bar_anim_speed = 8.f;

	// --- Forward declarations (internal) ---
	static bool read_ilist(mem::Memory* mem, uint32_t list_ptr, uint32_t* out_items, int* out_count);
	static bool dict_get_int(mem::Memory* mem, uint32_t dict_ptr, int key, uint32_t* out_val);
	static void fill_skeleton_from_bone(int bone_index, const Transform& t, Skeleton& out_skeleton);
	static bool has_head_or_neck_valid(const SkeletonData& data);
	static void draw_skeleton_branch(ImDrawList* draw_list, const float* matrix, const float* const* world_pts, int count, ImU32 color, float screen_w, float screen_h);
	static bool valid_bone(const float* p);
	static void draw_hitbox_outline_clipper(ImDrawList* draw, const float* matrix, float screen_w, float screen_h, const PlayerData& p, std::vector<glow_vertex>* out_glow, float alpha_scale, const float* color_top_override, const float* color_bottom_override);
	static void triangle_filled_gradient(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, const ImVec2& c,
		float y_min, float y_max, const float top[4], const float bottom[4], float alpha_scale);
	static void draw_hitbox_obbs(ImDrawList* draw, const float* matrix, float screen_w, float screen_h, Vec3 pos, const Quat& rot, Vec3 center, Vec3 size, ImU32 col);
	static void draw_sphere_wireframe(ImDrawList* draw, const float* matrix, float sw, float sh, Vec3 center, float radius, const Quat& orient, ImU32 color, int segments);
	static void draw_capsule_wireframe(ImDrawList* draw, const float* m, float sw, float sh, Vec3 from, Vec3 to, float r1, float r2, ImU32 color, int segments);
	static void draw_hitboxes(ImDrawList* draw, const float* matrix, float sw, float sh, const SkeletonData& skel, const HitboxData& hitboxes, ImU32 color);
	static Path64 get_convex_hull_path(const std::vector<ImVec2>& points);
	static void get_hitbox_screen_points(const float* matrix, float sw, float sh, const Hitbox& hb, const SkeletonData& skel, std::vector<ImVec2>& out);
	static void draw_text_outline_1px(ImDrawList* draw, ImFont* font, float fs, ImVec2 pos, ImU32 tc, ImU32 oc, const char* text);
	static ImU32 lerp_color(ImU32 low, ImU32 high, float t);
	static ImU32 color_with_alpha(ImU32 col, int alpha);
	static Vec3 vec3(float x, float y, float z);
	static Vec3 quat_mul_vec3(const Quat& q, Vec3 v);
	static bool world_to_screen_vec3(const float* matrix, float width, float height, Vec3 pos, ImVec2& out);
	static bool IntersectRayCapsule(Vec3 o, Vec3 d, Vec3 a, Vec3 b, float r, float& t);
	static bool IntersectRayOBB(Vec3 rayOrigin, Vec3 rayDir, Vec3 boxCenter, const Quat& boxRot, Vec3 halfExtents, float& t);
	static float GetHitboxIntersectionDistance(Vec3 rayOrigin, Vec3 rayDir, const HitboxData& hitboxes, const SkeletonData& skel, const int* excludeBones, int excludeCount);
	static const std::vector<std::string>& GetPenetrableMaterialSubstrings();
	static void matrix4_inv(const float* m, float* out);
	static HitboxData get_real_hitboxes(const SkeletonData& skeleton);

	// --- String helpers ---
	static bool utf16_to_utf8(uint16_t c, std::string& out)
	{
		if (c < 0x80) out += (char)c;
		else if (c < 0x800) { out += (char)(0xC0 | (c >> 6)); out += (char)(0x80 | (c & 0x3F)); }
		else if (c < 0xD800 || c > 0xDFFF) {
			out += (char)(0xE0 | (c >> 12));
			out += (char)(0x80 | ((c >> 6) & 0x3F));
			out += (char)(0x80 | (c & 0x3F));
		}
		return true;
	}

	static bool read_unity_string(mem::Memory* mem, uint32_t string_ptr, std::string& out)
	{
		if (!string_ptr || !mem) return false;
		int32_t length = 0;
		if (!mem->Read(string_ptr + Offsets::Il2Cpp_::StringLength, &length, sizeof(length))) return false;
		if (length <= 0 || length > 64) return false;
		std::vector<char16_t> buf((size_t)length);
		if (!mem->Read(string_ptr + Offsets::Il2Cpp_::StringChars, buf.data(), (size_t)length * sizeof(char16_t))) return false;
		out.clear();
		for (int i = 0; i < length; i++) utf16_to_utf8(buf[i], out);
		return true;
	}

	// --- Weapon icon mapping ---
	static const char* get_weapon_icon_by_id(int id)
	{
		switch (id) {
		case 11: return "D";
		case 12: return "G";
		case 13: return "E";
		case 15: return "A";
		case 16: return "H";
		case 17: return "C";
		case 18: return "B";
		case 32: return "L";
		case 33: return "I";
		case 34: return "N";
		case 35: return "P";
		case 36: return "M";
		case 37: return "K";
		case 42: return "Q";
		case 43: return "T";
		case 44: return "W";
		case 45: return "V";
		case 46: return "S";
		case 47: return "U";
		case 48: return "R";
		case 49: return "Q";
		case 51: return "Z";
		case 52: return "a";
		case 53: return "X";
		case 54: return "a";
		case 62: return "b";
		case 63: return "c";
		case 64: return "g";
		case 65: return "e";
		case 70: return "[";
		case 71: return "1";
		case 72: return "4";
		case 73: return "7";
		case 75: return "8";
		case 77: return "2";
		case 78: return "3";
		case 79: return "0";
		case 80: return "7";
		case 81: return "9";
		case 82: return "6";
		case 83: return "]";
		case 85: return "4";
		case 86: return "4";
		case 88: return "7";
		case 91: return "j";
		case 92: return "k";
		case 93: return "i";
		case 94: return "n";
		case 95: return "n";
		case 100: return "o";
		default: return nullptr;
		}
	}

	static const char* get_weapon_icon_by_name(const char* weapon_name)
	{
		if (!weapon_name || weapon_name[0] == '\0') return nullptr;
		std::string name(weapon_name);
		std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
		if (name.find("g22") != std::string::npos) return get_weapon_icon_by_id(11);
		if (name.find("usp") != std::string::npos) return get_weapon_icon_by_id(12);
		if (name.find("p350") != std::string::npos) return get_weapon_icon_by_id(13);
		if (name.find("deagle") != std::string::npos || name.find("eagle") != std::string::npos || name.find("desert") != std::string::npos) return get_weapon_icon_by_id(15);
		if (name.find("tec") != std::string::npos) return get_weapon_icon_by_id(16);
		if (name.find("f/s") != std::string::npos || name.find("fs") != std::string::npos) return get_weapon_icon_by_id(17);
		if (name.find("beretta") != std::string::npos) return get_weapon_icon_by_id(18);
		if (name.find("ump") != std::string::npos) return get_weapon_icon_by_id(32);
		if (name.find("akimbo") != std::string::npos && name.find("uzi") != std::string::npos) return get_weapon_icon_by_id(33);
		if (name.find("mp7") != std::string::npos) return get_weapon_icon_by_id(34);
		if (name.find("p90") != std::string::npos) return get_weapon_icon_by_id(35);
		if (name.find("mp5") != std::string::npos) return get_weapon_icon_by_id(36);
		if (name.find("mac10") != std::string::npos || name.find("mac-10") != std::string::npos) return get_weapon_icon_by_id(37);
		if (name.find("val") != std::string::npos) return get_weapon_icon_by_id(42);
		if (name.find("m4a1") != std::string::npos) return get_weapon_icon_by_id(43);
		if (name.find("akr12") != std::string::npos) return get_weapon_icon_by_id(45);
		if (name.find("akr") != std::string::npos) return get_weapon_icon_by_id(44);
		if (name.find("m40") != std::string::npos) return get_weapon_icon_by_id(52);
		if (name.find("m110") != std::string::npos) return get_weapon_icon_by_id(53);
		if (name.find("m4") != std::string::npos) return get_weapon_icon_by_id(46);
		if (name.find("m16") != std::string::npos) return get_weapon_icon_by_id(47);
		if (name.find("famas") != std::string::npos) return get_weapon_icon_by_id(48);
		if (name.find("fal") != std::string::npos) return get_weapon_icon_by_id(49);
		if (name.find("awm") != std::string::npos) return get_weapon_icon_by_id(51);
		if (name.find("mallard") != std::string::npos) return get_weapon_icon_by_id(54);
		if (name.find("sm1014") != std::string::npos || name.find("m1014") != std::string::npos) return get_weapon_icon_by_id(62);
		if (name.find("fabm") != std::string::npos) return get_weapon_icon_by_id(63);
		if (name.find("m60") != std::string::npos) return get_weapon_icon_by_id(64);
		if (name.find("spas") != std::string::npos) return get_weapon_icon_by_id(65);
		if (name.find("knife") != std::string::npos && name.find("bayonet") == std::string::npos &&
			name.find("karambit") == std::string::npos && name.find("butterfly") == std::string::npos &&
			name.find("flip") == std::string::npos && name.find("kunai") == std::string::npos &&
			name.find("scorpion") == std::string::npos && name.find("tanto") == std::string::npos &&
			name.find("dual") == std::string::npos && name.find("kukri") == std::string::npos &&
			name.find("stiletto") == std::string::npos && name.find("mantis") == std::string::npos &&
			name.find("fang") == std::string::npos && name.find("sting") == std::string::npos) return get_weapon_icon_by_id(70);
		if (name.find("bayonet") != std::string::npos) return get_weapon_icon_by_id(71);
		if (name.find("karambit") != std::string::npos) return get_weapon_icon_by_id(72);
		if (name.find("kommando") != std::string::npos) return get_weapon_icon_by_id(73);
		if (name.find("butterfly") != std::string::npos) return get_weapon_icon_by_id(75);
		if (name.find("flip") != std::string::npos) return get_weapon_icon_by_id(77);
		if (name.find("kunai") != std::string::npos) return get_weapon_icon_by_id(78);
		if (name.find("scorpion") != std::string::npos) return get_weapon_icon_by_id(79);
		if (name.find("tanto") != std::string::npos) return get_weapon_icon_by_id(80);
		if (name.find("dual") != std::string::npos && name.find("dagger") != std::string::npos) return get_weapon_icon_by_id(81);
		if (name.find("kukri") != std::string::npos) return get_weapon_icon_by_id(82);
		if (name.find("stiletto") != std::string::npos) return get_weapon_icon_by_id(83);
		if (name.find("mantis") != std::string::npos) return get_weapon_icon_by_id(85);
		if (name.find("fang") != std::string::npos) return get_weapon_icon_by_id(86);
		if (name.find("sting") != std::string::npos) return get_weapon_icon_by_id(88);
		if (name.find("he") != std::string::npos || name.find("grenade") != std::string::npos) {
			if (name.find("smoke") == std::string::npos && name.find("flash") == std::string::npos &&
				name.find("molotov") == std::string::npos && name.find("thermite") == std::string::npos &&
				name.find("incendiary") == std::string::npos) return get_weapon_icon_by_id(91);
		}
		if (name.find("smoke") != std::string::npos) return get_weapon_icon_by_id(92);
		if (name.find("flash") != std::string::npos) return get_weapon_icon_by_id(93);
		if (name.find("molotov") != std::string::npos) return get_weapon_icon_by_id(94);
		if (name.find("thermite") != std::string::npos || name.find("incendiary") != std::string::npos) return get_weapon_icon_by_id(95);
		if (name.find("bomb") != std::string::npos || name.find("c4") != std::string::npos) return get_weapon_icon_by_id(100);
		return nullptr;
	}

	// --- 1.0.0 globals ---
	uint32_t g_client_world = 0;
	uint32_t g_local_player = 0;
	uint32_t g_net_object_registry = 0;
	uint32_t g_net_object_storage = 0;
	size_t g_unity_base = 0;
	float g_view_matrix[16] = { 0 };
	bool g_matrix_valid = false;
	std::vector<PlayerData> g_players;

	bool g_esp_enabled = true;
	bool g_esp_box = true;
	bool g_esp_hp = false;
	bool g_esp_name = false;
	bool g_esp_skeleton = false;
	float g_esp_color_skeleton[4] = { 1.f, 1.f, 1.f, 1.f };
	static const float g_skeleton_thickness = 1.5f;
	bool g_esp_weapon = false;
	bool g_esp_armor = false;
	bool g_esp_avatar = false;
	float g_esp_color_box[4] = { 1.f, 1.f, 1.f, 1.f };
	float g_esp_color_name[4] = { 1.f, 1.f, 1.f, 1.f };
	float g_esp_color_hp_full[4] = { 0.69f, 1.f, 0.31f, 1.f };
	float g_esp_color_hp_low[4] = { 1.f, 0.2f, 0.31f, 1.f };
	float g_esp_color_armor[4] = { 0.4f, 0.55f, 0.9f, 1.f };
	float g_esp_color_armor_low[4] = { 0.2f, 0.35f, 0.7f, 1.f };
	float g_esp_color_weapon[4] = { 1.f, 1.f, 1.f, 1.f };
	bool g_esp_hitbox_outline_fill = false;
	float g_esp_color_hitbox_fill[4] = { 1.f, 1.f, 1.f, 1.f };
	float g_esp_color_hitbox_fill_bottom[4] = { 1.f, 1.f, 1.f, 1.f };
	bool g_esp_hitbox_glow = false;
	float g_esp_glow_color[4] = { 1.f, 1.f, 1.f, 1.f };
	float g_esp_glow_radius = 30.f;
	bool g_esp_grenades = false;
	float g_esp_smoke_radius_m = 5.0f;
	float g_esp_smoke_circle_outline_color[4] = { 1.f, 1.f, 1.f, 1.f };
	float g_esp_molotov_circle_outline_color[4] = { 1.f, 0.3f, 0.f, 1.f };
	float g_esp_molotov_radius_m = 5.0f;

	bool g_aimbot_enabled = false;
	float g_aimbot_fov = 90.f;
	bool g_aimbot_fov_enabled = true;
	bool g_aimbot_fov_display = true;
	bool g_aimbot_visible_check = false;
	float g_aimbot_return_ms = 0.0f;
	bool g_antiaim_enabled = false;
	float g_antiaim_pitch_amp = 1.5f;
	float g_antiaim_yaw_amp = 1.5f;
	int g_aimbot_key = VK_XBUTTON2;
	int g_aimbot_mode = 0;
	int g_aimbot_target = 0;
	float g_aimbot_smooth = 0.5f;
	ImFont* g_esp_font_name = nullptr;
	ImFont* g_esp_font_hp = nullptr;
	ImFont* g_esp_font_weapons = nullptr;

	std::vector<GrenadeData> g_grenades;
	uint32_t g_grenade_manager = 0;
	std::vector<DeployedSmokeData> g_deployed_smokes;
	std::vector<DeployedMolotovData> g_deployed_molotovs;

	// =====================================================================
	// find_by_typeinfo — heap scan for an object whose klass matches TypeInfo
	// =====================================================================
	uint32_t find_by_typeinfo(mem::Memory* mem, uintptr_t typeinfo_rva)
	{
		if (!mem || !mem->IsReady())
			return 0;
		uint32_t klass = 0;
		if (!mem->Read((size_t)typeinfo_rva, &klass, sizeof(klass)) || klass < 0x10000)
			return 0;
		// Heap scan: iterate 4-byte aligned addresses through the managed heap.
		const size_t scan_start = 0x10000000;
		const size_t scan_end = 0x20000000;
		for (size_t addr = scan_start; addr < scan_end; addr += 4)
		{
			uint32_t obj = 0;
			if (mem->Read(addr, &obj, sizeof(obj)) && obj == klass)
				return (uint32_t)addr;
		}
		return 0;
	}

	// Internal: read a List<T>
	static bool read_ilist(mem::Memory* mem, uint32_t list_ptr, uint32_t* out_items, int* out_count)
	{
		if (!list_ptr || !mem) return false;
		if (!mem->Read(list_ptr + Offsets::Il2CppList_::Items, out_items, sizeof(*out_items))) return false;
		if (!mem->Read(list_ptr + Offsets::Il2CppList_::Size, out_count, sizeof(*out_count))) return false;
		return *out_items != 0 && *out_count >= 0 && *out_count <= 1024;
	}

	// Internal: Dictionary<int, TValue> lookup by int key
	static bool dict_get_int(mem::Memory* mem, uint32_t dict_ptr, int key, uint32_t* out_val)
	{
		if (!dict_ptr || !mem || !out_val) return false;
		uint32_t entries = 0;
		int count = 0;
		if (!mem->Read(dict_ptr + Offsets::Il2CppDictionary_::Entries, &entries, sizeof(entries))) return false;
		if (!mem->Read(dict_ptr + Offsets::Il2CppDictionary_::Count, &count, sizeof(count))) return false;
		if (!entries || count <= 0) return false;
		for (int i = 0; i < count; i++)
		{
			uint32_t entry = entries + (i * 16);
			int k = 0;
			mem->Read(entry + 0x08, &k, sizeof(k));
			if (k == key) {
				mem->Read(entry + 0x0C, out_val, sizeof(*out_val));
				return *out_val != 0;
			}
		}
		return false;
	}

	// =====================================================================
	// get_transform_info — read position + rotation from a managed Transform
	// Pattern (from offsets doc): native+0x18 иерархия, +0x1C индекс
	// Managed Transform (32-bit Il2Cpp): +0x00 klass, +0x04 monitor, +0x08 m_CachedPtr
	// =====================================================================
	Transform get_transform_info(mem::Memory* mem, uint32_t transform_ptr)
	{
		Transform result = { {0,0,0}, {0,0,0,1} };
		if (!mem || !transform_ptr || transform_ptr < 0x10000)
			return result;

		// Read m_CachedPtr (native Transform pointer)
		uint32_t cached_ptr = 0;
		if (!mem->Read(transform_ptr + 0x08, &cached_ptr, sizeof(cached_ptr)) || !cached_ptr || cached_ptr < 0x10000)
			return result;

		// Read native transform: +0x18 parent chain array, +0x1C parent index
		// Following the standard Standoff 2 pattern: read parent chain and accumulate matrices
		// The native transform stores parent at +0x08 (or via hierarchy at +0x18/+0x1C)
		float matrix[16];
		// Read the world matrix from the native transform
		// Standard path: cachedPtr + 0x18 -> hierarchy list, +0x1C -> index
		// But for a direct approach, read the position from the transform's internal data

		// Try reading the position directly. In Unity native transforms,
		// position can be at different offsets depending on version.
		// Pattern from the offsets doc: native+0x18 and +0x1C
		uint32_t parent_list = 0;
		int parent_index = 0;
		mem->Read(cached_ptr + 0x18, &parent_list, sizeof(parent_list));
		mem->Read(cached_ptr + 0x1C, &parent_index, sizeof(parent_index));

		// For Standoff 2, the position is in the matrix. Read the 4x4 matrix:
		// The native transform has the matrix at +0x10 or via parent chain.
		// Actually, the position is at cached_ptr + 0x34 (native transform position for Unity 2019+)
		// Let's try the standard approach: read position from the transform's world matrix

		// Try: read 3 floats at cached_ptr + 0x30 (row 3 of column-major world matrix = position)
		float pos[3] = {0};
		if (mem->Read(cached_ptr + 0x30, &pos[0], sizeof(float)) &&
		    mem->Read(cached_ptr + 0x34, &pos[1], sizeof(float)) &&
		    mem->Read(cached_ptr + 0x38, &pos[2], sizeof(float)))
		{
			if ((std::isfinite)(pos[0]) && (std::isfinite)(pos[1]) && (std::isfinite)(pos[2])) {
				if (std::abs(pos[0]) < 10000.f && std::abs(pos[1]) < 10000.f && std::abs(pos[2]) < 10000.f) {
					result.position[0] = pos[0];
					result.position[1] = pos[1];
					result.position[2] = pos[2];
					result.rot[0] = 0; result.rot[1] = 0; result.rot[2] = 0; result.rot[3] = 1;
					return result;
				}
			}
		}

		// Fallback: read the full matrix from the native transform's parent chain
		// Build world matrix by following parent chain
		float world[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
		uint32_t current = cached_ptr;
		for (int depth = 0; depth < 64; depth++)
		{
			if (!current || current < 0x10000) break;

			// Read local matrix (16 floats) from the native transform
			float local[16];
			if (!mem->Read(current + 0x10, local, sizeof(float) * 16)) break;

			// Multiply world = local * world
			float new_world[16];
			for (int c = 0; c < 4; c++)
			{
				for (int r = 0; r < 4; r++)
				{
					new_world[c * 4 + r] =
						local[c * 4 + 0] * world[0 * 4 + r] +
						local[c * 4 + 1] * world[1 * 4 + r] +
						local[c * 4 + 2] * world[2 * 4 + r] +
						local[c * 4 + 3] * world[3 * 4 + r];
				}
			}
			memcpy(world, new_world, sizeof(world));

			// Get parent: follow parent_list[parent_index]
			uint32_t parent_ptr = 0;
			if (parent_list && parent_list > 0x10000 && parent_index >= 0) {
				mem->Read(parent_list + parent_index * 4, &parent_ptr, sizeof(parent_ptr));
				if (!parent_ptr || parent_ptr < 0x10000) break;
				// Read parent's cached_ptr
				mem->Read(parent_ptr + 0x08, &current, sizeof(current));
			} else {
				break;
			}
		}

		// Position is the translation part of the world matrix (row 3 in column-major = column 12-14)
		result.position[0] = world[12];
		result.position[1] = world[13];
		result.position[2] = world[14];
		result.rot[0] = 0; result.rot[1] = 0; result.rot[2] = 0; result.rot[3] = 1;
		return result;
	}

	void OffsetPlayerForPrediction(PlayerData& p, float dx, float dy, float dz)
	{
		p.x += dx; p.y += dy; p.z += dz;
		for (int i = 0; i < ESP_MAX_BONE_COUNT; i++)
		{
			if (p.skeleton_data.valid[i]) {
				p.skeleton_data.bones[i][0] += dx;
				p.skeleton_data.bones[i][1] += dy;
				p.skeleton_data.bones[i][2] += dz;
			}
		}
		Transform* bones[] = {
			&p.skeleton.Head, &p.skeleton.Neck, &p.skeleton.Spine,
			&p.skeleton.LeftShoulder, &p.skeleton.LeftUpperarm, &p.skeleton.LeftForearm, &p.skeleton.LeftHand,
			&p.skeleton.RightShoulder, &p.skeleton.RightUpperarm, &p.skeleton.RightForearm, &p.skeleton.RightHand,
			&p.skeleton.Hip, &p.skeleton.LeftUpLeg, &p.skeleton.LeftLeg, &p.skeleton.LeftFoot, &p.skeleton.LeftToeBase,
			&p.skeleton.RightUpLeg, &p.skeleton.RightLeg, &p.skeleton.RightFoot, &p.skeleton.RightToeBase
		};
		for (int i = 0; i < 20; i++) {
			if (bones[i]->position[0] != 0 || bones[i]->position[1] != 0 || bones[i]->position[2] != 0) {
				bones[i]->position[0] += dx;
				bones[i]->position[1] += dy;
				bones[i]->position[2] += dz;
			}
		}
	}

	// =====================================================================
	// ClientWorld (njh) — replaces PlayerManager
	// =====================================================================
	bool read_client_world(mem::Memory* mem, size_t unity_base)
	{
		g_client_world = 0;
		g_local_player = 0;
		g_net_object_registry = 0;
		g_net_object_storage = 0;
		g_unity_base = unity_base;

		if (!mem || !mem->IsReady()) return false;

		uint32_t cw = find_by_typeinfo(mem, (uintptr_t)unity_base + Offsets::ClientWorld_TypeInfo);
		if (cw < 0x10000) return false;

		g_client_world = cw;
		Console::Log("[+] client_world: 0x%X", (unsigned)cw);

		mem->Read(cw + Offsets::ClientWorld_::NetworkObjectRegistry, &g_net_object_registry, sizeof(g_net_object_registry));
		if (g_net_object_registry && g_net_object_registry > 0x10000)
			mem->Read(g_net_object_registry + Offsets::NetObjectRegistry_::Storage, &g_net_object_storage, sizeof(g_net_object_storage));

		return true;
	}

	void refresh_client_world_cache(mem::Memory* mem, size_t unity_base)
	{
		if (!mem || !mem->IsReady()) return;
		uint32_t cw = find_by_typeinfo(mem, (uintptr_t)unity_base + Offsets::ClientWorld_TypeInfo);
		if (cw < 0x10000) return;
		g_client_world = cw;
		mem->Read(cw + Offsets::ClientWorld_::NetworkObjectRegistry, &g_net_object_registry, sizeof(g_net_object_registry));
		if (g_net_object_registry && g_net_object_registry > 0x10000)
			mem->Read(g_net_object_registry + Offsets::NetObjectRegistry_::Storage, &g_net_object_storage, sizeof(g_net_object_storage));
	}

	// =====================================================================
	// Update local player — find ake with OwnerId == our ActorId
	// =====================================================================
	bool update_local_player(mem::Memory* mem)
	{
		if (!mem || !mem->IsReady() || g_client_world < 0x10000)
			return false;

		uint32_t transport = 0;
		mem->Read(g_client_world + Offsets::ClientWorld_::Transport, &transport, sizeof(transport));
		if (!transport || transport < 0x10000) return false;

		uint32_t our_actor = 0;
		mem->Read(transport + 0x68, &our_actor, sizeof(our_actor));
		if (!our_actor || our_actor < 0x10000) return false;

		uint8_t my_actor_id = 0;
		mem->Read(our_actor + Offsets::Actor_::ActorId, &my_actor_id, sizeof(my_actor_id));

		if (!g_net_object_storage || g_net_object_storage < 0x10000) return false;

		uint32_t objects_list = 0, count = 0;
		if (!mem->Read(g_net_object_storage + Offsets::NetObjectStorage_::ObjectsList, &objects_list, sizeof(objects_list)) ||
		    !mem->Read(objects_list + Offsets::Il2CppList_::Size, &count, sizeof(count)))
			return false;

		uint32_t array_ptr = 0;
		mem->Read(objects_list + Offsets::Il2CppList_::Items, &array_ptr, sizeof(array_ptr));
		if (!array_ptr) return false;

		for (int i = 0; i < count; i++)
		{
			uint32_t net_obj = 0;
			if (!mem->Read(array_ptr + (uint32_t)(Offsets::Il2Cpp_::ObjectHeaderSize + i * 4), &net_obj, sizeof(net_obj)) || !net_obj)
				continue;
			uint32_t klass = 0;
			mem->Read(net_obj, &klass, sizeof(klass));
			if (klass == Offsets::Player_TypeInfo) {
				uint8_t owner_id = 0;
				mem->Read(net_obj + Offsets::NetObject_::OwnerId, &owner_id, sizeof(owner_id));
				if (owner_id == my_actor_id) {
					g_local_player = net_obj;
					return true;
				}
			}
		}
		return false;
	}

	// =====================================================================
	// Skeleton via CharacterBindings (replaces BipedMap)
	// =====================================================================
	bool get_skeleton_bones(mem::Memory* mem, uint32_t player_base, Skeleton& out_skeleton, SkeletonData& out_skeleton_data)
	{
		memset(&out_skeleton, 0, sizeof(Skeleton));
		memset(&out_skeleton_data, 0, sizeof(SkeletonData));
		if (!mem || !mem->IsReady() || player_base < 0x10000) return false;

		uint32_t parts_list = 0;
		mem->Read(player_base + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list));
		if (!parts_list || parts_list < 0x10000) return false;

		uint32_t parts_array = 0, parts_count = 0;
		if (!read_ilist(mem, parts_list, &parts_array, &parts_count)) return false;

		uint32_t model_part = 0;
		for (int i = 0; i < parts_count && i < 64; i++)
		{
			uint32_t part = 0;
			if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
			uint32_t klass = 0;
			mem->Read(part, &klass, sizeof(klass));
			if (klass == Offsets::ModelPart_TypeInfo) {
				model_part = part;
				break;
			}
		}
		if (!model_part) return false;

		// jgi +0x18 -> CharacterBindings +0x10 -> BonesDictionary
		uint32_t ch_bindings = 0;
		mem->Read(model_part + Offsets::ModelPart_::CharacterBindings, &ch_bindings, sizeof(ch_bindings));
		if (!ch_bindings || ch_bindings < 0x10000) return false;

		uint32_t bones_dict = 0;
		mem->Read(ch_bindings + Offsets::CharacterBindings_::BonesDictionary, &bones_dict, sizeof(bones_dict));
		if (!bones_dict || bones_dict < 0x10000) return false;

		uint32_t entries = 0;
		int bone_count = 0;
		mem->Read(bones_dict + Offsets::Il2CppDictionary_::Entries, &entries, sizeof(entries));
		mem->Read(bones_dict + Offsets::Il2CppDictionary_::Count, &bone_count, sizeof(bone_count));
		if (!entries || bone_count <= 0 || bone_count > ESP_MAX_BONE_COUNT) return false;

		for (int i = 0; i < bone_count && i < ESP_MAX_BONE_COUNT; i++)
		{
			uint32_t entry = entries + (i * 16); // entry stride
			int bone_index = 0;
			uint32_t transform_ptr = 0;
			mem->Read(entry + 0x08, &bone_index, sizeof(bone_index));
			mem->Read(entry + 0x0C, &transform_ptr, sizeof(transform_ptr));
			if (bone_index < 0 || bone_index >= ESP_MAX_BONE_COUNT || !transform_ptr) continue;

			Transform t = get_transform_info(mem, transform_ptr);
			bool valid = (std::abs(t.position[0]) > 0.001f || std::abs(t.position[1]) > 0.001f || std::abs(t.position[2]) > 0.001f);
			out_skeleton_data.bones[bone_index][0] = t.position[0];
			out_skeleton_data.bones[bone_index][1] = t.position[1];
			out_skeleton_data.bones[bone_index][2] = t.position[2];
			out_skeleton_data.boneRotations[bone_index] = { t.rot[0], t.rot[1], t.rot[2], t.rot[3] };
			out_skeleton_data.valid[bone_index] = valid;
			fill_skeleton_from_bone(bone_index, t, out_skeleton);
		}
		out_skeleton_data.has_data = true;
		return has_head_or_neck_valid(out_skeleton_data);
	}

	static bool has_head_or_neck_valid(const SkeletonData& data)
	{
		if (data.valid[Offsets::HumanBodyBones_::Head] &&
			(std::abs(data.bones[Offsets::HumanBodyBones_::Head][0]) > 0.01f ||
			 std::abs(data.bones[Offsets::HumanBodyBones_::Head][1]) > 0.01f)) return true;
		if (data.valid[Offsets::HumanBodyBones_::Neck] &&
			(std::abs(data.bones[Offsets::HumanBodyBones_::Neck][0]) > 0.01f ||
			 std::abs(data.bones[Offsets::HumanBodyBones_::Neck][1]) > 0.01f)) return true;
		return false;
	}

	static void fill_skeleton_from_bone(int bone_index, const Transform& t, Skeleton& out_skeleton)
	{
		switch (bone_index)
		{
		case 10: out_skeleton.Head = t; break;
		case 9:  out_skeleton.Neck = t; break;
		case 7:  out_skeleton.Spine = t; break;
		case 8:  out_skeleton.Spine = t; break; // UpperChest
		case 11: out_skeleton.LeftShoulder = t; break;
		case 12: out_skeleton.RightShoulder = t; break;
		case 13: out_skeleton.LeftUpperarm = t; break;
		case 14: out_skeleton.RightUpperarm = t; break;
		case 15: out_skeleton.LeftForearm = t; break;
		case 16: out_skeleton.RightForearm = t; break;
		case 17: out_skeleton.LeftHand = t; break;
		case 18: out_skeleton.RightHand = t; break;
		case 0:  out_skeleton.Hip = t; break;
		case 1:  out_skeleton.LeftUpLeg = t; break;
		case 2:  out_skeleton.RightUpLeg = t; break;
		case 3:  out_skeleton.LeftLeg = t; break;
		case 4:  out_skeleton.RightLeg = t; break;
		case 5:  out_skeleton.LeftFoot = t; break;
		case 6:  out_skeleton.RightFoot = t; break;
		case 19: out_skeleton.LeftToeBase = t; break;
		case 20: out_skeleton.RightToeBase = t; break;
		}
	}

	// =====================================================================
	// Camera (ios) access
	// =====================================================================
	static uint32_t get_local_camera_component(mem::Memory* mem)
	{
		if (!mem || g_local_player < 0x10000) return 0;
		uint32_t parts_list = 0;
		mem->Read(g_local_player + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list));
		if (!parts_list || parts_list < 0x10000) return 0;
		uint32_t parts_array = 0, parts_count = 0;
		if (!read_ilist(mem, parts_list, &parts_array, &parts_count)) return 0;
		for (int i = 0; i < parts_count && i < 64; i++)
		{
			uint32_t part = 0;
			if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
			uint32_t klass = 0;
			mem->Read(part, &klass, sizeof(klass));
			if (klass == Offsets::CameraComponent_TypeInfo) return part;
		}
		return 0;
	}

	bool get_camera_position_from_camera_component(mem::Memory* mem, uint32_t camera_component, float* out_x, float* out_y, float* out_z)
	{
		if (!mem || !mem->IsReady() || !camera_component || camera_component < 0x10000 || !out_x || !out_y || !out_z)
			return false;
		uint32_t cam_transform = 0;
		mem->Read(camera_component + Offsets::CameraComponent_::CameraTransform, &cam_transform, sizeof(cam_transform));
		if (!cam_transform)
			mem->Read(camera_component + Offsets::CameraComponent_::CameraHolderTransform, &cam_transform, sizeof(cam_transform));
		if (!cam_transform || cam_transform < 0x10000) return false;
		Transform t = get_transform_info(mem, cam_transform);
		*out_x = t.position[0]; *out_y = t.position[1]; *out_z = t.position[2];
		return (std::abs(t.position[0]) > 0.001f || std::abs(t.position[1]) > 0.001f || std::abs(t.position[2]) > 0.001f);
	}

	bool get_local_aim_angles(mem::Memory* mem, uint32_t camera_component, float* out_pitch, float* out_yaw)
	{
		if (!mem || !mem->IsReady() || !camera_component || !out_pitch || !out_yaw)
			return false;
		uint32_t rotation_part = 0;
		mem->Read(camera_component + Offsets::CameraComponent_::RotationPart, &rotation_part, sizeof(rotation_part));
		if (!rotation_part || rotation_part < 0x10000) return false;
		float q[4] = { 0 };
		mem->Read(rotation_part + Offsets::RotationPart_::ViewRotation, q, sizeof(q));
		if (q[0] == 0.f && q[1] == 0.f && q[2] == 0.f && q[3] == 0.f) return false;
		float yaw = std::atan2(2.f * (q[1] * q[3] - q[0] * q[2]), 1.f - 2.f * (q[1] * q[1] + q[2] * q[2])) * 57.2957795f;
		float pitch = -std::asin(2.f * (q[0] * q[1] + q[2] * q[3])) * 57.2957795f;
		while (yaw < 0) yaw += 360.f;
		while (yaw >= 360.f) yaw -= 360.f;
		*out_pitch = pitch;
		*out_yaw = yaw;
		return true;
	}

	bool set_local_aim_angles(mem::Memory* mem, uint32_t camera_component, float pitch, float yaw)
	{
		if (!mem || !mem->IsReady() || !camera_component) return false;
		uint32_t rotation_part = 0;
		mem->Read(camera_component + Offsets::CameraComponent_::RotationPart, &rotation_part, sizeof(rotation_part));
		if (!rotation_part || rotation_part < 0x10000) return false;
		float p = pitch * (3.14159265f / 180.f);
		float y = yaw * (3.14159265f / 180.f);
		float half_pitch = p * 0.5f, half_yaw = y * 0.5f;
		float hp = std::sin(half_pitch), wp = std::cos(half_pitch);
		float hy = std::sin(half_yaw), wy = std::cos(half_yaw);
		float q[4] = { wy * hp, hy * wp, 0.f, wy * wp };
		mem->Write(rotation_part + Offsets::RotationPart_::ViewRotation, q, sizeof(float) * 4);
		return true;
	}

	// =====================================================================
	// View matrix
	// =====================================================================
	static bool read_view_matrix_from_player(mem::Memory* mem, uint32_t player_base)
	{
		if (!player_base || player_base < 0x10000) return false;
		uint32_t cam_component = get_local_camera_component(mem);
		if (!cam_component || cam_component < 0x10000) return false;
		uint32_t cam_obj = 0;
		mem->Read(cam_component + Offsets::CameraComponent_::CameraTransform, &cam_obj, sizeof(cam_obj));
		if (!cam_obj || cam_obj < 0x10000) return false;
		uint32_t native_cam = 0;
		mem->Read(cam_obj + 0x08, &native_cam, sizeof(native_cam));
		if (!native_cam || native_cam < 0x10000) return false;
		uint32_t matrix_ptr = native_cam + Offsets::NativeCamera_::MatrixOffset;
		if (!mem->Read(matrix_ptr, g_view_matrix, sizeof(g_view_matrix))) return false;
		if (g_view_matrix[15] == 0.f) return false;
		bool all_zero = true;
		for (int i = 0; i < 16; i++)
			if (g_view_matrix[i] != 0.f && (std::isfinite)(g_view_matrix[i])) { all_zero = false; break; }
		return !all_zero;
	}

	void update_view_matrix(mem::Memory* mem)
	{
		g_matrix_valid = false;
		if (!mem || !mem->IsReady()) return;
		if (g_local_player >= 0x10000 && read_view_matrix_from_player(mem, g_local_player)) {
			g_matrix_valid = true;
			return;
		}
		if (!g_players.empty()) {
			for (const auto& p : g_players) {
				if (p.base >= 0x10000 && read_view_matrix_from_player(mem, p.base)) {
					g_matrix_valid = true;
					return;
				}
			}
		}
	}

	// =====================================================================
	// Update players
	// =====================================================================
	void update_players(mem::Memory* mem)
	{
		g_players.clear();
		if (!mem || !mem->IsReady() || g_net_object_storage < 0x10000) return;

		uint8_t local_team = 0;
		if (g_local_player)
			mem->Read(g_local_player + Offsets::Player_::Team, &local_team, sizeof(local_team));

		uint32_t objects_list = 0, count = 0;
		if (!mem->Read(g_net_object_storage + Offsets::NetObjectStorage_::ObjectsList, &objects_list, sizeof(objects_list)) ||
		    !mem->Read(objects_list + Offsets::Il2CppList_::Size, &count, sizeof(count)))
			return;

		uint32_t array_ptr = 0;
		mem->Read(objects_list + Offsets::Il2CppList_::Items, &array_ptr, sizeof(array_ptr));
		if (!array_ptr || count <= 0 || count > 128) return;

		// Read our actor id for name lookup
		uint32_t transport = 0;
		mem->Read(g_client_world + Offsets::ClientWorld_::Transport, &transport, sizeof(transport));
		uint8_t my_actor_id = 0;
		uint32_t our_actor = 0;
		std::string my_nick;
		if (transport && transport > 0x10000) {
			mem->Read(transport + 0x68, &our_actor, sizeof(our_actor));
			if (our_actor && our_actor > 0x10000)
				mem->Read(our_actor + Offsets::Actor_::ActorId, &my_actor_id, sizeof(my_actor_id));
		}

		for (int i = 0; i < count; i++)
		{
			uint32_t net_obj = 0;
			if (!mem->Read(array_ptr + (uint32_t)(Offsets::Il2Cpp_::ObjectHeaderSize + i * 4), &net_obj, sizeof(net_obj)) || !net_obj)
				continue;
			uint32_t klass = 0;
			mem->Read(net_obj, &klass, sizeof(klass));
			if (klass != Offsets::Player_TypeInfo) continue;

			uint8_t owner_id = 0;
			mem->Read(net_obj + Offsets::NetObject_::OwnerId, &owner_id, sizeof(owner_id));
			uint8_t team = 0;
			mem->Read(net_obj + Offsets::Player_::Team, &team, sizeof(team));

			if (net_obj == g_local_player) continue;
			if (team != 0 && team == local_team) continue;

			PlayerData p;
			p.base = net_obj;
			mem->Read(net_obj + Offsets::Player_::CameraLinks, &p.camera_links, sizeof(p.camera_links));
			p.name = "Player";
			p.team = team;

			// Position from transform part (dwc +0x14 -> root Transform)
			uint32_t transform_part = 0;
			mem->Read(net_obj + Offsets::Player_::TransformPart, &transform_part, sizeof(transform_part));
			if (transform_part && transform_part > 0x10000) {
				uint32_t root_trans = 0;
				mem->Read(transform_part + Offsets::TransformPart_::RootTransform, &root_trans, sizeof(root_trans));
				if (root_trans && root_trans > 0x10000) {
					Transform t = get_transform_info(mem, root_trans);
					p.x = t.position[0]; p.y = t.position[1]; p.z = t.position[2];
				}
			}

			// HP/Armor from stats part (ylw at ake+0x44)
			uint32_t stats_part = 0;
			mem->Read(net_obj + Offsets::Player_::StatsPart, &stats_part, sizeof(stats_part));
			if (stats_part && stats_part > 0x10000) {
				uint16_t hp = 0, armor = 0;
				mem->Read(stats_part + Offsets::StatsPart_::Health, &hp, sizeof(hp));
				mem->Read(stats_part + Offsets::StatsPart_::Armor, &armor, sizeof(armor));
				p.health = hp;
				p.armor = armor;
			}

			uint8_t state = 0;
			mem->Read(net_obj + Offsets::Player_::State, &state, sizeof(state));
			p.is_alive = (state != 0);

			p.is_valid = (p.x != 0.f || p.y != 0.f || p.z != 0.f);

			if (g_esp_skeleton || g_aimbot_enabled || g_esp_hitbox_outline_fill || g_esp_hitbox_glow ||
			    triggerbot::g_trigger_enabled || g_aimbot_visible_check)
				p.has_skeleton = get_skeleton_bones(mem, net_obj, p.skeleton, p.skeleton_data);

			p.weapon = get_player_weapon(mem, net_obj);
			g_players.push_back(std::move(p));
		}
	}

	// =====================================================================
	// Weapon name lookup
	// =====================================================================
	static std::string get_player_weapon(mem::Memory* mem, uint32_t player_base)
	{
		if (!mem || !mem->IsReady() || player_base < 0x10000) return "None";
		uint32_t parts_list = 0;
		mem->Read(player_base + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list));
		if (!parts_list || parts_list < 0x10000) return "None";

		uint32_t parts_array = 0, parts_count = 0;
		if (!read_ilist(mem, parts_list, &parts_array, &parts_count)) return "None";

		uint32_t weapon_part = 0;
		for (int i = 0; i < parts_count && i < 64; i++)
		{
			uint32_t part = 0;
			if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
			uint32_t klass = 0;
			mem->Read(part, &klass, sizeof(klass));
			if (klass == Offsets::WeaponPart_TypeInfo) {
				weapon_part = part;
				break;
			}
		}
		if (!weapon_part) return "None";

		// nrk +0x18 (WeaponContext) +0x54 -> qjf ActiveWeapon
		uint32_t weapon_ctx = weapon_part + Offsets::WeaponPart_::WeaponContext;
		uint32_t active_weapon = 0;
		mem->Read(weapon_ctx + Offsets::WeaponContext_::ActiveWeapon, &active_weapon, sizeof(active_weapon));
		if (!active_weapon || active_weapon < 0x10000) return "None";

		uint8_t weapon_id = 0;
		mem->Read(active_weapon + Offsets::WeaponInstance_::WeaponId, &weapon_id, sizeof(weapon_id));
		if (weapon_id == 0) return "None";

		// Find weapon provider and look up definition by WeaponId
		uint32_t weapon_provider = find_by_typeinfo(mem, g_unity_base + Offsets::WeaponProvider_TypeInfo);
		if (!weapon_provider || weapon_provider < 0x10000) return "None";

		uint32_t weapon_dict = 0;
		mem->Read(weapon_provider + 0x0C, &weapon_dict, sizeof(weapon_dict));
		if (!weapon_dict || weapon_dict < 0x10000) return "None";

		uint32_t definition = 0;
		if (dict_get_int(mem, weapon_dict, weapon_id, &definition) && definition && definition > 0x10000) {
			uint32_t name_ptr = 0;
			mem->Read(definition + Offsets::WeaponDefinition_::Name, &name_ptr, sizeof(name_ptr));
			if (name_ptr && name_ptr > 0x10000) {
				std::string name;
				if (read_unity_string(mem, name_ptr, name)) return name;
			}
		}
		return "None";
	}

	// =====================================================================
	// is_local_shooting — stub for triggerbot
	// =====================================================================
	bool is_local_shooting()
	{
		if (g_local_player < 0x10000) return false;
		return false;
	}

	// =====================================================================
	// Grenade tracking
	// =====================================================================
	void update_grenades(mem::Memory* mem)
	{
		g_grenades.clear();
		g_deployed_smokes.clear();
		g_deployed_molotovs.clear();
		if (!mem || !mem->IsReady() || g_net_object_storage < 0x10000) return;

		uint32_t objects_list = 0, count = 0;
		if (!mem->Read(g_net_object_storage + Offsets::NetObjectStorage_::ObjectsList, &objects_list, sizeof(objects_list)) ||
		    !mem->Read(objects_list + Offsets::Il2CppList_::Size, &count, sizeof(count)))
			return;

		uint32_t array_ptr = 0;
		mem->Read(objects_list + Offsets::Il2CppList_::Items, &array_ptr, sizeof(array_ptr));
		if (!array_ptr || count <= 0 || count > 256) return;

		for (int i = 0; i < count; i++)
		{
			uint32_t net_obj = 0;
			if (!mem->Read(array_ptr + (uint32_t)(Offsets::Il2Cpp_::ObjectHeaderSize + i * 4), &net_obj, sizeof(net_obj)) || !net_obj)
				continue;
			uint32_t klass = 0;
			mem->Read(net_obj, &klass, sizeof(klass));
			if (klass == Offsets::Grenade_TypeInfo) {
				GrenadeData gd;
				uint32_t transform_part = 0;
				mem->Read(net_obj + Offsets::Grenade_::TransformPart, &transform_part, sizeof(transform_part));
				if (transform_part && transform_part > 0x10000) {
					uint32_t root_trans = 0;
					mem->Read(transform_part + Offsets::TransformPart_::RootTransform, &root_trans, sizeof(root_trans));
					if (root_trans && root_trans > 0x10000) {
						Transform t = get_transform_info(mem, root_trans);
						gd.x = t.position[0]; gd.y = t.position[1]; gd.z = t.position[2];
					}
				}
				if (gd.x != 0.f || gd.y != 0.f || gd.z != 0.f)
					g_grenades.push_back(gd);
			}
		}
	}

	// =====================================================================
	// Rendering helpers
	// =====================================================================
	bool world_to_screen(const float* pos, float* out, const float* matrix, float width, float height)
	{
		float x = pos[0] * matrix[0] + pos[1] * matrix[4] + pos[2] * matrix[8] + matrix[12];
		float y = pos[0] * matrix[1] + pos[1] * matrix[5] + pos[2] * matrix[9] + matrix[13];
		float w = pos[0] * matrix[3] + pos[1] * matrix[7] + pos[2] * matrix[11] + matrix[15];
		if (w < 0.01f) return false;
		float inv_w = 1.0f / w;
		x *= inv_w; y *= inv_w;
		out[0] = (width * 0.5f) + (x * width * 0.5f);
		out[1] = (height * 0.5f) - (y * height * 0.5f);
		return true;
	}

	static void skeleton_correct_by_hip(PlayerData& p_draw)
	{
		Skeleton& skel = p_draw.skeleton;
		if (skel.Hip.position[0] == 0 && skel.Hip.position[1] == 0 && skel.Hip.position[2] == 0) return;
		float dx = (std::abs)(skel.Hip.position[0] - p_draw.x);
		float dy = (std::abs)(skel.Hip.position[1] - p_draw.y);
		float dz = (std::abs)(skel.Hip.position[2] - p_draw.z);
		if (dx <= 0.23f && dz <= 0.23f && dy <= 1.1f) return;
		float offset_x = p_draw.x - skel.Hip.position[0];
		float offset_y = p_draw.y - skel.Hip.position[1] + 0.9f;
		float offset_z = p_draw.z - skel.Hip.position[2];
		Transform* bones[] = {
			&skel.Head, &skel.Neck, &skel.Spine, &skel.LeftShoulder, &skel.LeftUpperarm,
			&skel.LeftForearm, &skel.LeftHand, &skel.RightShoulder, &skel.RightUpperarm,
			&skel.RightForearm, &skel.RightHand, &skel.Hip, &skel.LeftUpLeg, &skel.LeftLeg,
			&skel.LeftFoot, &skel.LeftToeBase, &skel.RightUpLeg, &skel.RightLeg,
			&skel.RightFoot, &skel.RightToeBase
		};
		for (int i = 0; i < 20; i++)
		{
			if (bones[i]->position[0] != 0 || bones[i]->position[1] != 0 || bones[i]->position[2] != 0) {
				bones[i]->position[0] += offset_x;
				bones[i]->position[1] += offset_y;
				bones[i]->position[2] += offset_z;
			}
		}
		for (int i = 0; i < ESP_MAX_BONE_COUNT; i++)
		{
			if (p_draw.skeleton_data.valid[i]) {
				p_draw.skeleton_data.bones[i][0] += offset_x;
				p_draw.skeleton_data.bones[i][1] += offset_y;
				p_draw.skeleton_data.bones[i][2] += offset_z;
			}
		}
	}

	static inline Vec3 vec3(float x, float y, float z) { Vec3 v = { x, y, z }; return v; }
	static inline Vec3 quat_mul_vec3(const Quat& q, Vec3 v) {
		float x = q.x * 2.f, y = q.y * 2.f, z = q.z * 2.f;
		float xx = q.x * x, yy = q.y * y, zz = q.z * z, xy = q.x * y, xz = q.x * z, yz = q.y * z;
		float wx = q.w * x, wy = q.w * y, wz = q.w * z;
		return vec3(
			(1.f - (yy + zz)) * v.x + (xy - wz) * v.y + (xz + wy) * v.z,
			(xy + wz) * v.x + (1.f - (xx + zz)) * v.y + (yz - wx) * v.z,
			(xz - wy) * v.x + (yz + wx) * v.y + (1.f - (xx + yy)) * v.z
		);
	}
	static bool world_to_screen_vec3(const float* matrix, float width, float height, Vec3 pos, ImVec2& out) {
		float s[2]; float p[3] = { pos.x, pos.y, pos.z };
		if (!world_to_screen(p, s, matrix, width, height)) return false;
		out.x = s[0]; out.y = s[1]; return true;
	}

	static const float g_head_radius = 0.100f;
	static const float g_head_height = 0.300f;
	static const float g_head_offset_y = 0.100f;

	struct HitboxDef { int bone; int type; Vec3 center; Vec3 size; int direction; };
	static const HitboxDef g_hitboxDefs[] = {
		{0, 1, {0.0000f, 0.1000f, 0.0000f}, {0.115f, 0.23f, 0.115f}, 1},
		{1, 1, {0.0000f, 0.0560f, -0.0230f}, {0.0776f, 0.1934f, 0.0776f}, 1},
		{3, 0, {0.0000f, -0.0100f, 0.0014f}, {0.3254f, 0.1884f, 0.2497f}, 0},
		{4, 0, {0.0000f, 0.1167f, 0.0013f}, {0.3654f, 0.2567f, 0.2315f}, 0},
		{5, 1, {-0.0933f, -0.0125f, 0.0000f}, {0.0926f, 0.2486f, 0.0926f}, 0},
		{6, 1, {-0.1420f, 0.0000f, 0.0000f}, {0.0700f, 0.3124f, 0.0700f}, 0},
		{7, 1, {-0.1141f, 0.0000f, -0.0000f}, {0.0600f, 0.3460f, 0.0600f}, 0},
		{8, 0, {-0.0466f, -0.0140f, -0.0090f}, {0.1511f, 0.0798f, 0.0994f}, 0},
		{9, 1, {0.0933f, 0.0162f, -0.0000f}, {0.0926f, 0.2486f, 0.0926f}, 1},
		{10, 1, {0.1141f, -0.0000f, -0.0001f}, {0.0600f, 0.3460f, 0.0600f}, 1},
		{11, 1, {0.1420f, -0.0000f, -0.0000f}, {0.0700f, 0.3124f, 0.0700f}, 0},
		{12, 0, {0.0466f, 0.0186f, 0.0090f}, {0.1511f, 0.0798f, 0.0994f}, 0},
		{13, 0, {0.0000f, 0.0000f, 0.0002f}, {0.3390f, 0.1367f, 0.2543f}, 0},
		{14, 1, {-0.2156f, 0.0067f, 0.0103f}, {0.0980f, 0.5744f, 0.0980f}, 0},
		{15, 1, {-0.2059f, -0.0079f, 0.0000f}, {0.0830f, 0.5760f, 0.0830f}, 0},
		{17, 0, {0.0419f, 0.0227f, -0.0019f}, {0.2887f, 0.0876f, 0.1218f}, 0},
		{18, 1, {0.2156f, 0.0067f, -0.0103f}, {0.0980f, 0.5744f, 0.0980f}, 0},
		{19, 1, {0.2059f, -0.0079f, 0.0000f}, {0.0830f, 0.5760f, 0.0830f}, 0},
		{21, 0, {-0.0374f, -0.0299f, 0.0025f}, {0.2887f, 0.0876f, 0.1218f}, 0},
	};

	HitboxData get_real_hitboxes(const SkeletonData& skeleton) {
		HitboxData data = {};
		if (!skeleton.has_data) return data;
		const int numDefs = (int)(sizeof(g_hitboxDefs) / sizeof(g_hitboxDefs[0]));
		for (int i = 0; i < numDefs && data.count < ESP_MAX_HITBOX_COUNT; i++) {
			const HitboxDef& def = g_hitboxDefs[i];
			if (def.bone < 0 || def.bone >= ESP_MAX_BONE_COUNT || !skeleton.valid[def.bone]) continue;
			Hitbox& hb = data.hitboxes[data.count];
			hb.bone = def.bone; hb.type = def.type; hb.center = def.center; hb.size = def.size; hb.direction = def.direction;
			if (def.bone == 0) {
				hb.size.x = g_head_radius; hb.size.z = g_head_radius; hb.size.y = g_head_height; hb.center.y = g_head_offset_y;
			}
			hb.valid = true; data.count++;
		}
		data.has_data = (data.count > 0);
		return data;
	}

	static bool IntersectRayCapsule(Vec3 o, Vec3 d, Vec3 a, Vec3 b, float r, float& t) {
		t = -1.f; Vec3 ba = b - a, oa = o - a;
		float baba = vec3_dot(ba, ba), bard = vec3_dot(ba, d), baoa = vec3_dot(ba, oa);
		float rdoa = vec3_dot(d, oa), oaoa = vec3_dot(oa, oa);
		float aa = baba - bard * bard, bb = baba * rdoa - baoa * bard, cc = baba * oaoa - baoa * baoa - r * r * baba;
		float h = bb * bb - aa * cc;
		if (h >= 0.0f) {
			float tempT = (-bb - std::sqrt(h)) / aa;
			float y = baoa + tempT * bard;
			if (y > 0.0f && y < baba) { t = tempT; return true; }
			Vec3 oc = (y <= 0.0f) ? oa : (o - b);
			float b2 = vec3_dot(d, oc), c2 = vec3_dot(oc, oc) - r * r, discr = b2 * b2 - c2;
			if (discr >= 0.0f) { t = -b2 - std::sqrt(discr); return t > 0.0f; }
		}
		return false;
	}

	static bool IntersectRayOBB(Vec3 rayOrigin, Vec3 rayDir, Vec3 boxCenter, const Quat& boxRot, Vec3 halfExtents, float& t) {
		Quat invRot = { -boxRot.x, -boxRot.y, -boxRot.z, boxRot.w };
		Vec3 localO = quat_mul_vec3(invRot, rayOrigin - boxCenter);
		Vec3 localD = quat_mul_vec3(invRot, rayDir);
		float tmin = -1e9f, tmax = 1e9f;
		const float* o = &localO.x, * d = &localD.x, * h = &halfExtents.x;
		for (int i = 0; i < 3; i++) {
			if (std::fabs(d[i]) < 1e-7f) {
				if (o[i] < -h[i] || o[i] > h[i]) return false;
				continue;
			}
			float t1 = (-h[i] - o[i]) / d[i], t2 = (h[i] - o[i]) / d[i];
			if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			if (tmin > tmax || tmax < 0.0f) return false;
		}
		t = (tmin > 0.0f) ? tmin : tmax;
		return t > 0.0f;
	}

	static const int s_triggerbot_exclude_bones[] = { 7, 8, 11, 12 };
	static const int s_triggerbot_exclude_count = (int)(sizeof(s_triggerbot_exclude_bones) / sizeof(s_triggerbot_exclude_bones[0]));

	static float GetHitboxIntersectionDistance(Vec3 rayOrigin, Vec3 rayDir, const HitboxData& hitboxes, const SkeletonData& skel, const int* excludeBones, int excludeCount) {
		auto is_excluded = [excludeBones, excludeCount](int bone) {
			for (int k = 0; k < excludeCount; k++) if (excludeBones[k] == bone) return true;
			return false;
		};
		float minHit = FLT_MAX; bool hitAny = false;
		for (int i = 0; i < hitboxes.count; i++) {
			const Hitbox& hb = hitboxes.hitboxes[i];
			if (!hb.valid) continue;
			if (hb.bone < 0 || hb.bone >= ESP_MAX_BONE_COUNT || !skel.valid[hb.bone]) continue;
			if (excludeBones && is_excluded(hb.bone)) continue;
			Vec3 bonePos = { skel.bones[hb.bone][0], skel.bones[hb.bone][1], skel.bones[hb.bone][2] };
			Quat boneRot = skel.boneRotations[hb.bone];
			Vec3 worldCenter = bonePos + quat_mul_vec3(boneRot, hb.center);
			float hitT = -1.f;
			if (hb.type == 0) {
				Vec3 half = vec3(hb.size.x * 0.5f, hb.size.y * 0.5f, hb.size.z * 0.5f);
				if (IntersectRayOBB(rayOrigin, rayDir, worldCenter, boneRot, half, hitT)) {}
			} else {
				Vec3 localUp = (hb.direction == 0) ? vec3(1, 0, 0) : ((hb.direction == 1) ? vec3(0, 1, 0) : vec3(0, 0, 1));
				Vec3 up = quat_mul_vec3(boneRot, localUp);
				float cylinderLen = (hb.size.y - (hb.size.x * 2.0f)) > 0.0f ? (hb.size.y - (hb.size.x * 2.0f)) : 0.0f;
				float halfLen = cylinderLen * 0.5f;
				Vec3 p1 = worldCenter - (up * halfLen), p2 = worldCenter + (up * halfLen);
				if (IntersectRayCapsule(rayOrigin, rayDir, p1, p2, hb.size.x, hitT)) {}
			}
			if (hitT > 0.0f && hitT < minHit) { minHit = hitT; hitAny = true; }
		}
		return hitAny ? minHit : -1.0f;
	}

	static const std::vector<std::string>& GetPenetrableMaterialSubstrings() {
		static const std::vector<std::string> sandstone = { };
		static const std::vector<std::string> province = sandstone;
		static const std::vector<std::string> dune = {
			"mesh_963", "mesh_805", "mesh_961", "mesh_796", "mesh_962", "mesh_811", "mesh_802", "mesh_964"
		};
		static const std::vector<std::string> breeze = { "mesh_1510", "mesh_1619", "mesh_1601" };
		static const std::vector<std::string> hanami = { "mesh_824 atlas_2", "mesh_825 atlas_6" };
		static const std::vector<std::string> rust = { "Mesh_732.004 13_atlas.001", "Mesh_735.004 13_atlas.001" };
		int idx = (map_service::g_collider_map_index >= 0 && map_service::g_collider_map_index < map_service::g_collider_map_count) ? map_service::g_collider_map_index : 0;
		if (idx == 6) return hanami; if (idx == 5) return breeze;
		if (idx == 3) return rust; if (idx == 2) return dune;
		if (idx == 1) return province; return sandstone;
	}

	static void matrix4_inv(const float* m, float* out) {
		float inv[16];
		inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
		inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
		inv[2]  =  m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
		inv[3]  = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
		inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
		inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
		inv[6]  = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
		inv[7]  =  m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
		inv[8]  =  m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
		inv[9]  = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
		inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
		inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
		inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
		inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
		inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
		inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];
		float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
		if (std::fabs(det) < 1e-10f) { for (int i = 0; i < 16; i++) out[i] = (i % 5 == 0) ? 1.0f : 0.0f; return; }
		float invDet = 1.0f / det;
		for (int i = 0; i < 16; i++) out[i] = inv[i] * invDet;
	}

	bool GetRayFromScreenCenter(const float* viewProj, Vec3& origin, Vec3& dir) {
		float inv[16]; matrix4_inv(viewProj, inv);
		float cn[4] = { 0, 0, 0, 1 }, cf[4] = { 0, 0, 1, 1 }, wn[4], wf[4];
		for (int i = 0; i < 4; i++) {
			wn[i] = inv[i]*cn[0] + inv[4+i]*cn[1] + inv[8+i]*cn[2] + inv[12+i]*cn[3];
			wf[i] = inv[i]*cf[0] + inv[4+i]*cf[1] + inv[8+i]*cf[2] + inv[12+i]*cf[3];
		}
		if (std::fabs(wn[3]) < 1e-8f || std::fabs(wf[3]) < 1e-8f) return false;
		float invWn = 1.0f / wn[3], invWf = 1.0f / wf[3];
		origin.x = wn[0] * invWn; origin.y = wn[1] * invWn; origin.z = wn[2] * invWn;
		Vec3 farPt = { wf[0] * invWf, wf[1] * invWf, wf[2] * invWf };
		dir.x = farPt.x - origin.x; dir.y = farPt.y - origin.y; dir.z = farPt.z - origin.z;
		float len = std::sqrt(dir.x*dir.x + dir.y*dir.y + dir.z*dir.z);
		if (len < 1e-6f) return false;
		dir.x /= len; dir.y /= len; dir.z /= len;
		return true;
	}

	const PlayerData* GetEntityInCrosshairWithMapCheck(const Vec3& rayOrigin, const Vec3& rayDir, float maxWallDist,
		const std::vector<PlayerData>& players, uint8_t local_team) {
		(void)maxWallDist;
		if (!map_service::g_loaded) return nullptr;
		MapVector3 o(rayOrigin.x, rayOrigin.y, rayOrigin.z);
		MapVector3 d(rayDir.x, rayDir.y, rayDir.z);
		float wallInFace = map_service::RaycastWallClosest(o, d, 0.5f);
		if (wallInFace < FLT_MAX && wallInFace <= 0.5f) return nullptr;
		const std::vector<std::string>& penetrable = GetPenetrableMaterialSubstrings();
		static const char* boxNames[] = { "Mesh_1573", "Mesh_1576", "Mesh_1451", "Mesh_1574" };
		const float box_margin = 0.45f;
		static MapVector3 s_boxMin[4], s_boxMax[4];
		static int s_boxCacheMapIndex = -1;
		if (s_boxCacheMapIndex != map_service::g_collider_map_index) {
			s_boxCacheMapIndex = map_service::g_collider_map_index;
			for (int b = 0; b < 4; b++)
				map_service::GetColliderAABB(boxNames[b], &s_boxMin[b], &s_boxMax[b]);
		}
		float closestEnemyDist = FLT_MAX;
		const PlayerData* bestTarget = nullptr;
		for (const auto& p : players) {
			if (!p.is_valid || !p.has_skeleton) continue;
			if (p.base == g_local_player) continue;
			if (p.team != 0 && p.team == local_team) continue;
			if (p.health < 1) continue;
			HitboxData hitboxes = get_real_hitboxes(p.skeleton_data);
			if (!hitboxes.has_data) continue;
			float hitDist = GetHitboxIntersectionDistance(rayOrigin, rayDir, hitboxes, p.skeleton_data, s_triggerbot_exclude_bones, s_triggerbot_exclude_count);
			if (hitDist <= 0.0f || hitDist >= closestEnemyDist) continue;
			MapVector3 o2(rayOrigin.x, rayOrigin.y, rayOrigin.z);
			MapVector3 d2(rayDir.x, rayDir.y, rayDir.z);
			float wallT = map_service::RaycastWallExcluding(o2, d2, hitDist - 0.02f, penetrable);
			if (wallT < FLT_MAX && wallT < hitDist - 0.05f) {
				std::string hitName;
				float closestT = map_service::RaycastClosestWithName(o2, d2, hitDist - 0.02f, &hitName);
				if (closestT >= hitDist - 0.05f) continue;
				std::string nameLower = hitName;
				for (char& c : nameLower) c = (char)std::tolower((unsigned char)c);
				int boxIdx = -1;
				if (nameLower.find("mesh_1573") != std::string::npos) boxIdx = 0;
				else if (nameLower.find("mesh_1576") != std::string::npos) boxIdx = 1;
				else if (nameLower.find("mesh_1451") != std::string::npos) boxIdx = 2;
				else if (nameLower.find("mesh_1574") != std::string::npos) boxIdx = 3;
				if (boxIdx < 0) continue;
				const MapVector3& boxMin = s_boxMin[boxIdx], &boxMax = s_boxMax[boxIdx];
				float px = p.x, py = p.y, pz = p.z;
				if (px < boxMin.x - box_margin || px > boxMax.x + box_margin ||
				    py < boxMin.y - box_margin || py > boxMax.y + box_margin ||
				    pz < boxMin.z - box_margin || pz > boxMax.z + box_margin)
					continue;
			}
			closestEnemyDist = hitDist;
			bestTarget = &p;
		}
		return bestTarget;
	}

	// =====================================================================
	// Skeleton rendering
	// =====================================================================
	void render_skeleton_lines(ImDrawList* draw_list, const float* matrix, const Skeleton& sk, ImU32 color, float screen_w, float screen_h)
	{
		const float* spine[] = { sk.Neck.position, sk.Spine.position, sk.Hip.position };
		const float* left_arm[] = { sk.Neck.position, sk.LeftUpperarm.position, sk.LeftForearm.position, sk.LeftHand.position };
		const float* right_arm[] = { sk.Neck.position, sk.RightUpperarm.position, sk.RightForearm.position, sk.RightHand.position };
		const float* left_leg[] = { sk.Hip.position, sk.LeftLeg.position, sk.LeftFoot.position };
		const float* right_leg[] = { sk.Hip.position, sk.RightLeg.position, sk.RightFoot.position };
		draw_skeleton_branch(draw_list, matrix, spine, 3, color, screen_w, screen_h);
		draw_skeleton_branch(draw_list, matrix, left_arm, 4, color, screen_w, screen_h);
		draw_skeleton_branch(draw_list, matrix, right_arm, 4, color, screen_w, screen_h);
		draw_skeleton_branch(draw_list, matrix, left_leg, 3, color, screen_w, screen_h);
		draw_skeleton_branch(draw_list, matrix, right_leg, 3, color, screen_w, screen_h);
	}

	static void draw_skeleton_branch(ImDrawList* draw_list, const float* matrix, const float* const* world_pts, int count, ImU32 color, float screen_w, float screen_h)
	{
		ImVec2 screen_pts[8];
		int n = 0;
		for (int i = 0; i < count && n < 8; i++) {
			if (!valid_bone(world_pts[i])) return;
			float s[2];
			if (!world_to_screen(world_pts[i], s, matrix, screen_w, screen_h)) return;
			screen_pts[n++] = ImVec2(s[0], s[1]);
		}
		if (n < 2) return;
		draw_list->AddPolyline(screen_pts, n, color, ImDrawFlags_None, g_skeleton_thickness);
	}

	static bool valid_bone(const float* p)
	{
		if (p[0] == 0.f && p[1] == 0.f && p[2] == 0.f) return false;
		if (!(std::isfinite)(p[0]) || !(std::isfinite)(p[1]) || !(std::isfinite)(p[2])) return false;
		if ((std::abs)(p[0]) > 10000.f || (std::abs)(p[1]) > 10000.f || (std::abs)(p[2]) > 10000.f) return false;
		return true;
	}

	// =====================================================================
	// Hitbox drawing (chams/glow)
	// =====================================================================
	static void draw_hitbox_obbs(ImDrawList* draw, const float* matrix, float screen_w, float screen_h, Vec3 pos, const Quat& rot, Vec3 center, Vec3 size, ImU32 col) {
		Vec3 halfSize = vec3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
		Vec3 corners[8] = {
			vec3(-halfSize.x, -halfSize.y, -halfSize.z), vec3(halfSize.x, -halfSize.y, -halfSize.z),
			vec3(halfSize.x, halfSize.y, -halfSize.z), vec3(-halfSize.x, halfSize.y, -halfSize.z),
			vec3(-halfSize.x, -halfSize.y, halfSize.z), vec3(halfSize.x, -halfSize.y, halfSize.z),
			vec3(halfSize.x, halfSize.y, halfSize.z), vec3(-halfSize.x, halfSize.y, halfSize.z)
		};
		ImVec2 sc[8]; bool vis[8];
		for (int i = 0; i < 8; i++) {
			Vec3 wc = pos + quat_mul_vec3(rot, center + corners[i]);
			vis[i] = world_to_screen_vec3(matrix, screen_w, screen_h, wc, sc[i]);
		}
		auto dl = [&](int i, int j) { if (vis[i] && vis[j]) draw->AddLine(sc[i], sc[j], col, 1.0f); };
		dl(0,1); dl(1,2); dl(2,3); dl(3,0); dl(4,5); dl(5,6); dl(6,7); dl(7,4); dl(0,4); dl(1,5); dl(2,6); dl(3,7);
	}

	static void draw_sphere_wireframe(ImDrawList* draw, const float* matrix, float sw, float sh, Vec3 center, float radius, const Quat& orient, ImU32 color, int segments) {
		Vec3 up = quat_mul_vec3(orient, vec3(0, 1, 0));
		float ul = vec3_mag(up);
		if (ul < 0.0001f) up = vec3(0, 1, 0); else up = up * (1.f / ul);
		Vec3 right = vec3_cross(up, vec3(0, 0, 1));
		float rl = vec3_mag(right);
		if (rl < 0.0001f) right = vec3_cross(up, vec3(1, 0, 0));
		right = vec3_normalize(right);
		Vec3 fwd = vec3_normalize(vec3_cross(right, up));
		const float tp = 6.283185307f;
		for (float lat : { 0.52f, 1.05f, 1.57f, 2.09f, 2.62f }) {
			float cy = radius * std::cos(lat), r = radius * std::sin(lat);
			if (r < 0.001f) continue;
			Vec3 rc = center + (up * cy); ImVec2 prev; bool ok = false;
			for (int i = 0; i <= segments; i++) {
				float a = (float)i / segments * tp;
				Vec3 pt = rc + (right * (std::cos(a) * r)) + (fwd * (std::sin(a) * r));
				ImVec2 scr; if (world_to_screen_vec3(matrix, sw, sh, pt, scr)) {
					if (ok) draw->AddLine(prev, scr, color, 1.f);
					prev = scr; ok = true;
				} else ok = false;
			}
		}
		for (int m = 0; m < 6; m++) {
			float phi = (float)m / 6 * tp;
			Vec3 axis = (right * std::cos(phi)) + (fwd * std::sin(phi));
			ImVec2 prev; bool ok = false;
			for (int i = 0; i <= segments; i++) {
				float th = (float)i / segments * 3.14159265f;
				Vec3 pt = center + (axis * (radius * std::sin(th))) + (up * (radius * std::cos(th)));
				ImVec2 scr; if (world_to_screen_vec3(matrix, sw, sh, pt, scr)) {
					if (ok) draw->AddLine(prev, scr, color, 1.f);
					prev = scr; ok = true;
				} else ok = false;
			}
		}
	}

	static void draw_capsule_wireframe(ImDrawList* draw, const float* m, float sw, float sh, Vec3 from, Vec3 to, float r1, float r2, ImU32 color, int segments) {
		Vec3 d = to - from, len_v = d;
		float len = vec3_mag(len_v);
		Vec3 dir = (len < 0.0001f) ? vec3(0, 1, 0) : vec3(d.x/len, d.y/len, d.z/len);
		Vec3 right = vec3_normalize(vec3_cross(dir, (std::abs(dir.y) < 0.99f) ? vec3(0,1,0) : vec3(1,0,0)));
		Vec3 fwd = vec3_normalize(vec3_cross(right, dir));
		const float tp = 6.283185307f;
		auto circle = [&](Vec3 c, Vec3 u, Vec3 v, float r) {
			ImVec2 prev; bool ok = false;
			for (int i = 0; i <= segments; i++) {
				float a = (float)i / segments * tp;
				Vec3 pt = c + vec3(u.x*(std::cos(a)*r) + v.x*(std::sin(a)*r), u.y*(std::cos(a)*r) + v.y*(std::sin(a)*r), u.z*(std::cos(a)*r) + v.z*(std::sin(a)*r));
				ImVec2 scr; if (world_to_screen_vec3(m, sw, sh, pt, scr)) { if (ok) draw->AddLine(prev, scr, color, 1.f); prev = scr; ok = true; } else ok = false;
			}
		};
		circle(from, right, fwd, r1); circle(to, right, fwd, r2);
		circle(from, dir, fwd, r1); circle(from, dir, right, r1);
		circle(to, dir, fwd, r2); circle(to, dir, right, r2);
		Vec3 d1 = vec3_normalize(vec3(right.x+fwd.x, right.y+fwd.y, right.z+fwd.z));
		Vec3 d2 = vec3_normalize(vec3(right.x-fwd.x, right.y-fwd.y, right.z-fwd.z));
		circle(from, dir, d2, r1); circle(from, dir, d1, r1);
		circle(to, dir, d2, r2); circle(to, dir, d1, r2);
		if (len >= 0.01f) {
			for (int i = 0; i < 4; i++) {
				float a = (float)i / 4 * tp;
				Vec3 off = vec3(right.x*std::cos(a)+fwd.x*std::sin(a), right.y*std::cos(a)+fwd.y*std::sin(a), right.z*std::cos(a)+fwd.z*std::sin(a));
				ImVec2 p1, p2;
				if (world_to_screen_vec3(m, sw, sh, vec3(from.x+off.x*r1, from.y+off.y*r1, from.z+off.z*r1), p1) &&
					world_to_screen_vec3(m, sw, sh, vec3(to.x+off.x*r2, to.y+off.y*r2, to.z+off.z*r2), p2))
					draw->AddLine(p1, p2, color, 1.f);
			}
		}
	}

	static void draw_hitboxes(ImDrawList* draw, const float* matrix, float sw, float sh, const SkeletonData& skel, const HitboxData& hitboxes, ImU32 color) {
		if (!skel.has_data || !hitboxes.has_data) return;
		for (int i = 0; i < hitboxes.count; i++) {
			const Hitbox& hb = hitboxes.hitboxes[i];
			if (!hb.valid || hb.bone < 0 || hb.bone >= ESP_MAX_BONE_COUNT || !skel.valid[hb.bone]) continue;
			Vec3 bp = { skel.bones[hb.bone][0], skel.bones[hb.bone][1], skel.bones[hb.bone][2] };
			const Quat& br = skel.boneRotations[hb.bone];
			if (hb.type == 0) draw_hitbox_obbs(draw, matrix, sw, sh, bp, br, hb.center, hb.size, color);
			else {
				float r = hb.size.x, h = hb.size.y, halfH = (h - (r * 2.0f)) * 0.5f;
				if (halfH < 0.001f) halfH = 0.001f;
				Vec3 ha = (hb.direction == 0) ? vec3(halfH,0,0) : ((hb.direction == 1) ? vec3(0,halfH,0) : vec3(0,0,halfH));
				Vec3 f = bp + quat_mul_vec3(br, hb.center - ha), t = bp + quat_mul_vec3(br, hb.center + ha);
				draw_capsule_wireframe(draw, matrix, sw, sh, f, t, r, r, color, 24);
			}
		}
	}

	static Path64 get_convex_hull_path(const std::vector<ImVec2>& points) {
		if (points.size() < 3) return Path64();
		std::vector<ImVec2> hull = points;
		std::sort(hull.begin(), hull.end(), [](const ImVec2& a, const ImVec2& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
		auto cross = [](ImVec2 a, ImVec2 b, ImVec2 c) { return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x); };
		std::vector<ImVec2> res;
		for (size_t i = 0; i < hull.size(); ++i) { while (res.size() >= 2 && cross(res[res.size()-2], res.back(), hull[i]) <= 0) res.pop_back(); res.push_back(hull[i]); }
		for (int i = (int)hull.size()-2, t = (int)res.size()+1; i >= 0; i--) { while (res.size() >= (size_t)t && cross(res[res.size()-2], res.back(), hull[i]) <= 0) res.pop_back(); res.push_back(hull[i]); }
		if (!res.empty()) res.pop_back();
		Path64 path;
		for (const auto& p : res) path.push_back(Point64((int64_t)(p.x*100.0), (int64_t)(p.y*100.0)));
		return path;
	}

	static void get_hitbox_screen_points(const float* matrix, float sw, float sh, const Hitbox& hb, const SkeletonData& skel, std::vector<ImVec2>& out) {
		if (hb.bone < 0 || hb.bone >= ESP_MAX_BONE_COUNT || !skel.valid[hb.bone]) return;
		Vec3 bp = { skel.bones[hb.bone][0], skel.bones[hb.bone][1], skel.bones[hb.bone][2] };
		const Quat& br = skel.boneRotations[hb.bone];
		if (hb.type == 0) {
			Vec3 hs = vec3(hb.size.x*0.5f, hb.size.y*0.5f, hb.size.z*0.5f);
			Vec3 corners[8] = {
				vec3(-hs.x,-hs.y,-hs.z), vec3(hs.x,-hs.y,-hs.z), vec3(hs.x,hs.y,-hs.z), vec3(-hs.x,hs.y,-hs.z),
				vec3(-hs.x,-hs.y,hs.z), vec3(hs.x,-hs.y,hs.z), vec3(hs.x,hs.y,hs.z), vec3(-hs.x,hs.y,hs.z)
			};
			for (int i = 0; i < 8; i++) { Vec3 wc = bp + quat_mul_vec3(br, hb.center + corners[i]); ImVec2 scr; if (world_to_screen_vec3(matrix, sw, sh, wc, scr)) out.push_back(scr); }
			return;
		}
		float r = hb.size.x, h = hb.size.y, halfH = (h - (r*2.0f)) * 0.5f;
		if (halfH < 0.001f) halfH = 0.001f;
		Vec3 ha = (hb.direction == 0) ? vec3(halfH,0,0) : ((hb.direction == 1) ? vec3(0,halfH,0) : vec3(0,0,halfH));
		Vec3 from = bp + quat_mul_vec3(br, hb.center - ha), to = bp + quat_mul_vec3(br, hb.center + ha);
		Vec3 d = to - from; float len = vec3_mag(d);
		Vec3 dir = (len < 0.0001f) ? vec3(0,1,0) : vec3(d.x/len, d.y/len, d.z/len);
		Vec3 right = vec3_normalize(vec3_cross(dir, (std::abs(dir.y)<0.99f) ? vec3(0,1,0) : vec3(1,0,0)));
		Vec3 fwd = vec3_normalize(vec3_cross(right, dir));
		const float tp = 6.283185307f;
		auto add_hemi = [&](Vec3 cc, Vec3 pd) {
			Vec3 pole = cc + vec3(pd.x*r, pd.y*r, pd.z*r);
			ImVec2 scr; if (world_to_screen_vec3(matrix, sw, sh, pole, scr)) out.push_back(scr);
			for (float phi : { 0.523599f, 1.0472f, 1.5708f }) {
				float rr = r*std::sin(phi), al = r*std::cos(phi);
				Vec3 cr = cc + vec3(pd.x*al, pd.y*al, pd.z*al);
				for (int i = 0; i < 24; i++) {
					float a = (float)i/24.f*tp;
					Vec3 pt = cr + vec3(right.x*(std::cos(a)*rr)+fwd.x*(std::sin(a)*rr), right.y*(std::cos(a)*rr)+fwd.y*(std::sin(a)*rr), right.z*(std::cos(a)*rr)+fwd.z*(std::sin(a)*rr));
					if (world_to_screen_vec3(matrix, sw, sh, pt, scr)) out.push_back(scr);
				}
			}
		};
		add_hemi(from, vec3(-dir.x,-dir.y,-dir.z));
		add_hemi(to, dir);
	}

	static void draw_hitbox_outline_clipper(ImDrawList* draw, const float* matrix, float sw, float sh, const PlayerData& p, std::vector<glow_vertex>* out_glow, float alpha_scale, const float* ct, const float* cb) {
		if (!p.has_skeleton || !p.skeleton_data.has_data || !draw) return;
		HitboxData hitboxes = get_real_hitboxes(p.skeleton_data);
		if (!hitboxes.has_data) return;
		Paths64 hulls;
		std::vector<ImVec2> pts;
		for (int i = 0; i < hitboxes.count; i++) {
			if (!hitboxes.hitboxes[i].valid) continue;
			pts.clear();
			get_hitbox_screen_points(matrix, sw, sh, hitboxes.hitboxes[i], p.skeleton_data, pts);
			if (pts.size() < 3) continue;
			Path64 hull = get_convex_hull_path(pts);
			if (hull.size() >= 3) hulls.push_back(hull);
		}
		if (hulls.empty()) return;
		Paths64 sil = Union(hulls, FillRule::NonZero);
		if (sil.empty()) return;
		const float* c_top = ct ? ct : g_esp_color_hitbox_fill;
		const float* c_bot = cb ? cb : g_esp_color_hitbox_fill;
		float ymin = 1e9f, ymax = -1e9f;
		for (const auto& path : sil) for (const auto& pt : path) { float py = (float)pt.y/100.f; if (py < ymin) ymin = py; if (py > ymax) ymax = py; }
		if (ymax <= ymin) ymax = ymin + 1.f;
		Paths64 triangles;
		const bool need_tri = (g_esp_hitbox_outline_fill && alpha_scale > 0.f) || (out_glow != nullptr) || (ct && alpha_scale > 0.f);
		if (need_tri && Triangulate(sil, triangles, true) == TriangulateResult::success) {
			for (const auto& tri : triangles) {
				if (tri.size() != 3) continue;
				ImVec2 a((float)tri[0].x/100.f, (float)tri[0].y/100.f), b((float)tri[1].x/100.f, (float)tri[1].y/100.f), c((float)tri[2].x/100.f, (float)tri[2].y/100.f);
				if ((g_esp_hitbox_outline_fill || ct) && alpha_scale > 0.f)
					triangle_filled_gradient(draw, a, b, c, ymin, ymax, c_top, c_bot, alpha_scale);
				if (out_glow) { out_glow->push_back({a.x,a.y}); out_glow->push_back({b.x,b.y}); out_glow->push_back({c.x,c.y}); }
			}
		}
	}

	static void triangle_filled_gradient(ImDrawList* draw_list, const ImVec2& a, const ImVec2& b, const ImVec2& c,
		float y_min, float y_max, const float top[4], const float bottom[4], float alpha_scale) {
		float inv_range = (y_max > y_min) ? (1.f/(y_max-y_min)) : 0.f;
		auto vc = [y_min, inv_range, top, bottom, alpha_scale](float y) {
			float t = (y-y_min)*inv_range; if (t<0) t=0; else if (t>1) t=1;
			float r = top[0]+(bottom[0]-top[0])*t, g = top[1]+(bottom[1]-top[1])*t;
			float b_ = top[2]+(bottom[2]-top[2])*t, a_ = (top[3]+(bottom[3]-top[3])*t)*alpha_scale;
			return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b_, a_));
		};
		const ImU32 uv = ImGui::GetFontTexUvWhitePixel();
		draw_list->PrimReserve(3, 3);
		draw_list->PrimWriteIdx((ImDrawIdx)draw_list->_VtxCurrentIdx);
		draw_list->PrimWriteIdx((ImDrawIdx)(draw_list->_VtxCurrentIdx+1));
		draw_list->PrimWriteIdx((ImDrawIdx)(draw_list->_VtxCurrentIdx+2));
		draw_list->PrimWriteVtx(a, uv, vc(a.y));
		draw_list->PrimWriteVtx(b, uv, vc(b.y));
		draw_list->PrimWriteVtx(c, uv, vc(c.y));
	}

	static void draw_text_outline_1px(ImDrawList* draw, ImFont* font, float fs, ImVec2 pos, ImU32 tc, ImU32 oc, const char* text) {
		for (int dx = -1; dx <= 1; dx++) for (int dy = -1; dy <= 1; dy++)
			if (dx != 0 || dy != 0) draw->AddText(font, fs, ImVec2(floorf(pos.x+dx), floorf(pos.y+dy)), oc, text);
		draw->AddText(font, fs, ImVec2(floorf(pos.x), floorf(pos.y)), tc, text);
	}

	static ImU32 lerp_color(ImU32 low, ImU32 high, float t) {
		t = (t < 0.f) ? 0.f : (t > 1.f) ? 1.f : t;
		int r1=(low>>0)&0xFF, g1=(low>>8)&0xFF, b1=(low>>16)&0xFF, a1=(low>>24)&0xFF;
		int r2=(high>>0)&0xFF, g2=(high>>8)&0xFF, b2=(high>>16)&0xFF, a2=(high>>24)&0xFF;
		return IM_COL32((int)(r1+(r2-r1)*t), (int)(g1+(g2-g1)*t), (int)(b1+(b2-b1)*t), (int)(a1+(a2-a1)*t));
	}

	static ImU32 color_with_alpha(ImU32 col, int alpha) {
		return (col & 0x00FFFFFF) | ((alpha & 0xFF) << 24);
	}

	// =====================================================================
	// Render
	// =====================================================================
	void render(ImDrawList* draw, float screen_w, float screen_h)
	{
		s_glow_triangles.clear();
		if (!draw) return;

		if (misc::g_misc_show_material) {
			if (map_service::g_loaded && g_matrix_valid && map_service::g_collider_map_index >= 0 && map_service::g_collider_map_index < map_service::g_collider_map_count) {
				const char* mapName = map_service::g_collider_map_display_names[map_service::g_collider_map_index];
				if (mapName && std::strcmp(mapName, "sandstone") == 0) {
					map_service::RenderColliderHighlight(draw, g_view_matrix, screen_w, screen_h, "Mesh_1573", IM_COL32(100, 255, 180, 255));
					map_service::RenderColliderHighlight(draw, g_view_matrix, screen_w, screen_h, "Mesh_1576", IM_COL32(100, 255, 180, 255));
					map_service::RenderColliderHighlight(draw, g_view_matrix, screen_w, screen_h, "Mesh_1451", IM_COL32(100, 255, 180, 255));
					map_service::RenderColliderHighlight(draw, g_view_matrix, screen_w, screen_h, "Mesh_1574", IM_COL32(100, 255, 180, 255));
				}
			}
			if (map_service::g_loaded && g_matrix_valid) {
				Vec3 rayOrigin = { 0, 0, 0 }, rayDir = { 0, 0, 1 };
				if (GetRayFromScreenCenter(g_view_matrix, rayOrigin, rayDir)) {
					MapVector3 o(rayOrigin.x, rayOrigin.y, rayOrigin.z);
					MapVector3 d(rayDir.x, rayDir.y, rayDir.z);
					std::string hitName;
					float hitT = map_service::RaycastClosestWithName(o, d, 500.0f, &hitName);
					if (hitT < FLT_MAX && !hitName.empty()) {
						std::string nl = hitName;
						for (char& c : nl) c = (char)std::tolower((unsigned char)c);
						if (nl.find("wood_bl_261") != std::string::npos)
							map_service::RenderColliderHighlight(draw, g_view_matrix, screen_w, screen_h, "Wood_BL_261", IM_COL32(0, 255, 200, 255));
						float hx = rayOrigin.x + rayDir.x*hitT, hy = rayOrigin.y + rayDir.y*hitT, hz = rayOrigin.z + rayDir.z*hitT;
						float pos[3] = { hx, hy, hz }; float scr[2];
						if (world_to_screen(pos, scr, g_view_matrix, screen_w, screen_h)) {
							ImFont* pf = g_esp_font_hp ? g_esp_font_hp : ImGui::GetFont();
							float fs = pf ? pf->FontSize : 10.f;
							draw_text_outline_1px(draw, pf, fs, ImVec2(scr[0], scr[1]), IM_COL32(255,255,255,255), IM_COL32(0,0,0,255), hitName.c_str());
						}
					}
				}
			}
		}

		if (triggerbot::g_trigger_active) {
			float cx = screen_w*0.5f, cy = screen_h*0.5f;
			ImFont* pf = g_esp_font_hp ? g_esp_font_hp : ImGui::GetFont();
			float fs = pf ? pf->FontSize : 10.f;
			draw_text_outline_1px(draw, pf, fs, ImVec2(cx+10.f, cy+10.f), IM_COL32(255,255,255,255), IM_COL32(0,0,0,255), "trigger");
		}

		if (!g_esp_enabled || !g_matrix_valid || g_players.empty()) return;

		const ImU32 oc = IM_COL32(0,0,0,255);
		ImU32 bc = ImGui::ColorConvertFloat4ToU32(ImVec4(g_esp_color_box[0],g_esp_color_box[1],g_esp_color_box[2],g_esp_color_box[3]));

		for (const auto& p : g_players)
		{
			if (!p.is_valid || (p.x==0.f&&p.y==0.f&&p.z==0.f) || p.health <= 0 || p.health > 1000) continue;
			float feet[2], head[2];
			float wf[3] = { p.x, p.y, p.z }, wh[3] = { p.x, p.y + 1.8f, p.z };
			if (!world_to_screen(wf, feet, g_view_matrix, screen_w, screen_h) || !world_to_screen(wh, head, g_view_matrix, screen_w, screen_h)) continue;
			float h = (std::abs)(head[1] - feet[1]);
			float w = h * 0.5f;
			if (h < 2.f) continue;
			float bx = floorf(head[0] - w*0.5f), by = floorf(head[1]);
			float bw = floorf(w), bh = floorf(feet[1] - head[1]);
			ImVec2 min_p(bx, by), max_p(bx + bw, by + bh);
			const float gap = 3.f, bar_w = 2.5f;

			if (g_esp_box) {
				draw->AddRect(ImVec2(min_p.x-1.f, min_p.y-1.f), ImVec2(max_p.x+1.f, max_p.y+1.f), oc, 0.f, 0, 1.f);
				draw->AddRect(min_p, max_p, bc, 0.f, 0, 1.f);
				draw->AddRect(ImVec2(min_p.x+1.f, min_p.y+1.f), ImVec2(max_p.x-1.f, max_p.y-1.f), oc, 0.f, 0, 1.f);
			}

			int hp = (p.health<0)?0:(p.health>100)?100:p.health;
			int arm = (p.armor<0)?0:(p.armor>100)?100:p.armor;
			float dt = ImGui::GetIO().DeltaTime;
			float& dh = s_display_hp[p.base];
			if (s_display_hp.count(p.base)==0) dh = (float)hp;
			else { float d = (float)hp - dh; dh += d*(1.f-(std::exp)(-s_bar_anim_speed*dt)); dh = (std::max)(0.f,(std::min)(100.f,dh)); }
			float& da = s_display_armor[p.base];
			if (s_display_armor.count(p.base)==0) da = (float)arm;
			else { float d = (float)arm - da; da += d*(1.f-(std::exp)(-s_bar_anim_speed*dt)); da = (std::max)(0.f,(std::min)(100.f,da)); }

			ImFont* hp_f = g_esp_font_hp ? g_esp_font_hp : ImGui::GetFont();
			if (g_esp_hp) {
				float bx2 = floorf(bx - bar_w - gap), bt = by, bb = by + bh;
				float fh = (bb - bt)*(dh/100.f);
				fh = (std::max)(0.f,(std::min)(fh, bb-bt));
				float ft = bb - fh;
				ImU32 fhc = ImGui::ColorConvertFloat4ToU32(ImVec4(g_esp_color_hp_full[0],g_esp_color_hp_full[1],g_esp_color_hp_full[2],g_esp_color_hp_full[3]));
				ImU32 lhc = ImGui::ColorConvertFloat4ToU32(ImVec4(g_esp_color_hp_low[0],g_esp_color_hp_low[1],g_esp_color_hp_low[2],g_esp_color_hp_low[3]));
				draw->AddRect(ImVec2(floorf(bx2-1.f), floorf(bt-1.f)), ImVec2(floorf(bx2+bar_w+1.f), floorf(bb+1.f)), oc, 0.f, 0, 1.f);
				draw->AddRectFilled(ImVec2(floorf(bx2), floorf(bt)), ImVec2(floorf(bx2+bar_w), floorf(bb)), IM_COL32(0,0,0,128));
				draw->AddRectFilledMultiColor(ImVec2(floorf(bx2),floorf(ft)), ImVec2(floorf(bx2+bar_w),floorf(bb)), fhc, fhc, lhc, lhc);
			}

			float ay1 = 0, ay2 = 0;
			if (g_esp_armor) {
				ay1 = floorf(max_p.y + gap); ay2 = floorf(ay1 + 2.5f);
				float fw = (max_p.x - min_p.x)*(da/100.f);
				fw = (std::max)(0.f,(std::min)(fw, max_p.x - min_p.x));
				draw->AddRect(ImVec2(floorf(min_p.x-1.f), floorf(ay1-1.f)), ImVec2(floorf(max_p.x+1.f), floorf(ay2+1.f)), oc, 0.f, 0, 1.f);
				draw->AddRectFilled(ImVec2(floorf(min_p.x), floorf(ay1)), ImVec2(floorf(max_p.x), floorf(ay2)), IM_COL32(0,0,0,128));
				if (fw > 0.f) {
					ImU32 ac = ImGui::ColorConvertFloat4ToU32(ImVec4(g_esp_color_armor[0],g_esp_color_armor[1],g_esp_color_armor[2],g_esp_color_armor[3]));
					ImU32 alc = ImGui::ColorConvertFloat4ToU32(ImVec4(g_esp_color_armor_low[0],g_esp_color_armor_low[1],g_esp_color_armor_low[2],g_esp_color_armor_low[3]));
					draw->AddRectFilledMultiColor(ImVec2(floorf(min_p.x),floorf(ay1)), ImVec2(floorf(min_p.x+fw),floorf(ay2)), alc, ac, ac, alc);
				}
			}

			if (g_esp_armor && p.armor > 0 && hp_f) {
				float fs = hp_f->FontSize;
				float hkx = floorf(bx + bw + 4.f);
				draw->AddText(hp_f, fs, ImVec2(hkx+1.f, by+1.f), IM_COL32(0,0,0,255), "HK");
				draw->AddText(hp_f, fs, ImVec2(hkx, by), IM_COL32(255,255,255,255), "HK");
			}

			if (g_esp_weapon && !p.weapon.empty()) {
				ImFont* pf = g_esp_font_hp ? g_esp_font_hp : ImGui::GetFont();
				float nfs = pf ? pf->FontSize : 10.f;
				ImVec2 nsz = pf ? pf->CalcTextSizeA(nfs, FLT_MAX, 0.f, p.weapon.c_str()) : ImGui::CalcTextSize(p.weapon.c_str());
				float wy = floorf(max_p.y + 2.f);
				if (g_esp_armor) wy = ay2 + 2.f;
				float nx = floorf(bx + (bw - nsz.x)*0.5f);
				ImU32 tc = ImGui::ColorConvertFloat4ToU32(ImVec4(g_esp_color_weapon[0],g_esp_color_weapon[1],g_esp_color_weapon[2],g_esp_color_weapon[3]));
				draw->AddText(pf, nfs, ImVec2(nx+1.f, wy+1.f), IM_COL32(0,0,0,255), p.weapon.c_str());
				draw->AddText(pf, nfs, ImVec2(nx, wy), tc, p.weapon.c_str());
			}

			if (g_esp_name && !p.name.empty()) {
				ImFont* nf = g_esp_font_name ? g_esp_font_name : ImGui::GetFont();
				float nx = floorf(bx + (bw - ImGui::CalcTextSize(p.name.c_str()).x)*0.5f);
				float ny = floorf(by - 15.f);
				draw->AddText(nf, 12.f, ImVec2(nx+1.f, ny+1.f), IM_COL32(0,0,0,255), p.name.c_str());
				draw->AddText(nf, 12.f, ImVec2(nx, ny), IM_COL32(255,255,255,255), p.name.c_str());
			}

			if (g_esp_skeleton && p.has_skeleton) {
				PlayerData pd = p;
				skeleton_correct_by_hip(pd);
				render_skeleton_lines(draw, g_view_matrix, pd.skeleton, IM_COL32(255,255,255,255), screen_w, screen_h);
			}

			if ((g_esp_hitbox_outline_fill || g_esp_hitbox_glow) && p.has_skeleton && p.skeleton_data.has_data) {
				PlayerData pd = p;
				skeleton_correct_by_hip(pd);
				draw_hitbox_outline_clipper(draw, g_view_matrix, screen_w, screen_h, pd, g_esp_hitbox_glow ? &s_glow_triangles : nullptr, 1.f, nullptr, nullptr);
			}
		}
	}

	void get_hitbox_glow_triangles(std::vector<glow_vertex>& out)
	{
		out.swap(s_glow_triangles);
	}
}