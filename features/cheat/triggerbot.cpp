#include "triggerbot.h"
#include "Offset.h"
#include "esp.h"
#include "map_service.h"
#include "memory.h"
#include "menu.h"
#include <windows.h>
#include <chrono>
#include <cmath>

namespace triggerbot
{
	bool g_trigger_enabled = false;
	bool g_trigger_active = false;
	int g_trigger_key_vk = VK_LBUTTON;
	int g_trigger_mode = 0;
	bool g_trigger_fired = false;

	static esp::Vec3 ray_direction_from_matrix(const float* matrix) {
		float x = -matrix[8];
		float y = -matrix[9];
		float z = -matrix[10];
		float len = std::sqrt(x*x + y*y + z*z);
		if (len < 0.0001f) return { 0.f, 0.f, 1.f };
		return { x/len, y/len, z/len };
	}

	static void normalize_dir(esp::Vec3& d) {
		float len = std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
		if (len > 0.0001f) { d.x /= len; d.y /= len; d.z /= len; }
	}

	void run(mem::Memory* mem, float screen_w, float screen_h)
	{
		// Сбрасываем флаг выстрела в начале тика
		g_trigger_fired = false;
		if (!mem || !mem->IsReady()) {
			g_trigger_active = false;
			return;
		}
		if (!map_service::g_loaded) {
			g_trigger_active = false;
			return;
		}
		if (Drawing::is_active() || !esp::g_matrix_valid || esp::g_players.empty()) {
			g_trigger_active = false;
			return;
		}
		if (esp::g_local_player < 0x10000)
			return;

		int vk = (g_trigger_key_vk > 0 && g_trigger_key_vk < 256) ? g_trigger_key_vk : VK_LBUTTON;
		bool trigger_active = false;
		if (g_trigger_enabled) {
			static bool s_toggle = false;
			if (g_trigger_mode == 1) {
				if (GetAsyncKeyState(vk) & 1) s_toggle = !s_toggle;
				trigger_active = s_toggle;
			} else {
				trigger_active = (GetAsyncKeyState(vk) & 0x8000) != 0;
			}
		}
		g_trigger_active = trigger_active;
		if (!trigger_active)
			return;

		esp::Vec3 rayOrigin = { 0, 0, 0 };
		esp::Vec3 rayDir = { 0, 0, 1 };
		if (!esp::GetRayFromScreenCenter(esp::g_view_matrix, rayOrigin, rayDir)) {
			const float* m = esp::g_view_matrix;
			rayOrigin.x = -(m[0]*m[12] + m[4]*m[13] + m[8]*m[14]);
			rayOrigin.y = -(m[1]*m[12] + m[5]*m[13] + m[9]*m[14]);
			rayOrigin.z = -(m[2]*m[12] + m[6]*m[13] + m[10]*m[14]);
			rayDir = ray_direction_from_matrix(m);
		}

		uint8_t local_team = 0;
		mem->Read(esp::g_local_player + Offsets::Player_::Team, &local_team, sizeof(local_team));

		static esp::Vec3 s_prev_origin = { 0, 0, 0 };
		static esp::Vec3 s_prev_dir = { 0, 0, 1 };
		static bool s_has_prev = false;

		const esp::PlayerData* target = esp::GetEntityInCrosshairWithMapCheck(rayOrigin, rayDir, 300.0f, esp::g_players, local_team);
		if (!target && s_has_prev) {
			esp::Vec3 midOrigin = { (s_prev_origin.x + rayOrigin.x) * 0.5f, (s_prev_origin.y + rayOrigin.y) * 0.5f, (s_prev_origin.z + rayOrigin.z) * 0.5f };
			esp::Vec3 midDir = { s_prev_dir.x + rayDir.x, s_prev_dir.y + rayDir.y, s_prev_dir.z + rayDir.z };
			normalize_dir(midDir);
			target = esp::GetEntityInCrosshairWithMapCheck(midOrigin, midDir, 300.0f, esp::g_players, local_team);
		}

		s_prev_origin = rayOrigin;
		s_prev_dir = rayDir;
		s_has_prev = true;

		if (target != nullptr) {
			using namespace std::chrono;
			static steady_clock::time_point s_last_trigger = {};
			auto now = steady_clock::now();
			if (duration_cast<milliseconds>(now - s_last_trigger).count() >= 80) {
				// 1.0.0: Fire by setting WeaponState to Attack via WeaponContext.
				// Find weapon part (nrk) in player's parts list.
				uint32_t parts_list = 0;
				mem->Read(esp::g_local_player + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list));
				if (parts_list && parts_list > 0x10000) {
					uint32_t parts_array = 0;
					int parts_count = 0;
					if (mem->Read(parts_list + Offsets::Il2CppList_::Items, &parts_array, sizeof(parts_array)) &&
					    mem->Read(parts_list + Offsets::Il2CppList_::Count, &parts_count, sizeof(parts_count)) &&
					    parts_array && parts_count > 0) {
						uint32_t weapon_part = 0;
						for (int i = 0; i < parts_count && i < 64; i++) {
							uint32_t part = 0;
							if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
							uint32_t klass = 0;
							mem->Read(part, &klass, sizeof(klass));
							if (klass == Offsets::WeaponPart_TypeInfo) {
								weapon_part = part;
								break;
							}
						}
						if (weapon_part && weapon_part > 0x10000) {
							// nrk +0x18 (WeaponContext) +0x58 -> State<WeaponState>
							// WeaponState::Attack = 1 (from Offsets::WeaponState_ namespace)
							uint8_t attack_state = Offsets::WeaponState_::Attack;
							mem->Write(weapon_part + Offsets::WeaponPart_::WeaponContext + Offsets::WeaponContext_::WeaponState,
								&attack_state, sizeof(attack_state));
							g_trigger_fired = true;
						}
					}
				}
				s_last_trigger = now;
			}
		}
	}
}
