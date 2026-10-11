#include "aimbot.h"
#include "esp.h"
#include "Offset.h"
#include "memory.h"
#include "imgui.h"
#include "map_service.h"
#include "triggerbot.h"
#include <windows.h>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace aimbot
{
	static const int BONE_HEAD = 10;   // HumanBodyBones::Head
	static const int BONE_NECK = 9;    // HumanBodyBones::Neck

	static int get_target_bone() {
		return BONE_HEAD;
	}

	static void get_target_world_position(const esp::PlayerData& p, float* out_x, float* out_y, float* out_z)
	{
		int bone = get_target_bone();
		float hx = p.skeleton_data.bones[bone][0];
		float hy = p.skeleton_data.bones[bone][1];
		float hz = p.skeleton_data.bones[bone][2];

		if (esp::g_aimbot_target == 1)
		{
			float hip_x = p.skeleton_data.bones[0][0];
			float hip_y = p.skeleton_data.bones[0][1];
			float hip_z = p.skeleton_data.bones[0][2];
			float dx = std::fabs(hip_x - p.x);
			float dy = std::fabs(hip_y - p.y);
			float dz = std::fabs(hip_z - p.z);
			if (dx > 0.5f || dy > 0.5f || dz > 0.5f)
			{
				float ox = p.x - hip_x;
				float oy = p.y - hip_y + 0.9f;
				float oz = p.z - hip_z;
				hx += ox; hy += oy; hz += oz;
			}
		}
		*out_x = hx; *out_y = hy; *out_z = hz;
	}

	static void calc_angles_to_target(float camX, float camY, float camZ, float tX, float tY, float tZ, float* out_pitch, float* out_yaw)
	{
		float dx = tX - camX, dy = tY - camY, dz = tZ - camZ;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (dist < 0.001f) { *out_pitch = 0.f; *out_yaw = 0.f; return; }
		const float rad2deg = 57.2957795f;
		*out_yaw = std::atan2(dx, dz) * rad2deg;
		*out_pitch = -std::asin(dy / dist) * rad2deg;
	}

	static float angle_diff_deg(float a1, float a2)
	{
		float d = std::fmod(a1 - a2 + 180.0f, 360.0f);
		if (d < 0.0f) d += 360.0f;
		return d - 180.0f;
	}

	void run(mem::Memory* mem)
	{
		using namespace std::chrono;
		static bool s_return_armed = false;
		static float s_return_orig_vec[3] = { 0.f, 0.f, 0.f };
		static bool s_done_for_press = false;

		if (!esp::g_aimbot_enabled || !mem || !mem->IsReady() || esp::g_local_player < 0x10000)
			return;

		static bool toggle_state = false;
		if (esp::g_aimbot_key != 0)
		{
			if (GetAsyncKeyState(esp::g_aimbot_key) & 1)
				toggle_state = !toggle_state;
		}
		bool bind_active = toggle_state;
		if (!bind_active)
		{
			s_return_armed = false;
			s_done_for_press = false;
			return;
		}

		if (!esp::g_aimbot_visible_check && !esp::g_aimbot_fov_enabled)
			return;

		if (s_done_for_press)
			return;

		// 1.0.0: Get camera component (ios) from local player's parts list
		uint32_t parts_list = 0;
		mem->Read(esp::g_local_player + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list));
		if (!parts_list || parts_list < 0x10000) return;

		uint32_t parts_array = 0;
		int parts_count = 0;
		if (!mem->Read(parts_list + Offsets::Il2CppList_::Items, &parts_array, sizeof(parts_array)) ||
		    !mem->Read(parts_list + Offsets::Il2CppList_::Count, &parts_count, sizeof(parts_count)) ||
		    !parts_array || parts_count <= 0)
			return;

		uint32_t camera_component = 0;
		for (int i = 0; i < parts_count && i < 64; i++)
		{
			uint32_t part = 0;
			if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
			uint32_t klass = 0;
			mem->Read(part, &klass, sizeof(klass));
			if (klass == Offsets::CameraComponent_TypeInfo) {
				camera_component = part;
				break;
			}
		}
		if (!camera_component || camera_component < 0x10000) return;

		float cam_x, cam_y, cam_z;
		if (!esp::get_camera_position_from_camera_component(mem, camera_component, &cam_x, &cam_y, &cam_z)) return;

		uint8_t local_team = 0;
		mem->Read(esp::g_local_player + Offsets::Player_::Team, &local_team, sizeof(local_team));

		float cam_pitch, cam_yaw;
		if (!esp::get_local_aim_angles(mem, camera_component, &cam_pitch, &cam_yaw)) {
			// Fallback: try reading from the rotation part directly
			uint32_t rot_part = 0;
			mem->Read(camera_component + Offsets::CameraComponent_::RotationPart, &rot_part, sizeof(rot_part));
			if (!rot_part || rot_part < 0x10000) return;
			float q[4] = { 0 };
			mem->Read(rot_part + Offsets::RotationPart_::ViewRotation, q, sizeof(q));
			if (q[0] == 0.f && q[1] == 0.f && q[2] == 0.f && q[3] == 0.f) return;
			// Direct quaternion to euler approximation
			cam_yaw = std::atan2(2.f * (q[1] * q[3]), 1.f - 2.f * (q[1] * q[1])) * 57.2957795f;
			cam_pitch = -std::asin(2.f * (q[0] * q[1] + q[2] * q[3])) * 57.2957795f;
		}

		int best_idx = -1;
		float best_angle = esp::g_aimbot_fov + 1.0f;
		float target_pos[3] = { 0.f, 0.f, 0.f };

		for (size_t i = 0; i < esp::g_players.size(); i++)
		{
			const auto& p = esp::g_players[i];
			if (!p.is_valid || !p.has_skeleton || p.health < 1)
				continue;
			if (p.team != 0 && p.team == local_team)
				continue;

			float hx, hy, hz;
			get_target_world_position(p, &hx, &hy, &hz);

			float diff[3] = { hx - cam_x, hy - cam_y, hz - cam_z };
			float dist = esp::vec3_mag({ diff[0], diff[1], diff[2] });
			if (dist < 0.5f)
				continue;

			float p_val, y_val;
			calc_angles_to_target(cam_x, cam_y, cam_z, hx, hy, hz, &p_val, &y_val);
			float angle = std::sqrt(std::pow(angle_diff_deg(p_val, cam_pitch), 2) + std::pow(angle_diff_deg(y_val, cam_yaw), 2));

			if (angle < best_angle) {
				best_angle = angle;
				best_idx = (int)i;
				target_pos[0] = hx; target_pos[1] = hy; target_pos[2] = hz;
			}
		}

		if (best_idx < 0) {
			s_return_armed = false;
			return;
		}

		float pitch, yaw;
		calc_angles_to_target(cam_x, cam_y, cam_z, target_pos[0], target_pos[1], target_pos[2], &pitch, &yaw);
		float vec[3] = { pitch, yaw, 0.0f };

		// Return aim mode
		if (esp::g_aimbot_return_ms > 0.0f) {
			if (!s_return_armed) {
				if (!esp::get_local_aim_angles(mem, camera_component, &s_return_orig_vec[0], &s_return_orig_vec[1])) {
					s_return_orig_vec[0] = 0.f; s_return_orig_vec[1] = 0.f;
				}
				s_return_armed = true;
			}

			if (!triggerbot::g_trigger_fired) {
				for (int i = 0; i < 10; i++) {
					esp::set_local_aim_angles(mem, camera_component, pitch, yaw);
				}
			} else {
				for (int i = 0; i < 10; i++) {
					esp::set_local_aim_angles(mem, camera_component, s_return_orig_vec[0], s_return_orig_vec[1]);
				}
				s_return_armed = false;
				s_done_for_press = true;
			}
		} else {
			for (int i = 0; i < 10; i++) {
				esp::set_local_aim_angles(mem, camera_component, pitch, yaw);
			}
		}
	}

	void draw_fov(ImDrawList* draw, float screen_w, float screen_h)
	{
		if (!esp::g_aimbot_enabled || !esp::g_aimbot_fov_display || !esp::g_aimbot_fov_enabled)
			return;
		float cx = screen_w * 0.5f, cy = screen_h * 0.5f;
		float fov_deg = std::clamp(esp::g_aimbot_fov, 1.f, 180.f);
		float half_vfov_rad = 35.f * (3.14159265f / 180.0f);
		float radius = (cy / std::tan(half_vfov_rad)) * std::tan(fov_deg * 0.5f * (3.14159265f / 180.0f));
		ImVec2 center(cx, cy);
		const int SEGMENTS = 64;
		const ImU32 col = IM_COL32(255, 255, 255, 180);
		const ImU32 outline = IM_COL32(0, 0, 0, 255);
		for (int i = 0; i < SEGMENTS; i++) {
			float a1 = (float)i / SEGMENTS * 2.0f * 3.14159265f;
			float a2 = (float)(i + 1) / SEGMENTS * 2.0f * 3.14159265f;
			ImVec2 p1(center.x + std::cos(a1) * radius, center.y + std::sin(a1) * radius);
			ImVec2 p2(center.x + std::cos(a2) * radius, center.y + std::sin(a2) * radius);
			draw->AddLine(p1, p2, outline, 2.0f);
			draw->AddLine(p1, p2, col, 1.0f);
		}
	}
}
