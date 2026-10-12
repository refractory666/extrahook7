#include "misc.h"
#include "memory.h"
#include "esp.h"
#include "Offset.h"
#include "console.h"
#include <windows.h>
#include <string>

namespace misc
{
	bool g_misc_stop_enabled = false;
	int g_misc_stop_key = 0;
	bool g_misc_teleport_enabled = false;
	int g_misc_teleport_key = 0;
	bool g_misc_stop_active = false;
	bool g_misc_teleport_active = false;
	bool g_misc_show_material = false;

	bool g_fov_enabled = false;
	float g_fov_value = 90.0f;
	bool g_thirdperson_enabled = false;
	float g_thirdperson_dist = 2.0f;

	bool g_crouch_enabled = false;
	bool g_aim_enabled = false;
	bool g_jump_enabled = false;
	bool g_shoot_enabled = false;

	static bool s_last_stop_state = false;

	static bool read_unity_string(mem::Memory* mem, uint32_t string_ptr, std::string& out)
	{
		if (!string_ptr || !mem) return false;
		int32_t length = 0;
		if (!mem->Read(string_ptr + 0x8, &length, sizeof(length))) return false;
		if (length <= 0 || length > 64) return false;
		std::string result;
		result.reserve((size_t)length);
		for (int i = 0; i < length; ++i)
		{
			char16_t ch = 0;
			if (!mem->Read(string_ptr + 0xC + i * (int)sizeof(char16_t), &ch, sizeof(ch)))
				return false;
			if (ch < 0x80)
				result.push_back((char)ch);
			else if (ch < 0x800)
			{
				result.push_back((char)(0xC0 | (ch >> 6)));
				result.push_back((char)(0x80 | (ch & 0x3F)));
			}
			else if (ch < 0xD800 || ch > 0xDFFF)
			{
				result.push_back((char)(0xE0 | (ch >> 12)));
				result.push_back((char)(0x80 | ((ch >> 6) & 0x3F)));
				result.push_back((char)(0x80 | (ch & 0x3F)));
			}
		}
		out.swap(result);
		return true;
	}

	// Find the weapon part (nrk) in a player's parts list.
	static uint32_t find_weapon_part(mem::Memory* mem, uint32_t player_base)
	{
		if (!mem || player_base < 0x10000) return 0;
		uint32_t parts_list = 0;
		mem->Read(player_base + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list));
		if (!parts_list || parts_list < 0x10000) return 0;
		uint32_t parts_array = 0;
		int parts_count = 0;
		if (!mem->Read(parts_list + Offsets::Il2CppList_::Items, &parts_array, sizeof(parts_array)) ||
		    !mem->Read(parts_list + Offsets::Il2CppList_::Count, &parts_count, sizeof(parts_count)) ||
		    !parts_array || parts_count <= 0)
			return 0;
		for (int i = 0; i < parts_count && i < 64; i++)
		{
			uint32_t part = 0;
			if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
			uint32_t klass = 0;
			mem->Read(part, &klass, sizeof(klass));
			if (klass == Offsets::WeaponPart_TypeInfo)
				return part;
		}
		return 0;
	}

	// Find the camera component (ios) in a player's parts list.
	static uint32_t find_camera_component(mem::Memory* mem, uint32_t player_base)
	{
		if (!mem || player_base < 0x10000) return 0;
		uint32_t parts_list = 0;
		mem->Read(player_base + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list));
		if (!parts_list || parts_list < 0x10000) return 0;
		uint32_t parts_array = 0;
		int parts_count = 0;
		if (!mem->Read(parts_list + Offsets::Il2CppList_::Items, &parts_array, sizeof(parts_array)) ||
		    !mem->Read(parts_list + Offsets::Il2CppList_::Count, &parts_count, sizeof(parts_count)) ||
		    !parts_array || parts_count <= 0)
			return 0;
		for (int i = 0; i < parts_count && i < 64; i++)
		{
			uint32_t part = 0;
			if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
			uint32_t klass = 0;
			mem->Read(part, &klass, sizeof(klass));
			if (klass == Offsets::CameraComponent_TypeInfo)
				return part;
		}
		return 0;
	}

	// 1.0.0: Health is a ushort in the stats part (ylw at ake+0x44).
	void set_local_health(mem::Memory* mem, int health_value)
	{
		if (!mem || !mem->IsReady())
			return;
		if (health_value < 0)
			health_value = 0;

		uint32_t player_base = esp::g_local_player;
		if (player_base < 0x10000)
			return;

		// ake +0x44 -> ylw (stats part)
		uint32_t stats_part = 0;
		mem->Read(player_base + Offsets::Player_::StatsPart, &stats_part, sizeof(stats_part));
		if (!stats_part || stats_part < 0x10000)
			return;

		// Health is a ushort at ylw +0x08
		uint16_t hp = (uint16_t)health_value;
		mem->Write(stats_part + Offsets::StatsPart_::Health, &hp, sizeof(hp));
	}

	// 1.0.0: No Photon props — skins are in qjf WeaponMetaData.
	// set_local_badge is deprecated; skins now come directly from weapon instances.
	void set_local_badge(mem::Memory* mem, const char* prop_substr, int badge_id)
	{
		(void)mem; (void)prop_substr; (void)badge_id;
		// In 1.0.0, badge/skin customization is handled client-side via the loadout
		// service (gkw), not via Photon props. This function is kept for API compatibility.
	}

	void run(mem::Memory* mem, size_t unity_base)
	{
		(void)unity_base;
		if (!mem || !mem->IsReady()) return;

		auto key_pressed_edge = [](int vk) {
			if (vk <= 0) return false;
			return (GetAsyncKeyState(vk) & 1) != 0;
		};

		// --- Stop (anti-aim / movement stop) ---
		// In 1.0.0, movement is controlled via the input command (owv).
		// We toggle input flags in the navigation context (nip part +0x28).
		if (!g_misc_stop_enabled) {
			if (s_last_stop_state) {
				// Restore normal movement by setting movement flags back
				s_last_stop_state = false;
			}
			g_misc_stop_active = false;
		} else {
			if (g_misc_stop_key != 0 && key_pressed_edge(g_misc_stop_key))
				g_misc_stop_active = !g_misc_stop_active;

			if (g_misc_stop_active) {
				// Write zero velocity to local player's navigation part (tof at nip+0x24)
				if (esp::g_local_player >= 0x10000) {
					uint32_t parts_list = 0;
					if (mem->Read(esp::g_local_player + Offsets::NetObject_::Parts, &parts_list, sizeof(parts_list)) &&
					    parts_list && parts_list > 0x10000) {
						uint32_t parts_array = 0, parts_count = 0;
						if (mem->Read(parts_list + Offsets::Il2CppList_::Items, &parts_array, sizeof(parts_array)) &&
						    mem->Read(parts_list + Offsets::Il2CppList_::Count, &parts_count, sizeof(parts_count)) &&
						    parts_array && parts_count > 0) {
							for (int i = 0; i < parts_count && i < 64; i++) {
								uint32_t part = 0;
								if (!mem->Read(parts_array + i * 4, &part, sizeof(part)) || !part) continue;
								uint32_t klass = 0;
								mem->Read(part, &klass, sizeof(klass));
								if (klass == Offsets::NavigationPart_TypeInfo) {
									// nip +0x24 -> tof (movement state), tof +0x14/0x20 = velocity
									uint32_t movement_state = 0;
									mem->Read(part + Offsets::NavigationPart_::MovementState, &movement_state, sizeof(movement_state));
									if (movement_state && movement_state > 0x10000) {
										float zero[3] = { 0.f, 0.f, 0.f };
										mem->Write(movement_state + Offsets::MovementState_::Velocity1, zero, sizeof(zero));
									}
									break;
								}
							}
						}
					}
				}
				s_last_stop_state = true;
			} else if (s_last_stop_state) {
				s_last_stop_state = false;
			}
		}

		// --- Teleport (fake lag) ---
		// 1.0.0: No PhotonView sync. Delay sends via ClientWorldOptions.
		if (!g_misc_teleport_enabled) {
			g_misc_teleport_active = false;
		} else {
			if (g_misc_teleport_key != 0 && key_pressed_edge(g_misc_teleport_key))
				g_misc_teleport_active = !g_misc_teleport_active;

			if (esp::g_client_world >= 0x10000) {
				// ClientWorldOptions is embedded directly in ClientWorld at +0x0C (not a pointer).
				// InputPrediction is at ClientWorldOptions +0x0C -> absolute: g_client_world + 0x0C + 0x0C.
				// Toggle InputPrediction off for fake lag.
				bool prediction = !g_misc_teleport_active;
				mem->Write(esp::g_client_world + Offsets::ClientWorld_::Options +
					Offsets::ClientWorldOptions_::InputPrediction, &prediction, sizeof(bool));
			}
		}

		// --- Camera Logic (FOV & Thirdperson) ---
		// 1.0.0: Camera via ios component in player parts list.
		if (esp::g_local_player != 0)
		{
			uint32_t camera_component = find_camera_component(mem, esp::g_local_player);
			if (camera_component && camera_component > 0x10000)
			{
				// FOV — via native Camera. The managed camera object is at ios+0x28 (CameraTransform)
				// managed +0x08 -> native, native matrix at 0xE0, but FOV is a different field.
				if (g_fov_enabled)
				{
					// ios +0x2C -> CameraHolderTransform, but for FOV we need the native Camera.
					uint32_t cam_obj = 0;
					mem->Read(camera_component + Offsets::CameraComponent_::CameraTransform, &cam_obj, sizeof(cam_obj));
					if (cam_obj && cam_obj > 0x10000)
					{
						uint32_t native_cam = 0;
						mem->Read(cam_obj + 0x08, &native_cam, sizeof(native_cam));
						if (native_cam && native_cam > 0x10000)
						{
							// Native camera fieldOfView — newer Unity (1.0.0).
							mem->Write(native_cam + Offsets::NativeCamera_::FieldOfView, &g_fov_value, sizeof(float));
						}
					}
				}

				// Thirdperson — modify CameraHolderTransform local position.
				if (g_thirdperson_enabled)
				{
					uint32_t cam_holder_transform = 0;
					mem->Read(camera_component + Offsets::CameraComponent_::CameraHolderTransform, &cam_holder_transform, sizeof(cam_holder_transform));
					if (cam_holder_transform && cam_holder_transform > 0x10000)
					{
						// Write local position z = -dist, y = some offset
						float z = -g_thirdperson_dist;
						float y = g_thirdperson_dist * 0.3f;
						mem->Write(cam_holder_transform + Offsets::ManagedTransform_::LocalPositionY, &y, sizeof(float));
						mem->Write(cam_holder_transform + Offsets::ManagedTransform_::LocalPositionZ, &z, sizeof(float));
					}
				}
			}
		}

		// --- Controls Logic (Crouch, Aim, Jump, Shoot) ---
		// 1.0.0: Use znn input manager with named ButtonInput actions.
		// Find input manager by TypeInfo, then press buttons via ButtonInput flags.
		if (esp::g_client_world >= 0x10000)
		{
			uint32_t input_manager = esp::find_by_typeinfo(mem, (uintptr_t)esp::g_unity_base + Offsets::InputManager_TypeInfo);
			if (input_manager && input_manager > 0x10000)
			{
				// znn +0x10 -> yjk<InputEntry<ButtonInput>> buttons list
				auto force_button = [&](const char* action_name, bool enabled) {
					if (!enabled) return;
					uint32_t button_list = 0;
					mem->Read(input_manager + Offsets::InputManager_::ButtonInputs, &button_list, sizeof(button_list));
					if (!button_list || button_list < 0x10000) return;

					uint32_t btn_array = 0;
					int btn_count = 0;
					if (!mem->Read(button_list + Offsets::Il2CppList_::Items, &btn_array, sizeof(btn_array)) ||
					    !mem->Read(button_list + Offsets::Il2CppList_::Count, &btn_count, sizeof(btn_count)) ||
					    !btn_array || btn_count <= 0)
						return;

					// InputEntry<T> = { string name (+0x08), T value (+0x10) }
					// ButtonInput: +0x0 WasPressedInThisFrame, +0x1 WasReleasedInThisFrame, +0x2 IsPressed
					for (int i = 0; i < btn_count && i < 32; i++)
					{
						uint32_t entry = btn_array + i * 4;
						uint32_t entry_obj = 0;
						mem->Read(entry, &entry_obj, sizeof(entry_obj));
						if (!entry_obj || entry_obj < 0x10000) continue;

						uint32_t name_ptr = 0;
						mem->Read(entry_obj + 0x08, &name_ptr, sizeof(name_ptr));
						if (!name_ptr || name_ptr < 0x10000) continue;

						std::string btn_name;
						if (!read_unity_string(mem, name_ptr, btn_name)) continue;

						if (btn_name == action_name)
						{
							// ButtonInput is stored as a boxed value at InputEntry +0x10.
							// Boxed object header is 8 bytes, then ButtonInput fields at +0x0/+0x1/+0x2.
							// So IsPressed is at boxed_ptr + 0x0A.
							uint32_t button_input = 0;
							mem->Read(entry_obj + 0x10, &button_input, sizeof(button_input));
							if (button_input && button_input > 0x10000) {
								bool val = true;
								mem->Write(button_input + Offsets::ButtonInput_::IsPressed, &val, sizeof(bool));
							}
							return;
						}
					}
				};

				force_button("Crouch", g_crouch_enabled);
				force_button("Scope", g_aim_enabled);
				force_button("Jump", g_jump_enabled);
				force_button("Attack", g_shoot_enabled);
			}
		}
	}
}