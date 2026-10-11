#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <cmath>
#include "imgui.h"
#include "glow_shader.h"

struct ImDrawList;
struct ImVec2;
struct ImFont;

namespace mem
{
	class Memory;
}

namespace esp
{
	struct Transform {
		float position[3];
		float rot[4];
	};

	struct Skeleton {
		Transform Head;
		Transform Neck;
		Transform Spine;
		Transform LeftShoulder;
		Transform LeftUpperarm;
		Transform LeftForearm;
		Transform LeftHand;
		Transform RightShoulder;
		Transform RightUpperarm;
		Transform RightForearm;
		Transform RightHand;
		Transform Hip;
		Transform LeftUpLeg;
		Transform LeftLeg;
		Transform LeftFoot;
		Transform LeftToeBase;
		Transform RightUpLeg;
		Transform RightLeg;
		Transform RightFoot;
		Transform RightToeBase;
	};

	static const int ESP_MAX_BONE_COUNT = 64;
	static const int ESP_MAX_HITBOX_COUNT = 32;

	struct Vec3 {
		float x, y, z;
		friend inline Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
		friend inline Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
		friend inline Vec3 operator*(const Vec3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
		friend inline Vec3 operator*(float s, const Vec3& a) { return { a.x * s, a.y * s, a.z * s }; }
		friend inline Vec3 operator/(const Vec3& a, float s) { float inv = 1.0f / s; return { a.x * inv, a.y * inv, a.z * inv }; }
		inline Vec3& operator+=(const Vec3& b) { x += b.x; y += b.y; z += b.z; return *this; }
		inline Vec3& operator-=(const Vec3& b) { x -= b.x; y -= b.y; z -= b.z; return *this; }
		inline Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
		inline Vec3& operator/=(float s) { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }
	};
	inline float vec3_mag(const Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
	inline float vec3_dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
	inline Vec3 vec3_cross(const Vec3& a, const Vec3& b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
	inline Vec3 vec3_normalize(const Vec3& v) { float m = vec3_mag(v); if (m < 0.0001f) return v; return v * (1.f / m); }
	struct Quat { float x, y, z, w; };

	struct SkeletonData {
		bool has_data = false;
		float bones[ESP_MAX_BONE_COUNT][3];
		Quat boneRotations[ESP_MAX_BONE_COUNT];
		bool valid[ESP_MAX_BONE_COUNT];
	};

	struct Hitbox {
		int bone;
		int type;
		Vec3 center;
		Vec3 size;
		int direction;
		bool valid;
	};

	struct HitboxData {
		bool has_data = false;
		int count = 0;
		Hitbox hitboxes[ESP_MAX_HITBOX_COUNT];
	};

	struct PlayerData
	{
		std::string name;
		int health = -1;
		int armor = -1;
		int money = -1;
		std::string weapon;
		uint32_t base = 0;           // ake player base pointer
		uint32_t camera_links = 0;   // yyl — ссылки на камеру
		float x = 0.f, y = 0.f, z = 0.f;
		uint8_t team = 0;
		bool is_valid = false;
		bool is_alive = true;
		Skeleton skeleton = {};
		SkeletonData skeleton_data = {};
		bool has_skeleton = false;
	};

	// Cached pointers (updated by read_client_world)
	extern uint32_t g_client_world;
	extern uint32_t g_local_player;
	extern uint32_t g_net_object_registry;
	extern uint32_t g_net_object_storage;
	extern size_t g_unity_base;

	// View matrix (updated by update_view_matrix)
	extern float g_view_matrix[16];
	extern bool g_matrix_valid;

	// Player list (updated by update_players)
	extern std::vector<PlayerData> g_players;

	// Grenade tracking
	struct GrenadeData {
		float x = 0.f, y = 0.f, z = 0.f;
		int id = 0;
	};
	extern std::vector<GrenadeData> g_grenades;
	extern uint32_t g_grenade_manager;

	struct DeployedSmokeData {
		float x = 0.f, y = 0.f, z = 0.f;
		uint64_t added_ms = 0;
		int id = 0;
	};
	extern std::vector<DeployedSmokeData> g_deployed_smokes;
	struct DeployedMolotovData {
		float x = 0.f, y = 0.f, z = 0.f;
		uint64_t added_ms = 0;
		int id = 0;
	};
	extern std::vector<DeployedMolotovData> g_deployed_molotovs;

	// Aimbot state
	extern bool g_aimbot_enabled;
	extern float g_aimbot_fov;
	extern bool g_aimbot_fov_enabled;
	extern bool g_aimbot_fov_display;
	extern bool g_aimbot_visible_check;
	extern float g_aimbot_return_ms;
	extern bool g_antiaim_enabled;
	extern float g_antiaim_pitch_amp;
	extern float g_antiaim_yaw_amp;
	extern int g_aimbot_key;
	extern int g_aimbot_mode;
	extern int g_aimbot_target;
	extern float g_aimbot_smooth;

	// Triggerbot
	extern bool g_trigger_enabled;
	extern bool g_trigger_active;
	bool is_local_shooting();

	// Camera / angles
	bool get_camera_position_from_camera_component(mem::Memory* mem, uint32_t camera_component, float* out_x, float* out_y, float* out_z);
	bool get_local_aim_angles(mem::Memory* mem, uint32_t camera_component, float* out_pitch, float* out_yaw);
	bool set_local_aim_angles(mem::Memory* mem, uint32_t camera_component, float pitch, float yaw);

	// ClientWorld (njh) — replaces PlayerManager. Found by heap scan on klass.
	bool read_client_world(mem::Memory* mem, size_t unity_base);
	// Refresh client world cache (quiet update).
	void refresh_client_world_cache(mem::Memory* mem, size_t unity_base);

	// Update view matrix from local player camera. Call when memory ready.
	void update_view_matrix(mem::Memory* mem);

	// Update local player pointer from client world.
	bool update_local_player(mem::Memory* mem);

	// Update g_players from net object storage; filter by team. Call when memory ready.
	void update_players(mem::Memory* mem);

	// Update grenade tracking (flying grenades, deployed smoke/molotov).
	void update_grenades(mem::Memory* mem);

	// Find an instance of a managed class by its TypeInfo RVA. Returns object pointer or 0.
	uint32_t find_by_typeinfo(mem::Memory* mem, uintptr_t typeinfo_rva);

	// Read skeleton bones for a player via CharacterBindings. Fills out_skeleton and out_skeleton_data.
	bool get_skeleton_bones(mem::Memory* mem, uint32_t player_base, Skeleton& out_skeleton, SkeletonData& out_skeleton_data);

	// Internal: read a Unity Transform (position + rotation). Returns identity if invalid.
	Transform get_transform_info(mem::Memory* mem, uint32_t transform_ptr);

	// Internal: world_to_screen and skeleton drawing.
	bool world_to_screen(const float* pos, float* out, const float* matrix, float width, float height);
	void render_skeleton_lines(ImDrawList* draw_list, const float* matrix, const Skeleton& sk, ImU32 color, float screen_w, float screen_h);

	/// Луч через центр экрана (как в ааа: unproject NDC). Возвращает true если луч получен.
	bool GetRayFromScreenCenter(const float* viewProj, Vec3& origin, Vec3& dir);
	/// Цель в прицеле по лучу с проверкой стены по карте. nullptr если нет цели.
	const PlayerData* GetEntityInCrosshairWithMapCheck(const Vec3& rayOrigin, const Vec3& rayDir, float maxWallDist,
		const std::vector<PlayerData>& players, uint8_t local_team);
	/// Сместить позицию игрока (x,y,z + все клоны скелета) на (dx,dy,dz) — для предикта триггера.
	void OffsetPlayerForPrediction(PlayerData& p, float dx, float dy, float dz);

	// Draw ESP (boxes, names, health, skeleton) to ImGui background draw list. Call after update_*.
	void render(ImDrawList* draw, float screen_w, float screen_h);

	// Glow: fill out with silhouette triangles (3 vertices per triangle, screen coords). Call after render().
	void get_hitbox_glow_triangles(std::vector<glow_vertex>& out);

	// ESP toggles
	extern bool g_esp_enabled;
	extern bool g_esp_box;
	extern bool g_esp_hp;
	extern bool g_esp_name;
	extern bool g_esp_skeleton;
	extern float g_esp_color_skeleton[4];
	extern bool g_esp_hitbox_outline_fill;
	extern float g_esp_color_hitbox_fill[4];
	extern float g_esp_color_hitbox_fill_bottom[4];
	extern bool g_esp_hitbox_glow;
	extern float g_esp_glow_color[4];
	extern float g_esp_glow_radius;
	extern bool g_esp_weapon;
	extern bool g_esp_armor;
	extern bool g_esp_avatar;
	extern bool g_esp_grenades;
	extern float g_esp_smoke_radius_m;
	extern float g_esp_smoke_circle_outline_color[4];
	extern float g_esp_molotov_circle_outline_color[4];
	extern float g_esp_molotov_radius_m;
	extern float g_esp_color_box[4];
	extern float g_esp_color_name[4];
	extern float g_esp_color_hp_full[4];
	extern float g_esp_color_hp_low[4];
	extern float g_esp_color_armor[4];
	extern float g_esp_color_armor_low[4];
	extern float g_esp_color_weapon[4];

	extern ImFont* g_esp_font_name;
	extern ImFont* g_esp_font_hp;
	extern ImFont* g_esp_font_weapons;
}
