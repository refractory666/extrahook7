#pragma once

struct ImDrawList;

namespace mem { class Memory; }

namespace aimbot
{
	// Run aimbot logic: read view angles, find best target in FOV, write new angles (with smooth).
	void run(mem::Memory* mem);

	// Draw FOV circle on screen when g_aimbot_fov_display is true.
	void draw_fov(ImDrawList* draw, float screen_w, float screen_h);
}
