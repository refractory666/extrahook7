#pragma once

#include <cstddef>

namespace mem { class Memory; }

namespace misc
{
	extern bool g_misc_stop_enabled;
	extern int g_misc_stop_key;
	extern bool g_misc_teleport_enabled;
	extern int g_misc_teleport_key;
	extern bool g_misc_stop_active;     // true когда застопил (нажал бинд)
	extern bool g_misc_teleport_active; // true когда нажал тп
	extern bool g_misc_show_material;   // надписи с названиями материалов/мешей при наведении + отображение Mesh_1573 на карте

	extern bool g_fov_enabled;
	extern float g_fov_value;
	extern bool g_thirdperson_enabled;
	extern float g_thirdperson_dist;

	extern bool g_crouch_enabled;
	extern bool g_aim_enabled;
	extern bool g_jump_enabled;
	extern bool g_shoot_enabled;

	void run(mem::Memory* mem, size_t unity_base);

	// Установить значение Photon-проперти "health" для локального игрока.
	// Вызывается из меню (Misc) при нажатии на кнопку.
	void set_local_health(mem::Memory* mem, int health_value);

	// Установить целочисленную Photon-проперти (по части имени ключа) для всех игроков.
	// prop_substr — подстрока в имени ключа (например "badgeId", "badge").
	void set_local_badge(mem::Memory* mem, const char* prop_substr, int badge_id);
}
